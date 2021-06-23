// RUN: %clang_cc1 -triple arm64-apple-ios -fptrauth-calls -fptrauth-intrinsics -emit-llvm %s -o - | FileCheck %s
#if __has_feature(ptrauth_qualifier_authentication_mode)

typedef void *NonePointer;
typedef void *__ptrauth(1, 1, 101, "strip") StripPointer;
typedef void *__ptrauth(1, 1, 102, "sign-and-strip") SignAndStripPointer;
typedef void *__ptrauth(1, 1, 103, "sign-and-auth") SignAndAuthPointer;
typedef __UINT64_TYPE__ NoneIntptr;
typedef __UINT64_TYPE__ __ptrauth_restricted_intptr(1, 0, 105, "strip") StripIntptr;
typedef __UINT64_TYPE__ __ptrauth_restricted_intptr(1, 0, 106, "sign-and-strip") SignAndStripIntptr;
typedef __UINT64_TYPE__ __ptrauth_restricted_intptr(1, 0, 107, "sign-and-auth") SignAndAuthIntptr;

NonePointer globalNonePointer = "foo0";
StripPointer globalStripPointer = "foo1";
SignAndStripPointer globalSignAndStripPointer = "foo2";
SignAndAuthPointer globalSignAndAuthPointer = "foo3";
NoneIntptr globalNoneIntptr = (__UINT64_TYPE__)&globalNonePointer;
StripIntptr globalStripIntptr = (__UINT64_TYPE__)&globalStripPointer;
SignAndStripIntptr globalSignAndStripIntptr = (__UINT64_TYPE__)&globalSignAndStripPointer;
SignAndAuthIntptr globalSignAndAuthIntptr = (__UINT64_TYPE__)&globalSignAndAuthPointer;

// CHECK: @.str = private unnamed_addr constant [5 x i8] c"foo0\00", align 1
// CHECK: @globalNonePointer = {{.*}} global i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str, i32 0, i32 0), align 8
// CHECK: @.str.1 = private unnamed_addr constant [5 x i8] c"foo1\00", align 1
// CHECK: @globalStripPointer = {{.*}} global i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str.1, i32 0, i32 0), align 8
// CHECK: @.str.2 = private unnamed_addr constant [5 x i8] c"foo2\00", align 1
// CHECK: @.str.2.ptrauth = private constant { i8*, i32, i64, i64 } { i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str.2, i32 0, i32 0), i32 1, i64 ptrtoint (i8** @globalSignAndStripPointer to i64), i64 102 }, section "llvm.ptrauth", align 8
// CHECK: @globalSignAndStripPointer = {{.*}} global i8* bitcast ({ i8*, i32, i64, i64 }* @.str.2.ptrauth to i8*), align 8
// CHECK: @.str.3 = private unnamed_addr constant [5 x i8] c"foo3\00", align 1
// CHECK: @.str.3.ptrauth = private constant { i8*, i32, i64, i64 } { i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str.3, i32 0, i32 0), i32 1, i64 ptrtoint (i8** @globalSignAndAuthPointer to i64), i64 103 }, section "llvm.ptrauth", align 8
// CHECK: @globalSignAndAuthPointer = {{.*}} global i8* bitcast ({ i8*, i32, i64, i64 }* @.str.3.ptrauth to i8*), align 8
// CHECK: @globalNoneIntptr = {{.*}} global i64 ptrtoint (i8** @globalNonePointer to i64), align 8
// CHECK: @globalStripPointer.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (i8** @globalStripPointer to i8*), i32 1, i64 0, i64 105 }, section "llvm.ptrauth", align 8
// CHECK: @globalStripIntptr = {{.*}} global i64 ptrtoint ({ i8*, i32, i64, i64 }* @globalStripPointer.ptrauth to i64), align 8
// CHECK: @globalSignAndStripPointer.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (i8** @globalSignAndStripPointer to i8*), i32 1, i64 0, i64 106 }, section "llvm.ptrauth", align 8
// CHECK: @globalSignAndStripIntptr = {{.*}} global i64 ptrtoint ({ i8*, i32, i64, i64 }* @globalSignAndStripPointer.ptrauth to i64), align 8
// CHECK: @globalSignAndAuthPointer.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (i8** @globalSignAndAuthPointer to i8*), i32 1, i64 0, i64 107 }, section "llvm.ptrauth", align 8
// CHECK: @globalSignAndAuthIntptr = {{.*}} global i64 ptrtoint ({ i8*, i32, i64, i64 }* @globalSignAndAuthPointer.ptrauth to i64), align 8

