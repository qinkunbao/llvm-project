// RUN: %clang_cc1 -no-opaque-pointers -triple arm64-apple-ios -fptrauth-calls -fptrauth-intrinsics -emit-llvm %s -O0 -o - | FileCheck %s

// This is largely a duplicate of CodeGen/ptrauth-authenticated-null-values.c as
// there are C++ specific branches in some struct init and copy implementations
// so we want to be sure that the behaviour is still correct in C++ mode.

typedef void *__ptrauth(2, 0, 0, "authenticates-null-values") authenticated_null;
typedef void *__ptrauth(2, 1, 0, "authenticates-null-values") authenticated_null_addr_disc;
typedef void *__ptrauth(2, 0, 0) unauthenticated_null;

int test_global;

// CHECK: define void @f0(i8** [[AUTH1_ARG:%.*]], i8** [[AUTH2_ARG:%.*]])
extern "C" void f0(authenticated_null *auth1, authenticated_null *auth2) {
  *auth1 = *auth2;
  // CHECK: [[AUTH1_ARG]].addr = alloca i8**
  // CHECK: [[AUTH2_ARG]].addr = alloca i8**
  // CHECK: store i8** [[AUTH1_ARG]], i8*** [[AUTH1_ARG]].addr
  // CHECK: store i8** [[AUTH2_ARG]], i8*** [[AUTH2_ARG]].addr
  // CHECK: [[AUTH1_ADDR:%.*]] = load i8**, i8*** [[AUTH1_ARG]].addr
  // CHECK: [[AUTH2_ADDR:%.*]] = load i8**, i8*** [[AUTH2_ARG]].addr
  // CHECK: [[AUTH2_VALUE:%.*]] = load i8*, i8** [[AUTH2_ADDR]]
  // CHECK: store i8* [[AUTH2_VALUE]], i8** [[AUTH1_ADDR]]
}

// CHECK: define void @f1(i8** [[AUTH1_ARG:%.*]], i8** [[AUTH2_ARG:%.*]])
extern "C" void f1(unauthenticated_null *auth1, authenticated_null *auth2) {
  *auth1 = *auth2;
  // CHECK: [[ENTRY:.*]]:
  // CHECK: [[AUTH1_ARG]].addr = alloca i8**
  // CHECK: [[AUTH2_ARG]].addr = alloca i8**
  // CHECK: store i8** [[AUTH1_ARG]], i8*** [[AUTH1_ARG]].addr
  // CHECK: store i8** [[AUTH2_ARG]], i8*** [[AUTH2_ARG]].addr
  // CHECK: [[AUTH1_ADDR:%.*]] = load i8**, i8*** [[AUTH1_ARG]].addr
  // CHECK: [[AUTH2_ADDR:%.*]] = load i8**, i8*** [[AUTH2_ARG]].addr
  // CHECK: [[AUTH2:%.*]] = load i8*, i8** [[AUTH2_ADDR]]
  // CHECK: [[CAST_VALUE:%.*]] = ptrtoint i8* %2 to i64
  // CHECK: [[AUTHED_VALUE:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[CAST_VALUE]], i32 2, i64 0)
  // CHECK: [[TRUE_VALUE:%.*]] = inttoptr i64 [[AUTHED_VALUE]] to i8*
  // CHECK: [[COMPARISON:%.*]] = icmp ne i8* [[TRUE_VALUE]], null
  // CHECK: br i1 [[COMPARISON]], label %resign.nonnull, label %resign.cont
  // CHECK: resign.nonnull:
  // CHECK: %7 = ptrtoint i8* %2 to i64
  // CHECK: %8 = call i64 @llvm.ptrauth.resign(i64 %7, i32 2, i64 0, i32 2, i64 0)
  // CHECK: %9 = inttoptr i64 %8 to i8*
  // CHECK: br label %resign.cont
  // CHECK: resign.cont:
  // CHECK: [[RESULT:%.*]] = phi i8* [ null, %entry ], [ %9, %resign.nonnull ]
  // CHECK: store i8* [[RESULT]], i8** [[AUTH1_ADDR]]
}

