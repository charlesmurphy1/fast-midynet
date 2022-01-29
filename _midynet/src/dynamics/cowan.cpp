#include "FastMIDyNet/dynamics/cowan.h"


static inline double sigmoid(double x) {
    return 1/(1+exp(-x));
}


namespace FastMIDyNet{

const double CowanDynamics::getActivationProb(const VertexNeighborhoodState& neighborState) const{
    return sigmoid(m_a*(getNu()*neighborState[1] - m_mu));
}

const double CowanDynamics::getDeactivationProb(const VertexNeighborhoodState& neighborState) const{
    return m_eta;
}

} // FastMIDyNet
