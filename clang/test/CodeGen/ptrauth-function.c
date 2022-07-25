// RUN: %clang_cc1 %s       -no-opaque-pointers -fptrauth-function-pointer-type-discrimination -triple arm64e-apple-ios13 -fptrauth-calls -fptrauth-intrinsics -disable-llvm-passes -emit-llvm -o- | FileCheck %s --check-prefix=CHECK --check-prefix=CHECKC
// RUN: %clang_cc1 -xc++ %s -no-opaque-pointers -fptrauth-function-pointer-type-discrimination -triple arm64e-apple-ios13 -fptrauth-calls -fptrauth-intrinsics -disable-llvm-passes -emit-llvm -o- | FileCheck %s --check-prefix=CHECK --check-prefix=CHECKCXX

#ifdef __cplusplus
extern "C" {
#endif

void f(void);
void f2(int);
void (*fptr)(void);
void *opaque;
unsigned long uintptr;

// CHECK: @test_constant_null = global void (i32)* null
void (*test_constant_null)(int) = 0;

// CHECK: @f.ptrauth = private constant { {{.*}} } { i8* bitcast (void ()* @f to i8*), i32 0, i64 0, i64 2712 }
// CHECK: @test_constant_cast = global void (i32)* bitcast ({ {{.*}} }* @f.ptrauth to void (i32)*)
void (*test_constant_cast)(int) = (void (*)(int))f;

// CHECK: @f.ptrauth.1 = private constant { {{.*}} } { i8* bitcast (void ()* @f to i8*), i32 0, i64 0, i64 0 }
// CHECK: @test_opaque = global i8* bitcast ({ {{.*}} }* @f.ptrauth.1 to i8*)
void *test_opaque =
#ifdef __cplusplus
    (void *)
#endif
    (void (*)(int))(double (*)(double))f;

// CHECK: @test_intptr_t = global i64 ptrtoint ({ {{.*}} }* @f.ptrauth.1 to i64)
unsigned long test_intptr_t = (unsigned long)f;

// CHECK: @test_through_long = global void (i32)* bitcast ({ {{.*}} }* @f.ptrauth to void (i32)*)
void (*test_through_long)(int) = (void (*)(int))(long)f;

// CHECK: @test_to_long = global i64 ptrtoint ({ {{.*}} }* @f.ptrauth.1 to i64)
long test_to_long = (long)(double (*)())f;

// CHECKC: @knr.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (void (i32)* @knr to i8*), i32 0, i64 0, i64 18983 }, section "llvm.ptrauth"

// CHECKC: @redecl.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (void (...)* @redecl to i8*), i32 0, i64 0, i64 18983 }, section "llvm.ptrauth"
// CHECKC: @redecl.ptrauth.3 = private constant { i8*, i32, i64, i64 } { i8* bitcast (void (...)* @redecl to i8*), i32 0, i64 0, i64 2712 }, section "llvm.ptrauth"

#ifdef __cplusplus
struct ptr_member {
  void (*fptr_)(int) = 0;
};
ptr_member pm;
void (*test_member)() = (void (*)())pm.fptr_;

// CHECKCXX-LABEL: define internal void @__cxx_global_var_init
// CHECKCXX: call i64 @llvm.ptrauth.resign(i64 {{.*}}, i32 0, i64 2712, i32 0, i64 18983)
#endif


// CHECK-LABEL: define void @test_cast_to_opaque
void test_cast_to_opaque() {
  opaque = (void *)f;

  // CHECK: br i1 true, label %[[RESIGN:.*]], label %[[CONT:.*]]
  // CHECK: [[RESIGN]]:
  // CHECK: [[RESIGN_VAL:%.*]] = call i64 @llvm.ptrauth.resign(i64 ptrtoint ({ i8*, i32, i64, i64 }* @f.ptrauth.2 to i64), i32 0, i64 18983, i32 0, i64 0)
  // CHECK: [[RESIGN_PTR:%.*]] = inttoptr i64 [[RESIGN_VAL]] to i8*

  // CHECK: [[CONT]]:
  // CHECK: phi i8* [ null, {{.*}} ], [ [[RESIGN_PTR]], %[[RESIGN]] ]
}

// CHECK-LABEL: define void @test_cast_from_opaque
void test_cast_from_opaque() {
  fptr = (void (*)(void))opaque;

  // CHECK: [[LOAD:%.*]] = load i8*, i8** @opaque
  // CHECK: [[CAST:%.*]] = bitcast i8* [[LOAD]] to void ()*
  // CHECK: [[CMP:%.*]] = icmp ne void ()* [[CAST]], null
  // CHECK: br i1 [[CMP]], label %[[RESIGN_LAB:.*]], label

  // CHECK: [[RESIGN_LAB]]:
  // CHECK: [[INT:%.*]] = ptrtoint void ()* [[CAST]] to i64
  // CHECK: [[RESIGN_INT:%.*]] = call i64 @llvm.ptrauth.resign(i64 [[INT]], i32 0, i64 0, i32 0, i64 18983)
}

// CHECK-LABEL: define void @test_cast_to_intptr
void test_cast_to_intptr() {
  uintptr = (unsigned long)fptr;

  // CHECK: [[ENTRY:.*]]:
  // CHECK: [[LOAD:%.*]] = load void ()*, void ()** @fptr
  // CHECK: [[CMP:%.*]] = icmp ne void ()* [[LOAD]], null
  // CHECK: br i1 [[CMP]], label %[[RESIGN_LAB:.*]], label %[[RESIGN_CONT:.*]]

  // CHECK: [[RESIGN_LAB]]:
  // CHECK: [[INT:%.*]] = ptrtoint void ()* [[LOAD]] to i64
  // CHECK: [[RESIGN_INT:%.*]] = call i64 @llvm.ptrauth.resign(i64 [[INT]], i32 0, i64 18983, i32 0, i64 0)
  // CHECK: [[RESIGN:%.*]] = inttoptr i64 [[RESIGN_INT]] to void ()*
  // CHECK: br label %[[RESIGN_CONT]]

  // CHECK: [[RESIGN_CONT]]:
  // CHECK: phi void ()* [ null, %[[ENTRY]] ], [ [[RESIGN]], %[[RESIGN_LAB]] ]
}

// CHECK-LABEL: define void @test_function_to_function_cast
void test_function_to_function_cast() {
  void (*fptr2)(int) = (void (*)(int))fptr;
  // CHECK: call i64 @llvm.ptrauth.resign(i64 {{.*}}, i32 0, i64 18983, i32 0, i64 2712)
}

// CHECK-LABEL: define void @test_call
void test_call() {
  fptr();
  // CHECK: call void %0() [ "ptrauth"(i32 0, i64 18983) ]
}

// CHECK-LABEL: define void @test_call_lvalue_cast
void test_call_lvalue_cast() {
  (*(void (*)(int))f)(42);

  // CHECK: entry:
  // CHECK-NEXT: [[RESIGN:%.*]] = call i64 @llvm.ptrauth.resign(i64 ptrtoint ({ i8*, i32, i64, i64 }* @f.ptrauth.2 to i64), i32 0, i64 18983, i32 0, i64 2712)
  // CHECK-NEXT: [[RESIGN_INT:%.*]] = inttoptr i64 [[RESIGN]] to void (i32)*
  // CHECK-NEXT: call void [[RESIGN_INT]](i32 noundef 42) [ "ptrauth"(i32 0, i64 2712) ]
}

#ifndef __cplusplus

void knr(param)
  int param;
{}

// CHECKC-LABEL: define void @test_knr
void test_knr() {
  void (*p)() = knr;
  p(0);

  // CHECKC: [[P:%.*]] = alloca void (...)*
  // CHECKC: store void (...)* bitcast ({ {{.*}} }* @knr.ptrauth to void (...)*), void (...)** [[P]]
  // CHECKC: [[LOAD:%.*]] = load void (...)*, void (...)** [[P]]
  // CHECKC: [[CAST:%.*]] = bitcast void (...)* [[LOAD]] to void (i32)*
  // CHECKC: call void [[CAST]](i32 noundef 0) [ "ptrauth"(i32 0, i64 18983) ]

  void *p2 = p;

  // CHECKC: call i64 @llvm.ptrauth.resign(i64 {{.*}}, i32 0, i64 18983, i32 0, i64 0)
}

// CHECKC-LABEL: define void @test_redeclaration
void test_redeclaration() {
  void redecl();
  void (*ptr)() = redecl;
  void redecl(int);
  void (*ptr2)(int) = redecl;
  ptr();
  ptr2(0);

  // CHECKC-NOT: call i64 @llvm.ptrauth.resign
  // CHECKC: call void {{.*}}() [ "ptrauth"(i32 0, i64 18983) ]
  // CHECKC: call void {{.*}}(i32 noundef 0) [ "ptrauth"(i32 0, i64 2712) ]
}

void knr2(param)
     int param;
{}

// CHECKC-LABEL: define void @test_redecl_knr
void test_redecl_knr() {
  void (*p)() = knr2;
  p();

  void knr2(int);

  void (*p2)(int) = knr2;
  p2(0);

  // CHECKC-NOT: call i64 @llvm.ptrauth.resign
  // CHECKC: call void {{.*}}() [ "ptrauth"(i32 0, i64 18983) ]
  // CHECKC: call void {{.*}}(i32 noundef 0) [ "ptrauth"(i32 0, i64 2712) ]
}

#endif

#ifdef __cplusplus
}
#endif
