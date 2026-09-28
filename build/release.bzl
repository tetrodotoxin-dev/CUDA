# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""CUDA SDK archives for the supported Linux target."""

load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_files", "strip_prefix")
load("@rules_pkg//pkg:zip.bzl", "pkg_zip")

def sdk_release(name, headers, library, sources, scripts):
    """Package public headers, the shared library and rebuildable source."""
    archive = native.module_name() + "-" + native.module_version()
    pkg_files(
        name = name + "_headers",
        srcs = headers,
        prefix = "include",
        strip_prefix = strip_prefix.from_root(),
    )
    pkg_files(
        name = name + "_library",
        srcs = [library],
        prefix = "lib",
    )
    pkg_files(
        name = name + "_sources",
        srcs = sources,
        strip_prefix = strip_prefix.from_root(),
    )
    pkg_files(
        name = name + "_scripts",
        srcs = scripts,
        attributes = pkg_attributes(mode = "0755"),
        strip_prefix = strip_prefix.from_root(),
    )
    for suffix, contents in {
        "headers": [":" + name + "_headers", "LICENSE"],
        "linux-x86_64-v3": [":" + name + "_library", "LICENSE"],
        "source": [":" + name + "_sources", ":" + name + "_scripts"],
    }.items():
        pkg_zip(
            name = name + "_" + suffix + "_archive",
            srcs = contents,
            out = archive + "-" + suffix + ".zip",
            package_dir = "/",
            mode = "0644",
        )
    native.filegroup(
        name = name,
        srcs = [":" + name + "_" + suffix + "_archive" for suffix in ["headers", "linux-x86_64-v3", "source"]],
    )
