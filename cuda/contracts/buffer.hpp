// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "cuda/contracts/buffer.h"
#include "ttx/concept/abstract.hpp"
#include "ttx/semantic/negotiation/query.hpp"

TTX_DATA_RECORD(
    cuda_buffer,
    TTX_DATA_MEMBER(cuda_buffer, source),
    TTX_DATA_MEMBER(cuda_buffer, address),
    TTX_DATA_MEMBER(cuda_buffer, size),
    TTX_DATA_MEMBER(cuda_buffer, read),
    TTX_DATA_MEMBER(cuda_buffer, write));

namespace Cuda::Contracts {

// Buffer exposes a CUDA address and explicit host transfers. Calling address
// observes device coordinates and never materializes a host representation.
class Buffer {
 public:
  static constexpr Perimortem::System::Uuid contract_id{
    TTX_CUDA_BUFFER_HIGH,
    TTX_CUDA_BUFFER_LOW,
  };

  using Api = cuda_buffer;
  explicit constexpr Buffer(Api api) : api(api) {}
  auto get_address() const -> U64 { return api.address(api.source); }
  auto get_size() const -> Count { return api.size(api.source); }
  auto read(Count offset, Perimortem::Core::Access::Bytes output) const
      -> Ttx::Data::Status {
    return static_cast<Ttx::Data::Status>(
        api.read(api.source, offset, output.get_data(), output.get_size()));
  }

  auto write(Count offset, Perimortem::Core::View::Bytes input) const
      -> Ttx::Data::Status {
    return static_cast<Ttx::Data::Status>(api.write(
        api.source, offset,
        {
          input.get_data(),
          input.get_size(),
        }));
  }

 private:
  Api api;
};

}  // namespace Cuda::Contracts