// CHECK: define void @f2(i8** [[AUTH1_ARG:%.*]], i8** [[AUTH2_ARG:%.*]])
extern "C" void f2(authenticated_null *auth1, unauthenticated_null *auth2) {
  *auth1 = *auth2;
  // CHECK: [[ENTRY:.*]]:
  // CHECK: [[AUTH1_ARG]].addr = alloca i8**
  // CHECK: [[AUTH2_ARG]].addr = alloca i8**
  // CHECK: store i8** [[AUTH1_ARG]], i8*** [[AUTH1_ARG]].addr
  // CHECK: store i8** [[AUTH2_ARG]], i8*** [[AUTH2_ARG]].addr
  // CHECK: [[AUTH1_ADDR:%.*]] = load i8**, i8*** [[AUTH1_ARG]].addr
  // CHECK: [[AUTH2_ADDR:%.*]] = load i8**, i8*** [[AUTH2_ARG]].addr
  // CHECK: [[AUTH2:%.*]] = load i8*, i8** [[AUTH2_ADDR]]
  // CHECK: [[COMPARE:%.*]] = icmp ne i8* [[AUTH2]], null
  // CHECK: br i1 [[COMPARE]], label %[[NON_NULL:resign.*]], label %[[NULL:resign.*]]
  // CHECK: [[NULL]]:
  // CHECK: [[SIGNED_ZERO:%.*]] = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 0)
  // CHECK: [[SIGNED_NULL:%.*]] = inttoptr i64 [[SIGNED_ZERO]] to i8*
  // CHECK: br label %[[CONT:resign.*]]
  // CHECK: [[NON_NULL]]:
  // CHECK: [[AUTH2_CAST:%.*]] = ptrtoint i8* [[AUTH2]] to i64
  // CHECK: [[AUTH2_AUTHED:%.*]] = call i64 @llvm.ptrauth.resign(i64 [[AUTH2_CAST]], i32 2, i64 0, i32 2, i64 0)
  // CHECK: [[AUTH2:%.*]] = inttoptr i64 [[AUTH2_AUTHED]] to i8*
  // CHECK: br label %[[CONT]]

  // CHECK: [[CONT]]:
  // CHECK: [[RESULT:%.*]] = phi i8* [ [[SIGNED_NULL]], %[[NULL]] ], [ [[AUTH2]], %[[NON_NULL]] ]
  // CHECK: store i8* [[RESULT]], i8** [[AUTH1_ADDR]]
}

// CHECK: define void @f3(i8** [[AUTH1:%.*]], i8* [[I:%.*]])
extern "C" void f3(authenticated_null *auth1, void *i) {
  *auth1 = i;
  // CHECK: [[AUTH1_ADDR:%.*]] = alloca i8**
  // CHECK: [[I_ADDR:%.*]] = alloca i8*
  // CHECK: store i8** [[AUTH1]], i8*** [[AUTH1_ADDR]]
  // CHECK: store i8* [[I]], i8** [[I_ADDR]]
  // CHECK: [[AUTH1:%.*]] = load i8**, i8*** [[AUTH1_ADDR]]
  // CHECK: [[I:%.*]] = load i8*, i8** [[I_ADDR]]
  // CHECK: [[CAST_I:%.*]] = ptrtoint i8* [[I]] to i64
  // CHECK: [[SIGNED_CAST_I:%.*]] = call i64 @llvm.ptrauth.sign(i64 [[CAST_I]], i32 2, i64 0)
  // CHECK: [[SIGNED_I:%.*]] = inttoptr i64 [[SIGNED_CAST_I]] to i8*
  // CHECK: store i8* [[SIGNED_I]], i8** [[AUTH1]]
}

