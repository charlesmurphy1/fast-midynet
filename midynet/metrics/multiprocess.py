import multiprocessing as mp
import multiprocessing.pool
import time
from dataclasses import dataclass, field

import numpy as np

__all__ = ("MultiProcess", "Expectation")


class NoDaemonProcess(mp.Process):
    @property
    def daemon(self):
        return False

    @daemon.setter
    def daemon(self, value):
        pass


class NoDaemonContext(type(mp.get_context())):
    Process = NoDaemonProcess


class NestablePool(multiprocessing.pool.Pool):
    def __init__(self, *args, **kwargs):
        kwargs['context'] = NoDaemonContext()
        super(NestablePool, self).__init__(*args, **kwargs)


@dataclass
class MultiProcess:
    num_procs: int = 1

    def func(self, inputs):
        raise NotImplementedError()

    def compute(self, inputs):
        with mp.Pool(self.num_procs) as p:
            out = p.map(self.func, inputs)
        return out


@dataclass
class Expectation(MultiProcess):
    seed: int = field(default_factory=lambda: int(time.time()))

    def func(self, seed: int) -> float:
        raise NotImplementedError()

    def compute(self, num_samples: int = 1) -> list[float]:
        seeds = self.seed + np.arange(num_samples)
        if self.num_procs > 1:
            with mp.Pool(self.num_procs) as p:
                out = p.map(self.func, seeds)
        else:
            out = [self.func(s) for s in seeds]
        return out
