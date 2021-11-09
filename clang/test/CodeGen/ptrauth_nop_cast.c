// RUN: %clang_cc1 -triple arm64e-apple-ios13.0 -fptrauth-calls -fptrauth-intrinsics -O1 %s -emit-llvm -o - | FileCheck %s

#include <ptrauth.h>

void *test_nop_cast(void (*fptr)(int)) {
  // CHECK: define {{.*}} i8* @test_nop_cast(void (i32)* readnone [[FPTR:%.*]])
  // CHECK: [[RES:%.*]] = bitcast void (i32)* [[FPTR]] to i8*
  // CHECK: ret i8* [[RES]]
  return ptrauth_nop_cast(void *, fptr);
}

typedef void (*VoidFn)(void);

VoidFn test_nop_cast_functype(void (*fptr)(int, int)) {
  // CHECK: define {{.*}} void ()* @test_nop_cast_functype(void (i32, i32)* readnone [[FPTR:%.*]])
  // CHECK: [[RES:%.*]] = bitcast void (i32, i32)* [[FPTR]] to void ()*
  // CHECK: ret void ()* [[RES]]
  return ptrauth_nop_cast(void (*)(void), fptr);
}

void *test_nop_cast_direct() {
  // CHECK: define {{.*}} i8* @test_nop_cast_direct()
  // CHECK: ret i8* bitcast ({ i8*, i32, i64, i64 }* @test_nop_cast_direct.ptrauth to i8*)
  return ptrauth_nop_cast(void *, test_nop_cast_direct);
}
