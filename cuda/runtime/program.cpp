// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cuda/runtime/program.hpp"

#include <nvrtc.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "cuda/runtime/buffer.hpp"
#include "cuda/runtime/current_context.hpp"
#include "cuda/runtime/kernel.hpp"

using namespace Perimortem;
using namespace Cuda;
using Cuda::Runtime::CurrentContext;
using namespace Ttx::Semantic::Negotiation;

static auto text(perimortem_view_bytes value) -> Memory::Dynamic::Bytes {
  Memory::Dynamic::Bytes result;
  result.forgetful_resize(value.size + 1);
  if (value.size) {
    Core::Data::copy(result.get_access().get_data(), value.data, value.size);
  }

  result.get_access().get_data()[value.size] = 0;
  return result;
}

static auto report(cuda_diagnostics sink, const char* message)
    -> ttx_data_status {
  const auto bytes = Core::NullTerminated::to_view(message);
  sink.write(sink.source, {bytes.get_data(), bytes.get_size()});
  return TTX_DATA_IO_ERROR;
}

auto Runtime::Program::create(
    cuda_compile_request request,
    cuda_diagnostics diagnostics)
    -> Utility::Result<Program&, Ttx::Data::Status> {
  CUresult driver = cuInit(0);
  if (driver != CUDA_SUCCESS) {
    return static_cast<Ttx::Data::Status>(
        report(diagnostics, "CUDA initialization failed."));
  }

  static const Core::Object<>::Descriptor descriptor(
      sizeof(Program), alignof(Program),
      [](U8* source) { reinterpret_cast<Program*>(source)->~Program(); });
  auto memory = Core::Object<>::create(descriptor).get_payload();
  auto* result = new (memory, Core::Placement::Construct) Program();
  driver = cuDeviceGet(&result->device, request.device);
  if (driver == CUDA_SUCCESS) {
    driver = cuDevicePrimaryCtxRetain(&result->context, result->device);
  }

  if (driver != CUDA_SUCCESS) {
    result->release();
    return static_cast<Ttx::Data::Status>(
        report(diagnostics, "CUDA device or context is unavailable."));
  }

  // Null termination and include names belong to NVRTC's boundary. Retain all
  // source owners before lending their pointers, then discard them after the
  // executable has been copied into CUDA's module owner.
  auto source = text(request.source.text);
  auto name = text(request.source.name);
  Memory::Dynamic::Vector<Memory::Dynamic::Bytes> names, headers, options;
  for (Count i = 0; i != request.header_count; ++i) {
    names.emplace(text(request.headers[i].name));
    headers.emplace(text(request.headers[i].text));
  }

  for (Count i = 0; i != request.option_count; ++i) {
    options.emplace(text(request.options[i]));
  }

  Memory::Dynamic::Vector<const char*> named, included, selected;
  for (Count i = 0; i != request.header_count; ++i) {
    named.insert(reinterpret_cast<const char*>(names[i].get_view().get_data()));
    included.insert(
        reinterpret_cast<const char*>(headers[i].get_view().get_data()));
  }

  for (Count i = 0; i != options.get_size(); ++i) {
    selected.insert(
        reinterpret_cast<const char*>(
            options.get_view().get_data()[i].get_view().get_data()));
  }

  // Use the selected device unless the author supplied a target explicitly.
  // This preserves native image code generation while letting a project choose
  // a deliberate compatibility target in its compiler options.
  bool targeted = false;
  for (Count i = 0; i != options.get_size(); ++i) {
    const auto option = options.get_view().get_data()[i].get_view();
    targeted = targeted || option.slice(0, 5) == "-arch"_view ||
               option.slice(0, 18) == "--gpu-architecture"_view;
  }

  Core::Static::Bytes<64> architecture;
  if (!targeted) {
    int major = 0, minor = 0;
    driver = cuDeviceGetAttribute(
        &major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, result->device);
    if (driver == CUDA_SUCCESS) {
      driver = cuDeviceGetAttribute(
          &minor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, result->device);
    }

    if (driver != CUDA_SUCCESS) {
      result->release();
      return static_cast<Ttx::Data::Status>(
          report(diagnostics, "CUDA target architecture is unavailable."));
    }

    Core::Writer::Textual writer(architecture);
    writer << "--gpu-architecture=compute_"_view << S32(major) << S32(minor)
           << '\0';
    selected.insert(reinterpret_cast<const char*>(architecture.get_data()));
  }

  nvrtcProgram compiler = nullptr;
  auto status = nvrtcCreateProgram(
      &compiler, reinterpret_cast<const char*>(source.get_view().get_data()),
      reinterpret_cast<const char*>(name.get_view().get_data()),
      included.get_size(), included.get_view().get_data(),
      named.get_view().get_data());
  if (status != NVRTC_SUCCESS) {
    result->release();
    return static_cast<Ttx::Data::Status>(
        report(diagnostics, nvrtcGetErrorString(status)));
  }

  status = nvrtcCompileProgram(
      compiler, selected.get_size(), selected.get_view().get_data());
  if (status != NVRTC_SUCCESS) {
    size_t size = 0;
    nvrtcGetProgramLogSize(compiler, &size);
    Memory::Dynamic::Bytes message;
    message.forgetful_resize(size);
    if (size) {
      nvrtcGetProgramLog(
          compiler, reinterpret_cast<char*>(message.get_access().get_data()));
      diagnostics.write(
          diagnostics.source, {message.get_view().get_data(), size - 1});
    }

    nvrtcDestroyProgram(&compiler);
    result->release();
    return Ttx::Data::Status::Invalid;
  }

  size_t size = 0;
  status = nvrtcGetPTXSize(compiler, &size);
  Memory::Dynamic::Bytes ptx;
  ptx.forgetful_resize(size);
  if (status == NVRTC_SUCCESS) {
    status = nvrtcGetPTX(
        compiler, reinterpret_cast<char*>(ptx.get_access().get_data()));
  }

  nvrtcDestroyProgram(&compiler);
  if (status != NVRTC_SUCCESS) {
    result->release();
    return static_cast<Ttx::Data::Status>(
        report(diagnostics, nvrtcGetErrorString(status)));
  }

  {
    CurrentContext current(result->context);
    driver = current.status;
    if (driver == CUDA_SUCCESS) {
      driver = cuModuleLoadData(&result->module, ptx.get_view().get_data());
    }
  }

  if (driver != CUDA_SUCCESS) {
    const char* message = nullptr;
    cuGetErrorString(driver, &message);
    result->release();
    return static_cast<Ttx::Data::Status>(
        report(diagnostics, message ? message : "CUDA module load failed."));
  }

  return *result;
}

