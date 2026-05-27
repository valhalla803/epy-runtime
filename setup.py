import sys
from setuptools import setup, Extension

include_dirs = ["include"]

extra_compile_args = []
extra_link_args = []

libraries = []

if sys.platform == "win32":
    libraries = ["crypt32"]
elif sys.platform == "darwin":
    libraries = ["ssl", "crypto", "z"]
    extra_compile_args = ["-O3"]
else:
    libraries = ["ssl", "crypto", "z"]
    extra_compile_args = ["-O3"]

epy_runtime_module = Extension(
    "epy_runtime",
    sources=[
        "runtime/antidebug.c",
        "runtime/bundle.c",
        "runtime/cache.c",
        "runtime/crypto.c",
        "runtime/importer.c",
        "runtime/loader.c",
        "runtime/main.c",
        "runtime/secure.c",
    ],
    include_dirs=include_dirs,
    libraries=libraries,
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args,
)

setup(
    name="epy_runtime",
    version="1.0.0",
    description="Stable Python Runtime Extension",
    ext_modules=[epy_runtime_module],
    py_modules=["builder"],
    package_dir={"": "python"},
)