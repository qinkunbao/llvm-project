//===- AArch64ExpandHardenedPseudos.cpp --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AArch64InstrInfo.h"
#include "AArch64Subtarget.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
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
  bool expandAuthCall(MachineInstr &MI);
  bool expandMI(MachineInstr &MI);
};

} // end anonymous namespace

char AArch64ExpandHardenedPseudos::ID = 0;

INITIALIZE_PASS(AArch64ExpandHardenedPseudos, DEBUG_TYPE, PASS_NAME, false, false)

bool AArch64ExpandHardenedPseudos::expandAuthCall(MachineInstr &MI) {
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  DebugLoc DL = MI.getDebugLoc();
  auto MBBI = MI.getIterator();

  const AArch64Subtarget &STI = MF.getSubtarget<AArch64Subtarget>();
  const AArch64InstrInfo *TII = STI.getInstrInfo();

  LLVM_DEBUG(dbgs() << "Expanding: " << MI << "\n");

  MachineOperand Callee = MI.getOperand(0);
  auto Key = (AArch64PACKey::ID)MI.getOperand(1).getImm();
  uint64_t Disc = MI.getOperand(2).getImm();
  unsigned AddrDisc = MI.getOperand(3).getReg();

  unsigned DiscReg = AddrDisc;
  unsigned DiscRegFlags = 0;
  if (Disc) {
    assert(isUInt<16>(Disc) && "Integer discriminator is too wide");

    if (AddrDisc != AArch64::NoRegister) {
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
    if (MI.isInsideBundle())
      DiscRegFlags = RegState::InternalRead;
  }

  assert((Key == AArch64PACKey::IA || Key == AArch64PACKey::IB) &&
         "Invalid auth call key");

  unsigned Opc;
  if (Key == AArch64PACKey::IA)
    Opc = DiscReg == AArch64::NoRegister ? AArch64::BLRAAZ : AArch64::BLRAA;
  else
    Opc = DiscReg == AArch64::NoRegister ? AArch64::BLRABZ : AArch64::BLRAB;

  auto MIB = BuildMI(MBB, MBBI, DL, TII->get(Opc))
    .add(Callee);
  if (DiscReg != AArch64::NoRegister)
    MIB.addReg(DiscReg, DiscRegFlags);

  if (MI.shouldUpdateCallSiteInfo())
    MBB.getParent()->moveCallSiteInfo(&MI, MIB);

  MIB.copyImplicitOps(MI);
  return true;
}

bool AArch64ExpandHardenedPseudos::expandMI(MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case AArch64::BLRA:
    return expandAuthCall(MI);
  default:
    return false;
  }
}

bool AArch64ExpandHardenedPseudos::runOnMachineFunction(MachineFunction &MF) {
  LLVM_DEBUG(dbgs() << "***** AArch64ExpandHardenedPseudos *****\n");

  bool Modified = false;
  for (auto &MBB : MF) {
    for (auto MBBI = MBB.instr_begin(), MBBE = MBB.instr_end(); MBBI != MBBE; ) {
      auto &MI = *MBBI++;
      if (expandMI(MI)) {
        MI.eraseFromBundle();
        Modified |= true;
      }
    }
  }
  return Modified;
}

FunctionPass *llvm::createAArch64ExpandHardenedPseudosPass() {
  return new AArch64ExpandHardenedPseudos();
}
