//===-- Address.h - An aligned address -------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This class provides a simple wrapper for a pair of a pointer and an
// alignment.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_CODEGEN_ADDRESS_H
#define LLVM_CLANG_LIB_CODEGEN_ADDRESS_H

#include "CGPointerAuthInfo.h"
#include "clang/AST/CharUnits.h"
#include "clang/AST/Type.h"
#include "llvm/IR/Constants.h"

namespace clang {
namespace CodeGen {

class CGBuilderTy;
class CodeGenFunction;
class CodeGenModule;
class Address;

/// An aligned address whose pointer isn't signed.
class RawAddress {
  llvm::Value *Pointer;
  CharUnits Alignment;

public:
  RawAddress(llvm::Value *pointer, CharUnits alignment)
      : Pointer(pointer), Alignment(alignment) {
    assert((!alignment.isZero() || pointer == nullptr) &&
           "creating valid address with invalid alignment");
  }

  inline RawAddress(Address Addr);

  static RawAddress invalid() { return RawAddress(nullptr, CharUnits()); }
  bool isValid() const { return Pointer != nullptr; }

  llvm::Value *getPointer() const {
    assert(isValid());
    return Pointer;
  }

  /// Return the type of the pointer value.
  llvm::PointerType *getType() const {
    return llvm::cast<llvm::PointerType>(getPointer()->getType());
  }

  /// Return the type of the values stored in this address.
  ///
  /// When IR pointer types lose their element type, we should simply
  /// store it in RawAddress instead for the convenience of writing code.
  llvm::Type *getElementType() const {
    return getType()->getElementType();
  }

  /// Return the address space that this address resides in.
  unsigned getAddressSpace() const {
    return getType()->getAddressSpace();
  }

  /// Return the IR name of the pointer value.
  llvm::StringRef getName() const {
    return getPointer()->getName();
  }

  /// Return the alignment of this pointer.
  CharUnits getAlignment() const {
    assert(isValid());
    return Alignment;
  }
};

/// An abstract representation of an aligned address. This is designed to be an
/// IR-level abstraction, carrying just the information necessary to perform IR
/// operations on an address like loads and stores.  In particular, it doesn't
/// carry C type information or allow the representation of things like
/// bit-fields; clients working at that level should generally be using
/// `LValue`.
///
/// An address may be either *raw*, meaning that it's an ordinary machine
/// pointer, or *signed*, meaning that the pointer carries an embedded
/// pointer-authentication signature. Representing signed pointers directly in
/// this abstraction allows the authentication to be delayed as long as possible
/// without forcing IRGen to use totally different code paths for signed and
/// unsigned values or to separately propagate signature information through
/// every API that manipulates addresses. Pointer arithmetic on signed addresses
/// (e.g. drilling down to a struct field) is accumulated into a separate offset
/// which is applied when the address is finally accessed.
class Address {
  friend class CGBuilderTy;

  llvm::Value *Pointer;
  CharUnits Alignment;

  /// The ptrauth information needed to authenticate the base pointer.
  CGPointerAuthInfo PtrAuthInfo;

  /// Offset from the base pointer. This is non-null only when the base
  /// pointer is signed.
  llvm::Value *Offset = nullptr;

  /// The expected IR type of the pointer. When the address is a raw pointer,
  /// this is currently redundant with the pointer's type, but for signed
  /// pointers it is useful if the pointer has been offsetted or cast from the
  /// original type. In the long run, when LLVM adopts opaque pointer types,
  /// this should become the notional element type of the address.
  ///
  /// Carrying accurate element type information in Address makes it more
  /// convenient to work with Address values and allows frontend assertions to
  /// catch simple mistakes even after LLVM adopts opaque pointer types.
  llvm::PointerType *EffectiveType = nullptr;

  void init() {
    assert((!Alignment.isZero() || Pointer == nullptr) &&
           "creating valid address with invalid alignment");
    // Need this check since Addr's pointer doesn't always have a pointer
    // type. For example, a MetadataAsValue pointer is passed when an
    // l-value for a global named register is created.
    if (Pointer)
      if (auto *PtrTy = dyn_cast<llvm::PointerType>(Pointer->getType()))
        EffectiveType = PtrTy;
  }

  llvm::Value *getRawPointerSlow(CodeGenFunction &CGF) const;

public:
  Address(llvm::Value *pointer, CharUnits alignment)
      : Pointer(pointer), Alignment(alignment) {
    init();
  }

  Address(llvm::Value *BasePtr, CharUnits Alignment,
          CGPointerAuthInfo PtrAuthInfo, llvm::Value *Offset,
          llvm::PointerType *EffectiveType)
      : Pointer(BasePtr), Alignment(Alignment), PtrAuthInfo(PtrAuthInfo),
        Offset(Offset), EffectiveType(EffectiveType) {
    assert(Pointer->getType() == EffectiveType &&
           "effective type doesn't match the pointer type");
  }

