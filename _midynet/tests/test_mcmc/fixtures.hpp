#include "gtest/gtest.h"
#include <cmath>
#include <random>
#include <time.h>

#include "FastMIDyNet/prior/sbm/block_count.h"
#include "FastMIDyNet/prior/sbm/block.h"
#include "FastMIDyNet/prior/sbm/edge_count.h"
#include "FastMIDyNet/prior/sbm/edge_matrix.h"
#include "FastMIDyNet/random_graph/sbm.h"
#include "FastMIDyNet/random_graph/erdosrenyi.h"
#include "FastMIDyNet/dynamics/sis.hpp"
#include "FastMIDyNet/mcmc/mcmc.h"
#include "FastMIDyNet/rng.h"


using namespace std;

namespace FastMIDyNet{

class DummySBM: public StochasticBlockModelFamily{
    size_t size;
    size_t edgeCount;
    size_t blockCount;

    BlockCountPoissonPrior blockCountPrior;
    BlockUniformPrior blockPrior;
    EdgeCountDeltaPrior edgeCountPrior;
    EdgeMatrixUniformPrior edgeMatrixPrior;

public:
    DummySBM(size_t size=10, size_t edgeCount=25, size_t blockCount=5):
    StochasticBlockModelFamily(size),
    blockCountPrior(blockCount),
    blockPrior(size, blockCountPrior),
    edgeCountPrior(edgeCount),
    edgeMatrixPrior(edgeCountPrior, blockPrior)
     {
        setBlockPrior(blockPrior);
        setEdgeMatrixPrior(edgeMatrixPrior);
    }
    using StochasticBlockModelFamily::sample;
};

class DummyGraphPrior: public ErdosRenyiFamily{
public:
    EdgeCountDeltaPrior prior;
    DummyGraphPrior(size_t size=10, size_t edgeCount=25):
    ErdosRenyiFamily(size), prior(edgeCount) { setEdgeCountPrior(prior); }
};

class DummyDynamics: public SISDynamics<RandomGraph>{
public:
    DummyDynamics(RandomGraph& graphPrior, size_t numSteps=10, double infection = 0.1):
    SISDynamics<RandomGraph>(graphPrior, numSteps, infection){}
};

class DummyLabeledDynamics: public SISDynamics<VertexLabeledRandomGraph<BlockIndex>>{
public:
    DummyLabeledDynamics(VertexLabeledRandomGraph<BlockIndex>& graphPrior, size_t numSteps=10, double infection = 0.1):
    SISDynamics<VertexLabeledRandomGraph<BlockIndex>>(graphPrior, numSteps, infection){}
};



class DummyMCMC: public MCMC{
public:
    bool doMetropolisHastingsStep() override {
        onStepBegin();
        m_lastLogJointRatio = 0;
        m_lastLogAcceptance = -log(2);
        if (m_uniform(rng) < exp(m_lastLogAcceptance))
            m_isLastAccepted = true;
        else
            m_isLastAccepted = false;
            onStepEnd();
        return m_isLastAccepted;

    }
    void sample() override { }
    void samplePrior() override { }
    const double getLogLikelihood() const override { return 1; }
    const double getLogPrior() const override { return 2; }
    const double getLogJoint() const override { return getLogLikelihood() + getLogPrior(); }
};



}
