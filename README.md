# CUDA

This library exposes CUDA compilation, kernels and device storage through TTX
contracts. Consumers supply source bytes, named includes and compiler options.
NVRTC is used to compile provided source for the selected device on demand, with
the resulting TTX Program owning the loaded code and a reference to its CUDA context.

A Program prepares Kernels and allocates Buffers. Each has its own publication
and retains the Program, so releasing the original Program publication leaves
existing kernels and allocations usable. Kernel arguments are described with
TTX Data representations so preparation can check their sizes and placement against
the loaded function and the caller's frame. The source author remains responsible
for matching the argument types.

The provider keeps CUDA resource management behind the interface. Tetrodotoxin
and Godot consumers contain examples of defining their own operations and policies on
top of the CUDA layer. An image operation for example chooses its pixel format, kernel
and launch geometry in the consuming project.

## TTX Compatability

The module exports `ttx_module_open` and publishes a Compiler service for TTX `0.1`.
The public contracts use standard TTX C records and function pointers, with C++ interfaces
over those records. Native callers can also use the runtime directly which saves the effort
of negotiating the system's native ABI and calling convention.

Publications are used serially on their owning worker, and the caller keeps the module
loaded until all publications have been released. Launches and host transfers are guaranteed
to finish before returning.

## Building and testing

The build targets Linux x86_64 with x86-64-v3 and RDRAND. Install Python 3, the
Bazel version in [.bazelversion](.bazelversion), and a CUDA toolkit with NVRTC and
driver headers. Bazel downloads the pinned LLVM tools, TTX and Perimortem SDKs.
The toolkit defaults to `/opt/cuda`; set `CUDA_ROOT` to use another installation.

From the repository root:

```sh
bazel build --config=release //cuda:plugin
bazel test --config=debug //validation:consumer //validation:runtime
bazel test --config=release //validation:consumer //validation:runtime
```

The plugin is written to `.bin/bin/cuda/libttx_cuda.so`. Running it requires NVRTC
and the NVIDIA driver. The tests require a CUDA capable GPU and exercise both C
and C++ consumers, compilation diagnostics, argument agreement, buffer transfers
and retained resource lifetimes.

Native consumers use `//cuda/contracts` and `//cuda/runtime`. The optional
`//build:fft` target supplies cuFFT for callers that need it.
