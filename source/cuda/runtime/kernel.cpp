// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cuda/runtime/kernel.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "cuda/runtime/current_context.hpp"
#include "ttx/concept/capabilities/borrow.hpp"

using namespace Perimortem;
using namespace Cuda;
using Cuda::Runtime::CurrentContext;
using namespace Ttx::Semantic::Negotiation;

Runtime::Kernel::Kernel(
    Program& program,
    CUfunction function,
    const ttx_representation& frame,
    Memory::Dynamic::Vector<Count> offsets)
    : program(program),
      function(function),
      form(frame.get_bytes()),
      offsets(Core::Data::take(offsets)) {
  program.retain();
  arguments.resize(this->offsets.get_size());
}

Runtime::Kernel::~Kernel() {
  program.release();
}

auto Runtime::Kernel::prepare(
    Program& program,
    perimortem_view_bytes entry,
    const ttx_representation& frame,
    const cuda_argument* parameters,
    Count count,
    cuda_diagnostics diagnostics,
    ttx_borrowed* output) -> ttx_data_status {
  CurrentContext current(program.get_context());
  if (current.status != CUDA_SUCCESS) {
    return TTX_DATA_IO_ERROR;
  }

  Memory::Dynamic::Bytes name;
  CUfunction function = nullptr;
  name.forgetful_resize(entry.size + 1);
  Core::Data::copy(name.get_access().get_data(), entry.data, entry.size);
  name.get_access().get_data()[entry.size] = 0;
  if (cuModuleGetFunction(
          &function, program.get_module(),
          reinterpret_cast<const char*>(name.get_view().get_data())) !=
      CUDA_SUCCESS) {
    constexpr auto message = "CUDA entry was not found."_view;
    diagnostics.write(
        diagnostics.source, {
                              message.get_data(),
                              message.get_size(),
                            });
    return TTX_DATA_UNSUPPORTED;
  }

  size_t actual_count = 0;
  if (cuFuncGetParamCount(function, &actual_count) != CUDA_SUCCESS) {
    return TTX_DATA_IO_ERROR;
  }

  if (actual_count != count) {
    return TTX_DATA_INCOMPATIBLE;
  }

  // Preserve formal parameter boundaries and verify their host placements once.
  // CUDA supplies occupied parameter sizes, while the authored forms supply
  // type meaning. Neither size equality nor device offsets replace that
  // promise.
  Memory::Dynamic::Vector<Count> offsets;
  Memory::Dynamic::Vector<ttx_representation_position> expected;
  Count end = 0;
  for (Count i = 0; i != count; ++i) {
    const auto& argument = parameters[i];
    const auto extent = argument.representation->get_extent();
    size_t device_offset = 0, size = 0;
    if (cuFuncGetParamInfo(function, i, &device_offset, &size) !=
        CUDA_SUCCESS) {
      return TTX_DATA_IO_ERROR;
    }

    if (size != extent || argument.offset < end ||
        argument.offset > frame.get_extent() ||
        extent > frame.get_extent() - argument.offset ||
        argument.offset % argument.representation->get_alignment()) {
      return TTX_DATA_INCOMPATIBLE;
    }

    argument.representation->visit([&](ttx_representation_position position) {
      position.offset += argument.offset;
      expected.insert(position);
      return Ttx::Data::Status::Success;
    });
    end = argument.offset + extent;
    offsets.insert(argument.offset);
  }

  // Both walks advance in byte order. Admission covers every occupied frame
  // position once, including aggregates, without repeating a root search for
  // each parameter or treating padding as an extra argument.
  Count next = 0;
  const auto agreement = frame.visit([&](ttx_representation_position position) {
    if (next == expected.get_size()) {
      return Ttx::Data::Status::Incompatible;
    }

    const auto& wanted = expected[next];
    ++next;
    return position.offset == wanted.offset && position.compatible(wanted)
               ? Ttx::Data::Status::Success
               : Ttx::Data::Status::Incompatible;
  });
  if (agreement != Ttx::Data::Status::Success || next != expected.get_size()) {
    return TTX_DATA_INCOMPATIBLE;
  }

  static const Core::Object<>::Descriptor descriptor(
      sizeof(Kernel), alignof(Kernel),
      [](U8* source) { reinterpret_cast<Kernel*>(source)->~Kernel(); });
  auto memory = Core::Object<>::create(descriptor).get_payload();
  auto* kernel = new (memory, Core::Placement::Construct)
      Kernel(program, function, frame, Core::Data::take(offsets));
  *output = Ttx::Concept::Policies::Borrowed::provide(*kernel).get_abi();
  return TTX_DATA_SUCCESS;
}

