#ifndef FAST_MIDYNET_DYNAMICS_MCMC_H
#define FAST_MIDYNET_DYNAMICS_MCMC_H

#include "FastMIDyNet/dynamics/dynamics.h"
#include "FastMIDyNet/mcmc/mcmc.h"
#include "FastMIDyNet/mcmc/graph_mcmc.h"
#include "FastMIDyNet/mcmc/callbacks/callback.h"
#include "FastMIDyNet/proposer/edge_proposer/edge_proposer.h"

namespace FastMIDyNet{

class DynamicsMCMC: public MCMC{
private:
    Dynamics* m_dynamicsPtr;
    RandomGraphMCMC* m_randomGraphMCMCPtr;
    double m_betaLikelihood, m_betaPrior, m_sampleGraphPriorProb;
    std::uniform_real_distribution<double> m_uniform;
    bool m_lastMoveWasGraphMove;
public:
    DynamicsMCMC(
        Dynamics& dynamics,
        RandomGraphMCMC& randomGraphMCMC,
        double betaLikelihood=1,
        double betaPrior=1,
        double sampleGraphPriorProb=0.5,
        const CallBackList& callBacks={} ):
    MCMC(callBacks),
    m_dynamicsPtr(&dynamics),
    m_randomGraphMCMCPtr(&randomGraphMCMC),
    m_betaLikelihood(betaLikelihood),
    m_betaPrior(betaPrior),
    m_sampleGraphPriorProb(sampleGraphPriorProb),
    m_uniform(0., 1.) {}

    DynamicsMCMC(
        double betaLikelihood=1,
        double betaPrior=1,
        double sampleGraphPriorProb=0.5,
        const CallBackList& callBacks={}):
    MCMC(callBacks),
    m_sampleGraphPriorProb(sampleGraphPriorProb),
    m_uniform(0., 1.) {}

    void setUp() override {
        m_randomGraphMCMCPtr->setRandomGraph(m_dynamicsPtr->getRandomGraphRef());
        m_randomGraphMCMCPtr->setUp();
        MCMC::setUp();
    };

    const MultiGraph& getGraph() const override { return m_randomGraphMCMCPtr->getGraph(); }
    const BlockSequence& getBlocks() const override { return m_randomGraphMCMCPtr->getBlocks(); }
    const int getSize() const { return m_dynamicsPtr->getSize(); }

    double getBetaPrior() const { return m_betaPrior; }
    void setBetaPrior(double betaPrior) { m_betaPrior = betaPrior; }

    double getBetaLikelihood() const { return m_betaLikelihood; }
    void setBetaLikelihood(double betaLikelihood) { m_betaLikelihood = betaLikelihood; }

    double getSampleGraphPriorProb() const { return m_sampleGraphPriorProb; }
    void setSampleGraphPriorProb(double sampleGraphPriorProb) { m_sampleGraphPriorProb = sampleGraphPriorProb; }

    const RandomGraphMCMC& getRandomGraphMCMC() const { return *m_randomGraphMCMCPtr; }
    void setRandomGraphMCMC(RandomGraphMCMC& randomGraphMCMC) {
        m_randomGraphMCMCPtr = &randomGraphMCMC;
        if (m_dynamicsPtr != nullptr)
            m_randomGraphMCMCPtr->setRandomGraph(m_dynamicsPtr->getRandomGraphRef());
    }

    const Dynamics& getDynamics() const { return *m_dynamicsPtr; }
    void setDynamics(Dynamics& dynamics) {
        m_dynamicsPtr = &dynamics;
        if (m_randomGraphMCMCPtr != nullptr)
            m_randomGraphMCMCPtr->setRandomGraph(m_dynamicsPtr->getRandomGraphRef());
    }

    double getLogLikelihood() const override { return m_dynamicsPtr->getLogLikelihood(); }
    double getLogPrior() const override { return m_dynamicsPtr->getLogPrior(); }
    double getLogJoint() const override { return m_dynamicsPtr->getLogJoint(); }
    void sample() override {
        m_dynamicsPtr->sample();
        m_hasState=true;
    }
    void sampleState() {
        m_dynamicsPtr->sampleState();
        m_hasState=true;
    }
    void sampleGraph() {
        m_dynamicsPtr->sampleGraph();
    }
    void sampleGraphOnly() {
        m_randomGraphMCMCPtr->sampleGraphOnly();
    }

    void doMetropolisHastingsStep() override ;
};

}

#endif