typedef struct {
  NonePointer ptr;
  NoneIntptr i;
} NoneStruct;
typedef struct {
  StripPointer ptr;
  StripIntptr i;
} StripStruct;
typedef struct {
  SignAndStripPointer ptr;
  SignAndStripIntptr i;
} SignAndStripStruct;
typedef struct {
  SignAndAuthPointer ptr;
  SignAndAuthIntptr i;
} SignAndAuthStruct;

// CHECK-LABEL: define {{.*}} [2 x i64] @testNone
NoneStruct testNone(NoneStruct *a, NoneStruct *b, NoneStruct c) {
  globalNonePointer += 1;
  // CHECK: [[GLOBALP:%.*]] = load i8*, i8** @globalNonePointer
  // CHECK: [[GLOBALPP:%.*]] = getelementptr i8, i8* [[GLOBALP]], i64 1
  // CHECK: store i8* [[GLOBALPP]], i8** @globalNonePointer
  globalNoneIntptr += 1;
  // CHECK: [[GLOBALI:%.*]] = load i64, i64* @globalNoneIntptr
  // CHECK: [[GLOBALIP:%.*]] = add i64 [[GLOBALI]], 1
  // CHECK: store i64 [[GLOBALIP]], i64* @globalNoneIntptr
  a->ptr += 1;
  // CHECK: [[PTR:%.*]] = load %struct.NoneStruct*, %struct.NoneStruct** %a.addr, align 8
  // CHECK: [[PTR_PTR:%.*]] = getelementptr inbounds %struct.NoneStruct, %struct.NoneStruct* [[PTR]], i32 0, i32 0
  // CHECK: [[PTR:%.*]] = load i8*, i8** [[PTR_PTR]], align 8
  // CHECK: [[AP:%.*]] = getelementptr i8, i8* [[PTR]], i64 1
  // CHECK: store i8* [[AP]], i8** [[PTR_PTR]], align 8
  a->i += 1;
  // CHECK: [[PTR:%.*]] = load %struct.NoneStruct*, %struct.NoneStruct** %a.addr, align 8
  // CHECK: [[I_PTR:%.*]] = getelementptr inbounds %struct.NoneStruct, %struct.NoneStruct* [[PTR]], i32 0, i32 1
  // CHECK: [[I:%.*]] = load i64, i64* [[I_PTR]], align 8
  // CHECK: [[IP:%.*]] = add i64 [[I]], 1
  // CHECK: store i64 [[IP]], i64* [[I_PTR]], align 8
  *b = *a;
  // CHECK: [[B_ADDR:%.*]] = load %struct.NoneStruct*, %struct.NoneStruct** %b.addr, align 8
  // CHECK: [[A_ADDR:%.*]] = load %struct.NoneStruct*, %struct.NoneStruct** %a.addr, align 8
  // CHECK: [[B_I8:%.*]] = bitcast %struct.NoneStruct* [[B_ADDR]] to i8*
  // CHECK: [[A_I8:%.*]] = bitcast %struct.NoneStruct* [[A_ADDR]] to i8*
  // CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 [[B_I8]], i8* align 8 [[A_I8]], i64 16, i1 false)
  return c;
}

// CHECK: define {{.*}} void @testStrip
StripStruct testStrip(StripStruct *a, StripStruct *b, StripStruct c) {
  globalStripPointer += 1;
  a->ptr += 1;
  a->i += 1;
  *b = *a;
  return c;
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
}

