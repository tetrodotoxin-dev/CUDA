# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

exports_files(["sdk.bzl"])

# Native CUDA policies can elect to use cuFFT. General compilation does not
# acquire this dependency merely because another consumer needs transforms.
cc_library(
    name = "fft",
    visibility = ["//visibility:public"],
    deps = ["@cuda_sdk//:fft"],
)
