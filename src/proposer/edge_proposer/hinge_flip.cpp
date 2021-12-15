#include "FastMIDyNet/utility/functions.h"
#include "FastMIDyNet/rng.h"
#include "FastMIDyNet/proposer/edge_proposer/hinge_flip.h"


namespace FastMIDyNet {


GraphMove HingeFlip::proposeMove() {
    auto edge = m_edgeSamplableSet.sample_ext_RNG(rng).first;
    BaseGraph::VertexIndex node = m_vertexSamplerPtr->sample();

    if (edge.first == node or edge.second == node)
        return GraphMove();

    BaseGraph::Edge newEdge;
    if (m_flipOrientationDistribution(rng)) {
        newEdge = {edge.first, node};
    }
    else {
        newEdge = {edge.second, node};
    }
    return {{edge}, {newEdge}};
};

void HingeFlip::acceptIsolated(bool accept){
    m_withIsolatedVertices = accept;
    m_vertexSamplerPtr->acceptIsolated(accept);
}

void HingeFlip::setUp(const MultiGraph& graph){
    m_vertexSamplerPtr->setUp(graph);
    for (auto vertex: graph) {
        for (auto neighbor: graph.getNeighboursOfIdx(vertex)) {
            if (vertex <= neighbor.vertexIndex)
                m_edgeSamplableSet.insert({vertex, neighbor.vertexIndex}, neighbor.label);
        }
    }
}

void HingeFlip::updateProbabilities(const GraphMove& move) {
    m_vertexSamplerPtr->update(move);

    for (auto edge: move.removedEdges) {
        edge = getOrderedEdge(edge);
        size_t edgeWeight = round(m_edgeSamplableSet.get_weight(edge));
        if (edgeWeight == 1)
            m_edgeSamplableSet.erase(edge);
        else
            m_edgeSamplableSet.set_weight(edge, edgeWeight-1);
    }

    for (auto edge: move.addedEdges) {
        edge = getOrderedEdge(edge);
        if (m_edgeSamplableSet.count(edge) == 0)
            m_edgeSamplableSet.insert(edge, 1);
        else {
            m_edgeSamplableSet.set_weight(edge, round(m_edgeSamplableSet.get_weight(edge))+1);
        }
    }
}


} // namespace FastMIDyNet
