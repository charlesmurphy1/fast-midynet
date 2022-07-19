#ifndef FAST_MIDYNET_RANDOM_GRAPH_H
#define FAST_MIDYNET_RANDOM_GRAPH_H

// #include <random>
#include <vector>

#include "FastMIDyNet/types.h"
#include "FastMIDyNet/rv.hpp"
#include "FastMIDyNet/proposer/movetypes.h"
#include "FastMIDyNet/prior/prior.hpp"
#include "FastMIDyNet/prior/sbm/block.h"
#include "FastMIDyNet/utility/maps.hpp"
#include "FastMIDyNet/random_graph/likelihood/likelihood.hpp"

namespace FastMIDyNet{

class RandomGraph: public NestedRandomVariable{
private:

protected:
    GraphLikelihoodModel* m_likelihoodModelPtr = nullptr;
    size_t m_size;
    MultiGraph m_graph;
    virtual void _applyGraphMove(const GraphMove&);
    virtual const double _getLogPrior() const { return 0; }
    virtual const double _getLogPriorRatioFromGraphMove(const GraphMove& move) const { return 0; }
    virtual void _samplePrior() { };
    virtual void setUpLikelihood() {
        m_likelihoodModelPtr->m_graphPtr = &m_graph;
    }
public:
    RandomGraph(size_t size, GraphLikelihoodModel& likelihoodModel):
        m_size(size), m_graph(size), m_likelihoodModelPtr(&likelihoodModel) { }
    const MultiGraph& getGraph() const { return m_graph; }

    virtual void setGraph(const MultiGraph state) {
        m_graph = state;
    }
    const size_t getSize() const { return m_size; }
    void setSize(const size_t size) { m_size = size; }
    virtual const size_t& getEdgeCount() const = 0;
    const double getAverageDegree() const {
        double avgDegree = 2 * (double) getEdgeCount();
        avgDegree /= (double) getSize();
        return avgDegree;
    }

    void sample() {
        samplePrior();
        sampleState();
    }
    virtual void sampleState() = 0;
    void samplePrior() {  processRecursiveFunction([&](){ _samplePrior(); }); };

    const double getLogLikelihood() const { return m_likelihoodModelPtr->getLogLikelihood(); }
    const double getLogLikelihoodRatioFromGraphMove (const GraphMove& move) const { return m_likelihoodModelPtr->getLogLikelihoodRatioFromGraphMove(move); }

    const double getLogPrior() const {
        return processRecursiveFunction<double>([&](){ return _getLogPrior();}, 0);
    }
    const double getLogPriorRatioFromGraphMove (const GraphMove& move) const {
        return processRecursiveConstFunction<double>([&](){return _getLogPriorRatioFromGraphMove(move);}, 0);
    }

    const double getLogJoint() const {
        return getLogLikelihood() + getLogPrior();
    }
    const double getLogJointRatioFromGraphMove (const GraphMove& move) const{
        return getLogPriorRatioFromGraphMove(move) + getLogLikelihoodRatioFromGraphMove(move);
    }

    void applyGraphMove(const GraphMove& move){
        processRecursiveFunction([&](){ _applyGraphMove(move); });
        #if DEBUG
        checkConsistency();
        #endif
    }

    virtual const bool isCompatible(const MultiGraph& graph) const { return graph.getSize() == m_size; }
    virtual bool isSafe() const override { return m_likelihoodModelPtr and m_likelihoodModelPtr->isSafe(); }
    void checkSelfSafety() const override {
        if (not m_likelihoodModelPtr)
            throw SafetyError("RandomGraph: unsafe likelihood model with value `nullptr`.");
    }
    virtual void checkSelfConsistency() const override {
        m_likelihoodModelPtr->checkConsistency();
    }
};

template <typename Label>
class VertexLabeledRandomGraph: public RandomGraph{
private:
    VertexLabeledGraphLikelihoodModel<Label>* m_vertexLabeledlikelihoodModelPtr = nullptr;
protected:
    virtual void _applyLabelMove(const LabelMove<Label>&) { };
    virtual const double _getLogPriorRatioFromLabelMove (const LabelMove<Label>& move) const { }
public:
    VertexLabeledRandomGraph(size_t size, VertexLabeledGraphLikelihoodModel<Label>& likelihoodModel):
        RandomGraph(size, likelihoodModel), m_vertexLabeledlikelihoodModelPtr(&likelihoodModel){ }
    virtual const std::vector<Label>& getLabels() const = 0;
    virtual const size_t getLabelCount() const = 0;
    virtual const CounterMap<Label>& getLabelCounts() const = 0;
    virtual const CounterMap<Label>& getEdgeLabelCounts() const = 0;
    virtual const MultiGraph& getLabelGraph() const = 0;
    const Label& getLabelOfIdx(BaseGraph::VertexIndex vertexIdx) const { return getLabels()[vertexIdx]; }

    virtual void setLabels(const std::vector<Label>&) = 0;
    virtual void sampleLabels() = 0;

    const double getLogLikelihoodRatioFromLabelMove (const LabelMove<Label>& move) const {
        return m_vertexLabeledlikelihoodModelPtr->getLogLikelihoodRatioFromLabelMove(move);
    }
    const double getLogPriorRatioFromLabelMove (const LabelMove<Label>& move) const {
        return processRecursiveConstFunction<double>([&]() { return _getLogPriorRatioFromLabelMove(move); }, 0);
    }
    const double getLogJointRatioFromLabelMove (const LabelMove<Label>& move) const{
        return getLogPriorRatioFromLabelMove(move) + getLogLikelihoodRatioFromLabelMove(move);
    }
    void applyLabelMove(const LabelMove<Label>& move) {
        processRecursiveFunction([&](){ _applyLabelMove(move); });
        #if DEBUG
        checkConsistency();
        #endif
    }
};

using BlockLabeledRandomGraph = VertexLabeledRandomGraph<BlockIndex>;

} // namespace FastMIDyNet

#endif