// CHECK: define void @f4(i8** [[AUTH1:%.*]])
extern "C" void f4(authenticated_null *auth1) {
  *auth1 = 0;
  // CHECK: [[AUTH1_ADDR:%.*]] = alloca i8**
  // CHECK: store i8** [[AUTH1]], i8*** [[AUTH1_ADDR]]
  // CHECK: [[AUTH1:%.*]] = load i8**, i8*** [[AUTH1_ADDR]]
  // CHECK: [[SIGNED:%.*]] = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 0)
  // CHECK: [[RESULT:%.*]] = inttoptr i64 [[SIGNED]] to i8*
  // CHECK: store i8* [[RESULT]], i8** [[AUTH1]]
}

// CHECK: define void @f5(i8** [[AUTH1:%.*]])
extern "C" void f5(authenticated_null *auth1) {
  *auth1 = &test_global;
  // CHECK: [[AUTH1_ADDR:%.*]] = alloca i8**
  // CHECK: store i8** [[AUTH1]], i8*** [[AUTH1_ADDR]]
  // CHECK: [[AUTH1:%.*]] = load i8**, i8*** [[AUTH1_ADDR]]
  // CHECK: [[SIGNED:%.*]] = call i64 @llvm.ptrauth.sign(i64 ptrtoint (i32* @test_global to i64), i32 2, i64 0)
  // CHECK: [[RESULT:%.*]] = inttoptr i64 [[SIGNED]] to i8*
  // CHECK: store i8* [[RESULT]], i8** [[AUTH1]]
}

// CHECK: define i32 @f6(i8** [[AUTH1:%.*]])
extern "C" int f6(authenticated_null *auth1) {
  return !!*auth1;
  // CHECK: [[AUTH1_ADDR:%.*]] = alloca i8**
  // CHECK: store i8** [[AUTH1]], i8*** [[AUTH1_ADDR]]
  // CHECK: [[AUTH1:%.*]] = load i8**, i8*** [[AUTH1_ADDR]]
  // CHECK: [[AUTH1_V:%.*]] = load i8*, i8** [[AUTH1]]
  // CHECK: [[CAST_AUTH1:%.*]] = ptrtoint i8* [[AUTH1_V]] to i64
  // CHECK: [[AUTHED:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[CAST_AUTH1]], i32 2, i64 0)
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[AUTHED]] to i8*
  // CHECK: [[TOBOOL:%.*]] = icmp ne i8* [[CAST]], null
  // CHECK: [[LNOT:%.*]] = xor i1 [[TOBOOL]], true
  // CHECK: [[LNOT1:%.*]] = xor i1 [[LNOT]], true
  // CHECK: [[LNOT_EXT:%.*]] = zext i1 [[LNOT1]] to i32
  // CHECK: ret i32 [[LNOT_EXT]]
}

// CHECK: define void @f7(i8** [[AUTH1_ARG:%.*]], i8** [[AUTH2_ARG:%.*]])
extern "C" void f7(authenticated_null_addr_disc *auth1, authenticated_null_addr_disc *auth2) {
  *auth1 = *auth2;
  // CHECK: [[AUTH1_ARG]].addr = alloca i8**
  // CHECK: [[AUTH2_ARG]].addr = alloca i8**
  // CHECK: store i8** [[AUTH1_ARG]], i8*** [[AUTH1_ARG]].addr
  // CHECK: store i8** [[AUTH2_ARG]], i8*** [[AUTH2_ARG]].addr
  // CHECK: [[AUTH1_ADDR:%.*]] = load i8**, i8*** [[AUTH1_ARG]].addr
  // CHECK: [[AUTH2_ADDR:%.*]] = load i8**, i8*** [[AUTH2_ARG]].addr
  // CHECK: [[AUTH2_VALUE:%.*]] = load i8*, i8** [[AUTH2_ADDR]]
  // CHECK: [[CAST_AUTH2_ADDR:%.*]] = ptrtoint i8** [[AUTH2_ADDR]] to i64
  // CHECK: [[CAST_AUTH1_ADDR:%.*]] = ptrtoint i8** [[AUTH1_ADDR]] to i64
  // CHECK: [[CAST_AUTH2:%.*]] = ptrtoint i8* [[AUTH2_VALUE]] to i64
  // CHECK: [[RESIGNED:%.*]] = call i64 @llvm.ptrauth.resign(i64 [[CAST_AUTH2]], i32 2, i64 [[CAST_AUTH2_ADDR]], i32 2, i64 [[CAST_AUTH1_ADDR]])
  // CHECK: [[CAST_RESIGNED_VALUE:%.*]] = inttoptr i64 [[RESIGNED]] to i8*
  // CHECK: store i8* [[CAST_RESIGNED_VALUE]], i8** [[AUTH1_ADDR]]
}

