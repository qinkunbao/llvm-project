//===----- CGPointerAuthInfo.h -  -------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Pointer auth info class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_CODEGEN_CGPOINTERAUTHINFO_H
#define LLVM_CLANG_LIB_CODEGEN_CGPOINTERAUTHINFO_H

#include "clang/AST/Type.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

namespace clang {
namespace CodeGen {

class CGPointerAuthInfo {
private:
  PointerAuthenticationMode AuthenticationMode : 2;
  unsigned Key : 30;
  llvm::Value *Discriminator;

public:
  CGPointerAuthInfo()
      : AuthenticationMode(PointerAuthenticationMode::None), Key(0),
        Discriminator(nullptr) {}
  CGPointerAuthInfo(unsigned key, PointerAuthenticationMode authenticationMode,
                    llvm::Value *discriminator)
      : AuthenticationMode(authenticationMode),
        Key(key), Discriminator(discriminator) {
    assert(!discriminator || discriminator->getType()->isIntegerTy() ||
           discriminator->getType()->isPointerTy());
  }

  explicit operator bool() const { return isSigned(); }

  bool isSigned() const {
    return AuthenticationMode != PointerAuthenticationMode::None;
  }

  unsigned getKey() const {
    assert(isSigned());
    return Key;
  }
  llvm::Value *getDiscriminator() const {
    assert(isSigned());
    return Discriminator;
  }

  PointerAuthenticationMode getAuthenticationMode() const {
    return AuthenticationMode;
  }

  bool shouldStrip() const {
    return AuthenticationMode == PointerAuthenticationMode::Strip ||
           AuthenticationMode == PointerAuthenticationMode::SignAndStrip;
  }

  bool shouldSign() const {
    return AuthenticationMode == PointerAuthenticationMode::SignAndStrip ||
           AuthenticationMode == PointerAuthenticationMode::SignAndAuth;
  }

  bool shouldAuth() const {
    return AuthenticationMode == PointerAuthenticationMode::SignAndAuth;
  }

  bool operator!=(const CGPointerAuthInfo &Other) const {
    return Key != Other.Key || Discriminator != Other.Discriminator ||
           AuthenticationMode != Other.AuthenticationMode;
  }

  bool operator==(const CGPointerAuthInfo &Other) const {
    return !(*this != Other);
  }
};

} // end namespace CodeGen
} // end namespace clang

#endif
