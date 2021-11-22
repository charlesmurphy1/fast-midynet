#ifndef FAST_MIDYNET_BLOCK_H
#define FAST_MIDYNET_BLOCK_H

#include <vector>

#include "FastMIDyNet/prior/prior.hpp"
#include "FastMIDyNet/prior/dcsbm/block_count.h"
#include "FastMIDyNet/proposer/movetypes.h"
#include "FastMIDyNet/types.h"

namespace FastMIDyNet{

class BlockPrior: public Prior<BlockSequence>{
    protected:
        size_t m_size;
        BlockCountPrior& m_blockCountPrior;
        std::vector<size_t> m_verticesInBlock;
    public:
        BlockPrior(size_t size, BlockCountPrior& blockCountPrior):
            m_size(size), m_blockCountPrior(blockCountPrior) { }

        void setState(const BlockSequence& blockSeq) override{
            m_blockCountPrior.setState(*max_element(blockSeq.begin(), blockSeq.end()) + 1);
            m_state = blockSeq;
            m_verticesInBlock = computeVertexCountInBlock(m_state);
        }
        void samplePriors() override { m_blockCountPrior.sample(); }
        const size_t& getBlockCount() const { return m_blockCountPrior.getState(); }
        std::vector<size_t> computeVertexCountInBlock(const BlockSequence&) const;
        const std::vector<size_t>& getVertexCountInBlock() const { return m_verticesInBlock; };
        const size_t& getSize() const { return m_size; }

        double getLogLikelihoodRatio(const GraphMove& move) const { return 0; };
        // getLogLikelihoodRatio(FastMIDyNet::BlockMove const&) const
        virtual double getLogLikelihoodRatio(const BlockMove& move) const = 0;

        double getLogPriorRatio(const GraphMove& move) { return 0; };
        virtual double getLogPriorRatio(const BlockMove& move) = 0;

        double getLogJointRatio(const GraphMove& move) { return 0; };
        virtual double getLogJointRatio(const BlockMove& move) = 0;

        void applyMove(const GraphMove&) { };
        virtual void applyMove(const BlockMove&) = 0;
        void computationFinished() override { m_isProcessed=false; m_blockCountPrior.computationFinished(); }

};

class BlockDeltaPrior: public BlockPrior{
    BlockCountDeltaPrior m_blockCountDeltaPrior;
public:
    BlockDeltaPrior(const BlockSequence& blockSeq):
        m_blockCountDeltaPrior(*max_element(blockSeq.begin(), blockSeq.end())),
        BlockPrior(blockSeq.size(), m_blockCountDeltaPrior) { }

    void sampleState() {  };

    void samplePriors() { };

    double getLogLikelihood(const BlockSequence& state) const {
        if (state.size() != getSize()) return -INFINITY;
        for (size_t i = 0; i < state.size(); i++) {
            if (state[i] != m_state[i]) return -INFINITY;
        }
        return 0.;
    };

    double getLogPrior() { return 0.; };

    void checkSelfConsistency() const { };

};

class BlockUniformPrior: public BlockPrior{
    public:
        BlockUniformPrior(size_t graphSize, BlockCountPrior& blockCountPrior):
            BlockPrior(graphSize, blockCountPrior) { }

        void sampleState();

        double getLogLikelihood(const BlockSequence& blockSeq) const ;

        double getLogPrior() { return m_blockCountPrior.getLogJoint(); };

        double getLogLikelihoodRatio(const BlockMove&) const;

<<<<<<< HEAD
    double getLogPriorRatio(const BlockMove& move) {
        return processRecursiveFunction<double>( [&]() { return m_blockCountPrior.getLogJointRatio(move); }, 0);
    };
=======
        double getLogPriorRatio(const BlockMove& move) {
            return processRecursiveFunction<double>( [&]() {
                    return m_blockCountPrior.getLogJointRatio(move); },
                    0);
        };
>>>>>>> 6315f3c71524ed5e975adafa723cd6fde146429b

        double getLogJointRatio(const BlockMove& move) {
            return getLogLikelihoodRatio(move) + getLogPriorRatio(move);
        };

        void applyMove(const BlockMove& move) { m_state[move.vertexIdx] = move.nextBlockIdx; };
        static void checkBlockSequenceConsistencyWithBlockCount(const BlockSequence& blockSeq, size_t expectedBlockCount) ;
        void checkSelfConsistency() const {
            checkBlockSequenceConsistencyWithBlockCount(m_state, getBlockCount());
        };

};

class BlockHyperPrior: public BlockPrior{

};

}

#endif
