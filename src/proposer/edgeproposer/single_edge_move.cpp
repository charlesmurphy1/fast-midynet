#include "FastMIDyNet/utility/functions.h"
#include "FastMIDyNet/rng.h"
#include "FastMIDyNet/proposer/edge_proposer/single_edge_move.h"


namespace FastMIDyNet {


GraphMove SingleEdgeMove::proposeMove() {
    auto vertex1 = m_vertexDistribution.sample().first;
    auto vertex2 = m_vertexDistribution.sample().first;
    BaseGraph::Edge proposedEdge = {vertex1, vertex2};

    if (m_graphPtr->getEdgeMultiplicityIdx(vertex1, vertex2) == 0)
        return {{}, {proposedEdge}};

    if (m_addOrRemoveDistribution(rng))
        return {{}, {proposedEdge}};
    return {{proposedEdge}, {}};
}

void SingleEdgeMove::setup(const RandomGraph& randomGraph) {
    const MultiGraph& graph = randomGraph.getState();
    m_graphPtr = &graph;
    m_vertexDistribution.clear();
    for (auto vertex: graph)
        m_vertexDistribution.insert(vertex, 1);
}

double SingleEdgeMove::getLogProposalProbRatio(const GraphMove& move) const {
    double logProbability = 0;

    for (auto edge: move.removedEdges)
        if (m_graphPtr->getEdgeMultiplicityIdx(edge) == 1)
            logProbability += -log(.5);
    return logProbability;
}

} // namespace FastMIDyNet
