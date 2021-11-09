// RUN: %clang_cc1 %s -triple arm64e-apple-ios13 -fptrauth-calls -fptrauth-intrinsics -emit-llvm -o- | FileCheck %s

typedef void (*fptr_t)(void);

char *cptr;
void (*fptr)(void);

typedef struct __attribute__((ptrauth_struct(0,42))) S {} S;
S *sptr;

// CHECK-LABEL: define {{.*}} void @test1
void test1() {
  // CHECK: [[LOAD:%.*]] = load i8*, i8** @cptr
  // CHECK: [[CAST:%.*]] = bitcast i8* [[LOAD]] to void ()*
  // CHECK: call void [[CAST]]() [ "ptrauth"(i32 0, i64 0) ]
  // CHECK: ret void

  (*(fptr_t)cptr)();
}

// CHECK-LABEL: define {{.*}} i8 @test2
char test2() {
  return *(char *)fptr;
  // CHECK: [[LOAD:%.*]] = load void ()*, void ()** @fptr
  // CHECK: [[CAST:%.*]] = bitcast void ()* [[LOAD]] to i8*
  // CHECK: [[LOAD:%.*]] = load i8, i8* [[CAST]]
  // CHECK: ret i8 %2
}

// CHECK-LABEL: define {{.*}} void @test3
void test3() {
  (S *)fptr;
  // CHECK: [[LOAD:%.*]] = load void ()*, void ()** @fptr
  // CHECK: [[CAST:%.*]] = bitcast void ()* [[LOAD]] to %struct.S*
  // CHECK: [[CMP:%.*]] = icmp ne %struct.S* [[CAST]], null

  // CHECK: [[TOINT:%.*]] = ptrtoint %struct.S* [[CAST]] to i64
  // CHECK: call i64 @llvm.ptrauth.resign(i64 [[TOINT]], i32 0, i64 0, i32 0, i64 42)
}
