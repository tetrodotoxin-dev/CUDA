# Using the CUDA SDK

The [0.1.0 release](https://github.com/tetrodotoxin-dev/CUDA/releases/tag/v0.1.0)
contains public headers and `libttx_cuda.so` for Linux x86-64-v3 with RDRAND.
Consumers download these archives and the TTX and Perimortem SDKs. They do not
need this repository or a CUDA library build.

The binary requires glibc 2.34 or later, libstdc++, NVIDIA's driver and NVRTC 13.
The driver must support the installed NVRTC and the selected GPU. NVIDIA's
libraries remain separate runtime dependencies. The SDK was built with CUDA
13.3 and tested with driver 610.57.04 on an RTX 5070.

## Bazel

Use Bazel 9.2.0 and Python 3. This complete `MODULE.bazel` imports the published
Linux SDKs through Toolchain 0.2.0:

```starlark
module(name = "cuda_consumer", version = "0.1.0")

bazel_dep(name = "rules_cc", version = "0.2.17")
bazel_dep(name = "tetro_toolchain", version = "0.2.0")
archive_override(
    module_name = "tetro_toolchain",
    sha256 = "83a4075e4fc96d53ca68319a6dd004d2f784ba8cd221f6c6a11126fb31e85085",
    urls = ["https://github.com/tetrodotoxin-dev/Toolchain/releases/download/v0.2.0/tetro_toolchain-0.2.0-source.tar.gz"],
)
register_toolchains("@tetro_toolchain//:cc_toolchain_for_linux_x86_64")

sdks = use_extension("@tetro_toolchain//:sdk.bzl", "dependencies")
sdks.release(
    name = "perimortem",
    project = "Perimortem",
    version = "0.1.0",
    archives = {
        "headers": "2bb19a82ceb2dd06a122fb51142e96fd13d52fd5ed323b8a662cb640082636ed",
        "linux-x86_64-v3": "f62c3b71c9c5f554467c9dc03237073fc34d670fa596aa9cd99972dabfb2e653",
    },
    deps = [],
)
use_repo(sdks, "perimortem")

sdks.release(
    name = "ttx",
    project = "TTX",
    version = "0.1.0",
    archives = {
        "headers": "05a389c9cc3a2a896614c0c8776778bec8e68ed801cd0c5d1796b147c534ee61",
        "linux-x86_64-v3": "7603d82082c5c1f9865b3416d5cc003655408eeaea399481dac80849f59e4526",
    },
    deps = ["@perimortem"],
)
use_repo(sdks, "ttx")

sdks.release(
    name = "ttx_cuda",
    project = "CUDA",
    version = "0.1.0",
    archives = {
        "headers": "87fb88ee048c6fa79d921e9b6bef28e6fb782705d86fb7f6f479e96b3888b10a",
        "linux-x86_64-v3": "e7bafbc6ea81b950bd0cdb4afcbe54f462b5ab25478bb844a807f13a56a8fffc",
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

Ship `libttx_cuda.so`, `libttx.so` and `libperimortem.so` together. Arrange for the
loader to find that directory through the application's runpath or
`LD_LIBRARY_PATH`. Keep the corresponding SDK licenses with redistributed
files. Install NVIDIA's driver and NVRTC separately and make their libraries
available to the loader. Bazel's local runfiles are not a deployment package.

Native C++ callers may include `cuda/runtime/*.hpp` and link the same binary.
These interfaces use NVIDIA types and the native C++ ABI, so callers also need
NVIDIA's public CUDA headers and a compatible C++ toolchain. The pinned Toolchain
above supplies the compiler used for this release. A caller using cuFFT declares
its own NVIDIA cuFFT dependency.

## Producing a release

From a CUDA source checkout with the toolkit installed:

```sh
bazel build --config=release //:release
bazel test --config=debug //validation:consumer //validation:runtime
bazel test --config=release //validation:consumer //validation:runtime
```

The release target writes `ttx_cuda-<version>-headers.zip`,
`ttx_cuda-<version>-linux-x86_64-v3.zip` and
`ttx_cuda-<version>-source.zip` beneath `.bin/bin`. The version comes from
`MODULE.bazel`. Publish those archives and their SHA-256 checksums under the
matching `v<version>` tag. Headers retain their `include/cuda/` paths, and the
binary archive contains `lib/libttx_cuda.so` with that same ELF library name.