auto Runtime::Kernel::launch(cuda_launch geometry, ttx_storage supplied) const
    -> ttx_data_status {
  const ttx_representation required(
      form.get_view().get_data(), form.get_size());
  if (!required.compatible(*supplied.representation)) {
    return TTX_DATA_INCOMPATIBLE;
  }

  const auto checked = ttx_storage_check(supplied);
  if (checked != TTX_DATA_SUCCESS) {
    return checked;
  }

  for (Count i = 0; i != offsets.get_size(); ++i) {
    arguments[i] = supplied.data + offsets[i];
  }

  CurrentContext current(program.get_context());
  if (current.status != CUDA_SUCCESS) {
    return TTX_DATA_IO_ERROR;
  }

  auto status = cuLaunchKernel(
      function, geometry.grid_x, geometry.grid_y, geometry.grid_z,
      geometry.block_x, geometry.block_y, geometry.block_z,
      geometry.shared_bytes, nullptr, arguments.get_access().get_data(),
      nullptr);
  // The caller may release its argument frame and buffer publications after
  // this call, so execution finishes before the context scope ends.
  if (status == CUDA_SUCCESS) {
    status = cuCtxSynchronize();
  }

  return status == CUDA_SUCCESS ? TTX_DATA_SUCCESS : TTX_DATA_IO_ERROR;
}

auto Runtime::Kernel::get_query() const -> ttx_semantic_query {
  return Ttx::Concept::Abstract::provide(*this).get_query();
}

auto Runtime::Kernel::supports(System::Uuid id) const -> Binding::Status {
  return id == Ttx::Concept::Capabilities::Borrow::contract_id ||
                 id == Ttx::Concept::Policies::Borrowed::contract_id ||
                 id == Contracts::Kernel::contract_id
             ? Binding::Status::Satisfied
             : Binding::Status::Unknown;
}

auto Runtime::Kernel::borrow() const
    -> Utility::Result<Ttx::Concept::Policies::Borrowed, Binding::Failure> {
  Core::Object<>(reinterpret_cast<U8*>(const_cast<Kernel*>(this))).retain();
  return Ttx::Concept::Policies::Borrowed::provide(*this);
}

auto Runtime::Kernel::bind_interface(
    System::Uuid id,
    Ttx::Data::Form::Storage requested) const -> Binding::Status {
  using namespace Ttx::Concept;
  if (id == Capabilities::Borrow::contract_id) {
    return Binding::provide<Capabilities::Borrow>(
        Capabilities::Borrow::provide(*this).get_abi(), requested);
  }
  if (id == Policies::Borrowed::contract_id) {
    return Binding::provide<Policies::Borrowed>(
        Policies::Borrowed::provide(*this).get_abi(), requested);
  }
  if (id != Contracts::Kernel::contract_id) {
    return Binding::Status::Unknown;
  }
  static const cuda_kernel_ops operations = cuda_kernel_ops(
      *Abstract::provide(*this).get_abi().operations,
      [](const void* source, cuda_launch geometry, ttx_storage arguments) {
        return static_cast<const Kernel*>(source)->launch(geometry, arguments);
      });
  return Binding::provide<Contracts::Kernel>(
      cuda_kernel(this, &operations), requested);
}

void Runtime::Kernel::release() const {
  Core::Object<>(reinterpret_cast<U8*>(const_cast<Kernel*>(this))).release();
}
