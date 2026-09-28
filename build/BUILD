# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

exports_files(["sdk.bzl"])

filegroup(
    name = "sources",
    srcs = glob([
        "BUILD",
        "*.bzl",
    ]),
    visibility = ["//visibility:public"],
)

# Native CUDA policies can elect to use cuFFT. General compilation does not
# acquire this dependency merely because another consumer needs transforms.
cc_library(
    name = "fft",
    target_compatible_with = [
        "@platforms//os:linux",
        "@platforms//cpu:x86_64",
    ],
    visibility = ["//visibility:public"],
    deps = ["@cuda_sdk//:fft"],
)
