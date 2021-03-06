#ifndef FAST_MIDYNET_PYWRAPPER_INIT_SBMPRIOR_DEGREE_H
#define FAST_MIDYNET_PYWRAPPER_INIT_SBMPRIOR_DEGREE_H

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>

#include "FastMIDyNet/prior/python/prior.hpp"
#include "FastMIDyNet/prior/sbm/block.h"
#include "FastMIDyNet/prior/sbm/edge_matrix.h"
#include "FastMIDyNet/prior/sbm/degree.h"
#include "FastMIDyNet/prior/sbm/python/degree.hpp"


namespace py = pybind11;
namespace FastMIDyNet{

void initDegreePrior(py::module& m){
    py::class_<DegreePrior, BlockLabeledPrior<std::vector<size_t>>, PyDegreePrior<>>(m, "DegreePrior")
        .def(py::init<BlockPrior&, EdgeMatrixPrior&>(), py::arg("block_prior"), py::arg("edge_matrix"))
        .def("get_degree_of_idx", &DegreePrior::getDegreeOfIdx)
        .def("get_degree_counts", &DegreePrior::getDegreeCounts)
        .def("set_partition", &DegreePrior::setPartition)
        .def("get_block_prior", &DegreePrior::getBlockPrior)
        .def("set_block_prior", &DegreePrior::setBlockPrior, py::arg("block_prior"))
        .def("get_edge_matrix_prior", &DegreePrior::getEdgeMatrixPrior)
        .def("set_edge_matrix_prior", &DegreePrior::setEdgeMatrixPrior, py::arg("edge_matrix_prior"))
        ;

    py::class_<DegreeDeltaPrior, DegreePrior>(m, "DegreeDeltaPrior")
        .def(py::init<>())
        .def(py::init<const DegreeSequence&>(), py::arg("degrees"))
        ;

    py::class_<DegreeUniformPrior, DegreePrior>(m, "DegreeUniformPrior")
        .def(py::init<>())
        .def(py::init<BlockPrior&, EdgeMatrixPrior&>(), py::arg("block_prior"), py::arg("edge_matrix"));

    py::class_<DegreeUniformHyperPrior, DegreePrior>(m, "DegreeUniformHyperPrior")
        .def(py::init<>())
        .def(py::init<BlockPrior&, EdgeMatrixPrior&>(), py::arg("block_prior"), py::arg("edge_matrix"));


}

}

#endif
