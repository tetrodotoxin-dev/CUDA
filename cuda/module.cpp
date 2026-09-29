// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "cuda/runtime/program.hpp"
#include "ttx/semantic/negotiation/library.h"

// The compiler has no discovery graph or device state. The native entry lends
// its Query directly, while compiled programs negotiate their own lifetimes.
PERIMORTEM_C ttx_binding_status
    ttx_query(ttx_semantic_query, ttx_query_receiver receive) {
  return receive.receive(receive.source, Cuda::Runtime::Program::compiler());
}
