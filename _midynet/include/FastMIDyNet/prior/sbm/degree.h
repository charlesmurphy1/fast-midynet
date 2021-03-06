#ifndef FAST_MIDYNET_DEGREE_H
#define FAST_MIDYNET_DEGREE_H

#include <map>
#include "FastMIDyNet/types.h"
#include "FastMIDyNet/prior/prior.hpp"
#include "FastMIDyNet/prior/sbm/edge_matrix.h"
#include "FastMIDyNet/proposer/movetypes.h"
#include "FastMIDyNet/utility/maps.hpp"


namespace FastMIDyNet{

class DegreePrior: public BlockLabeledPrior< DegreeSequence >{
protected:
    BlockPrior* m_blockPriorPtr = nullptr;
    EdgeMatrixPrior* m_edgeMatrixPriorPtr = nullptr;
    DegreeCountsMap m_degreeCounts;
    // const MultiGraph* m_graphPtr;

    void _applyGraphMove(const GraphMove& move) override;
    void _applyLabelMove(const BlockMove& move) override;

    const double _getLogJointRatioFromGraphMove(const GraphMove& move) const override{
        return getLogLikelihoodRatioFromGraphMove(move) + getLogPriorRatioFromGraphMove(move);
    }
    const double _getLogJointRatioFromLabelMove(const BlockMove& move) const override {
        return getLogLikelihoodRatioFromLabelMove(move) + getLogPriorRatioFromLabelMove(move);
    }

    // void onBlockCreation(const BlockMove&) override;

    void applyGraphMoveToState(const GraphMove&);
    void applyGraphMoveToDegreeCounts(const GraphMove&);
    void applyLabelMoveToDegreeCounts(const BlockMove&);
    void recomputeConsistentState() ;
public:
    using BlockLabeledPrior<DegreeSequence>::BlockLabeledPrior;
    /* Constructors */
    DegreePrior(){}
    DegreePrior(BlockPrior& blockPrior, EdgeMatrixPrior& edgeMatrixPrior){
            setBlockPrior(blockPrior);
            setEdgeMatrixPrior(edgeMatrixPrior);
        }
    DegreePrior(const DegreePrior& other){
        setBlockPrior(*other.m_blockPriorPtr);
        setEdgeMatrixPrior(*other.m_edgeMatrixPriorPtr);
    }
    virtual ~DegreePrior(){}
    const DegreePrior& operator=(const DegreePrior& other){
        setBlockPrior(*other.m_blockPriorPtr);
        setEdgeMatrixPrior(*other.m_edgeMatrixPriorPtr);
        return *this;
    }

    // void setGraph(const MultiGraph&);
    // const MultiGraph& getGraph() const { return *m_graphPtr; }
    virtual void setState(const DegreeSequence&) override;
    void setPartition(const std::vector<BlockIndex>&) ;
    static const DegreeCountsMap computeDegreeCounts(const std::vector<size_t>&,  const std::vector<BlockIndex>);

    const BlockPrior& getBlockPrior() const { return *m_blockPriorPtr; }
    BlockPrior& getBlockPriorRef() const { return *m_blockPriorPtr; }
    void setBlockPrior(BlockPrior& blockPrior) { m_blockPriorPtr = &blockPrior; m_blockPriorPtr->isRoot(false);}

    const EdgeMatrixPrior& getEdgeMatrixPrior() const { return *m_edgeMatrixPriorPtr; }
    EdgeMatrixPrior& getEdgeMatrixPriorRef() const { return *m_edgeMatrixPriorPtr; }
    void setEdgeMatrixPrior(EdgeMatrixPrior& edgeMatrixPrior) {
        m_edgeMatrixPriorPtr = &edgeMatrixPrior; m_edgeMatrixPriorPtr->isRoot(false);
    }

    const BlockIndex& getDegreeOfIdx(BaseGraph::VertexIndex idx) const { return m_state[idx]; }
    virtual const DegreeCountsMap& getDegreeCounts() const { return m_degreeCounts; }

    void samplePriors() override { m_blockPriorPtr->sample(); m_edgeMatrixPriorPtr->sample(); }

    const double getLogPrior() const override { return m_blockPriorPtr->getLogJoint() + m_edgeMatrixPriorPtr->getLogJoint(); }

