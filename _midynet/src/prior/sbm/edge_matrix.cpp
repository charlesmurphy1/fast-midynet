#include <string>
#include <vector>


#include "FastMIDyNet/prior/sbm/edge_matrix.h"
#include "FastMIDyNet/exceptions.h"
#include "FastMIDyNet/utility/functions.h"
#include "FastMIDyNet/utility/maps.hpp"
#include "FastMIDyNet/generators.h"


namespace FastMIDyNet {

/* DEFINITION OF EDGE MATRIX PRIOR BASE CLASS */
void EdgeMatrixPrior::recomputeConsistentState() {
    m_edgeCounts.clear();
    for (auto r : m_state)
        m_edgeCounts.set(r, m_state.getDegreeOfIdx(r));
    m_edgeCountPriorPtr->setState(m_state.getTotalEdgeNumber());
}
void EdgeMatrixPrior::recomputeStateFromGraph() {
    m_state = MultiGraph(m_blockPriorPtr->getMaxBlockCount());
    m_edgeCounts.clear();
    for (const auto& vertex: *m_graphPtr){
        for (const auto& neighbor: m_graphPtr->getNeighboursOfIdx(vertex)){
            if (vertex > neighbor.vertexIndex)
                continue;

            m_edgeCounts.increment(m_blockPriorPtr->getBlockOfIdx(vertex), neighbor.label);
            m_edgeCounts.increment(m_blockPriorPtr->getBlockOfIdx(neighbor.vertexIndex), neighbor.label);
            m_state.addMultiedgeIdx(
                m_blockPriorPtr->getBlockOfIdx(vertex), m_blockPriorPtr->getBlockOfIdx(neighbor.vertexIndex), neighbor.label
            );
        }
    }
}
void EdgeMatrixPrior::setGraph(const MultiGraph& graph) {
    m_graphPtr = &graph;
    recomputeStateFromGraph();
}

void EdgeMatrixPrior::setPartition(const std::vector<BlockIndex>& labels) {
    m_blockPriorPtr->setState(labels);
    recomputeStateFromGraph();
}


void EdgeMatrixPrior::setState(const MultiGraph& edgeMatrix) {
    m_state = edgeMatrix;
    recomputeConsistentState();
}

void EdgeMatrixPrior::applyLabelMoveToState(const BlockMove& move) {
    if (move.prevLabel == move.nextLabel)
        return;

    if (m_state.getSize() <= move.nextLabel)
        m_state.resize(move.nextLabel + 1);
    const auto& blockSeq = m_blockPriorPtr->getState();
    const auto& degree = m_graphPtr->getDegreeOfIdx(move.vertexIndex);

    m_edgeCounts.decrement(move.prevLabel, degree);
    m_edgeCounts.increment(move.nextLabel, degree);
    for (auto neighbor: m_graphPtr->getNeighboursOfIdx(move.vertexIndex)) {
        auto neighborBlock = blockSeq[neighbor.vertexIndex];

        if (move.vertexIndex == neighbor.vertexIndex) // for self-loops
            neighborBlock = move.prevLabel;
        m_state.removeMultiedgeIdx(move.prevLabel, neighborBlock, neighbor.label) ;

        if (move.vertexIndex == neighbor.vertexIndex) // for self-loops
            neighborBlock = move.nextLabel;
        m_state.addMultiedgeIdx(move.nextLabel, neighborBlock, neighbor.label) ;
    }

}

void EdgeMatrixPrior::applyGraphMoveToState(const GraphMove& move){
    const auto& blockSeq = m_blockPriorPtr->getState();

    for (auto removedEdge: move.removedEdges) {
        const BlockIndex& r(blockSeq[removedEdge.first]), s(blockSeq[removedEdge.second]);
        m_state.removeEdgeIdx(r, s);
        m_edgeCounts.decrement(r);
        m_edgeCounts.decrement(s);
    }
    for (auto addedEdge: move.addedEdges) {
        const BlockIndex& r(blockSeq[addedEdge.first]), s(blockSeq[addedEdge.second]);
        m_state.addEdgeIdx(r, s);
        m_edgeCounts.increment(r);
        m_edgeCounts.increment(s);
    }
}

void EdgeMatrixPrior::checkSelfConsistency() const {
    m_blockPriorPtr->checkSelfConsistency();
    m_edgeCountPriorPtr->checkSelfConsistency();

    size_t sumEdges = 0;
    for (BlockIndex r : m_state) {
        size_t actualEdgeCounts = m_state.getDegreeOfIdx(r);
        if (actualEdgeCounts != m_edgeCounts[r])
            throw ConsistencyError("EdgeMatrixPrior: Edge matrix row ("
            + std::to_string(actualEdgeCounts) + ") doesn't sum to edgeCounts["
            + std::to_string(r) + "] (" + std::to_string(m_edgeCounts[r]) + ").");
        sumEdges += actualEdgeCounts;
    }
    if (sumEdges != 2*m_edgeCountPriorPtr->getState())
        throw ConsistencyError("EdgeMatrixPrior: Sum of edge matrix isn't equal to the number of edges.");
}


const double EdgeMatrixDeltaPrior::getLogLikelihoodRatioFromGraphMove(const GraphMove& move) const {
    CounterMap<std::pair<BlockIndex, BlockIndex>> map;

    for (auto edge : move.addedEdges){
        BlockIndex r = m_blockPriorPtr->getBlockOfIdx(edge.first);
        BlockIndex s = m_blockPriorPtr->getBlockOfIdx(edge.second);
        map.decrement({r, s});
        map.decrement({s, r});
    }
    for (auto edge : move.addedEdges){
        BlockIndex r = m_blockPriorPtr->getBlockOfIdx(edge.first);
        BlockIndex s = m_blockPriorPtr->getBlockOfIdx(edge.second);
        map.increment({r, s});
        map.increment({s, r});
    }

    for (auto k: map){
        if (k.second != 0)
            return -INFINITY;
    }
    return 0.;
}

const double EdgeMatrixDeltaPrior::getLogLikelihoodRatioFromLabelMove(const BlockMove& move) const {
    if (move.prevLabel != move.nextLabel)
        return -INFINITY;
    return 0.;
}



/* DEFINITION OF EDGE MATRIX UNIFORM PRIOR */

void EdgeMatrixUniformPrior::sampleState() {
    const auto& blockCount = m_blockPriorPtr->getEffectiveBlockCount();
    const auto& blocks = m_blockPriorPtr->getState();
    auto flattenedEdgeMatrix = sampleRandomWeakComposition(
                m_edgeCountPriorPtr->getState(),
                blockCount*(blockCount+1)/2
            );
    MultiGraph edgeMatrix = MultiGraph(m_blockPriorPtr->getBlockCount());
    std::pair<BlockIndex, BlockIndex> rs;
    m_edgeCounts.clear();
    size_t index = 0;

    const auto& vertexCounts = m_blockPriorPtr->getVertexCounts();
    for (const auto ers: flattenedEdgeMatrix){

        rs = getUndirectedPairFromIndex(index, m_state.getSize());
        while (vertexCounts[rs.first] == 0 or vertexCounts[rs.second] == 0){
            ++index;
            rs = getUndirectedPairFromIndex(index, m_state.getSize());
        }
        edgeMatrix.addMultiedgeIdx(rs.first, rs.second, ers);
        ++index;
    }
    setState(edgeMatrix);
}

const double EdgeMatrixUniformPrior::getLogLikelihoodRatioFromGraphMove(const GraphMove& move) const {
    double currentLogLikelihood =  getLogLikelihood(m_blockPriorPtr->getEffectiveBlockCount(), m_edgeCountPriorPtr->getState());
    double newLogLikelihood =  getLogLikelihood(m_blockPriorPtr->getEffectiveBlockCount(), m_edgeCountPriorPtr->getState() + move.addedEdges.size() - move.removedEdges.size());
    return newLogLikelihood - currentLogLikelihood;
}

const double EdgeMatrixUniformPrior::getLogLikelihoodRatioFromLabelMove(const BlockMove& move) const {
    double currentLogLikelihood =  getLogLikelihood(
        m_blockPriorPtr->getEffectiveBlockCount(), m_edgeCountPriorPtr->getState()
    );
    double newLogLikelihood =  getLogLikelihood(
        m_blockPriorPtr->getEffectiveBlockCount() + m_blockPriorPtr->getAddedBlocks(move), m_edgeCountPriorPtr->getState()
    );
    return newLogLikelihood - currentLogLikelihood;
}

// /* DEFINITION OF EDGE MATRIX EXPONENTIAL PRIOR */
//
// void EdgeMatrixExponentialPrior::sampleState() {
//     auto blockCount = m_blockPriorPtr->getBlockCount();
//     auto flattenedEdgeMatrix = sampleRandomWeakComposition(
//             m_edgeCountPriorPtr->getState(),
//             blockCount*(blockCount+1)/2
//             );
//
//     m_state = EdgeMatrix(blockCount, std::vector<size_t>(blockCount, 0));
//     std::pair<BlockIndex, BlockIndex> rs;
//     m_edgeCounts = std::vector<size_t>(blockCount, 0);
//     size_t index = 0, correctedEdgeCount;
//     for (auto edgeCountBetweenBlocks: flattenedEdgeMatrix) {
//         rs = getUndirectedPairFromIndex(index, blockCount);
//         m_edgeCounts[rs.first] += edgeCountBetweenBlocks;
//         m_edgeCounts[rs.second] += edgeCountBetweenBlocks;
//         m_state[rs.first][rs.second] += edgeCountBetweenBlocks;
//         m_state[rs.second][rs.first] += edgeCountBetweenBlocks;
//         index++;
//     }
// }
//
// double EdgeMatrixExponentialPrior::getLogLikelihoodRatioFromGraphMove(const GraphMove& move) const {
//     double currentLogLikelihood =  getLogLikelihood(m_blockPriorPtr->getBlockCount(), m_edgeCountPriorPtr->getState());
//     double newLogLikelihood =  getLogLikelihood(m_blockPriorPtr->getBlockCount(), m_edgeCountPriorPtr->getState() + move.addedEdges.size() - move.removedEdges.size());
//     return newLogLikelihood - currentLogLikelihood;
// }
//
// double EdgeMatrixExponentialPrior::getLogLikelihoodRatioFromLabelMove(const BlockMove& move) const {
//     auto vertexCounts = m_blockPriorPtr->getVertexCounts();
//
//     bool creatingBlock = move.nextLabel == m_blockPriorPtr->getBlockCount();
//     bool destroyingBlock = move.nextLabel != move.prevLabel &&
//                         vertexCounts[move.prevLabel] == 1;
//     double currentLogLikelihood =  getLogLikelihood(m_blockPriorPtr->getBlockCount(), m_edgeCountPriorPtr->getState());
//     double newLogLikelihood =  getLogLikelihood(m_blockPriorPtr->getBlockCount() + creatingBlock - destroyingBlock, m_edgeCountPriorPtr->getState());
//     return newLogLikelihood - currentLogLikelihood;
// }



} // namespace FastMIDyNet
