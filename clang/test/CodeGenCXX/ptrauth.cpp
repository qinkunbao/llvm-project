// RUN: %clang_cc1 -triple arm64-apple-ios -fptrauth-calls -fptrauth-returns -fptrauth-intrinsics -emit-llvm -std=c++11 -fexceptions -fcxx-exceptions -o - %s | FileCheck %s

// CHECK: @_Z1fv.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (void ()* @_Z1fv to i8*), i32 0, i64 0, i64 0 },

void f(void);
auto &f_ref = f;

// CHECK-LABEL: define {{.*}} void @_Z1gv(
// CHECK: call void bitcast ({ i8*, i32, i64, i64 }* @_Z1fv.ptrauth to void ()*)() [ "ptrauth"(i32 0, i64 0) ]

void g() { f_ref(); }

void foo1();

void test_terminate() noexcept {
  foo1();
}

// CHECK: define {{.*}} void @_ZSt9terminatev() #[[ATTR4:.*]] {

namespace std {
  void terminate() noexcept {
  }
}

// CHECK: attributes #[[ATTR4]] = {{{.*}}"ptrauth-calls" "ptrauth-returns"{{.*}}}
