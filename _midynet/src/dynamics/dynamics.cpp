#include <chrono>
#include <cmath>
#include <map>

#include "BaseGraph/types.h"
#include "FastMIDyNet/dynamics/dynamics.h"
#include "FastMIDyNet/utility/functions.h"
#include "FastMIDyNet/utility/maps.hpp"
#include "FastMIDyNet/generators.h"
#include "FastMIDyNet/rng.h"
#include "FastMIDyNet/types.h"


using namespace std;
using namespace BaseGraph;


namespace FastMIDyNet {
void Dynamics::sampleState(const State& x0, bool async){
    m_state = x0;
    m_neighborsState = computeNeighborsState(m_state);

    StateSequence reversedPastState;
    StateSequence reversedFutureState;
    NeighborsStateSequence reversedNeighborsPastState;
    for (size_t t = 0; t < m_numSteps; t++) {
        reversedPastState.push_back(m_state);
        reversedNeighborsPastState.push_back(m_neighborsState);
        if (async) { asyncUpdateState(getSize()); }
        else { syncUpdateState(); }
        reversedFutureState.push_back(m_state);
    }


    m_pastStateSequence.clear();
    m_pastStateSequence.resize(getSize());
    m_futureStateSequence.clear();
    m_futureStateSequence.resize(getSize());
    m_neighborsPastStateSequence.clear();
    m_neighborsPastStateSequence.resize(getSize());
    for (const auto& idx : getGraph()){
        m_pastStateSequence[idx].resize(m_numSteps);
        m_futureStateSequence[idx].resize(m_numSteps);
        m_neighborsPastStateSequence[idx].resize(m_numSteps);
        for (size_t t = 0; t < m_numSteps; t++){
            m_pastStateSequence[idx][t] = reversedPastState[t][idx];
            m_futureStateSequence[idx][t] = reversedFutureState[t][idx];
            m_neighborsPastStateSequence[idx][t] = reversedNeighborsPastState[t][idx];
        }
    }

    #if DEBUG
    checkConsistency();
    #endif

}

void Dynamics::setGraph(const MultiGraph& graph) {
    m_randomGraphPtr->setGraph(graph);
    if (m_pastStateSequence.size() == 0)
        return;
    m_neighborsState = computeNeighborsState(m_state);
    m_neighborsPastStateSequence = computeNeighborsStateSequence(m_pastStateSequence);

    #if DEBUG
    checkConsistency();
    #endif
}

const State Dynamics::getRandomState() const{
    size_t N = m_randomGraphPtr->getSize();
    State rnd_state(N);
    uniform_int_distribution<int> dist(0, m_numStates - 1);

    for (size_t i = 0 ; i < N ; i ++)
        rnd_state[i] = dist(rng);

    return rnd_state;
};

const NeighborsState Dynamics::computeNeighborsState(const State& state) const {
    size_t N = m_randomGraphPtr->getSize();
    NeighborsState neighborsState(N);
    int neighborIdx, edgeMultiplicity;

    for ( auto idx: getGraph() ){
        neighborsState[idx].resize(m_numStates);
        for ( auto neighbor: getGraph().getNeighboursOfIdx(idx) ){
            neighborIdx = neighbor.vertexIndex;
            edgeMultiplicity = neighbor.label;
            neighborsState[ idx ][ state[neighborIdx] ] += edgeMultiplicity;
        }
    }
    return neighborsState;
};

const NeighborsStateSequence Dynamics::computeNeighborsStateSequence(const StateSequence& stateSequence) const {

    NeighborsStateSequence neighborsStateSequence(getSize());
    for ( const auto& idx: getGraph() ){
        neighborsStateSequence[idx].resize(m_numSteps);
        for (size_t t=0; t<m_numSteps; t++){
            neighborsStateSequence[idx][t].resize(m_numStates);
            for ( const auto& neighbor: getGraph().getNeighboursOfIdx(idx) )
                neighborsStateSequence[ idx ][t][ stateSequence[neighbor.vertexIndex][t] ] += neighbor.label;
        }
    }
    return neighborsStateSequence;
};


void Dynamics::updateNeighborsStateInPlace(
    VertexIndex vertexIdx,
    VertexState prevVertexState,
    VertexState newVertexState,
    NeighborsState& neighborsState) const {
    int neighborIdx, edgeMultiplicity;
    if (prevVertexState == newVertexState)
        return;
    for ( auto neighbor: getGraph().getNeighboursOfIdx(vertexIdx) ){
        neighborIdx = neighbor.vertexIndex;
        edgeMultiplicity = neighbor.label;
        neighborsState[neighborIdx][prevVertexState] -= edgeMultiplicity;
        neighborsState[neighborIdx][newVertexState] += edgeMultiplicity;
    }
};

void Dynamics::syncUpdateState(){
    State futureState(m_state);
    vector<double> transProbs(m_numStates);

    for (const auto idx: getGraph()){
        transProbs = getTransitionProbs(m_state[idx], m_neighborsState[idx]);
        futureState[idx] = generateCategorical<double, size_t>(transProbs);
    }
    for (const auto idx: getGraph())
        updateNeighborsStateInPlace(idx, m_state[idx], futureState[idx], m_neighborsState);
    m_state = futureState;
};

void Dynamics::asyncUpdateState(int numUpdates){
    size_t N = m_randomGraphPtr->getSize();
    VertexState newVertexState;
    State currentState(m_state);
    vector<double> transProbs(m_numStates);
    uniform_int_distribution<VertexIndex> idxGenerator(0, N-1);

    for (auto i=0; i < numUpdates; i++){
        VertexIndex idx = idxGenerator(rng);
        transProbs = getTransitionProbs(currentState[idx], m_neighborsState[idx]);
        newVertexState = generateCategorical<double, size_t>(transProbs);
        updateNeighborsStateInPlace(idx, currentState[idx], newVertexState, m_neighborsState);
        currentState[idx] = newVertexState;
    }
    m_state = currentState;
};

const double Dynamics::getLogLikelihood() const {
    double logLikelihood = 0;
    vector<int> neighborsState(getNumStates(), 0);
    int neighborIdx, edgeMultiplicity;
    for (size_t t = 0; t < m_numSteps; t++){
        for (auto idx: getGraph()){
            logLikelihood += log(getTransitionProb(
                m_pastStateSequence[idx][t],
                m_futureStateSequence[idx][t],
                m_neighborsPastStateSequence[idx][t]
            ));
        }
    }
    return logLikelihood;
};

const std::vector<double> Dynamics::getTransitionProbs(VertexState prevVertexState, VertexNeighborhoodState neighborhoodState) const{
    std::vector<double> transProbs(getNumStates());
    for (VertexState nextVertexState = 0; nextVertexState < getNumStates(); nextVertexState++) {
        transProbs[nextVertexState] = getTransitionProb(prevVertexState, nextVertexState, neighborhoodState);
    }
    return transProbs;
};

void Dynamics::updateNeighborsStateFromEdgeMove(
    BaseGraph::Edge edge,
    int counter,
    map<VertexIndex, VertexNeighborhoodStateSequence>& prevNeighborMap,
    map<VertexIndex, VertexNeighborhoodStateSequence>& nextNeighborMap) const{
    edge = getOrderedEdge(edge);
    VertexIndex v = edge.first, u = edge.second;

    if (m_randomGraphPtr->getGraph().getEdgeMultiplicityIdx(edge) == 0 and counter < 0)
        throw std::logic_error("Dynamics: Edge ("
                                + std::to_string(edge.first) + ", "
                                + std::to_string(edge.second) + ") "
                                + "with multiplicity 0 cannot be removed.");


    if (prevNeighborMap.count(v) == 0){
        prevNeighborMap.insert({v, m_neighborsPastStateSequence[v]}) ;
        nextNeighborMap.insert({v, m_neighborsPastStateSequence[v]}) ;
    }
    if (prevNeighborMap.count(u) == 0){
        prevNeighborMap.insert({u, m_neighborsPastStateSequence[u]}) ;
        nextNeighborMap.insert({u, m_neighborsPastStateSequence[u]}) ;
    }

    VertexState vState, uState;
    for (size_t t = 0; t < m_numSteps; t++) {
        uState = m_pastStateSequence[u][t];
        vState = m_pastStateSequence[v][t];
        nextNeighborMap[u][t][vState] += counter;
        if (u != v)
            nextNeighborMap[v][t][uState] += counter;
    }
};

const double Dynamics::getLogLikelihoodRatioFromGraphMove(const GraphMove& move) const{
    double logLikelihoodRatio = 0;
    set<size_t> verticesAffected;
    map<VertexIndex,VertexNeighborhoodStateSequence> prevNeighborMap, nextNeighborMap;


    for (const auto& edge : move.addedEdges){
        size_t v = edge.first, u = edge.second;
        verticesAffected.insert(v);
        verticesAffected.insert(u);
        updateNeighborsStateFromEdgeMove(edge, 1, prevNeighborMap, nextNeighborMap);
    }
    for (const auto& edge : move.removedEdges){
        size_t v = edge.first, u = edge.second;
        verticesAffected.insert(v);
        verticesAffected.insert(u);
        updateNeighborsStateFromEdgeMove(edge, -1, prevNeighborMap, nextNeighborMap);
    }

    for (const auto& idx: verticesAffected){
        for (size_t t = 0; t < m_numSteps; t++) {
            logLikelihoodRatio += log(
                getTransitionProb(m_pastStateSequence[idx][t], m_futureStateSequence[idx][t], nextNeighborMap[idx][t])
            );
            logLikelihoodRatio -= log(
                getTransitionProb(m_pastStateSequence[idx][t], m_futureStateSequence[idx][t], prevNeighborMap[idx][t])
            );
        }
    }

    return logLikelihoodRatio;
}


void Dynamics::_applyGraphMove(const GraphMove& move) {
    set<VertexIndex> verticesAffected;
    map<VertexIndex, VertexNeighborhoodStateSequence> prevNeighborMap, nextNeighborMap;
    VertexNeighborhoodStateSequence neighborsState(m_numSteps);
    size_t v, u;

    for (const auto& edge : move.addedEdges){
        v = edge.first;
        u = edge.second;
        verticesAffected.insert(v);
        verticesAffected.insert(u);
        updateNeighborsStateFromEdgeMove(edge, 1, prevNeighborMap, nextNeighborMap);
        m_neighborsState[u][m_state[v]] += 1;
        if (u != v)
            m_neighborsState[v][m_state[u]] += 1;
    }
    for (const auto& edge : move.removedEdges){
        v = edge.first;
        u = edge.second;
        verticesAffected.insert(v);
        verticesAffected.insert(u);
        updateNeighborsStateFromEdgeMove(edge, -1, prevNeighborMap, nextNeighborMap);
        m_neighborsState[u][m_state[v]] -= 1;
        if (u != v)
            m_neighborsState[v][m_state[u]] -= 1;
    }

    for (const auto& idx: verticesAffected){
        for (size_t t = 0; t < m_numSteps; t++) {
            m_neighborsPastStateSequence[idx][t] = nextNeighborMap[idx][t];
        }
    }
    m_randomGraphPtr->applyGraphMove(move);
}

void Dynamics::checkConsistencyOfNeighborsPastStateSequence() const {
    if (m_neighborsPastStateSequence.size() == 0)
        return;
    else if (m_neighborsPastStateSequence.size() != getSize())
            throw ConsistencyError("Dynamics: `m_neighborsPastStateSequence` is inconsistent with past states with size "
                + std::to_string(m_neighborsPastStateSequence.size())
                + ", expected size " + std::to_string(getSize()) + ".");
    const auto& actual = m_neighborsPastStateSequence;
    const auto expected = computeNeighborsStateSequence(m_pastStateSequence);
    for (size_t v=0; v<getSize(); ++v){
        if (actual[v].size() != getNumSteps())
            throw ConsistencyError("Dynamics: `m_neighborsPastStateSequence` is inconsistent with past states at (v="
                + std::to_string(v) + ") with size " + std::to_string(m_neighborsPastStateSequence[v].size())
                + ", expected size " + std::to_string(getNumSteps()) + ".");
        for (size_t t=0; t<getSize(); ++t){
            if (actual[v][t].size() != getNumStates())
                throw ConsistencyError("Dynamics: `m_neighborsPastStateSequence` is inconsistent with past states at (v="
                    + std::to_string(v) + ", t=" + std::to_string(t) + ") with size " + std::to_string(m_neighborsPastStateSequence[t][v].size())
                    + ", expected size " + std::to_string(getNumStates()) + ".");
            for (size_t s=0; s<m_numStates; ++s){
                if (actual[v][t][s] != expected[v][t][s])
                    throw ConsistencyError("Dynamics: `m_neighborsPastStateSequence` is inconsistent with past states at (v="
                        + std::to_string(v) + ", t=" + std::to_string(t) + ", s=" + std::to_string(s)
                        + ") with value " + std::to_string(actual[v][t][s])
                        + ", expected value" + std::to_string(expected[v][t][s]) + ".");
            }
        }
    }
}

void Dynamics::checkConsistencyOfNeighborsState() const {
    if (m_neighborsState.size() == 0)
        return;
    else if (m_neighborsState.size() != getSize())
        throw ConsistencyError("Dynamics: `m_neighborsState` is inconsistent with `m_states` with size "
            + std::to_string(m_neighborsState.size()) + ", expected size " + std::to_string(getSize()) + ".");
    const auto& actual = m_neighborsState;
    const auto expected = computeNeighborsState(m_state);
    for (size_t v=0; v<getSize(); ++v){
        if (actual[v].size() != getNumStates())
            throw ConsistencyError("Dynamics: `m_neighborsState` is inconsistent with `m_states` at (v="
                + std::to_string(v) + "_ with size " + std::to_string(m_neighborsState[v].size())
                + ", expected size " + std::to_string(getNumStates()) + ".");
        for (size_t s=0; s<m_numStates; ++s){
            if (actual[v][s] != expected[v][s])
                throw ConsistencyError("Dynamics: `m_neighborsState` is inconsistent with `m_states` at (v="
                    + std::to_string(v) + ", s=" + std::to_string(s) +
                    + ") with value " + std::to_string(actual[v][s])
                    + ", expected value" + std::to_string(expected[v][s]) + ".");
        }
    }

}

void Dynamics::checkSelfConsistency() const {
    checkConsistencyOfNeighborsPastStateSequence();
    checkConsistencyOfNeighborsState();
}

void Dynamics::checkSelfSafety() const {
    if (m_randomGraphPtr == nullptr)
        throw SafetyError("Dynamics: unsafe graph family since `m_randomGraphPtr` is empty.");
    m_randomGraphPtr->checkSafety();

    if (m_state.size() == 0)
        throw SafetyError("Dynamics: unsafe graph family since `m_state` is empty.");
    if (m_pastStateSequence.size() == 0)
        throw SafetyError("Dynamics: unsafe graph family since `m_pastStateSequence` is empty.");
    if (m_futureStateSequence.size() == 0)
        throw SafetyError("Dynamics: unsafe graph family since `m_futureStateSequence` is empty.");
    if (m_neighborsPastStateSequence.size() == 0)
        throw SafetyError("Dynamics: unsafe graph family since `m_neighborsPastStateSequence` is empty.");
}

}
