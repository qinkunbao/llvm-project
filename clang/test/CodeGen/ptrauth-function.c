// RUN: %clang_cc1 %s       -fptrauth-function-pointer-type-discrimination -triple arm64e-apple-ios13 -fptrauth-calls -fptrauth-intrinsics -disable-llvm-passes -emit-llvm -o- | FileCheck %s --check-prefix=CHECK --check-prefix=CHECKC
// RUN: %clang_cc1 -xc++ %s -fptrauth-function-pointer-type-discrimination -triple arm64e-apple-ios13 -fptrauth-calls -fptrauth-intrinsics -disable-llvm-passes -emit-llvm -o- | FileCheck %s --check-prefix=CHECK --check-prefix=CHECKCXX

#ifdef __cplusplus
extern "C" {
#endif

void f(void);
void f2(int);
void (*fptr)(void);
void (* __ptrauth(0, 0, 42) f2ptr_42_discm)(int);
void *opaque;
unsigned long uintptr;

// CHECK: @test_constant_null = {{.*}} global void (i32)* null
void (*test_constant_null)(int) = 0;

// CHECK: @f.ptrauth = private constant { {{.*}} } { i8* bitcast (void ()* @f to i8*), i32 0, i64 0, i64 2712 }
// CHECK: @test_constant_cast = {{.*}} global void (i32)* bitcast ({ {{.*}} }* @f.ptrauth to void (i32)*)
void (*test_constant_cast)(int) = (void (*)(int))f;

// CHECK: @f.ptrauth.1 = private constant { {{.*}} } { i8* bitcast (void ()* @f to i8*), i32 0, i64 0, i64 42 }
// CHECK: @test_qualifier = {{.*}} global void (i32)* bitcast ({ {{.*}} }* @f.ptrauth.1 to void (i32)*)
void (*__ptrauth(0,0,42) test_qualifier)(int) = (void (*)(int))f;

// CHECK: @f.ptrauth.2 = private constant { {{.*}} } { i8* bitcast (void ()* @f to i8*), i32 0, i64 0, i64 0 }
// CHECK: @test_opaque = {{.*}} global i8* bitcast ({ {{.*}} }* @f.ptrauth.2 to i8*)
void *test_opaque =
#ifdef __cplusplus
    (void *)
#endif
    (void (*)(int))(double (*)(double))f;

// CHECK: @test_intptr_t = {{.*}} global i64 ptrtoint ({ {{.*}} }* @f.ptrauth.2 to i64)
unsigned long test_intptr_t = (unsigned long)f;

// CHECK: @test_through_long = {{.*}} global void (i32)* bitcast ({ {{.*}} }* @f.ptrauth to void (i32)*)
void (*test_through_long)(int) = (void (*)(int))(long)f;

// CHECK: @test_to_long = {{.*}} global i64 ptrtoint ({ {{.*}} }* @f.ptrauth.2 to i64)
long test_to_long = (long)(double (*)())f;

// CHECKC: @knr.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (void (i32)* @knr to i8*), i32 0, i64 0, i64 18983 }, section "llvm.ptrauth"

// CHECKC: @redecl.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (void (...)* @redecl to i8*), i32 0, i64 0, i64 18983 }, section "llvm.ptrauth"
// CHECKC: @redecl.ptrauth.4 = private constant { i8*, i32, i64, i64 } { i8* bitcast (void (...)* @redecl to i8*), i32 0, i64 0, i64 2712 }, section "llvm.ptrauth"

#ifdef __cplusplus
struct ptr_member {
  void (*fptr_)(int) = 0;
};
ptr_member pm;
void (*test_member)() = (void (*)())pm.fptr_;

// CHECKCXX-LABEL: define internal void @__cxx_global_var_init
// CHECKCXX: call i64 @llvm.ptrauth.resign(i64 {{.*}}, i32 0, i64 2712, i32 0, i64 18983)
#endif


// CHECK-LABEL: define {{.*}} void @test_cast_to_opaque
void test_cast_to_opaque() {
  opaque = (void *)f;

  // CHECK: br i1 true, label %[[RESIGN:.*]], label %[[CONT:.*]]
  // CHECK: [[RESIGN]]:
  // CHECK: [[RESIGN_VAL:%.*]] = call i64 @llvm.ptrauth.resign(i64 ptrtoint ({ i8*, i32, i64, i64 }* @f.ptrauth.3 to i64), i32 0, i64 18983, i32 0, i64 0)
  // CHECK: [[RESIGN_PTR:%.*]] = inttoptr i64 [[RESIGN_VAL]] to i8*

  // CHECK: [[CONT]]:
  // CHECK: phi i8* [ null, {{.*}} ], [ [[RESIGN_PTR]], %[[RESIGN]] ]
}

// CHECK-LABEL: define {{.*}} void @test_cast_from_opaque
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

// CHECK-LABEL: define {{.*}} void @test_cast_to_intptr
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

// CHECK-LABEL: define {{.*}} void @test_function_to_function_cast
void test_function_to_function_cast() {
  void (*fptr2)(int) = (void (*)(int))fptr;
  // CHECK: call i64 @llvm.ptrauth.resign(i64 {{.*}}, i32 0, i64 18983, i32 0, i64 2712)
}

// CHECK-LABEL: define {{.*}} void @test_call
void test_call() {
  fptr();
  // CHECK: call void %0() [ "ptrauth"(i32 0, i64 18983) ]
}

// CHECK-LABEL: define {{.*}} void @test_assign_to_qualified
void test_assign_to_qualified() {
  f2ptr_42_discm = (void (*)(int))fptr;

  // CHECK: [[ENTRY:.*]]:{{$}}
  // CHECK: [[FPTR:%.*]] = load void ()*, void ()** @fptr
  // CHECK-NEXT: [[FPTR1:%.*]] = bitcast void ()* [[FPTR]] to void (i32)*
  // CHECK-NEXT: [[CMP:%.*]] = icmp ne void (i32)* [[FPTR1]], null
  // CHECK-NEXT: br i1 [[CMP]], label %[[RESIGN1:.*]], label %[[JOIN1:.*]]

  // CHECK: [[RESIGN1]]:
  // CHECK-NEXT: [[FPTR2:%.*]] = ptrtoint void (i32)* [[FPTR1]] to i64
  // CHECK-NEXT: [[FPTR4:%.*]] = call i64 @llvm.ptrauth.resign(i64 [[FPTR2]], i32 0, i64 18983, i32 0, i64 2712)
  // CHECK-NEXT: [[FPTR5:%.*]] = inttoptr i64 [[FPTR4]] to void (i32)*
  // CHECK-NEXT: br label %[[JOIN1]]

  // CHECK: [[JOIN1]]:
  // CHECK-NEXT: [[FPTR6:%.*]] = phi void (i32)* [ null, %[[ENTRY]] ], [ [[FPTR5]], %[[RESIGN1]] ]
  // CHECK-NEXT: [[CMP:%.*]] = icmp ne void (i32)* [[FPTR6]], null
  // CHECK-NEXT: br i1 [[CMP]], label %[[RESIGN2:.*]], label %[[JOIN2:.*]]

  // CHECK: [[RESIGN2]]:
  // CHECK-NEXT: [[FPTR7:%.*]] = ptrtoint void (i32)* [[FPTR6]] to i64
  // CHECK-NEXT: [[FPTR8:%.*]] = call i64 @llvm.ptrauth.resign(i64 [[FPTR7]], i32 0, i64 2712, i32 0, i64 42)
  // CHECK-NEXT: [[FPTR9:%.*]] = inttoptr i64 [[FPTR8]] to void (i32)*
  // CHECK-NEXT: br label %[[JOIN2]]

  // CHECK: [[JOIN2]]
  // CHECK-NEXT: [[FPTR10:%.*]] = phi void (i32)* [ null, %[[JOIN1]] ], [ [[FPTR9]], %[[RESIGN2]] ]
  // CHECK-NEXT store void (i32)* [[FPTR10]], void (i32)** @f2ptr_42_discm
}

// CHECK-LABEL: define {{.*}} void @test_assign_from_qualified
void test_assign_from_qualified() {
  fptr = (void (*)(void))f2ptr_42_discm;

  // CHECK: [[ENTRY:.*]]:{{$}}
  // CHECK: [[FPTR:%.*]] = load void (i32)*, void (i32)** @f2ptr_42_discm
  // CHECK-NEXT: [[CMP:%.*]] = icmp ne void (i32)* [[FPTR]], null
  // CHECK-NEXT: br i1 [[CMP]], label %[[RESIGN1:.*]], label %[[JOIN1:.*]]

  // CHECK: [[RESIGN1]]:
  // CHECK-NEXT: [[FPTR1:%.*]] = ptrtoint void (i32)* [[FPTR]] to i64
  // CHECK-NEXT: [[FPTR2:%.*]] = call i64 @llvm.ptrauth.resign(i64 [[FPTR1]], i32 0, i64 42, i32 0, i64 2712)
  // CHECK-NEXT: [[FPTR3:%.*]] = inttoptr i64 [[FPTR2]] to void (i32)*
  // CHECK-NEXT: br label %[[JOIN1]]

  // CHECK: [[JOIN1]]:
  // CHECK-NEXT: [[FPTR4:%.*]] = phi void (i32)* [ null, %[[ENTRY]] ], [ [[FPTR3]], %[[RESIGN1]] ]
  // CHECK-NEXT: [[FPTR5:%.*]] = bitcast void (i32)* [[FPTR4]] to void ()*
  // CHECK-NEXT: [[CMP:%.*]] = icmp ne void ()* [[FPTR5]], null
  // CHECK-NEXT: br i1 [[CMP]], label %[[RESIGN2:.*]], label %[[JOIN2:.*]]

  // CHECK: [[RESIGN2]]:
  // CHECK-NEXT: [[FPTR6:%.*]] = ptrtoint void ()* [[FPTR5]] to i64
  // CHECK-NEXT: [[FPTR7:%.*]] = call i64 @llvm.ptrauth.resign(i64 [[FPTR6]], i32 0, i64 2712, i32 0, i64 18983)
  // CHECK-NEXT: [[FPTR8:%.*]] = inttoptr i64 [[FPTR7]] to void ()*
  // CHECK-NEXT: br label %[[JOIN2]]

  // CHECK: [[JOIN2]]
  // CHECK-NEXT: [[FPTR9:%.*]] = phi void ()* [ null, %[[JOIN1]] ], [ [[FPTR8]], %[[RESIGN2]] ]
  // CHECK-NEXT store void ()* [[FPTR10]], void ()** @f2ptr_42_discm
}

// CHECK-LABEL: define {{.*}} void @test_call_lvalue_cast
void test_call_lvalue_cast() {
  (*(void (*)(int))f)(42);

  // CHECK: entry:
  // CHECK-NEXT: [[RESIGN:%.*]] = call i64 @llvm.ptrauth.resign(i64 ptrtoint ({ i8*, i32, i64, i64 }* @f.ptrauth.3 to i64), i32 0, i64 18983, i32 0, i64 2712)
  // CHECK-NEXT: [[RESIGN_INT:%.*]] = inttoptr i64 [[RESIGN]] to void (i32)*
  // CHECK-NEXT: call void [[RESIGN_INT]](i32 42) [ "ptrauth"(i32 0, i64 2712) ]
}

#ifndef __cplusplus

void knr(param)
  int param;
{}

// CHECKC-LABEL: define {{.*}} void @test_knr
void test_knr() {
  void (*p)() = knr;
  p(0);

  // CHECKC: [[P:%.*]] = alloca void (...)*
  // CHECKC: store void (...)* bitcast ({ {{.*}} }* @knr.ptrauth to void (...)*), void (...)** [[P]]
  // CHECKC: [[LOAD:%.*]] = load void (...)*, void (...)** [[P]]
  // CHECKC: [[CAST:%.*]] = bitcast void (...)* [[LOAD]] to void (i32)*
  // CHECKC: call void [[CAST]](i32 0) [ "ptrauth"(i32 0, i64 18983) ]

  void *p2 = p;

  // CHECKC: call i64 @llvm.ptrauth.resign(i64 {{.*}}, i32 0, i64 18983, i32 0, i64 0)
}

// CHECKC-LABEL: define {{.*}} void @test_redeclaration
void test_redeclaration() {
  void redecl();
  void (*ptr)() = redecl;
  void redecl(int);
  void (*ptr2)(int) = redecl;
  ptr();
  ptr2(0);

  // CHECKC-NOT: call i64 @llvm.ptrauth.resign
  // CHECKC: call void {{.*}}() [ "ptrauth"(i32 0, i64 18983) ]
  // CHECKC: call void {{.*}}(i32 0) [ "ptrauth"(i32 0, i64 2712) ]
}

void knr2(param)
     int param;
{}

// CHECKC-LABEL: define {{.*}} void @test_redecl_knr
void test_redecl_knr() {
  void (*p)() = knr2;
  p();

  void knr2(int);

  void (*p2)(int) = knr2;
  p2(0);

  // CHECKC-NOT: call i64 @llvm.ptrauth.resign
  // CHECKC: call void {{.*}}() [ "ptrauth"(i32 0, i64 18983) ]
  // CHECKC: call void {{.*}}(i32 0) [ "ptrauth"(i32 0, i64 2712) ]
}

#endif

#ifdef __cplusplus
}
#endif
