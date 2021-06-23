// RUN: %clang_cc1 -fptrauth-function-pointer-type-discrimination -triple arm64-apple-ios -fptrauth-calls -fcxx-exceptions -emit-llvm %s -o - | FileCheck %s

class Foo {
 public:
  ~Foo() {
  }
};

void f() {
  throw Foo();
}

// __cxa_throw is defined to take its destructor as "void (*)(void *)" in the ABI.
void __cxa_throw(void *exception, void *, void (*dtor)(void *)) {
  dtor(exception);
}

// CHECK: @_ZN3FooD1Ev.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (%class.Foo* (%class.Foo*)* @_ZN3FooD1Ev to i8*), i32 0, i64 0, i64 [[DISC:[0-9]+]] }, section "llvm.ptrauth", align 8

// CHECK: define {{.*}} void @_Z1fv()
// CHECK:  call void @__cxa_throw(i8* %{{.*}}, i8* bitcast ({ i8*, i8* }* @_ZTI3Foo to i8*), i8* bitcast ({ i8*, i32, i64, i64 }* @_ZN3FooD1Ev.ptrauth to i8*))

// CHECK: call void {{%.*}}(i8* {{%.*}}) [ "ptrauth"(i32 0, i64 [[DISC]]) ]