  Address(llvm::Value *BasePtr, CharUnits Alignment,
          CGPointerAuthInfo PtrAuthInfo)
      : Pointer(BasePtr), Alignment(Alignment), PtrAuthInfo(PtrAuthInfo) {
    init();
  }

  Address(RawAddress RawAddr)
      : Address(RawAddr.isValid() ? RawAddr.getPointer() : nullptr,
                RawAddr.isValid() ? RawAddr.getAlignment() : CharUnits::Zero()) {}

  static Address invalid() { return Address(nullptr, CharUnits()); }
  bool isValid() const { return Pointer != nullptr; }

  llvm::Value *getPointerIfNotSigned() const {
    assert(isValid() && "pointer isn't valid");
    return !isSigned() ? Pointer : nullptr;
  }

  /// This function is used in situations where the caller is doing some sort of
  /// opaque "laundering" of the pointer.
  void replaceBasePointer(llvm::Value *P) {
    assert(isValid() && "pointer isn't valid");
    assert(P->getType() == Pointer->getType() && "Pointer's type changed");
    Pointer = P;
    assert(isValid() && "pointer is invalid after replacement");
  }

  CharUnits getAlignment() const { return Alignment; }

  void setAlignment(CharUnits Value) { Alignment = Value; }

  llvm::Value *getBasePointer() const {
    assert(isValid() && "pointer isn't valid");
    return Pointer;
  }

  llvm::Value *getUnsignedPointer() const {
    assert(!isSigned() && "cannot call this function if pointer is signed");
    return getBasePointer();
  }

  /// Return the type of the pointer value.
  llvm::PointerType *getType() const { return EffectiveType; }

  /// Return the type of the values stored in this address.
  ///
  /// When IR pointer types lose their element type, we should simply
  /// store it in Address instead for the convenience of writing code.
  llvm::Type *getElementType() const {
    return getType()->getElementType();
  }

  /// Return the address space that this address resides in.
  unsigned getAddressSpace() const {
    return getType()->getAddressSpace();
  }

  /// Return the IR name of the pointer value.
  llvm::StringRef getName() const { return Pointer->getName(); }

  const CGPointerAuthInfo &getPointerAuthInfo() const { return PtrAuthInfo; }
  void setPointerAuthInfo(const CGPointerAuthInfo &Info) { PtrAuthInfo = Info; }

  // This function is called only in CGBuilderBaseTy::CreateBitCast.
  void setEffectiveType(llvm::Type *Ty) {
    assert(hasOffset() &&
           "this funcion shouldn't be called when there is no offset");
    EffectiveType = cast<llvm::PointerType>(Ty);
  }

  bool isSigned() const { return PtrAuthInfo.isSigned(); }

  /// Add a constant offset.
  void addOffset(CharUnits V, llvm::Type *Ty, CGBuilderTy &Builder);

  /// Add a variable offset.
  /// \param V An llvm value holding a variable offset.
  void addOffset(llvm::Value *V, llvm::Type *Ty, CGBuilderTy &Builder,
                 CharUnits NewAlignment);

  bool hasOffset() const { return Offset; }

  llvm::Value *getOffset() const { return Offset; }

  Address getResignedAddress(const CGPointerAuthInfo &NewInfo,
                             CodeGenFunction &CGF,
                             bool IsKnownNonNull) const;

  llvm::Value *getRawPointer(CodeGenFunction &CGF) const {
    if (!isSigned())
      return getUnsignedPointer();
    return getRawPointerSlow(CGF);
  }
};

inline RawAddress::RawAddress(Address Addr)
    : RawAddress(Addr.isValid() ? Addr.getUnsignedPointer() : nullptr,
                 Addr.isValid() ? Addr.getAlignment() : CharUnits::Zero()) {}

/// A specialization of Address that requires the address to be an
/// LLVM Constant.
class ConstantAddress : public RawAddress {
public:
  ConstantAddress(llvm::Constant *pointer, CharUnits alignment)
      : RawAddress(pointer, alignment) {}

  static ConstantAddress invalid() {
    return ConstantAddress(nullptr, CharUnits());
  }

  llvm::Constant *getPointer() const {
    return llvm::cast<llvm::Constant>(RawAddress::getPointer());
  }

  ConstantAddress getBitCast(llvm::Type *ty) const {
    return ConstantAddress(llvm::ConstantExpr::getBitCast(getPointer(), ty),
                           getAlignment());
  }

  ConstantAddress getElementBitCast(llvm::Type *ty) const {
    return getBitCast(ty->getPointerTo(getAddressSpace()));
  }

  static bool isaImpl(RawAddress addr) {
    return llvm::isa<llvm::Constant>(addr.getPointer());
  }
  static ConstantAddress castImpl(RawAddress addr) {
    return ConstantAddress(llvm::cast<llvm::Constant>(addr.getPointer()),
                           addr.getAlignment());
  }
};

}

// Present a minimal LLVM-like casting interface.
template <class U> inline U cast(CodeGen::Address addr) {
  return U::castImpl(addr);
}
template <class U> inline bool isa(CodeGen::Address addr) {
  return U::isaImpl(addr);
}

}

#endif
