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

#include "clang/AST/CharUnits.h"
#include "clang/AST/Type.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/MathExtras.h"

namespace clang {
namespace CodeGen {

class Address;
class CGBuilderTy;
class CodeGenFunction;
class CodeGenModule;

// We try to save some space by using 6 bits over two PointerIntPairs to store
// the alignment. However, some arches don't support 3 bits in a PointerIntPair
// so we fallback to storing the alignment separately.
template <typename T, bool = alignof(llvm::Value *) >= 8> class RawAddressImpl {};

template <typename T> class RawAddressImpl<T, false> {
  llvm::Value *Pointer;
  llvm::Type *ElementType;
  CharUnits Alignment;

public:
  RawAddressImpl(llvm::Value *Pointer, llvm::Type *ElementType,
                 CharUnits Alignment)
      : Pointer(Pointer), ElementType(ElementType), Alignment(Alignment) {}
  llvm::Value *getPointer() const { return Pointer; }
  llvm::Type *getElementType() const { return ElementType; }
  CharUnits getAlignment() const { return Alignment; }
};

template <typename T> class RawAddressImpl<T, true> {
  // Int portion stores upper 3 bits of the log of the alignment.
  llvm::PointerIntPair<llvm::Value *, 3, unsigned> Pointer;
  // Int portion stores lower 3 bits of the log of the alignment.
  llvm::PointerIntPair<llvm::Type *, 3, unsigned> ElementType;

public:
  RawAddressImpl(llvm::Value *Pointer, llvm::Type *ElementType,
                 CharUnits Alignment)
      : Pointer(Pointer), ElementType(ElementType) {
    if (Alignment.isZero())
      return;
    // Currently the max supported alignment is much less than 1 << 63 and is
    // guaranteed to be a power of 2, so we can store the log of the alignment
    // into 6 bits.
    assert(Alignment.isPowerOfTwo() && "Alignment cannot be zero");
    auto AlignLog = llvm::Log2_64(Alignment.getQuantity());
    assert(AlignLog < (1 << 6) && "cannot fit alignment into 6 bits");
    this->Pointer.setInt(AlignLog >> 3);
    this->ElementType.setInt(AlignLog & 7);
  }
  llvm::Value *getPointer() const { return Pointer.getPointer(); }
  llvm::Type *getElementType() const { return ElementType.getPointer(); }
  CharUnits getAlignment() const {
    unsigned AlignLog = (Pointer.getInt() << 3) | ElementType.getInt();
    return CharUnits::fromQuantity(CharUnits::QuantityType(1) << AlignLog);
  }
};

/// An aligned address whose pointer isn't signed.
class RawAddress {
  RawAddressImpl<void> A;

protected:
  RawAddress(std::nullptr_t) : A(nullptr, nullptr, CharUnits::Zero()) {}

public:
  RawAddress(llvm::Value *Pointer, llvm::Type *ElementType, CharUnits Alignment)
      : A(Pointer, ElementType, Alignment) {
    assert(Pointer != nullptr && "Pointer cannot be null");
    assert(ElementType != nullptr && "Element type cannot be null");
    assert(llvm::cast<llvm::PointerType>(Pointer->getType())
               ->isOpaqueOrPointeeTypeMatches(ElementType) &&
           "Incorrect pointer element type");
  }

  inline RawAddress(Address Addr);

  static RawAddress invalid() { return RawAddress(nullptr); }
  bool isValid() const { return A.getPointer() != nullptr; }

  llvm::Value *getPointer() const {
    assert(isValid());
    return A.getPointer();
  }

  /// Return the type of the pointer value.
  llvm::PointerType *getType() const {
    return llvm::cast<llvm::PointerType>(getPointer()->getType());
  }

