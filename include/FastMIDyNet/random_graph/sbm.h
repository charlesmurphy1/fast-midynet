#ifndef FAST_MIDYNET_SBM_H
#define FAST_MIDYNET_SBM_H

#include <map>
#include <utility>
#include <vector>

#include "BaseGraph/types.h"
#include "FastMIDyNet/prior/dcsbm/edge_matrix.h"
#include "FastMIDyNet/prior/dcsbm/block.h"
#include "FastMIDyNet/random_graph/random_graph.h"
#include "FastMIDyNet/generators.h"
#include "FastMIDyNet/types.h"

namespace FastMIDyNet{

class StochasticBlockModelFamily: public RandomGraph{
protected:
    EdgeMatrixPrior& m_edgeMatrixPrior;
    BlockPrior& m_blockPrior;
public:
    StochasticBlockModelFamily(BlockPrior& blockPrior, EdgeMatrixPrior& edgeMatrixPrior):
    m_blockPrior(blockPrior), m_edgeMatrixPrior(edgeMatrixPrior), RandomGraph(blockPrior.getSize()) { }

    void sampleState () ;
    void samplePriors () ;
    void sample(){
        samplePriors();
        sampleState();
        m_edgeMatrixPrior.setGraph(m_state);
        #if DEBUG
        checkSelfConsistency();
        #endif
    }

    const BlockSequence& getBlockSequence() const { return m_blockPrior.getState(); }
    const size_t& getBlockCount() const { return m_blockPrior.getBlockCount(); }
    const std::vector<size_t>& getVertexCountsInBlock() const { return m_blockPrior.getVertexCountsInBlock(); }
    const EdgeMatrix& getEdgeMatrix() const { return m_edgeMatrixPrior.getState(); }
    std::vector<size_t> getEdgeCountsInBlock() const { return m_edgeMatrixPrior.getEdgeCountsInBlock(); } // à changer lorsque EdgeMatrixPrior sera fait
    const size_t& getEdgeCount() const { return m_edgeMatrixPrior.getEdgeCount(); }

    void getDiffEdgeMatMapFromEdgeMove(const BaseGraph::Edge&, int, std::map<std::pair<BlockIndex, BlockIndex>, size_t>&);
    void getDiffAdjMatMapFromEdgeMove(const BaseGraph::Edge&, int, std::map<std::pair<BaseGraph::VertexIndex, BaseGraph::VertexIndex>, size_t>&);
    void getDiffEdgeMatMapFromBlockMove(const BlockMove&, std::map<std::pair<BlockIndex, BlockIndex>, size_t>&);

    double getLogLikelihood() const;
    double getLogPrior() ;
    double getLogJoint() ;

    double getLogLikelihoodRatioEdgeTerm (const GraphMove&) ;
    double getLogLikelihoodRatioAdjTerm (const GraphMove&) ;
    double getLogLikelihoodRatio (const GraphMove&) ;
    double getLogLikelihoodRatio (const BlockMove&) ;

    double getLogPriorRatio (const GraphMove&) ;
    double getLogPriorRatio (const BlockMove&) ;

    double getLogJointRatio (const GraphMove& move) { return getLogLikelihoodRatio(move) + getLogPriorRatio(move); }
    double getLogJointRatio (const BlockMove& move) { return getLogLikelihoodRatio(move) + getLogPriorRatio(move); }

    void applyMove (const GraphMove&);
    void applyMove (const BlockMove&);

    void computationFinished(){
        m_blockPrior.computationFinished();
        m_edgeMatrixPrior.computationFinished();
    }

    static EdgeMatrix getEdgeMatrixFromGraph(const MultiGraph&, const BlockSequence&) ;
    static DegreeSequence getDegreeSequenceFromGraph(const MultiGraph&) ;
    static void checkGraphConsistencyWithEdgeMatrix(const MultiGraph& graph, const BlockSequence& blockSeq, const EdgeMatrix& expectedEdgeMat);
    static void checkGraphConsistencyWithDegreeSequence(const MultiGraph& graph, const DegreeSequence& degreeSeq) ;


    void checkSelfConsistency() ;

};

}// end FastMIDyNet
#endif
