//===- lib/MC/AArch64ELFStreamer.cpp - ELF Object Output for AArch64 ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file assembles .s files and emits AArch64 ELF .o object files. Different
// from generic ELF streamer in emitting mapping symbols ($x and $d) to delimit
// regions of data and code.
//
//===----------------------------------------------------------------------===//

#include "AArch64TargetStreamer.h"
#include "AArch64WinCOFFStreamer.h"
#include "AArch64InstrInfo.h"
#include "MCTargetDesc/AArch64AddressingModes.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/ADT/Twine.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/MC/MCWinCOFFStreamer.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace llvm {
cl::opt<bool> FakePAC("fake-pac");
}

namespace {

class AArch64ELFStreamer;

class AArch64TargetAsmStreamer : public AArch64TargetStreamer {
  formatted_raw_ostream &OS;

  void emitInst(uint32_t Inst) override;

  void emitDirectiveVariantPCS(MCSymbol *Symbol) override {
    OS << "\t.variant_pcs " << Symbol->getName() << "\n";
  }

  void EmitARM64WinCFIAllocStack(unsigned Size) override {
    OS << "\t.seh_stackalloc " << Size << "\n";
  }
  void EmitARM64WinCFISaveR19R20X(int Offset) override {
    OS << "\t.seh_save_r19r20_x " << Offset << "\n";
  }
  void EmitARM64WinCFISaveFPLR(int Offset) override {
    OS << "\t.seh_save_fplr " << Offset << "\n";
  }
  void EmitARM64WinCFISaveFPLRX(int Offset) override {
    OS << "\t.seh_save_fplr_x " << Offset << "\n";
  }
  void EmitARM64WinCFISaveReg(unsigned Reg, int Offset) override {
    OS << "\t.seh_save_reg x" << Reg << ", " << Offset << "\n";
  }
  void EmitARM64WinCFISaveRegX(unsigned Reg, int Offset) override {
    OS << "\t.seh_save_reg_x x" << Reg << ", " << Offset << "\n";
  }
  void EmitARM64WinCFISaveRegP(unsigned Reg, int Offset) override {
    OS << "\t.seh_save_regp x" << Reg << ", " << Offset << "\n";
  }
  void EmitARM64WinCFISaveRegPX(unsigned Reg, int Offset) override {
    OS << "\t.seh_save_regp_x x" << Reg << ", " << Offset << "\n";
  }
  void EmitARM64WinCFISaveLRPair(unsigned Reg, int Offset) override {
    OS << "\t.seh_save_lrpair x" << Reg << ", " << Offset << "\n";
  }
  void EmitARM64WinCFISaveFReg(unsigned Reg, int Offset) override {
    OS << "\t.seh_save_freg d" << Reg << ", " << Offset << "\n";
  }
  void EmitARM64WinCFISaveFRegX(unsigned Reg, int Offset) override {
    OS << "\t.seh_save_freg_x d" << Reg << ", " << Offset << "\n";
  }
  void EmitARM64WinCFISaveFRegP(unsigned Reg, int Offset) override {
    OS << "\t.seh_save_fregp d" << Reg << ", " << Offset << "\n";
  }
  void EmitARM64WinCFISaveFRegPX(unsigned Reg, int Offset) override {
    OS << "\t.seh_save_fregp_x d" << Reg << ", " << Offset << "\n";
  }
  void EmitARM64WinCFISetFP() override { OS << "\t.seh_set_fp\n"; }
  void EmitARM64WinCFIAddFP(unsigned Size) override {
    OS << "\t.seh_add_fp " << Size << "\n";
  }
  void EmitARM64WinCFINop() override { OS << "\t.seh_nop\n"; }
  void EmitARM64WinCFISaveNext() override { OS << "\t.seh_save_next\n"; }
  void EmitARM64WinCFIPrologEnd() override { OS << "\t.seh_endprologue\n"; }
  void EmitARM64WinCFIEpilogStart() override { OS << "\t.seh_startepilogue\n"; }
  void EmitARM64WinCFIEpilogEnd() override { OS << "\t.seh_endepilogue\n"; }
  void EmitARM64WinCFITrapFrame() override { OS << "\t.seh_trap_frame\n"; }
  void EmitARM64WinCFIMachineFrame() override { OS << "\t.seh_pushframe\n"; }
  void EmitARM64WinCFIContext() override { OS << "\t.seh_context\n"; }
  void EmitARM64WinCFIClearUnwoundToCall() override {
    OS << "\t.seh_clear_unwound_to_call\n";
  }

public:
  AArch64TargetAsmStreamer(MCStreamer &S, formatted_raw_ostream &OS);
};

AArch64TargetAsmStreamer::AArch64TargetAsmStreamer(MCStreamer &S,
                                                   formatted_raw_ostream &OS)
  : AArch64TargetStreamer(S), OS(OS) {}

void AArch64TargetAsmStreamer::emitInst(uint32_t Inst) {
  OS << "\t.inst\t0x" << Twine::utohexstr(Inst) << "\n";
}

/// Extend the generic ELFStreamer class so that it can emit mapping symbols at
/// the appropriate points in the object files. These symbols are defined in the
/// AArch64 ELF ABI:
///    infocenter.arm.com/help/topic/com.arm.doc.ihi0056a/IHI0056A_aaelf64.pdf
///
/// In brief: $x or $d should be emitted at the start of each contiguous region
/// of A64 code or data in a section. In practice, this emission does not rely
/// on explicit assembler directives but on inherent properties of the
/// directives doing the emission (e.g. ".byte" is data, "add x0, x0, x0" an
/// instruction).
///
/// As a result this system is orthogonal to the DataRegion infrastructure used
/// by MachO. Beware!
class AArch64ELFStreamer : public MCELFStreamer {
public:
  AArch64ELFStreamer(MCContext &Context, std::unique_ptr<MCAsmBackend> TAB,
                     std::unique_ptr<MCObjectWriter> OW,
                     std::unique_ptr<MCCodeEmitter> Emitter)
      : MCELFStreamer(Context, std::move(TAB), std::move(OW),
                      std::move(Emitter)),
        MappingSymbolCounter(0), LastEMS(EMS_None) {}

