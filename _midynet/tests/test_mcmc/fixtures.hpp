#include "gtest/gtest.h"
#include <cmath>
#include <random>
#include <time.h>

#include "FastMIDyNet/proposer/block_proposer/uniform.h"
#include "FastMIDyNet/proposer/edge_proposer/hinge_flip.h"
#include "FastMIDyNet/prior/sbm/block_count.h"
#include "FastMIDyNet/prior/sbm/block.h"
#include "FastMIDyNet/prior/sbm/edge_count.h"
#include "FastMIDyNet/prior/sbm/edge_matrix.h"
#include "FastMIDyNet/random_graph/sbm.h"
#include "FastMIDyNet/rng.h"


using namespace std;

namespace FastMIDyNet{

class DummyRandomGraph: public RandomGraph{
    std::vector<size_t> m_blocks;
    size_t m_blockCount = 1;
    CounterMap<size_t> m_vertexCounts;
    MultiGraph m_edgeMatrix;
    CounterMap<size_t> m_edgeCounts;
    size_t m_edgeCount;
    std::vector<size_t> m_degrees;

public:
    using RandomGraph::RandomGraph;
    DummyRandomGraph(size_t size): RandomGraph(size), m_blocks(size, 0), m_vertexCounts({0}, {size}, 0){}

    void setGraph(const MultiGraph& graph) override{
        m_graph = graph;
        m_edgeCount = graph.getTotalEdgeNumber();
        m_edgeMatrix = MultiGraph(1); m_edgeMatrix.setEdgeMultiplicityIdx(0, 0, m_edgeCount);
        m_edgeCounts.set(0, 2 * m_edgeCount);
        m_degrees = graph.getDegrees();
    }

    const std::vector<BlockIndex>& getBlocks() const override { return m_blocks; }
    const size_t& getBlockCount() const override { return m_blockCount; }
    const CounterMap<size_t>& getVertexCountsInBlocks() const override { return m_vertexCounts; }
    const MultiGraph& getEdgeMatrix() const override { return m_edgeMatrix; }
    const CounterMap<size_t>& getEdgeCountsInBlocks() const override { return m_edgeCounts; }
    const size_t& getEdgeCount() const override { return m_edgeCount; }
    const std::vector<size_t>& getDegrees() const override { return m_degrees; }

    void sampleGraph() override { };
    virtual void samplePriors() override { };
    const double getLogLikelihood() const override { return 0; }
    const double getLogPrior() const override { return 0; }
    const double getLogLikelihoodRatioFromGraphMove(const GraphMove& move) const override{ return 0; }
    const double getLogPriorRatioFromGraphMove(const GraphMove& move) const override { return 0; }
    const double getLogLikelihoodRatioFromBlockMove(const BlockMove& move) const override { return 0; }
    const double getLogPriorRatioFromBlockMove(const BlockMove& move) const override { return 0; }

};


}