struct S0 {
  int i;
  authenticated_null p;
};

extern "C" void f8() {
  struct S0 t = {.i = 1, .p = 0};
  // CHECK: define void @f8()
  // CHECK: [[T:%.*]] = alloca %struct.S0
  // CHECK: [[I:%.*]] = getelementptr inbounds %struct.S0, %struct.S0* [[T]], i32 0, i32 0
  // CHECK: store i32 1, i32* [[I]]
  // CHECK: [[P:%.*]] = getelementptr inbounds %struct.S0, %struct.S0* [[T]], i32 0, i32 1
  // CHECK: [[SIGNED:%.*]] = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 0)
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[SIGNED]] to i8*
  // CHECK: store i8* [[CAST]], i8** [[P]]
}

extern "C" void f9() {
  struct S0 t = {};
  // CHECK: define void @f9()
  // CHECK: [[T:%.*]] = alloca %struct.S0
  // CHECK: [[I:%.*]] = getelementptr inbounds %struct.S0, %struct.S0* [[T]], i32 0, i32 0
  // CHECK: store i32 0, i32* [[I]]
  // CHECK: [[P:%.*]] = getelementptr inbounds %struct.S0, %struct.S0* [[T]], i32 0, i32 1
  // CHECK: [[SIGNED:%.*]] = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 0)
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[SIGNED]] to i8*
  // CHECK: store i8* [[CAST]], i8** [[P]]
}

extern "C" void f10() {
  struct S0 t = {.i = 12};
  // CHECK: define void @f10()
  // CHECK: [[T:%.*]] = alloca %struct.S0
  // CHECK: [[I:%.*]] = getelementptr inbounds %struct.S0, %struct.S0* [[T]], i32 0, i32 0
  // CHECK: store i32 12, i32* [[I]]
  // CHECK: [[P:%.*]] = getelementptr inbounds %struct.S0, %struct.S0* [[T]], i32 0, i32 1
  // CHECK: [[SIGNED:%.*]] = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 0)
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[SIGNED]] to i8*
  // CHECK: store i8* [[CAST]], i8** [[P]]
}

struct S1 {
  authenticated_null p;
  authenticated_null_addr_disc q;
};

extern "C" void f11() {
  struct S1 t = {.p = (void *)1};
  // CHECK-LABEL: define void @f11()
  // CHECK: [[T:%.*]] = alloca %struct.S1
  // CHECK: [[P:%.*]] = getelementptr inbounds %struct.S1, %struct.S1* [[T]], i32 0, i32 0
  // CHECK: [[SIGN:%.*]] = call i64 @llvm.ptrauth.sign(i64 1, i32 2, i64 0)
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[SIGN]] to i8*
  // CHECK: store i8* [[CAST]], i8** [[P]]
  // CHECK: [[Q:%.*]] = getelementptr inbounds %struct.S1, %struct.S1* [[T]], i32 0, i32 1
  // CHECK: [[ADDR_DISC:%.*]] = ptrtoint i8** [[Q]] to i64
  // CHECK: [[SIGN:%.*]] = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 [[ADDR_DISC]])
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[SIGN]] to i8*
  // CHECK: store i8* [[CAST]], i8** [[Q]]
}