  void changeSection(MCSection *Section, const MCExpr *Subsection) override {
    // We have to keep track of the mapping symbol state of any sections we
    // use. Each one should start off as EMS_None, which is provided as the
    // default constructor by DenseMap::lookup.
    LastMappingSymbols[getPreviousSection().first] = LastEMS;
    LastEMS = LastMappingSymbols.lookup(Section);

    MCELFStreamer::changeSection(Section, Subsection);
  }

  // Reset state between object emissions
  void reset() override {
    MappingSymbolCounter = 0;
    MCELFStreamer::reset();
    LastMappingSymbols.clear();
    LastEMS = EMS_None;
  }

  /// This function is the one used to emit instruction data into the ELF
  /// streamer. We override it to add the appropriate mapping symbol if
  /// necessary.
  void emitInstruction(const MCInst &Inst,
                       const MCSubtargetInfo &STI) override {
    EmitA64MappingSymbol();

    if (FakePAC) {
      switch (Inst.getOpcode()) {
      case AArch64::XPACI:
      case AArch64::XPACD: {
        MCInst MI1 = MCInstBuilder(AArch64::ANDXri)
              .addOperand(Inst.getOperand(0))
              .addOperand(Inst.getOperand(0))
              .addImm(AArch64_AM::encodeLogicalImmediate(0x7fffffffff, 64));
        MCELFStreamer::emitInstruction(MI1, STI);
        return;
      }
      case AArch64::PACIA:
      case AArch64::AUTIA:
      case AArch64::PACIB:
      case AArch64::AUTIB:
      case AArch64::PACDA:
      case AArch64::AUTDA:
      case AArch64::PACDB:
      case AArch64::AUTDB:
      case AArch64::PACIAZ:
      case AArch64::PACIZA:
      case AArch64::PACIA1716:
      case AArch64::PACIASP:
      case AArch64::PACIBZ:
      case AArch64::PACIZB:
      case AArch64::PACIB1716:
      case AArch64::PACIBSP:
      case AArch64::PACDZA:
      case AArch64::PACDZB:
      case AArch64::AUTIAZ:
      case AArch64::AUTIZA:
      case AArch64::AUTIA1716:
      case AArch64::AUTIASP:
      case AArch64::AUTIBZ:
      case AArch64::AUTIZB:
      case AArch64::AUTIB1716:
      case AArch64::AUTIBSP:
      case AArch64::AUTDZA:
      case AArch64::AUTDZB:
      case AArch64::RETAA:
      case AArch64::RETAB:
      case AArch64::BRAA:
      case AArch64::BRAB:
      case AArch64::BRAAZ:
      case AArch64::BRABZ:
      case AArch64::BLRAA:
      case AArch64::BLRAB:
      case AArch64::BLRAAZ:
      case AArch64::BLRABZ: {
        unsigned DisableBit;
        switch (Inst.getOpcode()) {
        case AArch64::PACIA:
        case AArch64::AUTIA:
        case AArch64::PACIAZ:
        case AArch64::PACIZA:
        case AArch64::PACIA1716:
        case AArch64::PACIASP:
        case AArch64::AUTIAZ:
        case AArch64::AUTIZA:
        case AArch64::AUTIA1716:
        case AArch64::AUTIASP:
        case AArch64::RETAA:
        case AArch64::BRAA:
        case AArch64::BRAAZ:
        case AArch64::BLRAA:
        case AArch64::BLRAAZ:
          DisableBit = 60;
          break;
        case AArch64::PACIB:
        case AArch64::AUTIB:
        case AArch64::PACIBZ:
        case AArch64::PACIZB:
        case AArch64::PACIB1716:
        case AArch64::PACIBSP:
        case AArch64::AUTIBZ:
        case AArch64::AUTIZB:
        case AArch64::AUTIB1716:
        case AArch64::AUTIBSP:
        case AArch64::RETAB:
        case AArch64::BRAB:
        case AArch64::BRABZ:
        case AArch64::BLRAB:
        case AArch64::BLRABZ:
          DisableBit = 61;
          break;
        case AArch64::PACDA:
        case AArch64::AUTDA:
        case AArch64::PACDZA:
        case AArch64::AUTDZA:
          DisableBit = 62;
          break;
        case AArch64::PACDB:
        case AArch64::AUTDB:
        case AArch64::PACDZB:
        case AArch64::AUTDZB:
          DisableBit = 63;
          break;
        }

        unsigned Reg, DiscReg;
        switch (Inst.getOpcode()) {
        case AArch64::PACIAZ:
        case AArch64::AUTIAZ:
        case AArch64::PACIASP:
        case AArch64::AUTIASP:
        case AArch64::PACIBSP:
        case AArch64::AUTIBSP:
        case AArch64::RETAA:
        case AArch64::RETAB:
        case AArch64::BRAAZ:
        case AArch64::BLRAAZ:
        case AArch64::BRABZ:
        case AArch64::BLRABZ:
          Reg = AArch64::LR;
          DiscReg = AArch64::XZR; // not correct for *SP or RET* but we don't have a spare reg
          break;
        case AArch64::PACIA1716:
        case AArch64::AUTIA1716:
        case AArch64::PACIB1716:
        case AArch64::AUTIB1716:
          Reg = AArch64::X17;
          DiscReg = AArch64::X16;
          break;
        case AArch64::PACIZA:
        case AArch64::AUTIZA:
        case AArch64::PACDZA:
        case AArch64::AUTDZA:
          Reg = Inst.getOperand(0).getReg();
          DiscReg = AArch64::XZR;
          break;
        case AArch64::BRAA:
        case AArch64::BRAB:
        case AArch64::BLRAA:
        case AArch64::BLRAB:
          Reg = Inst.getOperand(0).getReg();
          DiscReg = Inst.getOperand(1).getReg();
          if (DiscReg == AArch64::SP)
            DiscReg = AArch64::XZR;
          break;
        default:
          Reg = Inst.getOperand(0).getReg();
          DiscReg = Inst.getOperand(2).getReg();
          if (DiscReg == AArch64::SP)
            DiscReg = AArch64::XZR;
          break;
        }

        if (Inst.getOpcode() == AArch64::BLRAA ||
            Inst.getOpcode() == AArch64::BLRAAZ ||
            Inst.getOpcode() == AArch64::BLRAB ||
            Inst.getOpcode() == AArch64::BLRABZ ||
            Inst.getFlags()) {
          unsigned TempReg = AArch64::X16;
          if (DiscReg == AArch64::X16)
            TempReg = AArch64::X17;
          if (Reg != TempReg) {
            MCInst MI0 = MCInstBuilder(AArch64::ORRXrs)
                             .addReg(TempReg)
                             .addReg(AArch64::XZR)
                             .addReg(Reg)
                             .addImm(0);
            MCELFStreamer::emitInstruction(MI0, STI);
          }
          Reg = TempReg;
        }

        if (Reg == DiscReg)
          assert(0 && "can't encode this");

        MCInst MI1 = MCInstBuilder(AArch64::TBNZX)
              .addReg(AArch64::X18)
              .addImm(DisableBit)
              .addImm(DiscReg == AArch64::XZR ? 2 : 5);
        MCELFStreamer::emitInstruction(MI1, STI);

        if (DiscReg != AArch64::XZR) {
          MCInst MI2 = MCInstBuilder(AArch64::EXTRXrri) // ror
                           .addReg(DiscReg)
                           .addReg(DiscReg)
                           .addReg(DiscReg)
                           .addImm(64 - 39);
          MCELFStreamer::emitInstruction(MI2, STI);

          MCInst MI3 = MCInstBuilder(AArch64::EORXrs)
                           .addReg(Reg)
                           .addReg(Reg)
                           .addReg(DiscReg)
                           .addImm(0);
          MCELFStreamer::emitInstruction(MI3, STI);

          MCInst MI4 = MCInstBuilder(AArch64::EXTRXrri) // ror
                           .addReg(DiscReg)
                           .addReg(DiscReg)
                           .addReg(DiscReg)
                           .addImm(39);
          MCELFStreamer::emitInstruction(MI4, STI);
        }

        MCInst MI5 = MCInstBuilder(AArch64::EORXri)
              .addReg(Reg)
              .addReg(Reg)
              .addImm(AArch64_AM::encodeLogicalImmediate(1ULL << (DisableBit - 8), 64));
        MCELFStreamer::emitInstruction(MI5, STI);

        if (Inst.getOpcode() == AArch64::RETAA ||
            Inst.getOpcode() == AArch64::RETAB) {
          MCInst MIRet = MCInstBuilder(AArch64::RET).addReg(AArch64::LR);
          MCELFStreamer::emitInstruction(MIRet, STI);
        } else if (Inst.getOpcode() == AArch64::BRAA ||
                   Inst.getOpcode() == AArch64::BRAB ||
                   Inst.getOpcode() == AArch64::BRAAZ ||
                   Inst.getOpcode() == AArch64::BRABZ) {
          MCInst MIRet = MCInstBuilder(AArch64::BR).addReg(Reg);
          MCELFStreamer::emitInstruction(MIRet, STI);
        } else if (Inst.getOpcode() == AArch64::BLRAA ||
                   Inst.getOpcode() == AArch64::BLRAB ||
                   Inst.getOpcode() == AArch64::BLRAAZ ||
                   Inst.getOpcode() == AArch64::BLRABZ) {
          MCInst MIRet = MCInstBuilder(AArch64::BLR).addReg(Reg);
          MCELFStreamer::emitInstruction(MIRet, STI);
        }
        return;
      }
      case AArch64::LDRAAindexed:
      case AArch64::LDRABindexed:
      case AArch64::LDRAAwriteback:
      case AArch64::LDRABwriteback: {
        bool Writeback;
        unsigned Reg0, Reg1;
        int64_t Addend;
        switch (Inst.getOpcode()) {
          case AArch64::LDRAAindexed:
          case AArch64::LDRABindexed:
            Writeback = false;
            Reg0 = Inst.getOperand(0).getReg();
            Reg1 = Inst.getOperand(1).getReg();
            Addend = Inst.getOperand(2).getImm();
            break;
          case AArch64::LDRAAwriteback:
          case AArch64::LDRABwriteback:
            Writeback = true;
            Reg0 = Inst.getOperand(1).getReg();
            Reg1 = Inst.getOperand(2).getReg();
            Addend = Inst.getOperand(3).getImm();
            break;
        }
        unsigned DisableBit = (Inst.getOpcode() == AArch64::LDRABindexed ||
                               Inst.getOpcode() == AArch64::LDRABwriteback)
                                  ? 63
                                  : 62;

        bool Sub;
        unsigned Imm0, Imm1;
        if (Addend == -512) {
          Sub = true;
          Imm0 = 1;
          Imm1 = 12;
        } else if (Addend < 0) {
          Sub = true;
          Imm0 = -Addend * 8;
          Imm1 = 0;
        } else {
          Sub = false;
          Imm0 = Addend * 8;
          Imm1 = 0;
        }

        MCInst MI1 = MCInstBuilder(Sub ? AArch64::SUBXri : AArch64::ADDXri)
                         .addReg(Reg1)
                         .addReg(Reg1)
                         .addImm(Imm0)
                         .addImm(Imm1);
        MCELFStreamer::emitInstruction(MI1, STI);

        MCInst MI2 = MCInstBuilder(AArch64::TBNZX)
                         .addReg(AArch64::X18)
                         .addImm(DisableBit)
                         .addImm(2);
        MCELFStreamer::emitInstruction(MI2, STI);

        MCInst MI3 = MCInstBuilder(AArch64::EORXri)
              .addReg(Reg1)
              .addReg(Reg1)
              .addImm(AArch64_AM::encodeLogicalImmediate(1ULL << (DisableBit - 8), 64));
        MCELFStreamer::emitInstruction(MI3, STI);

        MCInst MI4 = MCInstBuilder(AArch64::LDRXui)
              .addReg(Reg0)
              .addReg(Reg1)
              .addImm(0);
        MCELFStreamer::emitInstruction(MI4, STI);

        if (!Writeback && Reg0 != Reg1) {
          MCInst MI5 = MCInstBuilder(AArch64::TBNZX)
                           .addReg(AArch64::X18)
                           .addImm(DisableBit)
                           .addImm(2);
          MCELFStreamer::emitInstruction(MI5, STI);

          MCInst MI6 = MCInstBuilder(AArch64::EORXri)
                           .addReg(Reg1)
                           .addReg(Reg1)
                           .addImm(AArch64_AM::encodeLogicalImmediate(
                               1ULL << (DisableBit - 8), 64));
          MCELFStreamer::emitInstruction(MI6, STI);

          MCInst MI7 = MCInstBuilder(Sub ? AArch64::ADDXri : AArch64::SUBXri)
                           .addReg(Reg1)
                           .addReg(Reg1)
                           .addImm(Imm0)
                           .addImm(Imm1);
          MCELFStreamer::emitInstruction(MI7, STI);
        }
        return;
      }
    }
    }

    MCELFStreamer::emitInstruction(Inst, STI);
  }

