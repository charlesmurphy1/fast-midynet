#include <algorithm>
#include <random>
#include <string>
#include <vector>

#include "FastMIDyNet/prior/sbm/block.h"
#include "FastMIDyNet/utility/functions.h"
#include "FastMIDyNet/utility/maps.hpp"
#include "FastMIDyNet/proposer/movetypes.h"
#include "FastMIDyNet/rng.h"
#include "FastMIDyNet/generators.h"
#include "FastMIDyNet/exceptions.h"


using namespace std;


namespace FastMIDyNet{

CounterMap<size_t> BlockPrior::computeVertexCountsInBlocks(const BlockSequence& state) {
    size_t blockCount = *max_element(state.begin(), state.end()) + 1;

    CounterMap<size_t> vertexCount;
    for (auto blockIdx: state) {
        vertexCount.increment(blockIdx);
    }

    return vertexCount;
}

void BlockPrior::checkBlockSequenceConsistencyWithBlockCount(const BlockSequence& blockSeq, size_t expectedBlockCount) {
    // size_t actualBlockCount = *max_element(blockSeq.begin(), blockSeq.end()) + 1;
    // if (actualBlockCount < expectedBlockCount)
    //     throw ConsistencyError("BlockPrior: blockSeq is inconsistent with expected block count.");

}

void BlockPrior::checkBlockSequenceConsistencyWithVertexCountsInBlocks(const BlockSequence& blockSeq, CounterMap<size_t> expectedVertexCountsInBlocks) {
    CounterMap<size_t> actualVertexCountsInBlocks = computeVertexCountsInBlocks(blockSeq);
    if (actualVertexCountsInBlocks.size() != expectedVertexCountsInBlocks.size())
        throw ConsistencyError("BlockPrior: size of vertex count in blockSeq is inconsistent with expected block count.");

    for (size_t i=0; i<actualVertexCountsInBlocks.size(); ++i){
        auto x = actualVertexCountsInBlocks[i];
        auto y = expectedVertexCountsInBlocks[i];
        if (x != y){
            throw ConsistencyError("BlockPrior: actual vertex count at index "
            + to_string(i) + " is inconsistent with expected vertex count: "
            + to_string(x) + " != " + to_string(y) + ".");
        }
    }

}

void BlockUniformPrior::sampleState() {
    BlockSequence blockSeq(getSize());
    uniform_int_distribution<size_t> dist(0, getBlockCount() - 1);
    for (size_t vertexIdx = 0; vertexIdx < getSize(); vertexIdx++) {
        blockSeq[vertexIdx] = dist(rng);
    }
    setState(blockSeq);
}


const double BlockUniformPrior::getLogLikelihood() const {
    return -logMultisetCoefficient(getSize(), getBlockCount());
}

const double BlockUniformPrior::getLogLikelihoodRatioFromBlockMove(const BlockMove& move) const {
    size_t prevNumBlocks = m_blockCountPriorPtr->getState();
    size_t newNumBlocks = m_blockCountPriorPtr->getStateAfterBlockMove(move);
    double logLikelihoodRatio = 0;
    logLikelihoodRatio += -logMultisetCoefficient(getSize(), newNumBlocks);
    logLikelihoodRatio -= -logMultisetCoefficient(getSize(), prevNumBlocks);
    return logLikelihoodRatio;
}

void BlockUniformHyperPrior::sampleState() {

    list<size_t> vertexCountList = sampleRandomComposition(getSize(), getBlockCount());
    vector<size_t> vertexCounts;
    for (auto nr : vertexCountList){
        vertexCounts.push_back(nr);
    }

    BlockSequence blockSeq = sampleRandomPermutation( vertexCounts );
    setState(blockSeq);
}


const double BlockUniformHyperPrior::getLogLikelihood() const {
    return -logMultinomialCoefficient(getVertexCountsInBlocks().values()) - logBinomialCoefficient(getSize() - 1, getBlockCount() - 1);

}

const double BlockUniformHyperPrior::getLogLikelihoodRatioFromBlockMove(const BlockMove& move) const {
    const auto& nr = getVertexCountsInBlocks();
    double logLikelihoodRatio = 0;
    logLikelihoodRatio -= log(nr[move.prevBlockIdx]);
    if (move.addedBlocks == 0)
        logLikelihoodRatio += log(nr[move.nextBlockIdx] + 1);
    logLikelihoodRatio -= logBinomialCoefficient(getSize() - 1, getBlockCount() - 1 + move.addedBlocks);
    logLikelihoodRatio += logBinomialCoefficient(getSize() - 1, getBlockCount() - 1);
    return logLikelihoodRatio;
}

}
