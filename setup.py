import sys
from setuptools import setup, Extension

if sys.platform == "win32":
    libs = ["libssl", "libcrypto", "zlib1"]
    extra_compile_args = ["/O2"]
else:
    libs = ["ssl", "crypto", "z"]
    extra_compile_args = ["-O3"]

epy_runtime_module = Extension(
    name="epy_runtime",
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
    include_dirs=["include"],
    libraries=libs,
    extra_compile_args=extra_compile_args,
)

setup(
    name="epy_runtime",
    version="1.0.0",
    description="Encrypted Python Runtime",
    ext_modules=[epy_runtime_module],
    py_modules=["builder"],
    package_dir={"": "python"},
    entry_points={
        "console_scripts": [
            "epy-build=builder:main",
        ],
    },
)