  /// Emit a 32-bit value as an instruction. This is only used for the .inst
  /// directive, EmitInstruction should be used in other cases.
  void emitInst(uint32_t Inst) {
    char Buffer[4];

    // We can't just use EmitIntValue here, as that will emit a data mapping
    // symbol, and swap the endianness on big-endian systems (instructions are
    // always little-endian).
    for (unsigned I = 0; I < 4; ++I) {
      Buffer[I] = uint8_t(Inst);
      Inst >>= 8;
    }

    EmitA64MappingSymbol();
    MCELFStreamer::emitBytes(StringRef(Buffer, 4));
  }

  /// This is one of the functions used to emit data into an ELF section, so the
  /// AArch64 streamer overrides it to add the appropriate mapping symbol ($d)
  /// if necessary.
  void emitBytes(StringRef Data) override {
    emitDataMappingSymbol();
    MCELFStreamer::emitBytes(Data);
  }

  /// This is one of the functions used to emit data into an ELF section, so the
  /// AArch64 streamer overrides it to add the appropriate mapping symbol ($d)
  /// if necessary.
  void emitValueImpl(const MCExpr *Value, unsigned Size, SMLoc Loc) override {
    emitDataMappingSymbol();
    MCELFStreamer::emitValueImpl(Value, Size, Loc);
  }

