//===- AArch64ExpandHardenedPseudos.cpp --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AArch64InstrInfo.h"
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
  bool expandMI(MachineInstr &MI);
};

} // end anonymous namespace

char AArch64ExpandHardenedPseudos::ID = 0;

INITIALIZE_PASS(AArch64ExpandHardenedPseudos, DEBUG_TYPE, PASS_NAME, false, false)

bool AArch64ExpandHardenedPseudos::expandMI(MachineInstr &MI) {
  switch (MI.getOpcode()) {
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
