//===-- size_class_map.h ----------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef SCUDO_SIZE_CLASS_MAP_H_
#define SCUDO_SIZE_CLASS_MAP_H_

#include "chunk.h"
#include "common.h"
#include "string_utils.h"

namespace scudo {

// SizeClassMap maps allocation sizes into size classes and back, in an
// efficient table-free manner.
//
// Class 0 is a special class that doesn't abide by the same rules as other
// classes. The allocator uses it to hold batches.
//
// The other sizes are controlled by the template parameters:
// - MinSizeLog: defines the first class as 2^MinSizeLog bytes.
// - MaxSizeLog: defines the last class as 2^MaxSizeLog bytes.
// - MidSizeLog: classes increase with step 2^MinSizeLog from 2^MinSizeLog to
//               2^MidSizeLog bytes.
// - NumBits: the number of non-zero bits in sizes after 2^MidSizeLog.
//            eg. with NumBits==3 all size classes after 2^MidSizeLog look like
//            0b1xx0..0 (where x is either 0 or 1).
//
// This class also gives a hint to a thread-caching allocator about the amount
// of chunks that can be cached per-thread:
// - MaxNumCachedHint is a hint for the max number of chunks cached per class.
// - 2^MaxBytesCachedLog is the max number of bytes cached per class.

inline uptr scaledLog2(uptr Size, uptr ZeroLog, uptr LogBits) {
  const uptr L = getMostSignificantSetBitIndex(Size);
  const uptr LBits = (Size >> (L - LogBits)) - (1 << LogBits);
  const uptr HBits = (L - ZeroLog) << LogBits;
  return LBits + HBits;
}

template <typename Config> struct SizeClassMapBase {
  static u32 getMaxCachedHint(uptr Size) {
    DCHECK_LE(Size, MaxSize);
    DCHECK_NE(Size, 0);
    u32 N;
    // Force a 32-bit division if the template parameters allow for it.
    if (Config::MaxBytesCachedLog > 31 || Config::MaxSizeLog > 31)
      N = static_cast<u32>((1UL << Config::MaxBytesCachedLog) / Size);
    else
      N = (1U << Config::MaxBytesCachedLog) / static_cast<u32>(Size);
    return Max(1U, Min(Config::MaxNumCachedHint, N));
  }
};

template <typename Config>
class TableSizeClassMap : public SizeClassMapBase<Config> {
  static const uptr MinSize = 1UL << Config::MinSizeLog;
  static const uptr MidSize = 1UL << Config::MidSizeLog;
  static const uptr MidClass = MidSize / MinSize;
  static const u8 S = Config::NumBits - 1;
  static const uptr M = (1UL << S) - 1;
  static const uptr ClassesSize =
      sizeof(Config::Classes) / sizeof(Config::Classes[0]);

  struct SizeTable {
    constexpr SizeTable() {
      uptr Pos = 1 << Config::MidSizeLog;
      uptr Inc = 1 << (Config::MidSizeLog - S);
      for (uptr i = 0; i != getTableSize(); ++i) {
        Pos += Inc;
        if ((Pos & (Pos - 1)) == 0)
          Inc *= 2;
        Tab[i] = computeClassId(Pos + Config::SizeDelta);
      }
    }

    constexpr static u8 computeClassId(uptr Size) {
      for (uptr i = 0; i != ClassesSize; ++i) {
        if (Size <= Config::Classes[i])
          return i + 1;
      }
      return -1;
    }

    constexpr static uptr getTableSize() {
      return (Config::MaxSizeLog - Config::MidSizeLog) << S;
    }

    u8 Tab[getTableSize()] = {};
  };

  static constexpr SizeTable Table =  {}; 

public:
  static const u32 MaxNumCachedHint = Config::MaxNumCachedHint;

  static const uptr NumClasses = ClassesSize + 1;
  static_assert(NumClasses < 256, "");
  static const uptr LargestClassId = NumClasses - 1;
  static const uptr BatchClassId = 0;
  static const uptr MaxSize = Config::Classes[LargestClassId - 1];

  static uptr getSizeByClassId(uptr ClassId) {
    return Config::Classes[ClassId - 1];
  }

  static uptr getClassIdBySize(uptr Size) {
    if (Size <= Config::Classes[0])
      return 1;
    Size -= Config::SizeDelta;
    DCHECK_LE(Size, MaxSize);
    if (Size <= (1 << Config::MidSizeLog))
      return ((Size - 1) >> Config::MinSizeLog) + 1;
    return Table.Tab[scaledLog2(Size - 1, Config::MidSizeLog, S)];
  }

  static void print() {}
  static void validate() {}
};

template <typename Config>
class FixedSizeClassMap : public SizeClassMapBase<Config> {
  typedef SizeClassMapBase<Config> Base;

  static const uptr MinSize = 1UL << Config::MinSizeLog;
  static const uptr MidSize = 1UL << Config::MidSizeLog;
  static const uptr MidClass = MidSize / MinSize;
  static const u8 S = Config::NumBits - 1;
  static const uptr M = (1UL << S) - 1;

  static const uptr SizeDelta = Chunk::getHeaderSize();

public:
  static const u32 MaxNumCachedHint = Config::MaxNumCachedHint;

  static const uptr MaxSize = (1UL << Config::MaxSizeLog) + SizeDelta;
  static const uptr NumClasses =
      MidClass + ((Config::MaxSizeLog - Config::MidSizeLog) << S) + 1;
  static_assert(NumClasses <= 256, "");
  static const uptr LargestClassId = NumClasses - 1;
  static const uptr BatchClassId = 0;

