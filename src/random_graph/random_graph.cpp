#include <random>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <map>

#include "FastMIDyNet/random_graph/random_graph.h"
#include "FastMIDyNet/types.h"

using namespace std;

namespace FastMIDyNet {

    void RandomGraph::applyMove(const GraphMove& move){
        for (const auto& edge: move.edges_added){
            v_idx = edge.first;
            u_idx = edge.second;
            m_state.addEdgeIdx()
        }

    };
}