  void emitFill(const MCExpr &NumBytes, uint64_t FillValue,
                                  SMLoc Loc) override {
    emitDataMappingSymbol();
    MCObjectStreamer::emitFill(NumBytes, FillValue, Loc);
  }
private:
  enum ElfMappingSymbol {
    EMS_None,
    EMS_A64,
    EMS_Data
  };

  void emitDataMappingSymbol() {
    if (LastEMS == EMS_Data)
      return;
    EmitMappingSymbol("$d");
    LastEMS = EMS_Data;
  }

  void EmitA64MappingSymbol() {
    if (LastEMS == EMS_A64)
      return;
    EmitMappingSymbol("$x");
    LastEMS = EMS_A64;
  }

  void EmitMappingSymbol(StringRef Name) {
    auto *Symbol = cast<MCSymbolELF>(getContext().getOrCreateSymbol(
        Name + "." + Twine(MappingSymbolCounter++)));
    emitLabel(Symbol);
    Symbol->setType(ELF::STT_NOTYPE);
    Symbol->setBinding(ELF::STB_LOCAL);
    Symbol->setExternal(false);
  }

  int64_t MappingSymbolCounter;

  DenseMap<const MCSection *, ElfMappingSymbol> LastMappingSymbols;
  ElfMappingSymbol LastEMS;
};

} // end anonymous namespace

