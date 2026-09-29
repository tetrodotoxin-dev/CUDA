# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@tetro_toolchain//:library.bzl", "LINUX", "WINDOWS")

exports_files(["sdk.bzl"])

filegroup(
    name = "sources",
    srcs = glob([
        "BUILD",
        "*.bzl",
    ]),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "compute",
    visibility = ["//visibility:public"],
    deps = select({
        LINUX: [
            "@cuda_cudart_linux//:library",
            "@cuda_nvrtc_linux//:library",
        ],
        WINDOWS: [
            "@cuda_cudart_windows//:library",
            "@cuda_nvrtc_windows//:library",
        ],
    }),
)

# Native CUDA policies can elect to use cuFFT. General compilation does not
# acquire this dependency merely because another consumer needs transforms.
cc_library(
    name = "fft",
    visibility = ["//visibility:public"],
    deps = select({
        LINUX: ["@libcufft_linux//:library"],
        WINDOWS: ["@libcufft_windows//:library"],
    }),
)
