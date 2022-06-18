#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <iostream>

#include "BaseGraph/types.h"
#include "FastMIDyNet/random_graph/sbm.h"
#include "FastMIDyNet/utility/functions.h"
#include "FastMIDyNet/utility/maps.hpp"
#include "FastMIDyNet/exceptions.h"
#include "FastMIDyNet/generators.h"
#include "FastMIDyNet/rng.h"
#include "FastMIDyNet/types.h"

using namespace std;
using namespace FastMIDyNet;
using namespace BaseGraph;

void StochasticBlockModelFamily::sampleGraph(){
    const BlockSequence& blockSeq = getVertexLabels();
    const MultiGraph& edgeMat = m_edgeMatrixPriorPtr->getState();
    MultiGraph graph = generateSBM(blockSeq, edgeMat.getAdjacencyMatrix());
    setGraph(graph);
}

void StochasticBlockModelFamily::samplePriors(){
    m_blockPriorPtr->sample();
    m_edgeMatrixPriorPtr->sample();
    computationFinished();
}

const double StochasticBlockModelFamily::getLogLikelihood() const{

    const MultiGraph& edgeMat = m_edgeMatrixPriorPtr->getState() ;
    const CounterMap<size_t>& edgeCounts = getEdgeLabelCounts();
    const CounterMap<size_t>& vertexCounts = getLabelCounts();

    double logLikelihood = 0;

    for (const auto& r : edgeMat) {
        if (edgeCounts.isEmpty(r))
            continue;
        logLikelihood -= edgeCounts[r] * log(vertexCounts[r]);
        for (const auto& s : edgeMat.getNeighboursOfIdx(r))
            if (r <= s.vertexIndex)
                logLikelihood += (r == s.vertexIndex) ? logDoubleFactorial(2 * s.label) : logFactorial(s.label);
    }
    for (const auto& idx : m_graph)
        for (const auto& neighbor : m_graph.getNeighboursOfIdx(idx))
            if (idx <= neighbor.vertexIndex)
                logLikelihood -= (idx == neighbor.vertexIndex) ? logDoubleFactorial(2 * neighbor.label) : logFactorial(neighbor.label);

    return logLikelihood;
};

const double StochasticBlockModelFamily::getLogPrior() const {
    double logPrior = m_blockPriorPtr->getLogJoint() + m_edgeMatrixPriorPtr->getLogJoint();
    computationFinished();
    return logPrior;
};

void StochasticBlockModelFamily::getDiffEdgeMatMapFromEdgeMove( const Edge& edge, int counter, IntMap<pair<BlockIndex, BlockIndex>>& diffEdgeMatMap ) const{
    const BlockSequence& blockSeq = getVertexLabels();
    diffEdgeMatMap.increment(
        getOrderedPair<BlockIndex>({blockSeq[edge.first], blockSeq[edge.second]}),
        counter
    );
};

const double StochasticBlockModelFamily::getLogLikelihoodRatioEdgeTerm (const GraphMove& move) const {
    const BlockSequence& blockSeq = getVertexLabels();
    const MultiGraph& edgeMat = m_edgeMatrixPriorPtr->getState();
    const CounterMap<size_t>& edgeCounts = getEdgeLabelCounts();
    const CounterMap<size_t>& vertexCounts = getLabelCounts();
    double logLikelihoodRatioTerm = 0;

    IntMap<pair<BlockIndex, BlockIndex>> diffEdgeMatMap;
    IntMap<BlockIndex> diffEdgeCountsMap;

    for (auto edge : move.addedEdges)
        getDiffEdgeMatMapFromEdgeMove(edge, 1, diffEdgeMatMap);
    for (auto edge : move.removedEdges)
        getDiffEdgeMatMapFromEdgeMove(edge, -1, diffEdgeMatMap);

    for (auto diff : diffEdgeMatMap){
        auto r = diff.first.first, s = diff.first.second;
        size_t ers = (r >= edgeMat.getSize() or s >= edgeMat.getSize()) ? 0 : edgeMat.getEdgeMultiplicityIdx(r, s);
        diffEdgeCountsMap.increment(r, diff.second);
        diffEdgeCountsMap.increment(s, diff.second);
        logLikelihoodRatioTerm += (r == s) ? logDoubleFactorial(2 * ers + 2 * diff.second) : logFactorial(ers + diff.second);
        logLikelihoodRatioTerm -= (r == s) ? logDoubleFactorial(2 * ers) : logFactorial(ers);
    }

    for (auto diff : diffEdgeCountsMap){
            logLikelihoodRatioTerm -= diff.second * log( vertexCounts[diff.first] ) ;
    }
    return logLikelihoodRatioTerm;
};

