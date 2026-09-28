// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/time.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "cuda/contracts/buffer.hpp"
#include "cuda/contracts/compiler.hpp"
#include "cuda/contracts/kernel.hpp"
#include "cuda/contracts/program.hpp"
#include "ttx/concept/modules/module.hpp"

using namespace Perimortem;
using namespace Ttx;
using namespace Cuda::Contracts;

static auto require(Bool value, Core::View::Bytes message) -> void {
  if (!value) {
    Core::Diagnostics::Log::fatal(message);
  }
}

template <typename T, typename E>
static auto accepted(Utility::Result<T, E> result) -> T {
  return result.visit(
      [](T& value) -> T { return Core::Data::take(value); },
      [](E) -> T {
        Core::Diagnostics::Log::fatal("CUDA capability was not acquired."_view);
      });
}

struct Parameters {
  R32 scale;
  U32 bias;
};

struct Arguments {
  U64 output;
  U64 input;
  U32 count;
  Parameters parameters;
  R64 adjustment;
};

TTX_DATA_RECORD(
    Parameters,
    TTX_DATA_MEMBER(Parameters, scale),
    TTX_DATA_MEMBER(Parameters, bias));
TTX_DATA_RECORD(
    Arguments,
    TTX_DATA_MEMBER(Arguments, output),
    TTX_DATA_MEMBER(Arguments, input),
    TTX_DATA_MEMBER(Arguments, count),
    TTX_DATA_MEMBER(Arguments, parameters),
    TTX_DATA_MEMBER(Arguments, adjustment));

template <typename Type>
static auto form() -> const Data::Form::Representation& {
  return Data::Form::Compiled<
      Data::Form::Native<Type>::reference>::get_representation();
}

int main(int argc, char** argv) {
  Core::Diagnostics::Log::set_sink(Core::Diagnostics::Log::stderr_sink);
  require(argc == 2, "Supply the independent CUDA provider."_view);

  Memory::Allocator::Arena errors;
  auto module = accepted(
      Concept::Modules::Module::load(
          Core::NullTerminated::to_view(argv[1]), errors));
  auto discovery = accepted(module.open());
  require(
      discovery.supports<Compiler>() ==
          Semantic::Negotiation::Binding::Status::Satisfied,
      "Compiler support initialized or declined the device capability."_view);
  auto compiler = accepted(discovery.bind<Compiler>());
  discovery.close();

  // This source and its include come from the consumer. The provider has no
  // built in knowledge of these kernels, parameter count or aggregate type.
  constexpr auto header =
      "struct Parameters { float scale; unsigned bias; };"_view;
  constexpr auto source = R"CUDA(
#include "parameters.cuh"
extern "C" __global__ void transform(float* out, const float* in, unsigned count,
    Parameters parameters, double adjustment) {
  unsigned i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < count) out[i] = in[i] * parameters.scale + parameters.bias + adjustment;
}

