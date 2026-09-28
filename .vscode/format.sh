#!/usr/bin/env bash
# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

set -euo pipefail

repository="$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel)"
cd "$repository"

if (($# > 0)); then
  clang-format -i -- "$@"
  exit 0
fi

# Keep generated Bazel trees and dependencies outside the formatter input.
rg --files --null cuda validation \
  -g '*.c' -g '*.h' -g '*.cpp' -g '*.hpp' |
  xargs --null --no-run-if-empty clang-format -i
