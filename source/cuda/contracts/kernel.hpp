// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "cuda/contracts/kernel.h"
#include "ttx/concept/policies/borrowed.hpp"

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
    cuda_kernel_ops,
    TTX_DATA_MEMBER(cuda_kernel_ops, abstract),
    TTX_DATA_MEMBER(cuda_kernel_ops, launch));
TTX_DATA_RECORD(
    cuda_kernel,
    TTX_DATA_MEMBER(cuda_kernel, source),
    TTX_DATA_MEMBER(cuda_kernel, operations));

namespace Cuda::Contracts {

// A retained kernel uses the agreed frame directly. Geometry and values may
// change between calls without recompiling or rediscovering the kernel.
class Kernel : public Ttx::Concept::Abstract {
 public:
  static constexpr auto contract_id =
      Perimortem::System::Uuid(TTX_CUDA_KERNEL_HIGH, TTX_CUDA_KERNEL_LOW);

  using Api = cuda_kernel;
  using Operations = cuda_kernel_ops;
  explicit constexpr Kernel(Api api)
      : Abstract(api.source, api.operations->abstract) {}
  static auto accept(Api value) -> Bool {
    return value.operations &&
           Abstract::accept(
               ttx_abstract(value.source, &value.operations->abstract)) &&
           value.operations->launch;
  }
  auto get_abi() const -> Api {
    const auto value = Abstract::get_abi();
    return Api(
        value.source, reinterpret_cast<const Operations*>(value.operations));
  }
  auto launch(cuda_launch geometry, Ttx::Data::Form::Storage arguments) const
      -> Ttx::Data::Status {
    return static_cast<Ttx::Data::Status>(get_abi().operations->launch(
        get_abi().source, geometry, arguments.get_abi()));
  }
};

}  // namespace Cuda::Contracts