extern "C" void f12() {
  struct S1 t = {.p = (void *)1, .q = (void *)0};
  // CHECK-LABEL: define void @f12()
  // CHECK: [[T:%.*]] = alloca %struct.S1
  // CHECK: [[P:%.*]] = getelementptr inbounds %struct.S1, %struct.S1* [[T]], i32 0, i32 0
  // CHECK: [[SIGN:%.*]] = call i64 @llvm.ptrauth.sign(i64 1, i32 2, i64 0)
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[SIGN]] to i8*
  // CHECK: store i8* [[CAST]], i8** [[P]]
  // CHECK: [[Q:%.*]] = getelementptr inbounds %struct.S1, %struct.S1* [[T]], i32 0, i32 1
  // CHECK: [[ADDR_DISC:%.*]] = ptrtoint i8** [[Q]] to i64
  // CHECK: [[SIGN:%.*]] = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 [[ADDR_DISC]])
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[SIGN]] to i8*
  // CHECK: store i8* [[CAST]], i8** [[Q]]
}

extern "C" void f13() {
  struct S1 t = {.q = (void *)1};
  // CHECK: define void @f13()
  // CHECK: [[T:%.*]] = alloca %struct.S1
  // CHECK: [[P:%.*]] = getelementptr inbounds %struct.S1, %struct.S1* [[T]], i32 0, i32 0
  // CHECK: [[SIGN:%.*]] = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 0)
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[SIGN]] to i8*
  // CHECK: store i8* [[CAST]], i8** [[P]]
  // CHECK: [[Q:%.*]] = getelementptr inbounds %struct.S1, %struct.S1* [[T]], i32 0, i32 1
  // CHECK: [[ADDR_DISC:%.*]] = ptrtoint i8** [[Q]] to i64
  // CHECK: [[SIGN:%.*]] = call i64 @llvm.ptrauth.sign(i64 1, i32 2, i64 [[ADDR_DISC]])
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[SIGN]] to i8*
  // CHECK: store i8* [[CAST]], i8** [[Q]]
}

struct S2 {
  int i;
  struct S1 s1;
};

extern "C" void f14() {
  struct S2 t = {};
  // CHECK-LABEL: define void @f14
  // CHECK: [[T:%.*]] = alloca %struct.S2
  // CHECK: [[I:%.*]] = getelementptr inbounds %struct.S2, %struct.S2* [[T]], i32 0, i32 0
  // CHECK: store i32 0, i32* [[I]]
  // CHECK: [[S1:%.*]] = getelementptr inbounds %struct.S2, %struct.S2* [[T]], i32 0, i32 1
  // CHECK: [[P:%.*]] = getelementptr inbounds %struct.S1, %struct.S1* [[S1]], i32 0, i32 0
  // CHECK: [[SIGNED_INT:%.*]] = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 0)
  // CHECK: [[SIGNED:%.*]] = inttoptr i64 [[SIGNED_INT]] to i8*
  // CHECK: store i8* [[SIGNED]], i8** [[P]]
  // CHECK: [[Q:%.*]] = getelementptr inbounds %struct.S1, %struct.S1* [[S1]], i32 0, i32 1
  // CHECK: [[ADDR_DISC:%.*]] = ptrtoint i8** [[Q]] to i64
  // CHECK: [[SIGN:%.*]] = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 [[ADDR_DISC]])
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[SIGN]] to i8*
  // CHECK: store i8* [[CAST]], i8** [[Q]]
}

extern "C" void f15() {
  struct S2 t = {.s1 = {}};
  // CHECK-LABEL: define void @f15
  // CHECK: [[T:%.*]] = alloca %struct.S2
  // CHECK: [[I:%.*]] = getelementptr inbounds %struct.S2, %struct.S2* [[T]], i32 0, i32 0
  // CHECK: store i32 0, i32* [[I]]
  // CHECK: [[S1:%.*]] = getelementptr inbounds %struct.S2, %struct.S2* [[T]], i32 0, i32 1
  // CHECK: [[P:%.*]] = getelementptr inbounds %struct.S1, %struct.S1* [[S1]], i32 0, i32 0
  // CHECK: [[SIGN:%.*]] = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 0)
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[SIGN]] to i8*
  // CHECK: store i8* [[CAST]], i8** [[P]]
  // CHECK: [[Q:%.*]] = getelementptr inbounds %struct.S1, %struct.S1* [[S1]], i32 0, i32 1
  // CHECK: [[ADDR_DISC:%.*]] = ptrtoint i8** [[Q]] to i64
  // CHECK: [[SIGN:%.*]] = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 [[ADDR_DISC]])
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[SIGN]] to i8*
  // CHECK: store i8* [[CAST]], i8** [[Q]]
}