// CHECK: define {{.*}} void @testSignAndStrip
SignAndStripStruct testSignAndStrip(SignAndStripStruct *a, SignAndStripStruct *b, SignAndStripStruct c) {
  globalSignAndStripPointer += 1;
  a->ptr += 1;
  a->i += 1;
  *b = *a;
  return c;
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK: [[BRANCH:%.*]] = icmp ne i8* [[TMP:%.*]], null
  // CHECK: br i1 [[BRANCH]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK: [[END_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK: [[BRANCH:%.*]] = icmp ne i8* [[TMP:%.*]], null
  // CHECK: br i1 [[BRANCH]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
  // CHECK: [[END_TARGET]]:
  // CHECK: [[BRANCH:%.*]] = icmp ne i8* [[TMP:%.*]], null
  // CHECK: br i1 [[BRANCH]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK: [[END_TARGET]]:
  // CHECK: [[BRANCH:%.*]] = icmp ne i8* [[TMP:%.*]], null
  // CHECK: br i1 [[BRANCH]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
  // CHECK: [[END_TARGET]]:
  // CHECK: br i1 [[TMP:%.*]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK: [[END_TARGET]]:
  // CHECK: br i1 [[TMP:%.*]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
  // CHECK: [[END_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK: br i1 [[TMP:%.*]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
  // CHECK: [[END_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK: br i1 [[TMP:%.*]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
  // CHECK: [[END_TARGET]]:
}

// CHECK: define {{.*}} void @testSignAndAuth
SignAndAuthStruct testSignAndAuth(SignAndAuthStruct *a, SignAndAuthStruct *b, SignAndAuthStruct c) {
  globalSignAndAuthPointer += 1;
  a->ptr += 1;
  a->i += 1;
  *b = *a;
  return c;
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK: [[BRANCH:%.*]] = icmp ne i8* [[TMP:%.*]], null
  // CHECK: br i1 [[BRANCH]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.auth
  // CHECK: [[END_TARGET]]:
  // CHECK: [[BRANCH:%.*]] = icmp ne i8* [[TMP:%.*]], null
  // CHECK: br i1 [[BRANCH]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
  // CHECK: [[END_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK: [[BRANCH:%.*]] = icmp ne i8* [[TMP:%.*]], null
  // CHECK: br i1 [[BRANCH]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.auth
  // CHECK: [[END_TARGET]]:
  // CHECK: [[BRANCH:%.*]] = icmp ne i8* [[TMP:%.*]], null
  // CHECK: br i1 [[BRANCH]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
  // CHECK: [[END_TARGET]]:
  // CHECK: br i1 [[TMP:%.*]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.auth
  // CHECK: br i1 [[TMP:%.*]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
  // CHECK: [[END_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK: br i1 [[TMP:%.*]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.resign
  // CHECK: [[END_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK: br i1 [[TMP:%.*]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.resign
}

void barrier(StripStruct *a, SignAndStripStruct *b, SignAndAuthStruct *c);

// CHECK: define {{.*}} void @testCoercions
void testCoercions(StripStruct *a, SignAndStripStruct *b, SignAndAuthStruct *c) {
  a->ptr = b->ptr;
  barrier(a, b, c);
  c->ptr = a->ptr;
  barrier(a, b, c);
  b->ptr = c->ptr;
  a->i = b->i;
  barrier(a, b, c);
  c->i = a->i;
  barrier(a, b, c);
  b->i = c->i;
  // CHECK: [[BRANCH:%.*]] = icmp ne i8* [[TMP:%.*]], null
  // CHECK: br i1 [[BRANCH]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK: [[BRANCH:%.*]] = icmp ne i8* [[TMP:%.*]], null
  // CHECK: br i1 [[BRANCH]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
  // CHECK: [[END_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK: [[BRANCH:%.*]] = icmp ne i8* [[TMP:%.*]], null
  // CHECK: br i1 [[BRANCH]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.auth
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
  // CHECK: [[END_TARGET]]:
  // CHECK: br i1 [[TMP:%.*]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK: [[END_TARGET]]:
  // CHECK: br i1 [[TMP:%.*]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
  // CHECK: [[END_TARGET]]:
  // CHECK: br i1 [[TMP:%.*]], label %[[NON_NULL_TARGET:.*]], label %[[END_TARGET:.*]]
  // CHECK: [[NON_NULL_TARGET]]:
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.auth
  // CHECK: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
  // CHECK: [[END_TARGET]]:
}

#endif
