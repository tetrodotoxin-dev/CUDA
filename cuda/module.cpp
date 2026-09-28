// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/modules/module.h"

#include "perimortem/core/null_terminated.hpp"

#include "cuda/runtime/program.hpp"

using namespace Perimortem;

// The compiler service has no discovery allocation or device state. Program
// compilation creates the independently owned publications. The module owner
// keeps this code loaded through every operation acquired from those results.
class Compiler {
 public:
  auto get_data() const -> Core::View::Bytes { return "CUDA"_view; }
  auto supports(System::Uuid id) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    return Cuda::Runtime::Program::compiler().supports(id);
  }
  auto bind_interface(System::Uuid id, Ttx::Data::Form::Storage target) const
      -> Ttx::Semantic::Negotiation::Binding::Status {
    return Cuda::Runtime::Program::compiler().bind(id, target);
  }
};

PERIMORTEM_C __attribute__((visibility("default"))) ttx_data_status
    ttx_module_open(ttx_semantic_query, ttx_module_acquisition* output) {
  static const Compiler compiler;
  *output = {
    Ttx::Concept::Abstract::provide(compiler).get_abi(),
    nullptr,
    [](const void*) {},
  };
  return TTX_DATA_SUCCESS;
}
