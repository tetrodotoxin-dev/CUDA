# Using the CUDA SDK

The [0.2.0 release](https://github.com/tetrodotoxin-dev/CUDA/releases/tag/v0.2.0)
contains public headers, `libttx_cuda.so` for Linux x86-64-v3 with RDRAND, and
`ttx_cuda.dll` with its import library for Windows x64 with the MSVC ABI.
Consumers download these archives and the TTX and Perimortem SDKs. They do not
need this repository or a CUDA library build.

The Linux binary requires glibc 2.34 or later, libstdc++, NVIDIA's driver and
NVRTC 13.3 with its matching builtins library. Windows requires the NVIDIA driver
and the corresponding NVRTC and builtins DLLs.
The driver must support the installed NVRTC and the selected GPU. NVIDIA's
libraries remain separate runtime dependencies. The Linux SDK was built with CUDA
13.3 and tested with driver 610.57.04 on an RTX 5070.

## Bazel

Use Bazel 9.2.0 and Python 3. This complete `MODULE.bazel` imports the published
native SDKs through Toolchain 0.2.1:

```starlark
module(name = "cuda_consumer", version = "0.1.0")

bazel_dep(name = "rules_cc", version = "0.2.17")
bazel_dep(name = "tetro_toolchain", version = "0.2.1")
archive_override(
    module_name = "tetro_toolchain",
    sha256 = "eea7461fb783d2b87a638c4ce1c8d9cbee73e54c90259516ca73cd7e850bdc0f",
    urls = ["https://github.com/tetrodotoxin-dev/Toolchain/releases/download/v0.2.1/tetro_toolchain-0.2.1-source.tar.gz"],
)
register_toolchains("@tetro_toolchain//:cc_toolchain_for_linux_x86_64", "@tetro_toolchain//:cc_toolchain_for_windows_x64")

sdks = use_extension("@tetro_toolchain//:sdk.bzl", "dependencies")
sdks.release(
    name = "perimortem",
    project = "Perimortem",
    version = "0.1.0",
    archives = {
        "headers": "2bb19a82ceb2dd06a122fb51142e96fd13d52fd5ed323b8a662cb640082636ed",
        "linux-x86_64-v3": "f62c3b71c9c5f554467c9dc03237073fc34d670fa596aa9cd99972dabfb2e653",
        "windows-x86_64-msvc": "543763f2a684bc410e264f60115a13cc92b20ae12ae25a9c5294c685f9d06d94",
    },
    deps = [],
)
use_repo(sdks, "perimortem")

sdks.release(
    name = "ttx",
    project = "TTX",
    version = "0.2.0",
    archives = {
        "headers": "b29fc0774de7631a94042a9fc0f08e8a537366b380403d714133ee014bdaedfa",
        "linux-x86_64-v3": "0214ef3b44b52ec10c1c112d62feb4656771f0d8886303756ec47b847e3489cb",
        "windows-x86_64-msvc": "2058106e0a0458f6ed322b9288ad0ed6f3d57f550b64bc7c342e6bbc526a8346",
    },
    deps = ["@perimortem"],
)
use_repo(sdks, "ttx")

sdks.release(
    name = "ttx_cuda",
    project = "CUDA",
    version = "0.2.0",
    archives = {
        "headers": "1d69b3769617c8c013890b305a607da3ca4a315208422d0071cf7e209d2ec0ca",
        "linux-x86_64-v3": "1fe135c07b70f4f814d2c512004dc369f70a69cfed41724eb0038620ff1701a6",
        "windows-x86_64-msvc": "dc1067239cf96fa0e531fb7ccb372ab88094e5480fa268ca52f2b0d13d1eec91",
    },
    deps = ["@ttx"],
)
use_repo(sdks, "ttx_cuda")
```

A consumer target links the SDK and can pass its module path to the application:

```starlark
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "app",
    srcs = ["app.c"],
    args = ["$(location @ttx_cuda//:build)"],
    copts = ["-std=c17"],
    data = ["@ttx_cuda//:build"],
    deps = ["@ttx_cuda"],
)
```

`bazel run //:app` supplies that path as the first argument. The
[C consumer](validation/consumer.c) demonstrates loading the module, acquiring
the Compiler, compiling source and transferring a retained Buffer. The
[C++ consumer](validation/runtime.cpp) covers the typed contracts and kernel
launches. C++ targets include the `.hpp` contracts and omit `-std=c17`.

The imported `@ttx_cuda` target links the binary, `@ttx_cuda//:headers` supplies
the headers, and `@ttx_cuda//:build` names the shared library file. These targets
do not configure or compile the CUDA project.

## Other build systems and deployment

Extract the headers archives for CUDA, TTX and Perimortem into one SDK directory,
and extract their Linux archives into the same directory. Add `include` to the
compiler's include paths and `lib` to the linker paths. Link `ttx_cuda`, `ttx`
and `perimortem`. The public C records and C++ contract facades require no NVIDIA
headers.

Ship `libttx_cuda.so`, `libttx.so` and `libperimortem.so` together on Linux, or
`ttx_cuda.dll`, `ttx.dll` and `perimortem.dll` on Windows. Arrange for the loader
to find that directory through the application's runpath or `LD_LIBRARY_PATH`
on Linux, or the application's directory or `PATH` on Windows. Keep the
corresponding SDK licenses with redistributed files. Install NVIDIA's driver and NVRTC separately and make their libraries
available to the loader. Bazel's local runfiles are not a deployment package.

Native C++ callers may include `cuda/runtime/*.hpp` and link the same binary.
These interfaces use NVIDIA types and the native C++ ABI, so callers also need
NVIDIA's public CUDA headers and a compatible C++ toolchain. The pinned Toolchain
above supplies the compiler used for this release. A caller using cuFFT declares
its own NVIDIA cuFFT dependency.

## Producing a release

From a CUDA source checkout:

```sh
bazel build --config=release //:release
bazel build --config=release --config=windows //:release
bazel test --config=debug //validation:consumer //validation:runtime
bazel test --config=release //validation:consumer //validation:runtime
```

The release target writes `ttx_cuda-<version>-headers.zip`,
`ttx_cuda-<version>-source.zip` and the selected platform's binary archive beneath
`.bin/bin`: `ttx_cuda-<version>-linux-x86_64-v3.zip` or
`ttx_cuda-<version>-windows-x86_64-msvc.zip`. The version comes from
`MODULE.bazel`. Publish those archives and their SHA-256 checksums under the
matching `v<version>` tag. Headers retain their `include/cuda/` paths, and the
Linux binary archive contains `lib/libttx_cuda.so`. The Windows archive contains
`lib/ttx_cuda.dll` and `lib/ttx_cuda.lib`.