extern "C" void f16() {
  struct S2 t = {.i = 13};
  // CHECK-LABEL: define void @f16
  // CHECK: [[T:%.*]] = alloca %struct.S2
  // CHECK: [[I:%.*]] = getelementptr inbounds %struct.S2, %struct.S2* [[T]], i32 0, i32 0
  // CHECK: store i32 13, i32* [[I]]
  // CHECK: [[S1:%.*]] = getelementptr inbounds %struct.S2, %struct.S2* [[T]], i32 0, i32 1
  // CHECK: [[P:%.*]] = getelementptr inbounds %struct.S1, %struct.S1* [[S1]], i32 0, i32 0
  // CHECK: [[SIGN:%.*]] = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 0)
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[SIGN]] to i8*
  // CHECK: store i8* [[CAST]], i8** [[P]]
  // CHECK: [[Q:%.*]] = getelementptr inbounds %struct.S1, %struct.S1* [[S1]], i32 0, i32 1
  // CHECK: [[ADDR_DISC:%.*]] = ptrtoint i8** [[Q]] to i64
  // CHECK: [[SIGN:%.*]] = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 [[ADDR_DISC]])
  // CHECK: [[CAST:%.*]] = inttoptr i64 [[SIGN]] to i8*
  // CHECK: store i8* [[CAST]], i8** [[Q]]
}

extern "C" void f17(struct S2 a, struct S2 b) {
  a = b;
  // CHECK-LABEL: define void @f17
  // CHECK: %call = call nonnull align 8 dereferenceable(24) %struct.S2* @_ZN2S2aSERKS_(%struct.S2*
}

// CHECK-LABEL: define linkonce_odr nonnull align 8 dereferenceable(24) %struct.S2* @_ZN2S2aSERKS_
// CHECK: %call = call nonnull align 8 dereferenceable(16) %struct.S1* @_ZN2S1aSERKS_(%struct.S1*

struct Subclass : S1 {
  int z;
};

extern "C" void f18() {
  Subclass t = Subclass();
  // CHECK-LABEL: define void @f18
  // CHECK: %t = alloca %struct.Subclass
  // CHECK: %0 = bitcast %struct.Subclass* %t to i8*
  // CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %0, i8* align 8 bitcast (%struct.Subclass* @0 to i8*), i64 24, i1 false)
  // CHECK: %1 = getelementptr inbounds %struct.Subclass, %struct.Subclass* %t, i32 0, i32 0
  // CHECK: %2 = getelementptr inbounds %struct.S1, %struct.S1* %1, i32 0, i32 0
  // CHECK: %3 = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 0)
  // CHECK: %4 = inttoptr i64 %3 to i8*
  // CHECK: store i8* %4, i8** %2
  // CHECK: %5 = getelementptr inbounds %struct.S1, %struct.S1* %1, i32 0, i32 1
  // CHECK: %6 = ptrtoint i8** %5 to i64
  // CHECK: %7 = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 %6)
  // CHECK: %8 = inttoptr i64 %7 to i8*
  // CHECK: store i8* %8, i8** %5
}