    virtual const double getLogLikelihoodRatioFromGraphMove(const GraphMove&) const = 0;
    virtual const double getLogLikelihoodRatioFromLabelMove(const BlockMove&) const = 0;
    virtual const double getLogPriorRatioFromGraphMove(const GraphMove& move) const { return m_blockPriorPtr->getLogJointRatioFromGraphMove(move) + m_edgeMatrixPriorPtr->getLogJointRatioFromGraphMove(move); }
    virtual const double getLogPriorRatioFromLabelMove(const BlockMove& move) const { return m_blockPriorPtr->getLogJointRatioFromLabelMove(move) + m_edgeMatrixPriorPtr->getLogJointRatioFromLabelMove(move); }

    virtual void computationFinished() const override {
        m_isProcessed = false;
        m_blockPriorPtr->computationFinished();
        m_edgeMatrixPriorPtr->computationFinished();
    }
    static void checkDegreeSequenceConsistencyWithEdgeCount(const DegreeSequence&, size_t);
    static void checkDegreeSequenceConsistencyWithDegreeCounts(const DegreeSequence&, const BlockSequence&, const DegreeCountsMap&);

    bool isSafe() const override {
        return (m_blockPriorPtr != nullptr) and (m_blockPriorPtr->isSafe())
           and (m_edgeMatrixPriorPtr != nullptr) and (m_edgeMatrixPriorPtr->isSafe());
    }
    void checkSelfConsistency() const override;
    virtual void checkSelfSafety() const override{
        if (m_blockPriorPtr == nullptr)
            throw SafetyError("DegreePrior: unsafe prior since `m_blockPriorPtr` is empty.");

        if (m_edgeMatrixPriorPtr == nullptr)
            throw SafetyError("DegreePrior: unsafe prior since `m_edgeMatrixPriorPtr` is empty.");
        m_blockPriorPtr->checkSafety();
        m_edgeMatrixPriorPtr->checkSafety();
    }


};


class DegreeDeltaPrior: public DegreePrior{
    DegreeSequence m_degreeSeq;

public:
    DegreeDeltaPrior(){}
    DegreeDeltaPrior(const DegreeSequence& degreeSeq):
        DegreePrior(){ setState(degreeSeq); }

    DegreeDeltaPrior(const DegreeDeltaPrior& degreeDeltaPrior):
        DegreePrior() {
            setState(degreeDeltaPrior.getState());
        }
    virtual ~DegreeDeltaPrior(){}
    const DegreeDeltaPrior& operator=(const DegreeDeltaPrior& other){
        this->setState(other.getState());
        return *this;
    }


    void setState(const DegreeSequence& degrees){
        m_degreeSeq = degrees;
        m_state = degrees;
    }
    void sampleState() override { };
    void samplePriors() override { };

    const double getLogLikelihood() const override { return 0.; }
    const double getLogPrior() const override { return 0.; };

    const double getLogLikelihoodRatioFromGraphMove(const GraphMove& move) const override;
    const double getLogLikelihoodRatioFromLabelMove(const BlockMove& move) const override{
        return (move.prevLabel != move.nextLabel) ? -INFINITY : 0.;
    }
    const double getLogPriorRatioFromGraphMove(const GraphMove& move) const override { return 0.; }
    const double getLogPriorRatioFromLabelMove(const BlockMove& move) const override{ return 0.; }

    void checkSelfConsistency() const override { };
    void checkSelfSafety() const override {
        if (m_degreeSeq.size() == 0)
            throw SafetyError("DegreeDeltaPrior: unsafe prior since `m_degreeSeq` is empty.");
    }

    void computationFinished() const override { m_isProcessed = false; }

};

class DegreeUniformPrior: public DegreePrior{
public:
    using DegreePrior::DegreePrior;
    void sampleState() override;

    const double getLogLikelihood() const override;
    const double getLogLikelihoodRatioFromGraphMove(const GraphMove&) const;
    const double getLogLikelihoodRatioFromLabelMove(const BlockMove&) const;
};

class DegreeUniformHyperPrior: public DegreePrior{
public:

    using DegreePrior::DegreePrior;
    void sampleState() override;

    const double getLogLikelihood() const override;
    const double getLogLikelihoodRatioFromGraphMove(const GraphMove&) const;
    const double getLogLikelihoodRatioFromLabelMove(const BlockMove&) const;
};


} // FastMIDyNet

#endif
