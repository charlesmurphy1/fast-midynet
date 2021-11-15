#include "gtest/gtest.h"
#include <list>

#include "FastMIDyNet/dynamics/ising-glauber.h"
#include "fixtures.hpp"


static const double COUPLING_CONSTANT = 2.;
static const std::list<std::vector<int>> neighbor_states = {{1, 3}, {2, 2}, {3, 1}};

static inline double sigmoid(double x) {
    return 1/(1+exp(-x));
}

class TestIsingGlauber: public::testing::Test{
    public:
        FastMIDyNet::RNG rng;
        FastMIDyNet::DummyRandomGraph graph = FastMIDyNet::DummyRandomGraph(7, rng);
        FastMIDyNet::IsingGlauberDynamic dynamic = FastMIDyNet::IsingGlauberDynamic(graph, rng, COUPLING_CONSTANT);
        void SetUp() {
            auto graph = FastMIDyNet::getUndirectedHouseMultiGraph();
            FastMIDyNet::State state = {0, 0, 0, 1, 1, 1, 2};
            dynamic.setState(state);
            dynamic.setGraph(graph);
        }
};


TEST_F(TestIsingGlauber, getActivationProb_forEachStateTransition_returnCorrectProbability) {

    for (auto neighbor_state: neighbor_states)
        EXPECT_EQ(sigmoid(2*COUPLING_CONSTANT*(neighbor_state[0]-neighbor_state[1])),
                  dynamic.getActivationProb(neighbor_state));
}

TEST_F(TestIsingGlauber, getDeactivationProb_forEachStateTransition_returnCorrectProbability) {

    for (auto neighbor_state: neighbor_states)
        EXPECT_EQ(sigmoid(-2*COUPLING_CONSTANT*(neighbor_state[0]-neighbor_state[1])),
                  dynamic.getDeactivationProb(neighbor_state));
}
