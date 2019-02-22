//===- Writer.h -------------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLD_ELF_WRITER_H
#define LLD_ELF_WRITER_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include <cstdint>
#include <memory>

namespace lld {
namespace elf {
class InputFile;
class OutputSection;
class InputSectionBase;
template <class ELFT> class ObjFile;
class Symbol;
class SymbolTable;
class SyntheticSection;
class StringTableSection;
class SymbolTableBaseSection;
class GnuHashTableSection;
class HashTableSection;
class RelocationBaseSection;
class RelrBaseSection;

template <class ELFT> void writeResult();

// This describes a program header entry.
// Each contains type, access flags and range of output sections that will be
// placed in it.
struct PhdrEntry {
  PhdrEntry(unsigned Type, unsigned Flags) : p_type(Type), p_flags(Flags) {}
  void add(OutputSection *Sec);

  uint64_t p_paddr = 0;
  uint64_t p_vaddr = 0;
  uint64_t p_memsz = 0;
  uint64_t p_filesz = 0;
  uint64_t p_offset = 0;
  uint32_t p_align = 0;
  uint32_t p_type = 0;
  uint32_t p_flags = 0;

  OutputSection *FirstSec = nullptr;
  OutputSection *LastSec = nullptr;
  bool HasLMA = false;

  uint64_t LMAOffset = 0;
};

struct LoadableModule {
  OutputSection *ElfHeader;
  OutputSection *ProgramHeaders;
  std::vector<PhdrEntry *> Phdrs;
  SyntheticSection *Dynamic = nullptr;
  StringTableSection *DynStrTab = nullptr;
  SymbolTableBaseSection *DynSymTab = nullptr;
  GnuHashTableSection *GnuHashTab = nullptr;
  HashTableSection *HashTab = nullptr;
  RelocationBaseSection *RelaDyn = nullptr;
  RelrBaseSection *RelrDyn = nullptr;

  void addSymbolToDynsym(Symbol *S, bool IsDefined);
};

extern std::vector<LoadableModule> Mods;
extern uint64_t NumDynsyms;

void addReservedSymbols();
llvm::StringRef getOutputSectionName(const InputSectionBase *S);

template <class ELFT> uint32_t calcMipsEFlags();

uint8_t getMipsFpAbiFlag(uint8_t OldFlag, uint8_t NewFlag,
                         llvm::StringRef FileName);

bool isMipsN32Abi(const InputFile *F);
bool isMicroMips();
bool isMipsR6();
} // namespace elf
} // namespace lld

#endif