auto Runtime::Program::compile(
    cuda_compile_request request,
    cuda_diagnostics diagnostics,
    ttx_publication* output) -> ttx_data_status {
  return create(request, diagnostics)
      .visit(
          [&](Program& result) -> ttx_data_status {
            *output = {
              result.get_query(), [](const void* source) {
                const_cast<Program*>(static_cast<const Program*>(source))
                    ->release();
              }};

            return TTX_DATA_SUCCESS;
          },
          [](Ttx::Data::Status status) {
            return static_cast<ttx_data_status>(status);
          });
}

auto Runtime::Program::compiler() -> Ttx::Semantic::Negotiation::Query {
  return Query(
      {nullptr,
       [](const void*, perimortem_uuid id,
          ttx_storage requested) -> ttx_binding_status {
         if (System::Uuid(id) != Cuda::Contracts::Compiler::contract_id) {
           return TTX_BINDING_UNSUPPORTED;
         }

         const cuda_compiler api{
           nullptr, [](const void*, cuda_compile_request request,
                       cuda_diagnostics errors, ttx_publication* output) {
             return compile(request, errors, output);
           }};

         return static_cast<ttx_binding_status>(
             Binding::provide<Cuda::Contracts::Compiler>(
                 api, Ttx::Data::Form::Storage(requested)));
       },
       [](const void*, perimortem_uuid id) -> ttx_binding_status {
         return System::Uuid(id) == Cuda::Contracts::Compiler::contract_id
                    ? TTX_BINDING_SATISFIED
                    : TTX_BINDING_UNSUPPORTED;
       }});
}

auto Runtime::Program::get_query() const -> ttx_semantic_query {
  return {
    this,
    [](const void* source, perimortem_uuid id,
       ttx_storage requested) -> ttx_binding_status {
      if (System::Uuid(id) != Cuda::Contracts::Program::contract_id) {
        return TTX_BINDING_UNSUPPORTED;
      }

      const cuda_program api{
        source,
        [](const void* source, perimortem_view_bytes entry,
           const ttx_representation* frame, const cuda_argument* arguments,
           Count count, cuda_diagnostics errors, ttx_publication* output) {
          return Kernel::prepare(
              *const_cast<Program*>(static_cast<const Program*>(source)), entry,
              *frame, arguments, count, errors, output);
        },
        [](const void* source, Count size, ttx_publication* output) {
          return Buffer::allocate(
              *const_cast<Program*>(static_cast<const Program*>(source)), size,
              output);
        }};

      return static_cast<ttx_binding_status>(
          Binding::provide<Cuda::Contracts::Program>(
              api, Ttx::Data::Form::Storage(requested)));
    },
    [](const void*, perimortem_uuid id) -> ttx_binding_status {
      return System::Uuid(id) == Cuda::Contracts::Program::contract_id
                 ? TTX_BINDING_SATISFIED
                 : TTX_BINDING_UNSUPPORTED;
    }};
}

void Runtime::Program::retain() {
  Core::Object<>(reinterpret_cast<U8*>(this)).retain();
}

void Runtime::Program::release() {
  Core::Object<>(reinterpret_cast<U8*>(this)).release();
}

Runtime::Program::~Program() {
  if (module) {
    CurrentContext current(context);
    if (current.status != CUDA_SUCCESS ||
        cuModuleUnload(module) != CUDA_SUCCESS) {
      Core::Diagnostics::Log::fatal(
          "CUDA program could not release its module."_view);
    }
  }

  if (context && cuDevicePrimaryCtxRelease(device) != CUDA_SUCCESS) {
    Core::Diagnostics::Log::fatal(
        "CUDA program could not release its context."_view);
  }
}
