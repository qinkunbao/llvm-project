//===- AArch64ExpandHardenedPseudos.cpp --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AArch64InstrInfo.h"
#include "AArch64Subtarget.h"
#include "AArch64MachineFunctionInfo.h"
#include "AArch64TargetObjectFile.h"
#include "MCTargetDesc/AArch64AddressingModes.h"
#include "Utils/AArch64BaseInfo.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Pass.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetMachine.h"
#include <cassert>

using namespace llvm;

#define DEBUG_TYPE "aarch64-expand-hardened-pseudos"

#define PASS_NAME "AArch64 Expand Hardened Pseudos"

namespace {

class AArch64ExpandHardenedPseudos : public MachineFunctionPass {
public:
  static char ID;

  AArch64ExpandHardenedPseudos() : MachineFunctionPass(ID) {
    initializeAArch64ExpandHardenedPseudosPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &Fn) override;

  StringRef getPassName() const override {
    return PASS_NAME;
  }

private:
  bool expandPtrAuthPseudo(MachineInstr &MI);
  bool expandMI(MachineInstr &MI);
};

} // end anonymous namespace

char AArch64ExpandHardenedPseudos::ID = 0;

INITIALIZE_PASS(AArch64ExpandHardenedPseudos, DEBUG_TYPE, PASS_NAME, false, false);