  /// Return the type of the values stored in this address.
  llvm::Type *getElementType() const {
    assert(isValid());
    return A.getElementType();
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
    return A.getAlignment();
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

  /// The expected IR type of the pointer. When the address is a raw pointer,
  /// this is currently redundant with the pointer's type, but for signed
  /// pointers it is useful if the pointer has been offsetted or cast from the
  /// original type.
  ///
  /// Carrying accurate element type information in Address makes it more
  /// convenient to work with Address values and allows frontend assertions to
  /// catch simple mistakes even after LLVM adopts opaque pointer types.
  llvm::Type *ElementType = nullptr;

  CharUnits Alignment;

  /// Offset from the base pointer.
  llvm::Value *Offset = nullptr;

  llvm::Value *getRawPointerSlow(CodeGenFunction &CGF) const;

protected:
  Address(std::nullptr_t) : Pointer(nullptr), ElementType(nullptr)  {}

public:
  Address(llvm::Value *pointer, llvm::Type *elementType, CharUnits alignment)
      : Pointer(pointer), ElementType(elementType), Alignment(alignment) {
    assert(pointer != nullptr && "Pointer cannot be null");
    assert(elementType != nullptr && "Element type cannot be null");
    assert(llvm::cast<llvm::PointerType>(pointer->getType())
               ->isOpaqueOrPointeeTypeMatches(elementType) &&
           "Incorrect pointer element type");
    assert(!alignment.isZero() && "Alignment cannot be zero");
  }

  Address(llvm::Value *BasePtr, llvm::Type *ElementType, CharUnits Alignment,
          llvm::Value *Offset)
      : Pointer(BasePtr), ElementType(ElementType), Alignment(Alignment),
        Offset(Offset) {
    assert(llvm::cast<llvm::PointerType>(Pointer->getType())
               ->isOpaqueOrPointeeTypeMatches(ElementType) &&
           "Incorrect pointer element type");
  }

  Address(RawAddress RawAddr)
      : Pointer(RawAddr.isValid() ? RawAddr.getPointer() : nullptr),
        ElementType(RawAddr.isValid() ? RawAddr.getElementType() : nullptr),
        Alignment(RawAddr.isValid() ? RawAddr.getAlignment()
                                    : CharUnits::Zero()) {
  }


  static Address invalid() { return Address(nullptr); }
  bool isValid() const { return Pointer != nullptr; }

  llvm::Value *getPointerIfNotSigned() const {
    assert(isValid() && "pointer isn't valid");
    return Pointer;
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
    return getBasePointer();
  }

  /// Return the type of the pointer value.
  llvm::PointerType *getType() const {
    return llvm::PointerType::get(ElementType,
        llvm::cast<llvm::PointerType>(Pointer->getType())->getAddressSpace());
  }

  /// Return the type of the values stored in this address.
  llvm::Type *getElementType() const {
    assert(isValid());
    return ElementType;
  }

  /// Return the address space that this address resides in.
  unsigned getAddressSpace() const {
    return getType()->getAddressSpace();
  }

  /// Return the IR name of the pointer value.
  llvm::StringRef getName() const { return Pointer->getName(); }

  // This function is called only in CGBuilderBaseTy::CreateElementBitCast.
  void setElementType(llvm::Type *Ty) {
    assert(hasOffset() &&
           "this funcion shouldn't be called when there is no offset");
    ElementType = Ty;
  }

  /// Add a constant offset.
  void addOffset(CharUnits V, llvm::Type *Ty, CGBuilderTy &Builder);

  /// Add a variable offset.
  /// \param V An llvm value holding a variable offset.
  void addOffset(llvm::Value *V, llvm::Type *Ty, CGBuilderTy &Builder,
                 CharUnits NewAlignment);

  bool hasOffset() const { return Offset; }

  llvm::Value *getOffset() const { return Offset; }

  llvm::Value *getRawPointer(CodeGenFunction &CGF) const {
    return getUnsignedPointer();
  }

  /// Return address with different pointer, but same element type and
  /// alignment.
  Address withPointer(llvm::Value *NewPointer) const {
    return Address(NewPointer, getElementType(), getAlignment());
  }

  /// Return address with different alignment, but same pointer and element
  /// type.
  Address withAlignment(CharUnits NewAlignment) const {
    return Address(Pointer, getElementType(), NewAlignment);
  }
};

inline RawAddress::RawAddress(Address Addr)
    : A(Addr.isValid() ? Addr.getUnsignedPointer() : nullptr,
        Addr.isValid() ? Addr.getElementType() : nullptr,
        Addr.isValid() ? Addr.getAlignment() : CharUnits::Zero())  {
    }

/// A specialization of Address that requires the address to be an
/// LLVM Constant.
class ConstantAddress : public RawAddress {
  ConstantAddress(std::nullptr_t) : RawAddress(nullptr) {}

public:
  ConstantAddress(llvm::Constant *pointer, llvm::Type *elementType,
                  CharUnits alignment)
      : RawAddress(pointer, elementType, alignment) {}

  static ConstantAddress invalid() {
    return ConstantAddress(nullptr);
  }

  llvm::Constant *getPointer() const {
    return llvm::cast<llvm::Constant>(RawAddress::getPointer());
  }

  ConstantAddress getElementBitCast(llvm::Type *ElemTy) const {
    llvm::Constant *BitCast = llvm::ConstantExpr::getBitCast(
        getPointer(), ElemTy->getPointerTo(getAddressSpace()));
    return ConstantAddress(BitCast, ElemTy, getAlignment());
  }

  static bool isaImpl(RawAddress addr) {
    return llvm::isa<llvm::Constant>(addr.getPointer());
  }
  static ConstantAddress castImpl(RawAddress addr) {
    return ConstantAddress(llvm::cast<llvm::Constant>(addr.getPointer()),
                           addr.getElementType(), addr.getAlignment());
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