void StochasticBlockModelFamily::getDiffAdjMatMapFromEdgeMove(
    const Edge& edge, int counter, IntMap<pair<VertexIndex, VertexIndex>>& diffAdjMatMap
) const{
    Edge orderedEdge = getOrderedEdge(edge);
    diffAdjMatMap.increment({orderedEdge.first, orderedEdge.second}, counter);
};

const double StochasticBlockModelFamily::getLogLikelihoodRatioAdjTerm (const GraphMove& move) const {
    IntMap<pair<VertexIndex, VertexIndex>> diffAdjMatMap;
    double logLikelihoodRatioTerm = 0;

    for (auto edge : move.addedEdges)
        getDiffAdjMatMapFromEdgeMove(edge, 1, diffAdjMatMap);
    for (auto edge : move.removedEdges)
        getDiffAdjMatMapFromEdgeMove(edge, -1, diffAdjMatMap);

    for (auto diff : diffAdjMatMap){
        auto u = diff.first.first, v = diff.first.second;
        auto edgeMult = m_graph.getEdgeMultiplicityIdx(u, v);
        logLikelihoodRatioTerm -= (u == v) ? logDoubleFactorial(2 * edgeMult + 2 * diff.second) : logFactorial(edgeMult + diff.second);
        logLikelihoodRatioTerm += (u == v) ? logDoubleFactorial(2 * edgeMult) : logFactorial(edgeMult);
    }
    return logLikelihoodRatioTerm;
};

const double StochasticBlockModelFamily::getLogLikelihoodRatioFromGraphMove (const GraphMove& move) const {
    return getLogLikelihoodRatioEdgeTerm(move) + getLogLikelihoodRatioAdjTerm(move);
}

void StochasticBlockModelFamily::getDiffEdgeMatMapFromBlockMove(
    const BlockMove& move, IntMap<pair<BlockIndex, BlockIndex>>& diffEdgeMatMap
) const {
    const BlockSequence& blockSeq = getVertexLabels();
    for (auto neighbor : m_graph.getNeighboursOfIdx(move.vertexIdx)){
        BlockIndex blockIdx = blockSeq[neighbor.vertexIndex];
        size_t edgeMult = neighbor.label;
        pair<BlockIndex, BlockIndex> orderedBlockPair = getOrderedPair<BlockIndex> ({move.prevBlockIdx, blockIdx});
        diffEdgeMatMap.decrement(orderedBlockPair, neighbor.label);

        if (neighbor.vertexIndex == move.vertexIdx) // handling self-loops
            blockIdx = move.nextBlockIdx;

        orderedBlockPair = getOrderedPair<BlockIndex> ({move.nextBlockIdx, blockIdx});
        diffEdgeMatMap.increment(orderedBlockPair, neighbor.label);
    }
};

const double StochasticBlockModelFamily::getLogLikelihoodRatioFromLabelMove(const BlockMove& move) const {
    const BlockSequence& blockSeq = getVertexLabels();
    const MultiGraph& edgeMat = m_edgeMatrixPriorPtr->getState();
    const CounterMap<size_t>& edgeCounts = getEdgeLabelCounts();
    const CounterMap<size_t>& vertexCounts = getLabelCounts();
    const size_t& degree = m_graph.getDegreeOfIdx(move.vertexIdx);
    double logLikelihoodRatio = 0;

    if (move.prevBlockIdx == move.nextBlockIdx)
        return 0;

    IntMap<pair<BlockIndex, BlockIndex>> diffEdgeMatMap;

    getDiffEdgeMatMapFromBlockMove(move, diffEdgeMatMap);

    for (auto diff : diffEdgeMatMap){
        auto r = diff.first.first, s = diff.first.second;
        size_t ers = (r >= edgeMat.getSize() or s >= edgeMat.getSize()) ? 0 : edgeMat.getEdgeMultiplicityIdx(r, s);
        logLikelihoodRatio += (r == s) ? logDoubleFactorial(2 * ers + 2 * diff.second) : logFactorial(ers + diff.second);
        logLikelihoodRatio -= (r == s) ? logDoubleFactorial(2 * ers) : logFactorial(ers);
    }

    logLikelihoodRatio += edgeCounts[move.prevBlockIdx] * log(vertexCounts[move.prevBlockIdx]) ;
    logLikelihoodRatio -= (edgeCounts[move.prevBlockIdx] == degree) ? 0: (edgeCounts[move.prevBlockIdx] - degree) * log(vertexCounts[move.prevBlockIdx] - 1);

    logLikelihoodRatio += (edgeCounts[move.nextBlockIdx] == 0) ? 0: edgeCounts[move.nextBlockIdx] * log(vertexCounts[move.nextBlockIdx]);
    logLikelihoodRatio -= (edgeCounts[move.nextBlockIdx] + degree) * log(vertexCounts[move.nextBlockIdx] + 1) ;

    return logLikelihoodRatio;
};

