import unittest
import numpy as np
import matplotlib.pyplot as plt

from netrd.distance import Hamming
from midynet.config import *
from midynet.util import convert_basegraph_to_networkx, MCMCConvergenceAnalysis
from _midynet.mcmc import DynamicsMCMC


class TestMCMCConvergence(unittest.TestCase):
    def setUp(self):
        self.config = ExperimentConfig.default("test", "sis", "er")
        self.config.dynamics.set_value("infection_prob", [0.0, 0.1, 0.2, 0.3, 0.4, 0.5])
        self.num_samples = 500
        self.numsteps_between_samples = 100

    @staticmethod
    def setup_convergence(config):
        g = RandomGraphFactory.build(config.graph)
        d = DynamicsFactory.build(config.dynamics)
        d.set_random_graph(g.get_wrap())
        g_mcmc = RandomGraphMCMCFactory.build(config.graph)
        mcmc = DynamicsMCMC(d, g_mcmc.get_wrap())
        mcmc.sample()
        mcmc.set_up()
        distance = Hamming()
        return Wrapper(
            MCMCConvergenceAnalysis(mcmc, distance),
            D_MCMC=mcmc,
            distance=distance,
            g_mcmc=g_mcmc,
            d=d,
            g=g,
        )

    def test_generic(self):
        for c in self.config.sequence():
            conv = TestMCMCConvergence.setup_convergence(c)
            conv.mcmc.sample()
            conv.mcmc.sample_graph()
            conv.mcmc.set_up()
            collected = conv.collect(self.num_samples, self.numsteps_between_samples)
            x = np.arange(self.num_samples) * self.numsteps_between_samples
            inf_prob = c.dynamics.infection_prob
            plt.plot(x, collected, label=rf"$\alpha = {inf_prob}$")

        plt.xlabel("Number of steps")
        plt.ylabel("Hamming distance")
        plt.legend()
        plt.savefig(
            f"./tests/util/convergence_{self.config.dynamics.name}_{self.config.graph.name}.png"
        )
        # self.D_MCMC.do_MH_sweep(burn=100)
