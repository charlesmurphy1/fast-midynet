import typing
from netrd.distance import BaseDistance

from basegraph.core import UndirectedMultigraph
from _midynet.mcmc import DynamicsMCMC
from midynet.util import convert_basegraph_to_networkx

#
class MCMCConvergenceAnalysis:
    def __init__(
        self,
        mcmc: DynamicsMCMC,
        distance: BaseDistance,
    ):
        self.mcmc = mcmc
        self.distance = distance
        self.collected = []

    def collect(self, num_samples=100, numsteps_between_samples=10):
        original_graph = convert_basegraph_to_networkx(self.mcmc.get_graph())
        for i in range(num_samples):
            self.mcmc.do_MH_sweep(numsteps_between_samples)
            current_graph = convert_basegraph_to_networkx(self.mcmc.get_graph())
            self.collected.append(self.distance.dist(original_graph, current_graph))
        return self.collected

    def clear(self):
        self.collected.clear()