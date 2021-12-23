#ifndef FAST_MIDYNET_PYWRAPPER_INIT_MCMCBASECLASS_H
#define AST_MIDYNET_PYWRAPPER_INIT_MCMCBASECLASS_H

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "FastMIDyNet/mcmc/mcmc.h"
#include "FastMIDyNet/mcmc/graph_mcmc.h"
#include "FastMIDyNet/mcmc/dynamics_mcmc.h"
#include "FastMIDyNet/mcmc/python/mcmc.hpp"
#include "FastMIDyNet/proposer/block_proposer/block_proposer.h"
#include "FastMIDyNet/proposer/edge_proposer/edge_proposer.h"

namespace py = pybind11;
namespace FastMIDyNet{

void initMCMCBaseClass(py::module& m){
    py::class_<MCMC, PyMCMC<>>(m, "MCMC")
        .def(py::init<std::vector<CallBack*>>(), py::arg("callbacks"))
        .def(py::init<const CallBackList&>(), py::arg("callback_list"))
        .def("get_last_log_joint_ratio", &MCMC::getLastLogJointRatio)
        .def("get_last_log_acceptance", &MCMC::getLastLogAcceptance)
        .def("is_last_accepted", &MCMC::isLastAccepted)
        .def("get_graph", &MCMC::getGraph)
        .def("has_state", &MCMC::hasState)
        .def("get_num_steps", &MCMC::getNumSteps)
        .def("get_num_sweeps", &MCMC::getNumSweeps)
        .def("get_log_likelihood", &MCMC::getLogLikelihood)
        .def("get_log_prior", &MCMC::getLogPrior)
        .def("get_log_joint", &MCMC::getLogJoint)
        .def("sample", &MCMC::sample)
        .def("add_callback", &MCMC::addCallBack, py::arg("callback"))
        .def("remove_callback", &MCMC::removeCallBack, py::arg("idx"))
        .def("set_up", &MCMC::setUp)
        .def("tear_down", &MCMC::tearDown)
        .def("do_metropolis_hastings_step", &MCMC::doMetropolisHastingsStep)
        .def("do_MH_sweep", &MCMC::doMHSweep, py::arg("burn")=1);

    py::class_<RandomGraphMCMC, MCMC, PyRandomGraphMCMC<>>(m, "RandomGraphMCMC")
        .def(py::init<RandomGraph&, double, double, const CallBackList&>(),
            py::arg("random_graph"), py::arg("beta_likelihood")=1, py::arg("beta_prior")=1,
            py::arg("callbacks") ) ;

    py::class_<StochasticBlockGraphMCMC, RandomGraphMCMC>(m, "StochasticBlockGraphMCMC")
        .def(py::init<StochasticBlockModelFamily&, BlockProposer&, double, double, const CallBackList&>(),
            py::arg("random_graph"), py::arg("block_proposer"),
            py::arg("beta_likelihood")=1, py::arg("beta_prior")=1, py::arg("callbacks") ) ;

    py::class_<DynamicsMCMC, MCMC>(m, "DynamicsMCMC")
        .def(py::init<Dynamics&, RandomGraphMCMC&, EdgeProposer&, double, double, double, const CallBackList&>(),
            py::arg("dynamics"), py::arg("random_graph_mcmc"), py::arg("edge_proposer"),
            py::arg("beta_likelihood")=1, py::arg("beta_prior")=1, py::arg("sample_graph_prior")=0.5,
            py::arg("callbacks") ) ;
}

}

#endif
