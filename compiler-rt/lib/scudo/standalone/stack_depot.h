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
  uptr ring[kRingSize];

public:
  u32 insert(uptr *begin, uptr *end) {
    MurMur2HashBuilder b;
    for (uptr *i = begin; i != end; ++i)
      b.add(*i);
    u32 hash = b.get();

    u32 pos = hash & kTabMask;
    u32 ring_pos = tab[pos];
    uptr entry = ring[ring_pos];
    uptr id = ((end - begin) << 33) | (uptr(hash) << 1) | 1;
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

  bool find(u32 hash) {
    u32 pos = hash & kTabMask;
    u32 ring_pos = tab[pos];
    uptr entry = ring[ring_pos];
    uptr hash_with_tag_bit = (uptr(hash) << 1) | 1;
    if ((entry & 0x1ffffffff) != hash_with_tag_bit)
      return false;
    u32 size = entry >> 33;
    MurMur2HashBuilder b;
    for (uptr i = 0; i != size; ++i) {
      ring_pos = (ring_pos + 1) & kRingMask;
      b.add(ring[ring_pos]);
    }
    return b.get() == hash;
  }
};

} // namespace scudo

#endif // SCUDO_STACK_DEPOT_H_