  static uptr getSizeByClassId(uptr ClassId) {
    DCHECK_NE(ClassId, BatchClassId);
    if (ClassId <= MidClass)
      return (ClassId << Config::MinSizeLog) + SizeDelta;
    ClassId -= MidClass;
    const uptr T = MidSize << (ClassId >> S);
    return T + (T >> S) * (ClassId & M) + SizeDelta;
  }

  static uptr getClassIdBySize(uptr Size) {
    if (Size <= SizeDelta + (1 << Config::MinSizeLog))
      return 1;
    Size -= SizeDelta;
    DCHECK_LE(Size, MaxSize);
    if (Size <= MidSize) {
      if (Size == 0)
        return 1;
      return (Size + MinSize - 1) >> Config::MinSizeLog;
    }
    return MidClass + 1 + scaledLog2(Size - 1, Config::MidSizeLog, S);
  }

  static void print() {
    ScopedString Buffer(1024);
    uptr PrevS = 0;
    uptr TotalCached = 0;
    for (uptr I = 0; I < NumClasses; I++) {
      if (I == BatchClassId)
        continue;
      const uptr S = getSizeByClassId(I);
      if (S >= MidSize / 2 && (S & (S - 1)) == 0)
        Buffer.append("\n");
      const uptr D = S - PrevS;
      const uptr P = PrevS ? (D * 100 / PrevS) : 0;
      const uptr L = S ? getMostSignificantSetBitIndex(S) : 0;
      const uptr Cached = Base::getMaxCachedHint(S) * S;
      Buffer.append(
          "C%02zu => S: %zu diff: +%zu %02zu%% L %zu Cached: %zu %zu; id %zu\n",
          I, getSizeByClassId(I), D, P, L, Base::getMaxCachedHint(S), Cached,
          getClassIdBySize(S));
      TotalCached += Cached;
      PrevS = S;
    }
    Buffer.append("Total Cached: %zu\n", TotalCached);
    Buffer.output();
  }

  static void validate() {
    for (uptr C = 0; C < NumClasses; C++) {
      if (C == BatchClassId)
        continue;
      const uptr S = getSizeByClassId(C);
      CHECK_NE(S, 0U);
      CHECK_EQ(getClassIdBySize(S), C);
      if (C < LargestClassId)
        CHECK_EQ(getClassIdBySize(S + 1), C + 1);
      CHECK_EQ(getClassIdBySize(S - 1), C);
      if (C - 1 != BatchClassId)
        CHECK_GT(getSizeByClassId(C), getSizeByClassId(C - 1));
    }
    // Do not perform the loop if the maximum size is too large.
    if (Config::MaxSizeLog > 19)
      return;
    for (uptr S = 1; S <= MaxSize; S++) {
      const uptr C = getClassIdBySize(S);
      CHECK_LT(C, NumClasses);
      CHECK_GE(getSizeByClassId(C), S);
      if (C - 1 != BatchClassId)
        CHECK_LT(getSizeByClassId(C - 1), S);
    }
  }
};

#if 1
struct AndroidSizeClassConfig {
  static const uptr NumBits = 5;
  static const uptr MinSizeLog = 4;
  static const uptr MidSizeLog = 6;
  static const uptr MaxSizeLog = 16;
  static const u32 MaxNumCachedHint = 14;
  static const uptr MaxBytesCachedLog = 14;

  static constexpr u32 Classes[32] = {
      0x20,   0x30,   0x40,   0x50,   0x60,   0x90,   0xa0,   0xb0,
      0xd0,   0xf0,   0x150,  0x1a0,  0x1c0,  0x210,  0x250,  0x330,
      0x450,  0x610,  0x810,  0xa10,  0xc10,  0x1010, 0x1310, 0x1c10,
      0x2210, 0x3210, 0x3610, 0x4010, 0x4810, 0x5c10, 0x7410, 0x10010,
  };
  static const uptr SizeDelta = 16;
};
typedef TableSizeClassMap<AndroidSizeClassConfig> AndroidSizeClassMap;
#else
struct AndroidSizeClassConfig {
  static const uptr NumBits = 2;
  static const uptr MinSizeLog = 5;
  static const uptr MidSizeLog = 9;
  static const uptr MaxSizeLog = 16;
  static const u32 MaxNumCachedHint = 14;
  static const uptr MaxBytesCachedLog = 14;
};
typedef FixedSizeClassMap<AndroidSizeClassConfig> AndroidSizeClassMap;
#endif

struct DefaultSizeClassConfig {
  static const uptr NumBits = 3;
  static const uptr MinSizeLog = 5;
  static const uptr MidSizeLog = 8;
  static const uptr MaxSizeLog = 17;
  static const u32 MaxNumCachedHint = 8;
  static const uptr MaxBytesCachedLog = 10;
};

typedef FixedSizeClassMap<DefaultSizeClassConfig> DefaultSizeClassMap;

struct SvelteSizeClassConfig {
#if SCUDO_WORDSIZE == 64U
  static const uptr NumBits = 4;
  static const uptr MinSizeLog = 4;
  static const uptr MidSizeLog = 8;
  static const uptr MaxSizeLog = 14;
  static const u32 MaxNumCachedHint = 4;
  static const uptr MaxBytesCachedLog = 10;
#else
  static const uptr NumBits = 4;
  static const uptr MinSizeLog = 3;
  static const uptr MidSizeLog = 7;
  static const uptr MaxSizeLog = 14;
  static const u32 MaxNumCachedHint = 5;
  static const uptr MaxBytesCachedLog = 10;
#endif
};

typedef FixedSizeClassMap<SvelteSizeClassConfig> SvelteSizeClassMap;

} // namespace scudo

#endif // SCUDO_SIZE_CLASS_MAP_H_