extern "C" void f19() {
  Subclass t = {};
  // CHECK-LABEL: define void @f19
  // CHECK: %t = alloca %struct.Subclass
  // CHECK: %0 = bitcast %struct.Subclass* %t to i8*
  // CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %0, i8* align 8 bitcast (%struct.Subclass* @1 to i8*), i64 24, i1 false)
  // CHECK: %1 = getelementptr inbounds %struct.Subclass, %struct.Subclass* %t, i32 0, i32 0
  // CHECK: %2 = getelementptr inbounds %struct.S1, %struct.S1* %1, i32 0, i32 0
  // CHECK: %3 = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 0)
  // CHECK: %4 = inttoptr i64 %3 to i8*
  // CHECK: store i8* %4, i8** %2
  // CHECK: %5 = getelementptr inbounds %struct.S1, %struct.S1* %1, i32 0, i32 1
  // CHECK: %6 = ptrtoint i8** %5 to i64
  // CHECK: %7 = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 %6)
  // CHECK: %8 = inttoptr i64 %7 to i8*
  // CHECK: store i8* %8, i8** %5
}

extern "C" void f21(Subclass *s1, Subclass *s2) {
  *s1 = *s2;
  // CHECK-LABEL: define void @f21
  // CHECK: %call = call nonnull align 8 dereferenceable(20) %struct.Subclass* @_ZN8SubclassaSERKS_
}

struct S3 {
  int *__ptrauth(2, 0, 0, "authenticates-null-values") f0;
};

extern "C" void f22() {
  struct S3 s;
  // CHECK-LABEL: define void @f22()
  // CHECK: [[S:%.*]] = alloca %struct.S3
  // CHECK: ret void
}

struct S4 : virtual S3 {
  authenticated_null_addr_disc new_field;
};

extern "C" void f23() {
  struct S4 s = {};
  // CHECK-LABEL: define void @f23()
  // CHECK: %s = alloca %struct.S4
  // CHECK: %0 = bitcast %struct.S4* %s to i8*
  // CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %0, i8* align 8 bitcast (%struct.S4* @2 to i8*), i64 24, i1 false)
  // CHECK: %1 = getelementptr inbounds %struct.S4, %struct.S4* %s, i32 0, i32 2
  // CHECK: %2 = getelementptr inbounds %struct.S3, %struct.S3* %1, i32 0, i32 0
  // CHECK: %3 = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 0)
  // CHECK: %4 = inttoptr i64 %3 to i32*
  // CHECK: store i32* %4, i32** %2
  // CHECK: %5 = getelementptr inbounds %struct.S4, %struct.S4* %s, i32 0, i32 1
  // CHECK: %6 = ptrtoint i8** %5 to i64
  // CHECK: %7 = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 %6)
  // CHECK: %8 = inttoptr i64 %7 to i8*
  // CHECK: store i8* %8, i8** %5
  // CHECK: %call = call %struct.S4* @_ZN2S4C1Ev
}

struct S5 : S1 {
  S5() : S1() {}
};

extern "C" void f24() {
  struct S5 s = {};
}

struct S6 {
  int i;
  authenticated_null p;
  S6(){};
};

extern "C" void f25() {
  struct S6 s = {};
}

struct S7 {
  void* __ptrauth(1,1,1) field1;
  void* __ptrauth(1,1,1) field2;
};

extern "C" void f26() {
  int i = 0;
  struct S7 s = { .field2 = &i};
  // CHECK: %field1 = getelementptr inbounds %struct.S7, %struct.S7* %s, i32 0, i32 0
  // CHECK: store i8* null, i8** %field1
  // CHECK: %field2 = getelementptr inbounds %struct.S7, %struct.S7* %s, i32 0, i32 1
  // CHECK: %0 = bitcast i32* %i to i8*
  // CHECK: %1 = ptrtoint i8** %field2 to i64
  // CHECK: %2 = call i64 @llvm.ptrauth.blend(i64 %1, i64 1)
  // CHECK: %3 = ptrtoint i8* %0 to i64
  // CHECK: %4 = call i64 @llvm.ptrauth.sign(i64 %3, i32 1, i64 %2)
  // CHECK: %5 = inttoptr i64 %4 to i8*
  // CHECK: store i8* %5, i8** %field2
}

struct AStruct;
const AStruct &foo();
class AClass { 
  public: 
  AClass() {} 
  private: 
  virtual void f(); 
};
void AClass::f() {
  const struct {
    unsigned long a;
    const AStruct &b;
    unsigned long c;
    unsigned long d;
    unsigned long e;
  } test []= {{ 0, foo(), 0, 0, 0 }};

}
AClass global;


