//===-- scudo/interface.h ---------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef SCUDO_INTERFACE_H_
#define SCUDO_INTERFACE_H_

#include <stddef.h>

extern "C" {

__attribute__((weak)) const char *__scudo_default_options();

// Post-allocation & pre-deallocation hooks.
// They must be thread-safe and not use heap related functions.
__attribute__((weak)) void __scudo_allocate_hook(void *ptr, size_t size);
__attribute__((weak)) void __scudo_deallocate_hook(void *ptr);

void __scudo_print_stats(void);

typedef void (*iterate_callback)(uintptr_t base, size_t size, void *arg);

enum scudo_error_type {
  UNKNOWN,
  USE_AFTER_FREE,
  BUFFER_OVERFLOW,
  BUFFER_UNDERFLOW,
};

struct scudo_error_info {
  enum scudo_error_type error_type;

  uintptr_t allocation_address;
  uintptr_t allocation_size;

  uintptr_t allocation_trace[64];
  uintptr_t deallocation_trace[64];
};

void __scudo_get_error_info(struct scudo_error_info *error_info, uintptr_t ptr,
                            const char *stack_depot, const char *region_info,
                            const char *memory, const char *memory_tags,
                            uintptr_t memory_addr, size_t memory_size);

const char *__scudo_get_stack_depot_addr();
size_t __scudo_get_stack_depot_size();
const char *__scudo_get_region_info_addr();
size_t __scudo_get_region_info_size();

} // extern "C"

#endif // SCUDO_INTERFACE_H_