bool AArch64ExpandHardenedPseudos::expandPtrAuthPseudo(MachineInstr &MI) {
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  DebugLoc DL = MI.getDebugLoc();
  auto MBBI = MI.getIterator();

  const AArch64Subtarget &STI = MF.getSubtarget<AArch64Subtarget>();
  const AArch64InstrInfo *TII = STI.getInstrInfo();

  if (MI.getOpcode() == AArch64::BR_JumpTable) {
    LLVM_DEBUG(dbgs() << "Expanding: " << MI << "\n");
    const MachineJumpTableInfo *MJTI = MF.getJumpTableInfo();
    assert(MJTI && "Can't lower jump-table dispatch without JTI");

    const std::vector<MachineJumpTableEntry> &JTs = MJTI->getJumpTables();
    assert(!JTs.empty() && "Invalid JT index for jump-table dispatch");

    // Emit:
    //     adrp xTable, Ltable@PAGE
    //     add xTable, Ltable@PAGEOFF
    //     mov xEntry, #<size of table> ; depending on table size, with MOVKs
    //     cmp xEntry, #<size of table> ; if table size fits in 12-bit immediate
    //     csel xEntry, xEntry, xzr, ls
    //     ldrsw xScratch, [xTable, xEntry, lsl #2] ; kill xEntry, xScratch = xEntry
    //   Ltmp:
    //     adr xTable, Ltmp
    //     add xDest, xTable, xScratch ; kill xTable, xDest = xTable
    //     br xDest

    MachineOperand JTOp = MI.getOperand(0);

    unsigned JTI = JTOp.getIndex();
    const uint64_t NumTableEntries = JTs[JTI].MBBs.size();

    // cmp only supports a 12-bit immediate.  If we need more, materialize the
    // immediate, using TableReg as a scratch register.
    uint64_t MaxTableEntry = NumTableEntries - 1;
    if (isUInt<12>(MaxTableEntry)) {
      BuildMI(MBB, MBBI, DL, TII->get(AArch64::SUBSXri), AArch64::XZR)
        .addReg(AArch64::X16)
        .addImm(MaxTableEntry)
        .addImm(0);
    } else {
      BuildMI(MBB, MBBI, DL, TII->get(AArch64::MOVZXi), AArch64::X17)
        .addImm(static_cast<uint16_t>(MaxTableEntry))
        .addImm(0);
      // It's sad that we have to manually materialize instructions, but we can't
      // trivially reuse the main pseudo expansion logic.
      // A MOVK sequence is easy enough to generate and handles the general case.
      for (int Offset = 16; Offset < 64; Offset += 16) {
        if ((MaxTableEntry >> Offset) == 0)
          break;
        BuildMI(MBB, MBBI, DL, TII->get(AArch64::MOVKXi), AArch64::X17)
          .addReg(AArch64::X17)
          .addImm(static_cast<uint16_t>(MaxTableEntry >> Offset))
          .addImm(Offset);
      }
      BuildMI(MBB, MBBI, DL, TII->get(AArch64::SUBSXrs), AArch64::XZR)
        .addReg(AArch64::X16)
        .addReg(AArch64::X17)
        .addImm(0);
    }

    // This picks entry #0 on failure.
    // We might want to trap instead.
    BuildMI(MBB, MBBI, DL, TII->get(AArch64::CSELXr), AArch64::X16)
      .addReg(AArch64::X16)
      .addReg(AArch64::XZR)
      .addImm(AArch64CC::LS);

    MachineOperand JTHiOp(JTOp);
    MachineOperand JTLoOp(JTOp);
    JTHiOp.setTargetFlags(AArch64II::MO_PAGE);
    JTLoOp.setTargetFlags(AArch64II::MO_PAGEOFF | AArch64II::MO_NC);

    BuildMI(MBB, MBBI, DL, TII->get(AArch64::ADRP), AArch64::X17)
      .add(JTHiOp);
    BuildMI(MBB, MBBI, DL, TII->get(AArch64::ADDXri), AArch64::X17)
      .addReg(AArch64::X17)
      .add(JTLoOp)
      .addImm(0);

    BuildMI(MBB, MBBI, DL, TII->get(AArch64::LDRSWroX), AArch64::X16)
      .addReg(AArch64::X17)
      .addReg(AArch64::X16)
      .addImm(0)
      .addImm(1);

    // Really an ADR with a label attached.
    BuildMI(MBB, MBBI, DL, TII->get(AArch64::JumpTableAnchor), AArch64::X17)
      .addJumpTableIndex(JTI);

    BuildMI(MBB, MBBI, DL, TII->get(AArch64::ADDXrs), AArch64::X16)
      .addReg(AArch64::X17)
      .addReg(AArch64::X16)
      .addImm(0);

    BuildMI(MBB, MBBI, DL, TII->get(AArch64::BR))
      .addReg(AArch64::X16);

    MI.eraseFromParent();
    return true;
  }

  if (MI.getOpcode() == AArch64::LOADauthptrgot) {
    LLVM_DEBUG(dbgs() << "Expanding: " << MI << "\n");

    const TargetMachine &TM = MF.getTarget();
    MachineModuleInfo &MMI = MF.getMMI();

    unsigned DstReg = MI.getOperand(0).getReg();
    MachineOperand GAOp = MI.getOperand(1);
    auto Key = (AArch64PACKey::ID)MI.getOperand(2).getImm();
    uint64_t Disc = MI.getOperand(3).getImm();

    assert(isUInt<16>(Disc) && "Constant discriminator is too wide");

    MCSymbol *GASym = TM.getSymbol(GAOp.getGlobal());
    uint64_t Offset = GAOp.getOffset();

    // On arm64e with ptrauth-calls we instead emit:
    // ADRP x16, l_symbol$auth_ptr$ia$0@PAGE
    // LDR x16, [x16, l_symbol$auth_ptr$ia$0@PAGEOFF]
    //
    // Where the $auth_ptr$ symbol is the stub slot containing the signed pointer
    // to _symbol.
    // We defined the stub ourselves, so we don't need a GOT access.
    assert(TM.getTargetTriple().isOSBinFormatMachO() &&
           "ptrauth chkstk_darwin only implemented on mach-o");
    auto *TLOF =
      static_cast<AArch64_MachoTargetObjectFile *>(TM.getObjFileLowering());
    MCSymbol *AuthPtrStubSym =
        TLOF->getAuthPtrSlotSymbol(TM, &MMI, GASym, Offset, Key, Disc);

    BuildMI(MBB, MBBI, DL, TII->get(AArch64::ADRP), DstReg)
      .addSym(AuthPtrStubSym, AArch64II::MO_PAGE);
    BuildMI(MBB, MBBI, DL, TII->get(AArch64::LDRXui), DstReg)
      .addUse(DstReg)
      .addSym(AuthPtrStubSym, AArch64II::MO_PAGEOFF | AArch64II::MO_NC);

    MI.eraseFromParent();
    return true;
  }

  if (MI.getOpcode() != AArch64::MOVaddrPAC &&
      MI.getOpcode() != AArch64::LOADgotPAC)
    return false;

  LLVM_DEBUG(dbgs() << "Expanding: " << MI << "\n");

  const bool IsGOTLoad = MI.getOpcode() == AArch64::LOADgotPAC;
  MachineOperand GAOp = MI.getOperand(0);
  auto Key = (AArch64PACKey::ID)MI.getOperand(1).getImm();
  unsigned AddrDisc = MI.getOperand(2).getReg();
  uint64_t Disc = MI.getOperand(3).getImm();

  uint64_t Offset = GAOp.getOffset();
  GAOp.setOffset(0);

  // Emit:
  // target materialization:
  //   via GOT:
  //     adrp x16, _target@GOTPAGE
  //     ldr x16, [x16, _target@GOTPAGEOFF]
  //     add x16, x16, #<offset> ; if offset != 0; up to 3 depending on width
  //
  //   direct:
  //     adrp x16, _target@PAGE
  //     add x16, x16, _target@PAGEOFF
  //     add x16, x16, #<offset> ; if offset != 0; up to 3 depending on width
  //
  // signing:
  // - 0 discriminator:
  //     paciza x16
  // - Non-0 discriminator, no address discriminator:
  //     mov x17, #Disc
  //     pacia x16, x17
  // - address discriminator (with potentially folded immediate discriminator):
  //     pacia x16, xAddrDisc

  MachineOperand GAHiOp(GAOp);
  MachineOperand GALoOp(GAOp);
  GAHiOp.setTargetFlags(AArch64II::MO_PAGE);
  GALoOp.setTargetFlags(AArch64II::MO_PAGEOFF | AArch64II::MO_NC);
  if (IsGOTLoad) {
    GAHiOp.addTargetFlag(AArch64II::MO_GOT);
    GALoOp.addTargetFlag(AArch64II::MO_GOT);
  }

  BuildMI(MBB, MBBI, DL, TII->get(AArch64::ADRP), AArch64::X16)
    .add(GAHiOp);

  if (IsGOTLoad) {
    BuildMI(MBB, MBBI, DL, TII->get(AArch64::LDRXui), AArch64::X16)
      .addReg(AArch64::X16)
      .add(GALoOp);
  } else {
    BuildMI(MBB, MBBI, DL, TII->get(AArch64::ADDXri), AArch64::X16)
      .addReg(AArch64::X16)
      .add(GALoOp)
      .addImm(0);
  }

  if (Offset) {
    if (!isUInt<32>(Offset))
      report_fatal_error("ptrauth global offset too large, 32bit max encoding");

    for (int BitPos = 0; BitPos < 32 && (Offset >> BitPos); BitPos += 12) {
      BuildMI(MBB, MBBI, DL, TII->get(AArch64::ADDXri), AArch64::X16)
        .addReg(AArch64::X16)
        .addImm((Offset >> BitPos) & 0xfff)
        .addImm(AArch64_AM::getShifterImm(AArch64_AM::LSL, BitPos));
    }
  }

  unsigned DiscReg = AddrDisc;
  if (Disc) {
    assert(isUInt<16>(Disc) && "Constant discriminator is too wide");

    if (AddrDisc != AArch64::XZR) {
      BuildMI(MBB, MBBI, DL, TII->get(AArch64::ORRXrs), AArch64::X17)
        .addReg(AArch64::XZR)
        .addReg(AddrDisc)
        .addImm(0);
      BuildMI(MBB, MBBI, DL, TII->get(AArch64::MOVKXi), AArch64::X17)
        .addReg(AArch64::X17)
        .addImm(Disc)
        .addImm(/*shift=*/48);
    } else {
      BuildMI(MBB, MBBI, DL, TII->get(AArch64::MOVZXi), AArch64::X17)
        .addImm(Disc)
        .addImm(/*shift=*/0);
    }
    DiscReg = AArch64::X17;
  }

  unsigned PACOpc = getPACOpcodeForKey(Key, DiscReg == AArch64::XZR);
  auto MIB = BuildMI(MBB, MBBI, DL, TII->get(PACOpc), AArch64::X16)
      .addReg(AArch64::X16);
  if (DiscReg != AArch64::XZR)
    MIB.addReg(DiscReg);

  // FIXME: transferImpOps ?
  MI.eraseFromParent();
  return true;
}

bool AArch64ExpandHardenedPseudos::expandMI(MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case AArch64::BR_JumpTable:
  case AArch64::LOADauthptrgot:
  case AArch64::LOADgotPAC:
  case AArch64::MOVaddrPAC:
    return expandPtrAuthPseudo(MI);
  default:
    return false;
  }
}

bool AArch64ExpandHardenedPseudos::runOnMachineFunction(MachineFunction &MF) {
  LLVM_DEBUG(dbgs() << "***** AArch64ExpandHardenedPseudos *****\n");

  bool Modified = false;
  for (auto &MBB : MF) {
    for (auto MBBI = MBB.begin(), MBBE = MBB.end(); MBBI != MBBE; ) {
      auto &MI = *MBBI++;
      Modified |= expandMI(MI);
    }
  }
  return Modified;
}

FunctionPass *llvm::createAArch64ExpandHardenedPseudosPass() {
  return new AArch64ExpandHardenedPseudos();
}