extern "C" __global__ void empty() {}
)CUDA"_view;
  const cuda_source include{
    {
      reinterpret_cast<const U8*>("parameters.cuh"),
      14,
    },
    {
      header.get_data(),
      header.get_size(),
    },
  };

  const cuda_compile_request request{
    {
      {
        reinterpret_cast<const U8*>("project.cu"),
        10,
      },
      {
        source.get_data(),
        source.get_size(),
      },
    },
    &include,
    1,
    nullptr,
    0,
    0,
  };

  Memory::Dynamic::Bytes diagnostic;
  const auto compilation_started = Core::Time::now();
  auto program_owner = accepted(compiler.compile(request, diagnostic));
  const auto compilation_ns =
      compilation_started.measure().convert_to_nanoseconds();
  auto program = accepted(program_owner.get_query().bind<Program>());
  auto input_owner = accepted(program.allocate(4 * sizeof(R32)));
  auto output_owner = accepted(program.allocate(4 * sizeof(R32)));
  auto input = accepted(input_owner.get_query().bind<Buffer>());
  auto output = accepted(output_owner.get_query().bind<Buffer>());
  const Core::Static::Vector<R32, 4> values = {
    {
      1,
      2,
      3,
      4,
    },
  };

  require(
      input.write(
          0,
          {
            reinterpret_cast<const U8*>(values.get_data()),
            sizeof(values),
          }) == Data::Status::Success,
      "Input upload failed."_view);
  require(
      input.write(
          sizeof(values),
          {
            reinterpret_cast<const U8*>(values.get_data()),
            sizeof(values),
          }) == Data::Status::Bounds,
      "Buffer accepted an out of range write."_view);

  const Core::Static::Vector<cuda_argument, 5> parameters = {
    {
      cuda_argument{
        __builtin_offsetof(Arguments, output),
        &form<U64>(),
      },
      {
        __builtin_offsetof(Arguments, input),
        &form<U64>(),
      },
      {
        __builtin_offsetof(Arguments, count),
        &form<U32>(),
      },
      {
        __builtin_offsetof(Arguments, parameters),
        &form<Parameters>(),
      },
      {
        __builtin_offsetof(Arguments, adjustment),
        &form<R64>(),
      },
    },
  };

  const auto preparation_started = Core::Time::now();
  auto kernel_owner = accepted(program.prepare(
      "transform"_view, form<Arguments>(), parameters.get_view(), diagnostic));
  const auto preparation_ns =
      preparation_started.measure().convert_to_nanoseconds();
  auto kernel = accepted(kernel_owner.get_query().bind<Kernel>());
  auto wrong = parameters;
  wrong[2].representation = &form<U64>();
  require(
      program
          .prepare(
              "transform"_view, form<Arguments>(), wrong.get_view(), diagnostic)
          .visit(
              [](auto&) { return False; },
              [](Data::Status status) {
                return Bool(status == Data::Status::Incompatible);
              }),
      "Wrong parameter extent was accepted."_view);
  require(
      program
          .prepare(
              "absent"_view, form<Arguments>(), parameters.get_view(),
              diagnostic)
          .visit(
              [](auto&) { return False; },
              [](Data::Status status) {
                return Bool(status == Data::Status::Unsupported);
              }),
      "Unknown entry was accepted."_view);
  require(
      program
          .prepare(
              "transform"_view, form<Arguments>(), parameters.slice(0, 4),
              diagnostic)
          .visit(
              [](auto&) { return False; },
              [](Data::Status status) {
                return Bool(status == Data::Status::Incompatible);
              }),
      "Wrong argument count was accepted."_view);

  // Failed compilation cannot replace a previously published program. Kernel
  // and buffer publications then outlive the original Program and discovery.
  constexpr auto broken = "this is not CUDA source"_view;
  auto invalid = request;
  invalid.source.text = {
    broken.get_data(),
    broken.get_size(),
  };

  require(
      compiler.compile(invalid, diagnostic)
          .visit(
              [](auto&) { return False; },
              [](Data::Status status) {
                return Bool(status == Data::Status::Invalid);
              }),
      "Invalid source compiled."_view);
  require(
      !diagnostic.is_empty(), "Compilation lost the source diagnostic."_view);
  program_owner.close();

  Arguments frame{
    output.get_address(),
    input.get_address(),
    4,
    {
      2.0f,
      3,
    },
    0.5,
  };

  const Data::Form::Storage storage({
    &form<Arguments>(),
    reinterpret_cast<U8*>(&frame),
    sizeof(frame),
  });
  const cuda_launch geometry{
    1, 1, 1, 32, 1, 1, 0,
  };

  require(
      kernel.launch(geometry, storage) == Data::Status::Success,
      "General argument launch failed."_view);

  Core::Static::Vector<R32, 4> observed;
  require(
      output.read(
          0,
          {
            reinterpret_cast<U8*>(observed.get_data()),
            sizeof(observed),
          }) == Data::Status::Success,
      "Output observation failed."_view);
  for (Count i = 0; i != 4; ++i) {
    require(
        observed[i] == values[i] * 2 + 3.5f,
        "Kernel produced the wrong value."_view);
  }

  U32 unrelated = 0;
  require(
      kernel.launch(
          geometry, Data::Form::Storage({
                      &form<U32>(),
                      reinterpret_cast<U8*>(&unrelated),
                      sizeof(unrelated),
                    })) == Data::Status::Incompatible,
      "A different input frame bypassed agreement."_view);

  const auto allocations = Core::Bibliotheca::check_out_requests();
  const auto launch_started = Core::Time::now();
  for (Count i = 0; i != 1000; ++i) {
    require(
        kernel.launch(geometry, storage) == Data::Status::Success,
        "Warm launch failed."_view);
  }

  const auto launch_ns = launch_started.measure().convert_to_nanoseconds();
  require(
      allocations == Core::Bibliotheca::check_out_requests(),
      "Warm dispatch allocated host state."_view);

  Core::Static::Bytes<256> measurement;
  Core::Writer::Textual writer(measurement);
  writer << "CUDA compilation_ns="_view << U64(compilation_ns)
         << " preparation_ns="_view << U64(preparation_ns)
         << " warm_launch_and_completion_ns="_view << R64(launch_ns) / 1000
         << " measured_host_allocations=0\n"_view;
  Core::Diagnostics::Log::info(Core::View::Bytes(writer));
  Core::Diagnostics::Log::info(
      "PASS project CUDA: source includes, general arguments, POD, diagnostics, ownership and retained launches\n"_view);
}
