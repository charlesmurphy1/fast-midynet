import time
from dataclasses import dataclass, field
from _midynet.mcmc import DynamicsMCMC
from _midynet import utility
from midynet.config import (
    Config,
    DynamicsFactory,
    RandomGraphFactory,
    RandomGraphMCMCFactory,
)
from .metrics import Metrics
from .multiprocess import Expectation
from .statistics import Statistics
from .util import get_log_evidence

__all__ = ("Predictability", "PredictabilityMetrics")


@dataclass
class Predictability(Expectation):
    config: Config = field(repr=False, default_factory=Config)

    def func(self, seed: int) -> float:
        utility.seed(seed)
        graph = RandomGraphFactory.build(self.config.graph)
        dynamics = DynamicsFactory.build(self.config.dynamics)
        dynamics.set_random_graph(graph.get_wrap())
        random_graph_mcmc = RandomGraphMCMCFactory.build(self.config.graph)
        dynamics.sample()
        mcmc = DynamicsMCMC()
        mcmc.set_dynamics(dynamics)
        mcmc.set_random_graph_mcmc(random_graph_mcmc.get_wrap())
        mcmc.set_sample_graph_prior_prob(self.config.graph.sample_graph_prior_prob)
        mcmc.set_up()
        hxg = -dynamics.get_log_likelihood()
        hx = -get_log_evidence(mcmc, self.config.metrics.predictability)

        return (hx - hxg) / hx


class PredictabilityMetrics(Metrics):
    def eval(self, config: Config):
        predictability = Predictability(
            config=config,
            num_procs=config.get_value("num_procs", 1),
            seed=config.get_value("seed", int(time.time())),
        )
        samples = predictability.compute(
            config.metrics.predictability.get_value("num_samples", 10)
        )
        return Statistics.compute(
            samples, error_type=config.metrics.predictability.error_type
        )


if __name__ == "__main__":
    pass
