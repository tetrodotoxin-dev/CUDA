// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "cuda/contracts/buffer.h"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/policies/borrowed.hpp"

TTX_DATA_RECORD(
    cuda_buffer_ops,
    TTX_DATA_MEMBER(cuda_buffer_ops, abstract),
    TTX_DATA_MEMBER(cuda_buffer_ops, address),
    TTX_DATA_MEMBER(cuda_buffer_ops, size),
    TTX_DATA_MEMBER(cuda_buffer_ops, read),
    TTX_DATA_MEMBER(cuda_buffer_ops, write));
TTX_DATA_RECORD(
    cuda_buffer,
    TTX_DATA_MEMBER(cuda_buffer, source),
    TTX_DATA_MEMBER(cuda_buffer, operations));

namespace Cuda::Contracts {

// Buffer exposes a CUDA address and explicit host transfers. Calling address
// observes device coordinates and never materializes a host representation.
class Buffer : public Ttx::Concept::Abstract {
 public:
  static constexpr auto contract_id =
      Perimortem::System::Uuid(TTX_CUDA_BUFFER_HIGH, TTX_CUDA_BUFFER_LOW);

  using Api = cuda_buffer;
  using Operations = cuda_buffer_ops;
  explicit constexpr Buffer(Api api)
      : Abstract(api.source, api.operations->abstract) {}
  static auto accept(Api value) -> Bool {
    return value.operations &&
           Abstract::accept(
               ttx_abstract(value.source, &value.operations->abstract)) &&
           value.operations->address && value.operations->size &&
           value.operations->read && value.operations->write;
  }
  auto get_abi() const -> Api {
    const auto value = Abstract::get_abi();
    return Api(
        value.source, reinterpret_cast<const Operations*>(value.operations));
  }
  auto get_address() const -> U64 {
    return get_abi().operations->address(get_abi().source);
  }
  auto get_size() const -> Count {
    return get_abi().operations->size(get_abi().source);
  }
  auto read(Count offset, Perimortem::Core::Access::Bytes output) const
      -> Ttx::Data::Status {
    return static_cast<Ttx::Data::Status>(get_abi().operations->read(
        get_abi().source, offset, output.get_data(), output.get_size()));
  }

  auto write(Count offset, Perimortem::Core::View::Bytes input) const
      -> Ttx::Data::Status {
    return static_cast<Ttx::Data::Status>(get_abi().operations->write(
        get_abi().source, offset,
        {
          input.get_data(),
          input.get_size(),
        }));
  }
};

}  // namespace Cuda::Contracts
