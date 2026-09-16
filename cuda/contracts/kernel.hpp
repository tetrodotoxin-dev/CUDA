// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "cuda/contracts/kernel.h"
#include "ttx/semantic/negotiation/query.hpp"

TTX_DATA_RECORD(
    cuda_launch,
    TTX_DATA_MEMBER(cuda_launch, grid_x),
    TTX_DATA_MEMBER(cuda_launch, grid_y),
    TTX_DATA_MEMBER(cuda_launch, grid_z),
    TTX_DATA_MEMBER(cuda_launch, block_x),
    TTX_DATA_MEMBER(cuda_launch, block_y),
    TTX_DATA_MEMBER(cuda_launch, block_z),
    TTX_DATA_MEMBER(cuda_launch, shared_bytes));
TTX_DATA_RECORD(
    cuda_kernel,
    TTX_DATA_MEMBER(cuda_kernel, source),
    TTX_DATA_MEMBER(cuda_kernel, launch));

namespace Cuda::Contracts {

// A retained kernel uses the agreed frame directly. Geometry and values may
// change between calls without recompiling or rediscovering the kernel.
class Kernel {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TTX_CUDA_KERNEL_HIGH,
    TTX_CUDA_KERNEL_LOW,
  };

  using Api = cuda_kernel;
  explicit constexpr Kernel(Api api) : api(api) {}
  auto launch(cuda_launch geometry, Ttx::Data::Form::Storage arguments) const
      -> Ttx::Data::Status {
    return static_cast<Ttx::Data::Status>(
        api.launch(api.source, geometry, arguments.get_abi()));
  }

 private:
  Api api;
};

}  // namespace Cuda::Contracts
