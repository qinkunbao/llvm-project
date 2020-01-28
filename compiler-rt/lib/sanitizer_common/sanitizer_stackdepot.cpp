//===-- sanitizer_stackdepot.cpp ------------------------------------------===//
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

#include "sanitizer_stackdepot.h"

#include "sanitizer_allocator_internal.h"
#include "sanitizer_common.h"
#include "sanitizer_hash.h"
#include "sanitizer_stackdepotbase.h"

namespace __sanitizer {

struct StackTraceLog {
  StackTraceLog *next;
  uptr tid;
  uptr alloc_size;
  uptr size;
  uptr *trace() { return reinterpret_cast<uptr *>(this + 1); }
};

__attribute__((aligned(4096))) struct {
  atomic_uintptr_t log;
  atomic_uintptr_t inited;
} theLog;

COMPILER_CHECK(StackDepotNode::kMaxUseCount == (u32)kStackDepotMaxUseCount);

u32 StackDepotHandle::id() { return node_->id; }
int StackDepotHandle::use_count() {
  return atomic_load(&node_->hash_and_use_count, memory_order_relaxed) &
         StackDepotNode::kUseCountMask;
}
void StackDepotHandle::inc_use_count_unsafe() {
  u32 prev =
      atomic_fetch_add(&node_->hash_and_use_count, 1, memory_order_relaxed) &
      StackDepotNode::kUseCountMask;
  CHECK_LT(prev + 1, StackDepotNode::kMaxUseCount);
}

// FIXME(dvyukov): this single reserved bit is used in TSan.
StackDepot theDepot;

StackDepotStats *StackDepotGetStats() {
  return theDepot.GetStats();
}

u32 StackDepotPut(StackTrace stack, uptr alloc_size) {
  uptr zero = 0;
  if (atomic_compare_exchange_strong(&theLog.inited, &zero, 1,
                                     memory_order_relaxed)) {
    DecorateMapping((uptr)&theLog, 4096, "depot log");
  }

  StackTraceLog *log = (StackTraceLog *)InternalAlloc(
      sizeof(StackTraceLog) + sizeof(uptr) * stack.size);
  log->tid = GetTid();
  log->alloc_size = alloc_size;
  log->size = stack.size;
  for (uptr i = 0; i != stack.size; ++i)
    log->trace()[i] = stack.trace[i];
  uptr next = atomic_load(&theLog.log, memory_order_acquire);
  do {
    log->next = (StackTraceLog *)next;
  } while (!atomic_compare_exchange_strong(&theLog.log, &next, (uptr)log,
                                           memory_order_release));

  return StackDepotPutNoRecord(stack);
}

u32 StackDepotPutNoRecord(StackTrace stack) {
  StackDepotHandle h = theDepot.Put(stack);
  return h.valid() ? h.id() : 0;
}

StackDepotHandle StackDepotPut_WithHandle(StackTrace stack) {
  return theDepot.Put(stack);
}

StackTrace StackDepotGet(u32 id) {
  return theDepot.Get(id);
}

void StackDepotLockAll() {
  theDepot.LockAll();
}

void StackDepotUnlockAll() {
  theDepot.UnlockAll();
}

bool StackDepotReverseMap::IdDescPair::IdComparator(
    const StackDepotReverseMap::IdDescPair &a,
    const StackDepotReverseMap::IdDescPair &b) {
  return a.id < b.id;
}

StackDepotReverseMap::StackDepotReverseMap() {
  map_.reserve(StackDepotGetStats()->n_uniq_ids + 100);
  for (int idx = 0; idx < StackDepot::kTabSize; idx++) {
    atomic_uintptr_t *p = &theDepot.tab[idx];
    uptr v = atomic_load(p, memory_order_consume);
    StackDepotNode *s = (StackDepotNode*)(v & ~1);
    for (; s; s = s->link) {
      IdDescPair pair = {s->id, s};
      map_.push_back(pair);
    }
  }
  Sort(map_.data(), map_.size(), &IdDescPair::IdComparator);
}

StackTrace StackDepotReverseMap::Get(u32 id) {
  if (!map_.size())
    return StackTrace();
  IdDescPair pair = {id, nullptr};
  uptr idx =
      InternalLowerBound(map_, 0, map_.size(), pair, IdDescPair::IdComparator);
  if (idx > map_.size() || map_[idx].id != id)
    return StackTrace();
  return map_[idx].desc->load();
}

} // namespace __sanitizer
