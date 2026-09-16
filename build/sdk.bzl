# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

def _cuda_impl(ctx):
    root = ctx.os.environ.get("CUDA_ROOT", "/opt/cuda")
    if not ctx.path(root + "/include/cuda.h").exists:
        fail("CUDA headers missing under " + root + "; set CUDA_ROOT to the toolkit installation")
    ctx.symlink(root + "/include", "include")
    ctx.symlink(root + "/lib64", "lib64")
    ctx.file("BUILD", """
load("@rules_cc//cc:cc_import.bzl", "cc_import")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
package(default_visibility = ["//visibility:public"])
cc_import(name = "nvrtc", shared_library = "lib64/libnvrtc.so")
cc_import(name = "cufft", shared_library = "lib64/libcufft.so")
cc_import(name = "driver", interface_library = "lib64/stubs/libcuda.so", system_provided = True)
cc_library(
    name = "compute",
    hdrs = glob(["include/**/*.h"]),
    includes = ["include"],
    deps = [":driver", ":nvrtc"],
    linkopts = ["-Wl,-rpath,%s/lib64"],
)
cc_library(name = "fft", hdrs = glob(["include/**/*.h"]), includes = ["include"], deps = [":cufft"])
""" % root)

cuda_repository = repository_rule(
    implementation = _cuda_impl,
    environ = ["CUDA_ROOT"],
    local = True,
)
