// RUN: %clang_cc1 %s -no-opaque-pointers -triple arm64e-apple-ios13 -fptrauth-calls -fptrauth-intrinsics -emit-llvm -o- | FileCheck %s

typedef void (*fptr_t)(void);

char *cptr;
void (*fptr)(void);

// CHECK-LABEL: define void @test1
void test1() {
  // CHECK: [[LOAD:%.*]] = load i8*, i8** @cptr
  // CHECK: [[CAST:%.*]] = bitcast i8* [[LOAD]] to void ()*
  // CHECK: call void [[CAST]]() [ "ptrauth"(i32 0, i64 0) ]
  // CHECK: ret void

  (*(fptr_t)cptr)();
}

// CHECK-LABEL: define i8 @test2
char test2() {
  return *(char *)fptr;
  // CHECK: [[LOAD:%.*]] = load void ()*, void ()** @fptr
  // CHECK: [[CAST:%.*]] = bitcast void ()* [[LOAD]] to i8*
  // CHECK: [[LOAD:%.*]] = load i8, i8* [[CAST]]
  // CHECK: ret i8 %2
}
