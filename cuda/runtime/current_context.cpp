// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cuda/runtime/current_context.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Cuda;
using namespace Perimortem;

Runtime::CurrentContext::CurrentContext(CUcontext context)
    : status(cuCtxPushCurrent(context)) {}
Runtime::CurrentContext::~CurrentContext() {
  if (status != CUDA_SUCCESS) {
    return;
  }

  CUcontext previous;
  if (cuCtxPopCurrent(&previous) != CUDA_SUCCESS) {
    Core::Diagnostics::Log::fatal(
        "CUDA could not restore the caller's current context."_view);
  }
}
