import time
from dataclasses import dataclass, field

from midynet.config import *
from midynet import utility
from .multiprocess import MultiProcess, Expectation
from .metrics import ExpectationMetrics

__all__ = ["MutualInformation", "MutualInformationMetrics"]


@dataclass
class MutualInformation(Expectation):
    config: Config = field(repr=False, default_factory=Config)

    def func(self, seed: int) -> float:
        utility.seed(seed)
        graph = RandomGraphFactory.build(self.config.graph)
        dynamics = DynamicsFactory.build(self.config.dynamics)
        dynamics.set_random_graph(graph.get_wrap())
        raise NotImplementedError()


class MutualInformationMetrics(ExpectationMetrics):
    def eval(self, config: Config):
        mutual_info = MutualInformation(
            config=config,
            num_procs=config.metrics.get_value("num_procs", 1),
            seed=config.metrics.get_value("seed", int(time.time())),
        )
        return mutual_info.compute(config.metrics.get_value("num_samples", 10))


if __name__ == "__main__":
    pass
