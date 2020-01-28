//===-- sanitizer_stackdepot.h ----------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is shared between AddressSanitizer and ThreadSanitizer
// run-time libraries.
//===----------------------------------------------------------------------===//

#ifndef SANITIZER_STACKDEPOT_H
#define SANITIZER_STACKDEPOT_H

#include "sanitizer_common.h"
#include "sanitizer_hash.h"
#include "sanitizer_internal_defs.h"
#include "sanitizer_stackdepotbase.h"
#include "sanitizer_stacktrace.h"

namespace __sanitizer {

// StackDepot efficiently stores huge amounts of stack traces.
struct StackDepotNode;
struct StackDepotHandle {
  StackDepotNode *node_;
  StackDepotHandle() : node_(nullptr) {}
  explicit StackDepotHandle(StackDepotNode *node) : node_(node) {}
  bool valid() { return node_; }
  u32 id();
  int use_count();
  void inc_use_count_unsafe();
};

struct StackDepotNode {
  StackDepotNode *link;
  u32 id;
  atomic_uint32_t hash_and_use_count; // hash_bits : 12; use_count : 20;
  u32 size;
  u32 tag;
  uptr stack[1];  // [size]

  static const u32 kTabSizeLog = SANITIZER_ANDROID ? 16 : 20;
  // Lower kTabSizeLog bits are equal for all items in one bucket.
  // We use these bits to store the per-stack use counter.
  static const u32 kUseCountBits = kTabSizeLog;
  static const u32 kMaxUseCount = 1 << kUseCountBits;
  static const u32 kUseCountMask = (1 << kUseCountBits) - 1;
  static const u32 kHashMask = ~kUseCountMask;

  typedef StackTrace args_type;
  bool eq(u32 hash, const args_type &args) const {
    u32 hash_bits =
        atomic_load(&hash_and_use_count, memory_order_relaxed) & kHashMask;
    if ((hash & kHashMask) != hash_bits || args.size != size || args.tag != tag)
      return false;
    uptr i = 0;
    for (; i < size; i++) {
      if (stack[i] != args.trace[i]) return false;
    }
    return true;
  }
  static uptr storage_size(const args_type &args) {
    return sizeof(StackDepotNode) + (args.size - 1) * sizeof(uptr);
  }
  static u32 hash(const args_type &args) {
    MurMur2HashBuilder H(args.size * sizeof(uptr));
    for (uptr i = 0; i < args.size; i++) H.add(args.trace[i]);
    return H.get();
  }
  static bool is_valid(const args_type &args) {
    return args.size > 0 && args.trace;
  }
  void store(const args_type &args, u32 hash) {
    atomic_store(&hash_and_use_count, hash & kHashMask, memory_order_relaxed);
    size = args.size;
    tag = args.tag;
    internal_memcpy(stack, args.trace, size * sizeof(uptr));
  }
  args_type load() const {
    return args_type(&stack[0], size, tag);
  }
  StackDepotHandle get_handle() { return StackDepotHandle(this); }

  typedef StackDepotHandle handle_type;
};

const int kStackDepotMaxUseCount = 1U << (SANITIZER_ANDROID ? 16 : 20);

typedef StackDepotBase<StackDepotNode, 1, StackDepotNode::kTabSizeLog>
    StackDepot;
extern StackDepot theDepot;

StackDepotStats *StackDepotGetStats();
u32 StackDepotPut(StackTrace stack, uptr alloc_size = 0);
u32 StackDepotPutNoRecord(StackTrace stack);
StackDepotHandle StackDepotPut_WithHandle(StackTrace stack);
// Retrieves a stored stack trace by the id.
StackTrace StackDepotGet(u32 id);

void StackDepotLockAll();
void StackDepotUnlockAll();

// Instantiating this class creates a snapshot of StackDepot which can be
// efficiently queried with StackDepotGet(). You can use it concurrently with
// StackDepot, but the snapshot is only guaranteed to contain those stack traces
// which were stored before it was instantiated.
class StackDepotReverseMap {
 public:
  StackDepotReverseMap();
  StackTrace Get(u32 id);

 private:
  struct IdDescPair {
    u32 id;
    StackDepotNode *desc;

    static bool IdComparator(const IdDescPair &a, const IdDescPair &b);
  };

  InternalMmapVector<IdDescPair> map_;

  // Disallow evil constructors.
  StackDepotReverseMap(const StackDepotReverseMap&);
  void operator=(const StackDepotReverseMap&);
};

} // namespace __sanitizer

#endif // SANITIZER_STACKDEPOT_H