namespace llvm {

AArch64ELFStreamer &AArch64TargetELFStreamer::getStreamer() {
  return static_cast<AArch64ELFStreamer &>(Streamer);
}

void AArch64TargetELFStreamer::emitInst(uint32_t Inst) {
  getStreamer().emitInst(Inst);
}

void AArch64TargetELFStreamer::emitDirectiveVariantPCS(MCSymbol *Symbol) {
  cast<MCSymbolELF>(Symbol)->setOther(ELF::STO_AARCH64_VARIANT_PCS);
}

MCTargetStreamer *createAArch64AsmTargetStreamer(MCStreamer &S,
                                                 formatted_raw_ostream &OS,
                                                 MCInstPrinter *InstPrint,
                                                 bool isVerboseAsm) {
  return new AArch64TargetAsmStreamer(S, OS);
}

MCELFStreamer *createAArch64ELFStreamer(MCContext &Context,
                                        std::unique_ptr<MCAsmBackend> TAB,
                                        std::unique_ptr<MCObjectWriter> OW,
                                        std::unique_ptr<MCCodeEmitter> Emitter,
                                        bool RelaxAll) {
  AArch64ELFStreamer *S = new AArch64ELFStreamer(
      Context, std::move(TAB), std::move(OW), std::move(Emitter));
  if (RelaxAll)
    S->getAssembler().setRelaxAll(true);
  return S;
}

} // end namespace llvm
