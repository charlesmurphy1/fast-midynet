#ifndef FAST_MIDYNET_SBM_H
#define FAST_MIDYNET_SBM_H

#include <map>
#include <utility>
#include <vector>

#include "BaseGraph/types.h"
#include "FastMIDyNet/prior/sbm/edge_matrix.h"
#include "FastMIDyNet/prior/sbm/block.h"
#include "FastMIDyNet/random_graph/random_graph.hpp"
#include "FastMIDyNet/utility/maps.hpp"
#include "FastMIDyNet/generators.h"
#include "FastMIDyNet/types.h"

namespace FastMIDyNet{

class StochasticBlockModelFamily: public BlockLabeledRandomGraph{
protected:
    BlockPrior* m_blockPriorPtr = nullptr;
    EdgeMatrixPrior* m_edgeMatrixPriorPtr = nullptr;

    virtual void _applyGraphMove (const GraphMove&) override;
    virtual void _applyLabelMove (const BlockMove&) override;
    virtual const double getLogLikelihoodRatioEdgeTerm (const GraphMove&) const;
    virtual const double getLogLikelihoodRatioAdjTerm (const GraphMove&) const;
    void getDiffEdgeMatMapFromEdgeMove(const BaseGraph::Edge&, int, IntMap<std::pair<BlockIndex, BlockIndex>>&) const;
    void getDiffAdjMatMapFromEdgeMove(const BaseGraph::Edge&, int, IntMap<std::pair<BaseGraph::VertexIndex, BaseGraph::VertexIndex>>&) const;
    void getDiffEdgeMatMapFromBlockMove(const BlockMove&, IntMap<std::pair<BlockIndex, BlockIndex>>&) const;

public:
    StochasticBlockModelFamily(size_t graphSize): VertexLabeledRandomGraph<BlockIndex>(graphSize) { }
    StochasticBlockModelFamily(size_t graphSize, BlockPrior& blockPrior, EdgeMatrixPrior& edgeMatrixPrior):
        VertexLabeledRandomGraph<BlockIndex>(graphSize){
            setBlockPrior(blockPrior);
            m_blockPriorPtr->setSize(graphSize);
            setEdgeMatrixPrior(edgeMatrixPrior);
        }

    void sample () override;
    void sampleLabels() override {
        m_blockPriorPtr->sample();
        setLabels(m_blockPriorPtr->getState());
    }


    void setGraph(const MultiGraph& graph) override{
        RandomGraph::setGraph(graph);
        m_edgeMatrixPriorPtr->setGraph(m_graph);
    }
    void setLabels(const std::vector<BlockIndex>& labels) override {
        m_edgeMatrixPriorPtr->setPartition(labels);
    }


    const BlockPrior& getBlockPrior() const { return *m_blockPriorPtr; }
    BlockPrior& getBlockPriorRef() const { return *m_blockPriorPtr; }
    virtual void setBlockPrior(BlockPrior& blockPrior) {
        m_blockPriorPtr = &blockPrior;
        m_blockPriorPtr->isRoot(false);
        m_blockPriorPtr->setSize(m_size);
        if (m_edgeMatrixPriorPtr)
            m_edgeMatrixPriorPtr->setBlockPrior(*m_blockPriorPtr);
    }

    const EdgeMatrixPrior& getEdgeMatrixPrior() const { return *m_edgeMatrixPriorPtr; }
    EdgeMatrixPrior& getEdgeMatrixPriorRef() const { return *m_edgeMatrixPriorPtr; }
    virtual void setEdgeMatrixPrior(EdgeMatrixPrior& edgeMatrixPrior) {
        m_edgeMatrixPriorPtr = &edgeMatrixPrior;
        m_edgeMatrixPriorPtr->isRoot(false);
        m_edgeMatrixPriorPtr->setBlockPrior(*m_blockPriorPtr);
    }

    const BlockSequence& getLabels() const override { return m_blockPriorPtr->getState(); }
    const size_t getLabelCount() const override { return m_blockPriorPtr->getBlockCount(); }
    const CounterMap<BlockIndex>& getLabelCounts() const override { return m_blockPriorPtr->getVertexCounts(); }
    const CounterMap<BlockIndex>& getEdgeLabelCounts() const override { return m_edgeMatrixPriorPtr->getEdgeCounts(); }
    const MultiGraph& getLabelGraph() const override { return m_edgeMatrixPriorPtr->getState(); }
    const size_t& getEdgeCount() const override { return m_edgeMatrixPriorPtr->getEdgeCount(); }

    virtual const double getLogLikelihood() const override;
    virtual const double getLogPrior() const override;

    virtual const double getLogLikelihoodRatioFromGraphMove (const GraphMove&) const override;
    virtual const double getLogLikelihoodRatioFromLabelMove (const BlockMove&) const override;

    virtual const double getLogPriorRatioFromGraphMove (const GraphMove&) const override;
    virtual const double getLogPriorRatioFromLabelMove (const BlockMove&) const override;


    virtual bool isSafe() const override { return m_blockPriorPtr != nullptr and m_edgeMatrixPriorPtr != nullptr; }

    static MultiGraph getEdgeMatrixFromGraph(const MultiGraph&, const BlockSequence&) ;
    static void checkGraphConsistencyWithEdgeMatrix(const MultiGraph& graph, const BlockSequence& blockSeq, const MultiGraph& expectedEdgeMat);
    virtual void checkSelfConsistency() const override;
    virtual void checkSelfSafety() const override;
    virtual const bool isCompatible(const MultiGraph& graph) const override{
        if (not VertexLabeledRandomGraph<BlockIndex>::isCompatible(graph)) return false;
        auto edgeMatrix = getEdgeMatrixFromGraph(graph, getLabels());
        return edgeMatrix.getAdjacencyMatrix() == m_edgeMatrixPriorPtr->getState().getAdjacencyMatrix();
    };
    virtual void computationFinished() const override {
        m_isProcessed = false;
        m_blockPriorPtr->computationFinished();
        m_edgeMatrixPriorPtr->computationFinished();
    }
};

}// end FastMIDyNet
#endif
