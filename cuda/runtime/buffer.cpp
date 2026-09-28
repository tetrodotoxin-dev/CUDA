// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cuda/runtime/buffer.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "cuda/runtime/current_context.hpp"

using namespace Perimortem;
using namespace Cuda;
using Cuda::Runtime::CurrentContext;
using namespace Ttx::Semantic::Negotiation;

auto Runtime::Buffer::allocate(
    Program& program,
    Count size,
    ttx_publication* output) -> ttx_data_status {
  if (!size) {
    return TTX_DATA_INVALID;
  }

  CurrentContext current(program.get_context());
  CUdeviceptr address = 0;
  auto status = current.status;
  if (status == CUDA_SUCCESS) {
    status = cuMemAlloc(&address, size);
  }

  if (status != CUDA_SUCCESS) {
    return TTX_DATA_IO_ERROR;
  }

  static const Core::Object<>::Descriptor descriptor(
      sizeof(Buffer), alignof(Buffer),
      [](U8* source) { reinterpret_cast<Buffer*>(source)->~Buffer(); });
  auto memory = Core::Object<>::create(descriptor).get_payload();
  auto* buffer =
      new (memory, Core::Placement::Construct) Buffer(program, address, size);
  *output = {
    buffer->get_query(),
    [](const void* source) {
      Core::Object<>(reinterpret_cast<U8*>(const_cast<void*>(source)))
          .release();
    },
  };

  return TTX_DATA_SUCCESS;
}

Runtime::Buffer::~Buffer() {
  {
    CurrentContext current(program.get_context());
    auto status = current.status;
    if (status == CUDA_SUCCESS) {
      status = cuMemFree(address);
    }

    if (status != CUDA_SUCCESS) {
      Core::Diagnostics::Log::fatal(
          "CUDA buffer could not release its allocation."_view);
    }
  }

  program.release();
}

auto Runtime::Buffer::get_query() const -> ttx_semantic_query {
  return {
    this,
    [](const void* source, perimortem_uuid id,
       ttx_storage requested) -> ttx_binding_status {
      if (System::Uuid(id) != Cuda::Contracts::Buffer::contract_id) {
        return TTX_BINDING_UNSUPPORTED;
      }

      const cuda_buffer api{
        source,
        [](const void* source) -> U64 {
          return static_cast<const Buffer*>(source)->address;
        },
        [](const void* source) -> Count {
          return static_cast<const Buffer*>(source)->size;
        },
        [](const void* source, Count offset, U8* output,
           Count size) -> ttx_data_status {
          const auto& buffer = *static_cast<const Buffer*>(source);
          if (offset > buffer.size || size > buffer.size - offset) {
            return TTX_DATA_BOUNDS;
          }

          CurrentContext current(buffer.program.get_context());
          if (current.status != CUDA_SUCCESS) {
            return TTX_DATA_IO_ERROR;
          }

          const auto status =
              cuMemcpyDtoH(output, buffer.address + offset, size);
          return status == CUDA_SUCCESS ? TTX_DATA_SUCCESS : TTX_DATA_IO_ERROR;
        },
        [](const void* source, Count offset,
           perimortem_view_bytes input) -> ttx_data_status {
          const auto& buffer = *static_cast<const Buffer*>(source);
          if (offset > buffer.size || input.size > buffer.size - offset) {
            return TTX_DATA_BOUNDS;
          }

          CurrentContext current(buffer.program.get_context());
          if (current.status != CUDA_SUCCESS) {
            return TTX_DATA_IO_ERROR;
          }

          const auto status =
              cuMemcpyHtoD(buffer.address + offset, input.data, input.size);
          return status == CUDA_SUCCESS ? TTX_DATA_SUCCESS : TTX_DATA_IO_ERROR;
        },
      };

      return static_cast<ttx_binding_status>(
          Binding::provide<Cuda::Contracts::Buffer>(
              api, Ttx::Data::Form::Storage(requested)));
    },
    [](const void*, perimortem_uuid id) -> ttx_binding_status {
      return System::Uuid(id) == Cuda::Contracts::Buffer::contract_id
                 ? TTX_BINDING_SATISFIED
                 : TTX_BINDING_UNSUPPORTED;
    },
  };
}