// struct S1 copy constructor
// CHECK-LABEL: define linkonce_odr nonnull align 8 dereferenceable(16) %struct.S1* @_ZN2S1aSERKS_
// CHECK: %this.addr = alloca %struct.S1*
// CHECK: [[ADDR:%.*]] = alloca %struct.S1*
// CHECK: store %struct.S1* %this, %struct.S1** %this.addr
// CHECK: store %struct.S1* %0, %struct.S1** [[ADDR]]
// CHECK: %this1 = load %struct.S1*, %struct.S1** %this.addr
// CHECK: %p = getelementptr inbounds %struct.S1, %struct.S1* %this1, i32 0, i32 0
// CHECK: [[S1PTR:%.*]] = load %struct.S1*, %struct.S1** [[ADDR]]
// CHECK: %p2 = getelementptr inbounds %struct.S1, %struct.S1* [[S1PTR]], i32 0, i32 0
// CHECK: [[P2:%.*]] = load i8*, i8** %p2
// CHECK: store i8* [[P2]], i8** %p
// CHECK: %q = getelementptr inbounds %struct.S1, %struct.S1* %this1, i32 0, i32 1
// CHECK: [[S1PTR:%.*]] = load %struct.S1*, %struct.S1** [[ADDR]]
// CHECK: %q3 = getelementptr inbounds %struct.S1, %struct.S1* [[S1PTR]], i32 0, i32 1
// CHECK: [[Q3_ADDR:%.*]] = load i8*, i8** %q3
// CHECK: [[Q3:%.*]] = ptrtoint i8** %q3 to i64
// CHECK: [[Q:%.*]] = ptrtoint i8** %q to i64
// CHECK: [[DISC:%.*]] = ptrtoint i8* [[Q3_ADDR]] to i64
// CHECK: [[RESIGNED:%.*]] = call i64 @llvm.ptrauth.resign(i64 [[DISC]], i32 2, i64 [[Q3]], i32 2, i64 [[Q]])
// CHECK: [[CAST:%.*]] = inttoptr i64 [[RESIGNED]] to i8*
// CHECK: store i8* [[CAST]], i8** %q

// CHECK-LABEL: define linkonce_odr %struct.S5* @_ZN2S5C2Ev(%struct.S5* nonnull
// CHECK: %this.addr = alloca %struct.S5*
// CHECK: store %struct.S5* %this, %struct.S5** %this.addr
// CHECK: %this1 = load %struct.S5*, %struct.S5** %this.addr
// CHECK: %0 = bitcast %struct.S5* %this1 to %struct.S1*
// CHECK: %1 = bitcast %struct.S1* %0 to i8*
// CHECK: %2 = getelementptr inbounds i8, i8* %1, i64 0
// CHECK: call void @llvm.memset.p0i8.i64(i8* align 8 %2, i8 0, i64 16, i1 false)
// CHECK: %3 = getelementptr inbounds %struct.S1, %struct.S1* %0, i32 0, i32 0
// CHECK: %4 = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 0)
// CHECK: %5 = inttoptr i64 %4 to i8*
// CHECK: store i8* %5, i8** %3
// CHECK: %6 = getelementptr inbounds %struct.S1, %struct.S1* %0, i32 0, i32 1
// CHECK: %7 = ptrtoint i8** %6 to i64
// CHECK: %8 = call i64 @llvm.ptrauth.sign(i64 0, i32 2, i64 %7)
// CHECK: %9 = inttoptr i64 %8 to i8*
// CHECK: store i8* %9, i8** %6

// CHECK-LABEL: define linkonce_odr %struct.S6* @_ZN2S6C2Ev
// CHECK: %this.addr = alloca %struct.S6*
// CHECK: store %struct.S6* %this, %struct.S6** %this.addr
// CHECK: %this1 = load %struct.S6*, %struct.S6** %this.addr
// CHECK: ret %struct.S6* %this1
