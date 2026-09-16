// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cuda/contracts/compiler.hpp"

PERIMORTEM_C const ttx_representation* cuda_compiler_representation(void) {
  return &Ttx::Semantic::Negotiation::Binding::representation<
      Cuda::Contracts::Compiler>();
}
