#include "gtest/gtest.h"
#include <list>

#include "FastMIDyNet/dynamics/degree.hpp"
#include "FastMIDyNet/random_graph/erdosrenyi.h"
#include "FastMIDyNet/proposer/edge/hinge_flip.h"
#include "fixtures.hpp"

namespace FastMIDyNet{

const double C = 10.;
const std::list<std::vector<VertexState>> NEIGHBOR_STATES = {{1, 3}, {2, 2}, {3, 1}, {2, 0}};


class TestDegreeDynamics: public::testing::Test{
public:
    EdgeCountDeltaPrior edgeCountPrior = {10};
    ErdosRenyiFamily randomGraph = ErdosRenyiFamily(10, edgeCountPrior);
    HingeFlipUniformProposer edgeProposer = HingeFlipUniformProposer();
    FastMIDyNet::DegreeDynamics<RandomGraph> dynamics = FastMIDyNet::DegreeDynamics<RandomGraph>(randomGraph, NUM_STEPS, C);
};


TEST_F(TestDegreeDynamics, getActivationProb_forEachStateTransition_returnCorrectProbability) {
    for (auto neighbor_state: NEIGHBOR_STATES)
        EXPECT_EQ(
            (neighbor_state[0] + neighbor_state[1])/C, dynamics.getActivationProb(neighbor_state)
        );
}

TEST_F(TestDegreeDynamics, getDeactivationProb_forEachStateTransition_returnCorrectProbability) {
    for (auto neighbor_state: NEIGHBOR_STATES)
        EXPECT_EQ(
            1 - dynamics.getActivationProb(neighbor_state), dynamics.getDeactivationProb(neighbor_state)
        );
}

TEST_F(TestDegreeDynamics, afterSample_getCorrectNeighborState){
    dynamics.sample();
    dynamics.checkConsistency();
}

TEST_F(TestDegreeDynamics, getLogLikelihood_returnCorrectLogLikelikehood){
    dynamics.sample();
    auto past = dynamics.getPastStates();
    auto future = dynamics.getFutureStates();
    auto neighborState = dynamics.getNeighborsPastStates();

    double expected = dynamics.getLogLikelihood();
    double actual = 0;
    for(size_t t=0; t<dynamics.getNumSteps(); ++t){
        for (auto vertex : dynamics.getGraph()){
            actual += log(dynamics.getTransitionProb(past[vertex][t], future[vertex][t], neighborState[vertex][t]));
        }
    }
    EXPECT_NEAR(expected, actual, 1E-6);
}

TEST_F(TestDegreeDynamics, getLogLikelihoodRatio_forSomeGraphMove_returnLogJointRatio){
    dynamics.sample();
    edgeProposer.setUp(randomGraph.getGraph());
    auto graphMove = edgeProposer.proposeMove();
    double ratio = dynamics.getLogLikelihoodRatioFromGraphMove(graphMove);
    double logLikelihoodBefore = dynamics.getLogLikelihood();
    dynamics.applyGraphMove(graphMove);
    double logLikelihoodAfter = dynamics.getLogLikelihood();
    EXPECT_NEAR(ratio, logLikelihoodAfter - logLikelihoodBefore, 1e-6);
}

}
