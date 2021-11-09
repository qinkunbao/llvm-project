// RUN: %clang_cc1 %s -triple arm64e-apple-ios13 -fptrauth-calls -fptrauth-intrinsics -emit-llvm -o-  -fptrauth-function-pointer-type-discrimination | FileCheck %s

typedef void (*fptr_t)(void);

char *cptr;
void (*fptr)(void);

typedef struct __attribute__((ptrauth_struct(0,42))) S {} S;
S *sptr;

// CHECK-LABEL: define {{.*}} void @test1
void test1() {
  // CHECK: [[LOAD:%.*]] = load i8*, i8** @cptr
  // CHECK: [[CAST:%.*]] = bitcast i8* [[LOAD]] to void ()*
  // CHECK: [[TOINT:%.*]] = ptrtoint void ()* [[CAST]] to i64
  // CHECK: call i64 @llvm.ptrauth.resign(i64 [[TOINT]], i32 0, i64 0, i32 0, i64 18983)
  // CHECK: call void {{.*}}() [ "ptrauth"(i32 0, i64 18983) ]

  (*(fptr_t)cptr)();
}

// CHECK-LABEL: define {{.*}} i8 @test2
char test2() {
  return *(char *)fptr;

  // CHECK: [[LOAD:%.*]] = load void ()*, void ()** @fptr
  // CHECK: [[CAST:%.*]] = bitcast void ()* [[LOAD]] to i8*
  // CHECK: [[CMP:%.*]] = icmp ne i8* [[CAST]], null

  // CHECK: [[TOINT:%.*]] = ptrtoint i8* [[CAST]] to i64
  // CHECK: call i64 @llvm.ptrauth.resign(i64 [[TOINT]], i32 0, i64 18983, i32 0, i64 0)
}

// CHECK-LABEL: define {{.*}} void @test3
void test3() {
  (S *)fptr;
  // CHECK: [[LOAD:%.*]] = load void ()*, void ()** @fptr
  // CHECK: [[CAST:%.*]] = bitcast void ()* [[LOAD]] to %struct.S*
  // CHECK: [[CMP:%.*]] = icmp ne %struct.S* [[CAST]], null

  // CHECK: [[TOINT:%.*]] = ptrtoint %struct.S* [[CAST]] to i64
  // CHECK: call i64 @llvm.ptrauth.resign(i64 [[TOINT]], i32 0, i64 18983, i32 0, i64 42)
}

// CHECK-LABEL: define {{.*}} void @test4
void test4() {
  (*((fptr_t)(&*((char *)(&*(fptr_t)cptr)))))();

  // CHECK: [[LOAD:%.*]] = load i8*, i8** @cptr
  // CHECK-NEXT: [[CAST:%.*]] = bitcast i8* [[LOAD]] to void ()*
  // CHECK-NEXT: [[CAST2:%.*]] = bitcast void ()* [[CAST]] to i8*
  // CHECK-NEXT: [[CAST3:%.*]] = bitcast i8* [[CAST2]] to void ()*
  // CHECK-NEXT: [[CAST4:%.*]] = ptrtoint void ()* [[CAST3]] to i64
  // CHECK-NEXT: [[RESIGN:%.*]] = call i64 @llvm.ptrauth.resign(i64 [[CAST4]], i32 0, i64 0, i32 0, i64 18983)
  // CHECK-NEXT: [[CAST5:%.*]] = inttoptr i64 [[RESIGN]] to void ()*
  // CHECK-NEXT: call void [[CAST5]]() [ "ptrauth"(i32 0, i64 18983) ]
}

void *vptr;
void test5() {
  vptr = &*(char *)fptr;

  // CHECK: [[LOAD:%.*]] = load void ()*, void ()** @fptr
  // CHECK-NEXT: [[CAST:%.*]] = bitcast void ()* [[LOAD]] to i8*
  // CHECK-NEXT: [[CMP]] = icmp ne i8* [[CAST]], null
  // CHECK-NEXT: br i1 [[CMP]], label %[[NONNULL:.*]], label %[[CONT:.*]]

  // CHECK: [[NONNULL]]:
  // CHECK: [[RESIGN:%.*]] = call i64 @llvm.ptrauth.resign(i64 {{.*}}, i32 0, i64 18983, i32 0, i64 0)
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[RESIGN]] to i8*

  // CHECK: [[CONT]]:
  // CHECK: [[PHI:%.*]] = phi i8* [ null, {{.*}} ], [ [[CAST]], %[[NONNULL]] ]
  // CHECK: store i8* [[PHI]], i8** @vptr
}
