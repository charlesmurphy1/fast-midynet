#ifndef FAST_MIDYNET_BLOCK_COUNT_H
#define FAST_MIDYNET_BLOCK_COUNT_H

#include "FastMIDyNet/exceptions.h"
#include "FastMIDyNet/types.h"
#include "FastMIDyNet/rng.h"
#include "FastMIDyNet/prior/prior.hpp"


namespace FastMIDyNet{

class BlockCountPrior: public BlockLabeledPrior<size_t> {
public:
    using BlockLabeledPrior<size_t>::BlockLabeledPrior;
    void samplePriors() override { }
    virtual const double getLogLikelihoodFromState(const size_t&) const = 0;
    const double getLogLikelihood() const override { return getLogLikelihoodFromState(m_state); }
    const double getLogPrior() const override { return 0; }
    const double getLogLikelihoodRatioFromGraphMove(const GraphMove& move) const { return 0; }
    const double getLogLikelihoodRatioFromLabelMove(const BlockMove& move) const { throw std::logic_error("BlockCount: this method should not be used."); }
    void setStateFromPartition(const BlockSequence& blocks) { setState(*max_element(blocks.begin(), blocks.end()) + 1);}
protected:
    void _applyGraphMove(const GraphMove& move) override { }
    void _applyLabelMove(const BlockMove& move) override { throw std::logic_error("BlockCount: this method should not be used."); }
    const double _getLogJointRatioFromGraphMove(const GraphMove& move) const override { return 0; }
    const double _getLogJointRatioFromLabelMove(const BlockMove& move) const override { return getLogLikelihoodRatioFromLabelMove(move); }
};

class BlockCountDeltaPrior: public BlockCountPrior{
public:
    BlockCountDeltaPrior() {}
    BlockCountDeltaPrior(size_t blockCount) { setState(blockCount); }
    BlockCountDeltaPrior(const BlockCountDeltaPrior& other) { setState(other.getState()); }
    virtual ~BlockCountDeltaPrior() {};
    const BlockCountDeltaPrior& operator=(const BlockCountDeltaPrior&other){
        setState(other.m_state);
        return *this;
    }

    void sampleState() override { }

    const double getLogLikelihoodFromState(const size_t& blockCount) const override{
        return (blockCount != m_state) ? -INFINITY : 0;
    }
    void checkSelfConsistency() const override { };

    void checkSelfSafety() const override {
        if (m_state == 0)
            throw SafetyError("BlockCountDeltaPrior: unsafe prior since `m_blockCount` is zero.");
    }

};

class BlockCountPoissonPrior: public BlockCountPrior{
    double m_mean;
    std::poisson_distribution<size_t> m_poissonDistribution;

    public:
        BlockCountPoissonPrior() {}
        BlockCountPoissonPrior(double mean) { setMean(mean); }
        BlockCountPoissonPrior(const BlockCountPoissonPrior& other) { setMean(other.m_mean); setState(other.m_state); }
        virtual ~BlockCountPoissonPrior() {};
        const BlockCountPoissonPrior& operator=(const BlockCountPoissonPrior& other) {
            setMean(other.m_mean);
            setState(other.m_state);
            return *this;
        }

        const double getMean() const { return m_mean; }
        void setMean(double mean){
            m_mean = mean;
            m_poissonDistribution = std::poisson_distribution<size_t>(mean);
        }
        void sampleState() override;
        const double getLogLikelihoodFromState(const size_t& state) const override;

        void checkSelfConsistency() const override;
};

class BlockCountUniformPrior: public BlockCountPrior{
    size_t m_min, m_max;
    std::uniform_int_distribution<size_t> m_uniformDistribution;

    public:
        BlockCountUniformPrior() {}
        BlockCountUniformPrior(size_t min, size_t max) { setMinMax(min, max); }
        BlockCountUniformPrior(const BlockCountUniformPrior& other) { setMinMax(other.m_min, other.m_max); setState(other.m_state); }
        virtual ~BlockCountUniformPrior() {};
        const BlockCountUniformPrior& operator=(const BlockCountUniformPrior& other) {
            setMin(other.m_min);
            setMax(other.m_max);
            setState(other.m_state);
            return *this;
        }

        const double getMin() const { return m_min; }
        const double getMax() const { return m_max; }
        void setMin(size_t min){
            m_min = min;
            checkMin();
            m_uniformDistribution = std::uniform_int_distribution<size_t>(m_min, m_max);
        }
        void setMax(size_t max){
            m_max = max;
            checkMax();
            m_uniformDistribution = std::uniform_int_distribution<size_t>(m_min, m_max);
        }
        void setMinMax(size_t min, size_t max){
            setMin(min);
            setMax(max);
        }
        void sampleState() override { setState(m_uniformDistribution(rng)); }
        const double getLogLikelihoodFromState(const size_t& state) const override{
            if (state > m_max or state < m_min)
                return -INFINITY;
            return -log(m_max - m_min);
        };

        void checkMin() const;
        void checkMax() const;
        void checkSelfConsistency() const override;
};

}

#endif
