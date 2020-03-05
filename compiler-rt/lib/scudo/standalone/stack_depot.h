//===-- stack_depot.h -------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef SCUDO_STACK_DEPOT_H_
#define SCUDO_STACK_DEPOT_H_

#include "mutex.h"

namespace scudo {

class MurMur2HashBuilder {
  static const u32 m = 0x5bd1e995;
  static const u32 seed = 0x9747b28c;
  static const u32 r = 24;
  u32 h;

 public:
  explicit MurMur2HashBuilder(u32 init = 0) { h = seed ^ init; }
  void add(u32 k) {
    k *= m;
    k ^= k >> r;
    k *= m;
    h *= m;
    h ^= k;
  }
  u32 get() {
    u32 x = h;
    x ^= x >> 13;
    x *= m;
    x ^= x >> 15;
    return x;
  }
};

class StackDepot {
  HybridMutex ring_end_mu;
  u32 ring_end;

  static const uptr kTabBits = 16;
  static const uptr kTabSize = 1 << kTabBits;
  static const uptr kTabMask = kTabSize - 1;
  u32 tab[kTabSize];

  static const uptr kRingBits = 19;
  static const uptr kRingSize = 1 << kRingBits;
  static const uptr kRingMask = kRingSize - 1;
  u64 ring[kRingSize];

public:
  u32 insert(uptr *begin, uptr *end) {
    MurMur2HashBuilder b;
    for (uptr *i = begin; i != end; ++i)
      b.add(u32(*i));
    u32 hash = b.get();

    u32 pos = hash & kTabMask;
    u32 ring_pos = tab[pos];
    u64 entry = ring[ring_pos];
    u64 id = (u64(end - begin) << 33) | (u64(hash) << 1) | 1;
    if (entry == id)
      return hash;

    ScopedLock lock(ring_end_mu);
    ring_pos = ring_end;
    tab[pos] = ring_pos;
    ring[ring_pos] = id;
    for (uptr *i = begin; i != end; ++i) {
      ring_pos = (ring_pos + 1) & kRingMask;
      ring[ring_pos] = *i;
    }
    ring_end = (ring_pos + 1) & kRingMask;
    return hash;
  }

  bool find(u32 hash, uptr *ring_pos_ptr, uptr *size_ptr) const {
    u32 pos = hash & kTabMask;
    u32 ring_pos = tab[pos];
    u64 entry = ring[ring_pos];
    u64 hash_with_tag_bit = (u64(hash) << 1) | 1;
    if ((entry & 0x1ffffffff) != hash_with_tag_bit)
      return false;
    u32 size = entry >> 33;
    *ring_pos_ptr = (ring_pos + 1) & kRingMask;
    *size_ptr = size;
    MurMur2HashBuilder b;
    for (uptr i = 0; i != size; ++i) {
      ring_pos = (ring_pos + 1) & kRingMask;
      b.add(u32(ring[ring_pos]));
    }
    return b.get() == hash;
  }

  u64 operator[](uptr ring_pos) const { return ring[ring_pos & kRingMask]; }
};

} // namespace scudo

#endif // SCUDO_STACK_DEPOT_H_
