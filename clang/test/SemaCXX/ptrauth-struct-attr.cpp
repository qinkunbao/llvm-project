// RUN: %clang_cc1 -triple arm64-apple-ios -fsyntax-only -verify %s

#define DISC0 100
#define DISC1 101

struct __attribute__((ptrauth_struct(2, DISC0))) SignedBase0 {};
struct __attribute__((ptrauth_struct(2, DISC1))) SignedBase1 {};
struct UnsignedBase0 {};
struct __attribute__((ptrauth_struct(2, DISC0))) SignedDerivded0 : SignedBase0 {};
struct __attribute__((ptrauth_struct(2, DISC1))) SignedDerivded1 : SignedBase0 {}; // expected-error {{key or discriminator of class 'SignedDerivded1' doesn't match that of base class}}
struct __attribute__((ptrauth_struct(2, DISC0))) SignedDerivded2 : SignedBase0, SignedBase1 {}; // expected-error {{key or discriminator of class 'SignedDerivded2' doesn't match that of base class 'SignedBase1'}}

struct __attribute__((ptrauth_struct(2, DISC0))) SignedDynamic0 { // expected-error {{cannot be used on class 'SignedDynamic0' because it is a dynamic class}}
  virtual void m0();
};
struct __attribute__((ptrauth_struct(2, DISC1))) SignedDynamic1 : virtual SignedBase1 {}; // expected-error {{cannot be used on class 'SignedDynamic1' because it is a dynamic class}}
struct UnsignedDynamic2 : virtual SignedBase0 {}; // expected-error {{key or discriminator of class 'UnsignedDynamic2' doesn't match that of base class}}

template <class T>
struct __attribute__((ptrauth_struct(2, DISC0))) Template0 : T {}; // expected-error 2 {{key or discriminator of class 'Template0' doesn't match that of base class}}

Template0<SignedBase0> g0;
Template0<SignedBase1> g1; // expected-note {{in instantiation of}}
Template0<UnsignedBase0> g2; // expected-note {{in instantiation of}}

struct DynamicBase0 {
  virtual void m0();
};

struct DynamicBase1 : virtual UnsignedBase0 {
};

template <class T>
struct __attribute__((ptrauth_struct(2, DISC0))) Template1 { // expected-error {{because it is a dynamic class}}
  virtual void m0();
};

template <class T>
struct __attribute__((ptrauth_struct(2, DISC0))) Template2 : DynamicBase0 {}; // expected-error {{doesn't match that of base class}} expected-error {{because it is a dynamic class}}

template <class T>
struct __attribute__((ptrauth_struct(2, DISC0))) Template3 : DynamicBase1 {}; // expected-error {{doesn't match that of base class}} expected-error {{because it is a dynamic class}}

template <class T>
struct __attribute__((ptrauth_struct(2, DISC1))) Template4 {
};

template <class T>
struct __attribute__((ptrauth_struct(2, DISC0))) Template5 : Template4<T> {}; // expected-error {{doesn't match that of base class}}

template <class T>
struct __attribute__((ptrauth_struct(2, DISC0))) Template6 : Template4<T> {};

template <>
struct __attribute__((ptrauth_struct(2, DISC0))) Template4<int> {
};

Template5<float> g3; // expected-note {{in instantiation of}}
Template5<int> g4;