const double StochasticBlockModelFamily::getLogPriorRatioFromGraphMove (const GraphMove& move) const {
    double logPriorRatio = m_blockPriorPtr->getLogJointRatioFromGraphMove(move) + m_edgeMatrixPriorPtr->getLogJointRatioFromGraphMove(move);
    return logPriorRatio;
};

const double StochasticBlockModelFamily::getLogPriorRatioFromLabelMove (const BlockMove& move) const {
    double logPriorRatio = m_blockPriorPtr->getLogJointRatioFromBlockMove(move) + m_edgeMatrixPriorPtr->getLogJointRatioFromBlockMove(move);
    return logPriorRatio;
};

void StochasticBlockModelFamily::_applyGraphMove (const GraphMove& move) {
    m_blockPriorPtr->applyGraphMove(move);
    m_edgeMatrixPriorPtr->applyGraphMove(move);
    RandomGraph::_applyGraphMove(move);
};

void StochasticBlockModelFamily::_applyLabelMove (const BlockMove& move){
    m_blockPriorPtr->applyBlockMove(move);
    m_edgeMatrixPriorPtr->applyBlockMove(move);
};

MultiGraph StochasticBlockModelFamily::getEdgeMatrixFromGraph(const MultiGraph& graph, const BlockSequence& blockSeq){
    size_t numBlocks = *max_element(blockSeq.begin(), blockSeq.end()) + 1;
    MultiGraph edgeMat(numBlocks);
    for (auto idx : graph){
        for (auto neighbor : graph.getNeighboursOfIdx(idx)){
            if (idx > neighbor.vertexIndex)
                continue;
            BlockIndex r = blockSeq[idx], s = blockSeq[neighbor.vertexIndex];
            edgeMat.addMultiedgeIdx(r, s, neighbor.label);
        }
    }
    return edgeMat;
};

void StochasticBlockModelFamily::checkGraphConsistencyWithEdgeMatrix(
    const MultiGraph& graph,
    const BlockSequence& blockSeq,
    const MultiGraph& expectedEdgeMat){
    MultiGraph actualEdgeMat = getEdgeMatrixFromGraph(graph, blockSeq);
    for (auto r : actualEdgeMat)
        for (auto s : actualEdgeMat.getNeighboursOfIdx(r))
            if (expectedEdgeMat.getEdgeMultiplicityIdx(r, s.vertexIndex) != s.label)
                throw ConsistencyError("StochasticBlockModelFamily: at indices ("
                + to_string(r) + ", " + to_string(s.vertexIndex) + ") edge matrix is inconsistent with graph:"
                + to_string(expectedEdgeMat.getEdgeMultiplicityIdx(r, s.vertexIndex)) + " != "
                + to_string(s.label));


};

void StochasticBlockModelFamily::checkSelfConsistency() const{
    m_blockPriorPtr->checkSelfConsistency();
    m_edgeMatrixPriorPtr->checkSelfConsistency();

    checkGraphConsistencyWithEdgeMatrix(m_graph, getVertexLabels(), m_edgeMatrixPriorPtr->getState());
}

void StochasticBlockModelFamily::checkSelfSafety()const{
    if (m_blockPriorPtr == nullptr)
        throw SafetyError("StochasticBlockModelFamily: unsafe family since `m_blockPriorPtr` is empty.");
    m_blockPriorPtr->checkSafety();

    if (m_edgeMatrixPriorPtr == nullptr)
        throw SafetyError("StochasticBlockModelFamily: unsafe family since `m_edgeMatrixPriorPtr` is empty.");
    m_edgeMatrixPriorPtr->checkSafety();
}
