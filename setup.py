import sys
from setuptools import setup, Extension

include_dirs = ["include"]
library_dirs = []
extra_compile_args = []
extra_link_args = []
libraries = []

if sys.platform in ("linux", "darwin"):
    libraries = ["ssl", "crypto", "z"]
    extra_compile_args = ["-O3"]

    if sys.platform == "darwin":
        include_dirs.extend([
            "/opt/homebrew/opt/openssl@3/include",
            "/usr/local/opt/openssl@3/include"
        ])
        library_dirs.extend([
            "/opt/homebrew/opt/openssl@3/lib",
            "/usr/local/opt/openssl@3/lib"
        ])

elif sys.platform == "win32":
    libraries = ["libssl", "libcrypto", "zlib"]
    extra_compile_args = ["/O2"]
    include_dirs.append(r"C:\vcpkg\installed\x64-windows\include")
    library_dirs.append(r"C:\vcpkg\installed\x64-windows\lib")

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
    library_dirs=library_dirs,
    libraries=libraries,
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args,
)

setup(
    name="epy_runtime",
    version="1.0.0",
    ext_modules=[epy_runtime_module],
    py_modules=["builder"],
    package_dir={"": "python"},
)