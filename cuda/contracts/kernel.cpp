// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cuda/contracts/kernel.hpp"

PERIMORTEM_C const ttx_representation* cuda_kernel_representation(void) {
  return &Ttx::Semantic::Negotiation::Binding::representation<
      Cuda::Contracts::Kernel>();
}
