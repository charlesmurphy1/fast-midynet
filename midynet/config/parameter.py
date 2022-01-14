import typing
import copy
import pathlib

from dataclasses import dataclass, field

__all__ = ["Parameter"]


@dataclass(order=True)
class Parameter:
    name: str
    value: typing.Any = None
    unique: bool = field(repr=False, default=False)
    with_repetition: bool = field(repr=False, default=False)
    force_non_sequence: bool = field(repr=False, default=False)
    sort_sequence: bool = field(repr=False, default=True)
    is_config: bool = False
    __cache__: bool = field(repr=False, default=True)
    __self_hash__: int = field(repr=False, default=None)

    @property
    def datatype(self) -> typing.Any:
        return self.infer_type(self.value)

    def get_sequence(self, values):
        if not issubclass(type(values), typing.Iterable) or isinstance(values, str):
            seq = [values]
        else:
            seq = values.copy()

        if not self.with_repetition and not self.is_config:
            seq = list(set(seq))
        if self.sort_sequence:
            seq.sort()
        return seq

    def __getitem__(self, key):
        if not self.is_sequenced():
            message = "this parameter is not sequenced."
            raise LookupError(message)
        return self.value[key]

    def __hash__(self):
        if self.__self_hash__ is None:
            if isinstance(self.value, list):
                h = hash(tuple(self.value))
            else:
                h = hash(self.value)
            if self.__cache__:
                self.__self_hash__ = h
            else:
                return h
        return self.__self_hash__

    def __reset_buffer__(self):
        self.__self_hash__ = None

    def set_value(self, value):
        value = value.value if issubclass(type(value), Parameter) else value
        if issubclass(type(value), typing.Iterable) and not isinstance(value, str):
            value = self.get_sequence(value)
        self.value = value
        self.__reset_buffer__()

    def add_value(self, value):
        if issubclass(type(self.value), typing.Iterable):
            self.value = self.get_sequence(list(self.value) + [value])
        else:
            self.value = self.get_sequence([self.value, value])
        if len(self.value) == 1:
            self.value = next(iter(self.value))
        self.__reset_buffer__()

    def add_values(self, values):
        seq = self.get_sequence(values)
        for v in seq:
            self.add_value(v)
        self.__reset_buffer__()

    def is_sequenced(self):
        return (
            issubclass(type(self.value), typing.Iterable)
            and not isinstance(self.value, str)
            and not self.force_non_sequence
        )

    def is_unique(self):
        return self.unique

    def infer_type(self, value: typing.Any):
        if self.force_non_sequence or isinstance(value, str):
            return type(value)
        if isinstance(value, dict):
            message = "invalid value type `dict`."
            raise TypeError(message)
        if issubclass(type(value), typing.Iterable):
            return self.infer_type(next(iter(value)))
        return type(value)

    def generate_sequence(self):
        if self.is_sequenced():
            for v in self.value:
                yield v
        else:
            yield self.value

    def format(self):
        if isinstance(self.value, str) or isinstance(self.value, pathlib.Path):
            return f"`{self.value}`"
        else:
            return f"{self.value}"


if __name__ == "__main__":
    pass
