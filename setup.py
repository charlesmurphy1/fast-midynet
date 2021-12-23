from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import sys
import os
import setuptools

__version__ = "1.0.0"


class get_pybind_include(object):
    """Helper class to determine the pybind11 include path

    The purpose of this class is to postpone importing pybind11
    until it is actually installed, so that the ``get_include()``
    method can be invoked."""

    def __init__(self, user=False):
        self.user = user

    def __str__(self):
        import pybind11

        return pybind11.get_include(self.user)


def find_compiled_basegraph(build_path):
    if not os.path.isdir(build_path) or os.listdir(build_path) == []:
        raise RuntimeError(
            'Submodule BaseGraph was not compiled. Run:\n\t"cd base_graph && mkdir build && cd build && cmake .. && make && cd ../../".'
        )

    basegraph_lib_path = None
    for extension in [".a"]:
        if os.path.isfile(os.path.join(build_path, "libBaseGraph" + extension)):
            basegraph_lib_path = os.path.join(build_path, "libBaseGraph" + extension)
            break

    if basegraph_lib_path is None:
        raise RuntimeError(
            f'Could not find libBaseGraph in "{build_path}". Verify that the library is compiled.'
        )
    return basegraph_lib_path


def find_files_recursively(path, ext=[]):
    if isinstance(ext, str):
        ext = [ext]
    elif not isinstance(ext, list):
        raise TypeError(
            f"type `{type(exp)}` for extension is incorrect, expect `str` or `list`."
        )
    file_list = []

    for e in ext:
        for root, subdirs, files in os.walk(path):
            for f in files:
                if f.split(".")[-1] == e:
                    file_list.append(os.path.join(root, f))
    return file_list


ext_modules = [
    Extension(
        "fast_midynet",
        include_dirs=[
            get_pybind_include(),
            get_pybind_include(user=True),
            "include",
            "./base_graph/include",
            "./SamplableSet/src",
        ],
        sources=[
            "src/rng.cpp",
            "src/exceptions.cpp",
            "src/generators.cpp",
            "src/utility/functions.cpp",
            "src/prior/sbm/block_count.cpp",
            "src/prior/sbm/vertex_count.cpp",
            "src/prior/sbm/block.cpp",
            "src/prior/sbm/edge_count.cpp",
            "src/prior/sbm/edge_matrix.cpp",
            "src/prior/sbm/degree.cpp",
            "src/random_graph/random_graph.cpp",
            "src/random_graph/sbm.cpp",
            "src/random_graph/dcsbm.cpp",
            "src/dynamics/dynamics.cpp",
            "src/dynamics/binary_dynamics.cpp",
            "src/dynamics/cowan.cpp",
            "src/dynamics/degree.cpp",
            "src/dynamics/ising-glauber.cpp",
            "src/dynamics/sis.cpp",
            "src/proposer/edge_proposer/vertex_sampler.cpp",
            "src/proposer/edge_proposer/double_edge_swap.cpp",
            "src/proposer/edge_proposer/hinge_flip.cpp",
            "src/proposer/edge_proposer/single_edge.cpp",
            "src/proposer/block_proposer/uniform.cpp",
            "src/proposer/block_proposer/peixoto.cpp",
            "src/mcmc/mcmc.cpp",
            "src/mcmc/callbacks/callback.cpp",
            "src/mcmc/callbacks/verbose.cpp",
            "src/mcmc/graph_mcmc.cpp",
            "src/mcmc/dynamics_mcmc.cpp",
            "pybind_wrapper/pybind_main.cpp",
        ],
        language="c++",
        extra_objects=[
            find_compiled_basegraph("./base_graph/build"),
            os.path.join(os.getcwd(), "SamplableSet/src/build/libsamplableset.a"),
        ],
    ),
]


# As of Python 3.6, C Compiler has a `has_flag` method.
# cf http://bugs.python.org/issue26689
def has_flag(compiler, flagname):
    """Return a boolean indicating whether a flag name is supported on
    the specified compiler.
    """
    import tempfile

    with tempfile.NamedTemporaryFile("w", suffix=".cpp") as f:
        f.write("int main (int argc, char **argv) { return 0; }")
        try:
            compiler.compile([f.name], extra_postargs=[flagname])
        except setuptools.distutils.errors.CompileError:
            return False
    return True


def cpp_flag(compiler):
    """Return the -std=c++[11/14/17] compiler flag.

    The newer version is prefered over c++11 (when it is available).
    """
    flags = ["-std=c++17", "-std=c++11"]

    for flag in flags:
        if has_flag(compiler, flag):
            return flag

    raise RuntimeError("Unsupported compiler -- at least C++11 support " "is needed!")


class BuildExt(build_ext):
    """A custom build extension for adding compiler-specific options."""

    c_opts = {
        "msvc": ["/EHsc"],
        "unix": [],
    }
    l_opts = {
        "msvc": [],
        "unix": [],
    }

    if sys.platform == "darwin":
        darwin_opts = ["-stdlib=libc++", "-mmacosx-version-min=10.7"]
        c_opts["unix"] += darwin_opts
        l_opts["unix"] += darwin_opts

    def build_extensions(self):
        ct = self.compiler.compiler_type
        opts = self.c_opts.get(ct, [])
        link_opts = self.l_opts.get(ct, [])
        if ct == "unix":
            opts.append('-DVERSION_INFO="%s"' % self.distribution.get_version())
            opts.append(cpp_flag(self.compiler))
            if has_flag(self.compiler, "-fvisibility=hidden"):
                opts.append("-fvisibility=hidden")
        elif ct == "msvc":
            opts.append('/DVERSION_INFO=\\"%s\\"' % self.distribution.get_version())
        for ext in self.extensions:
            ext.extra_compile_args = opts
            ext.extra_link_args = link_opts
        build_ext.build_extensions(self)


setup(
    name="fast_midynet",
    version=__version__,
    ext_modules=ext_modules,
    install_requires=["pybind11>=2.3"],
    setup_requires=["pybind11>=2.3"],
    cmdclass={"build_ext": BuildExt},
)
