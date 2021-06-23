// RUN: %clang_cc1 -triple arm64-apple-ios -fptrauth-calls -fptrauth-returns -fptrauth-intrinsics -fobjc-arc -fobjc-runtime-has-weak -fblocks -emit-llvm -o - %s | FileCheck %s

#define ATTR0 __attribute__((ptrauth_struct(1, 100)))
#define ATTR1 __attribute__((ptrauth_struct(1, 101)))
#define ATTR2 __attribute__((ptrauth_struct(1, 102)))
#define ATTR3 __attribute__((ptrauth_struct(1, 103)))
#define ATTR4 __attribute__((ptrauth_struct(1, 104)))

// CHECK: %[[STRUCT_S0:.*]] = type { i32, i32 }
// CHECK: %[[STRUCT_S1:.*]] = type { i32, i8*, %[[STRUCT_S0]]*, %[[STRUCT_S0]] }
// CHECK: %[[STRUCT_S2:.*]] = type opaque
// CHECK: %[[STRUCT_S4:.*]] = type { i32, i32, i8*, <4 x float> }
// CHECK: %[[STRUCT_BLOCK_DESCRIPTOR:.*]] = type { i64, i64 }
// CHECK: %[[STRUCT_S3:.*]] = type { i32, %[[STRUCT_S0]] }

// CHECK: @g0 = {{.*}} global %[[STRUCT_S0]] zeroinitializer, align 4
// CHECK: @[[G0_PTRAUTH:.*]] = private constant { i8*, i32, i64, i64 } { i8* bitcast (%[[STRUCT_S0]]* @g0 to i8*), i32 1, i64 0, i64 100 }, section "llvm.ptrauth", align 8
// CHECK: @gp0 = {{.*}} global %[[STRUCT_S0]]* bitcast ({ i8*, i32, i64, i64 }* @[[G0_PTRAUTH]] to %[[STRUCT_S0]]*), align 8
// CHECK: @[[PTRAUTH:.*]] = private constant { i8*, i32, i64, i64 } { i8* getelementptr (i8, i8* bitcast (%[[STRUCT_S1]]* @g1 to i8*), i64 24), i32 1, i64 0, i64 100 }, section "llvm.ptrauth", align 8
// CHECK: @gp1 = {{.*}} global %[[STRUCT_S0]]* bitcast ({ i8*, i32, i64, i64 }* @[[PTRAUTH]] to %[[STRUCT_S0]]*), align 8
// CHECK: @ga2 = {{.*}} global [4 x i32] zeroinitializer, align 4
// CHECK: @gf0 = {{.*}} global void ()* bitcast ([4 x i32]* @ga2 to void ()*), align 8
// CHECK: @[[CONST_TEST_COMPOUND_LITERAL0_T0:.*]] = private unnamed_addr constant %[[STRUCT_S0]] { i32 1, i32 2 }, align 4

typedef long intptr_t;

struct ATTR0 S0 {
  int a0, a1;
};

typedef struct S0 S0;

struct ATTR1 S1 {
  int a;
  id b;
  S0 *f0;
  S0 f1;
};

typedef struct S1 S1;

struct ATTR2 S2;
typedef struct S2 S2;

struct ATTR3 S3 {
  int f0;
  __attribute__((annotate("abc"))) S0 f1;
};

typedef struct S3 S3;

typedef __attribute__((ext_vector_type(4))) float float4;

struct ATTR4 S4 {
  int f0, f1;
  __weak id f2;
  float4 extv0;
};

typedef struct S4 S4;

S0 getS0(void);
S0 *func0(S0 *);
S0 func1(S0);
S4 func2(S4);

S0 g0;
S0 *gp0 = &g0;
S1 g1;
S0 *gp1 = &g1.f1;

S0 ga0[10][10][10];
S0 ga1[2];

int ga2[4] = {0};
void (*gf0)(void) = (void (*)(void))ga2;

// CHECK-LABEL: define {{.*}} void @test_member_access0(
// CHECK-NOT: @llvm.ptrauth
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.auth(i64 %{{.*}}, i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: %[[V4:.*]] = bitcast %[[STRUCT_S0]]* %[[V3]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V4]], i64 4
// CHECK: %[[V5:.*]] = bitcast i8* %[[RESIGNEDGEP]] to i32*
// CHECK: load i32, i32* %[[V5]], align 4

void test_member_access0(S0 *s) {
  int t = s->a1;
}

// CHECK-LABEL: define {{.*}} void @test_member_access1(
// CHECK-NOT: @llvm.ptrauth
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.auth(i64 %{{.*}}, i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: %[[V4:.*]] = bitcast %[[STRUCT_S0]]* %[[V3]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V4]], i64 4
// CHECK: %[[V5:.*]] = bitcast i8* %[[RESIGNEDGEP]] to i32*
// CHECK: load i32, i32* %[[V5]], align 4

void test_member_access1(S0 *s) {
  int t = (*s).a1;
}

// CHECK-LABEL: define {{.*}} void @test_member_access2(
// CHECK-NOT: @llvm.ptrauth

void test_member_access2(S0 s) {
  int t = s.a1;
}

// CHECK-LABEL: define {{.*}} void @test_member_initialization(
// CHECK: %[[T:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[T1:.*]] = alloca %[[STRUCT_S1]], align 8
// CHECK: %[[S0:.*]] = getelementptr inbounds %[[STRUCT_S1]], %[[STRUCT_S1]]* %[[T1]], i32 0, i32 2
// CHECK: %[[V0:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T]] to i64
// CHECK: %[[V1:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V0]], i32 1, i64 100)
// CHECK: %[[V2:.*]] = inttoptr i64 %[[V1]] to %[[STRUCT_S0]]*
// CHECK: store %[[STRUCT_S0]]* %[[V2]], %[[STRUCT_S0]]** %[[S0]], align 8

void test_member_initialization() {
  S0 t;
  S1 t1 = {1, 0, &t};
}

// CHECK-LABEL: define {{.*}} i32 @test_array0(
// CHECK: %[[A_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[A_ADDR]], align 8
// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V0]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V1]], i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: %[[V4:.*]] = bitcast %[[STRUCT_S0]]* %[[V3]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V4]], i64 84
// CHECK: %[[V5:.*]] = bitcast i8* %[[RESIGNEDGEP]] to i32*
// CHECK: load i32, i32* %[[V5]], align 4

int test_array0(S0 *a) {
  return a[10].a1;
}

// CHECK-LABEL: define {{.*}} i32 @test_array1(
// CHECK: %[[V1:.*]] = load i32, i32* %{{.*}}, align 4
// CHECK: %[[ADD:.*]] = add nsw i32 %[[V1]], 2
// CHECK: %[[IDXPROM:.*]] = sext i32 %[[ADD]] to i64
// CHECK: %[[ARRAYIDX_IDX:.*]] = mul nsw i64 %[[IDXPROM]], 8
// CHECK: %[[ADD:.*]] = add i64 %[[ARRAYIDX_IDX]], 4
// CHECK: %[[V2:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V0]] to i64
// CHECK: %[[V3:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V2]], i32 1, i64 100)
// CHECK: %[[V4:.*]] = inttoptr i64 %[[V3]] to %[[STRUCT_S0]]*
// CHECK: %[[V5:.*]] = bitcast %[[STRUCT_S0]]* %[[V4]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V5]], i64 %[[ADD]]
// CHECK: %[[V6:.*]] = bitcast i8* %[[RESIGNEDGEP]] to i32*
// CHECK: load i32, i32* %[[V6]], align 4

int test_array1(S0 *a, int i) {
  return a[i + 2].a1;
}

// CHECK-LABEL: define {{.*}} i32 @test_array2(
// CHECK: %[[V0:.*]] = load i32, i32* %{{.*}}, align 4
// CHECK: %[[IDXPROM:.*]] = sext i32 %[[V0]] to i64
// CHECK: %[[ARRAYIDX:.*]] = getelementptr inbounds [10 x [10 x [10 x %[[STRUCT_S0]]]]], [10 x [10 x [10 x %[[STRUCT_S0]]]]]* @ga0, i64 0, i64 %[[IDXPROM]]
// CHECK: %[[V1:.*]] = load i32, i32* %{{.*}}, align 4
// CHECK: %[[IDXPROM1:.*]] = sext i32 %[[V1]] to i64
// CHECK: %[[ARRAYIDX2:.*]] = getelementptr inbounds [10 x [10 x %[[STRUCT_S0]]]], [10 x [10 x %[[STRUCT_S0]]]]* %[[ARRAYIDX]], i64 0, i64 %[[IDXPROM1]]
// CHECK: %[[V2:.*]] = load i32, i32* %{{.*}}, align 4
// CHECK: %[[IDXPROM3:.*]] = sext i32 %[[V2]] to i64
// CHECK: %[[ARRAYIDX4:.*]] = getelementptr inbounds [10 x %[[STRUCT_S0]]], [10 x %[[STRUCT_S0]]]* %[[ARRAYIDX2]], i64 0, i64 %[[IDXPROM3]]
// CHECK: %[[A1:.*]] = getelementptr inbounds %[[STRUCT_S0]], %[[STRUCT_S0]]* %[[ARRAYIDX4]], i32 0, i32 1
// CHECK: load i32, i32* %[[A1]], align 4

int test_array2(int i, int j, int k) {
  return ga0[i][j][k].a1;
}

// CHECK: define {{.*}} %[[STRUCT_S0]]* @test_array3(
// CHECK: %[[V0:.*]] = call i64 @llvm.ptrauth.sign(i64 ptrtoint ([2 x %[[STRUCT_S0]]]* @ga1 to i64), i32 1, i64 100)
// CHECK: %[[V1:.*]] = inttoptr i64 %[[V0]] to %[[STRUCT_S0]]*
// CHECK: ret %[[STRUCT_S0]]* %[[V1]]

S0 *test_array3(void) {
  return ga1;
}

// CHECK-LABEL: define {{.*}} i32 @test_pointer_arithmetic0(
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.auth(i64 %{{.*}}, i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: %[[ADD_PTR:.*]] = getelementptr inbounds %[[STRUCT_S0]], %[[STRUCT_S0]]* %[[V3]], i64 10
// CHECK: %[[V4:.*]] = ptrtoint %[[STRUCT_S0]]* %[[ADD_PTR]] to i64
// CHECK: %[[V5:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V4]], i32 1, i64 100)
// CHECK: %[[V6:.*]] = inttoptr i64 %[[V5]] to %[[STRUCT_S0]]*
// CHECK: %[[V7:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V6]] to i64
// CHECK: %[[V8:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V7]], i32 1, i64 100)
// CHECK: %[[V9:.*]] = inttoptr i64 %[[V8]] to %[[STRUCT_S0]]*
// CHECK: %[[V10:.*]] = bitcast %[[STRUCT_S0]]* %[[V9]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V10]], i64 4
// CHECK: %[[V11:.*]] = bitcast i8* %[[RESIGNEDGEP]] to i32*
// CHECK: %[[V12:.*]] = load i32, i32* %[[V11]], align 4

int test_pointer_arithmetic0(S0 *a) {
  return (a + 10)->a1;
}

// CHECK: define {{.*}} %[[STRUCT_S0]]* @test_pointer_arithmetic1(
// CHECK: %[[A_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[A_ADDR]], align 8
// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V0]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V1]], i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: %[[INCDEC_PTR:.*]] = getelementptr inbounds %[[STRUCT_S0]], %[[STRUCT_S0]]* %[[V3]], i32 1
// CHECK: %[[V4:.*]] = ptrtoint %[[STRUCT_S0]]* %[[INCDEC_PTR]] to i64
// CHECK: %[[V5:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V4]], i32 1, i64 100)
// CHECK: %[[V6:.*]] = inttoptr i64 %[[V5]] to %[[STRUCT_S0]]*
// CHECK: store %[[STRUCT_S0]]* %[[V6]], %[[STRUCT_S0]]** %[[A_ADDR]], align 8
// CHECK: ret %[[STRUCT_S0]]* %[[V6]]

S0 *test_pointer_arithmetic1(S0 *a) {
  return ++a;
}

// CHECK: define {{.*}} %[[STRUCT_S0]]* @test_pointer_arithmetic2(
// CHECK: %[[A_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[A_ADDR]], align 8
// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V0]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V1]], i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: %[[INCDEC_PTR:.*]] = getelementptr inbounds %[[STRUCT_S0]], %[[STRUCT_S0]]* %[[V3]], i32 1
// CHECK: %[[V4:.*]] = ptrtoint %[[STRUCT_S0]]* %[[INCDEC_PTR]] to i64
// CHECK: %[[V5:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V4]], i32 1, i64 100)
// CHECK: %[[V6:.*]] = inttoptr i64 %[[V5]] to %[[STRUCT_S0]]*
// CHECK: store %[[STRUCT_S0]]* %[[V6]], %[[STRUCT_S0]]** %[[A_ADDR]], align 8
// CHECK: ret %[[STRUCT_S0]]* %[[V0]]

S0 *test_pointer_arithmetic2(S0 *a) {
  return a++;
}

// CHECK-LABEL: define {{.*}} void @test_dereference0(
// CHECK: %[[A0_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[A1_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[A0_ADDR]], align 8
// CHECK: %[[V1:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[A1_ADDR]], align 8
// CHECK: %[[V2:.*]] = bitcast %[[STRUCT_S0]]* %[[V0]] to i8*
// CHECK: %[[V3:.*]] = bitcast %[[STRUCT_S0]]* %[[V1]] to i8*
// CHECK: %[[V4:.*]] = ptrtoint i8* %[[V2]] to i64
// CHECK: %[[V5:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V4]], i32 1, i64 100)
// CHECK: %[[V6:.*]] = inttoptr i64 %[[V5]] to i8*
// CHECK: %[[V7:.*]] = ptrtoint i8* %[[V3]] to i64
// CHECK: %[[V8:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V7]], i32 1, i64 100)
// CHECK: %[[V9:.*]] = inttoptr i64 %[[V8]] to i8*
// CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %[[V6]], i8* align 4 %[[V9]], i64 8, i1 false)

void test_dereference0(S0 *a0, S0 *a1) {
  *a0 = *a1;
}

// CHECK-LABEL: define {{.*}} void @test_dereference1(
// CHECK: %[[S_ADDR:.*]] = alloca %[[STRUCT_S1]]*, align 8
// CHECK: %[[T:.*]] = alloca %[[STRUCT_S1]], align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S1]]*, %[[STRUCT_S1]]** %[[S_ADDR]], align 8
// CHECK: %[[V1:.*]] = bitcast %[[STRUCT_S1]]* %[[T]] to i8**
// CHECK: %[[V2:.*]] = bitcast %[[STRUCT_S1]]* %[[V0]] to i8**
// CHECK: %[[V3:.*]] = ptrtoint i8** %[[V2]] to i64
// CHECK: %[[V4:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V3]], i32 1, i64 101)
// CHECK: %[[V5:.*]] = inttoptr i64 %[[V4]] to i8**
// CHECK: call void @__copy_constructor_8_8_t0w4_s8_t16w16(i8** %[[V1]], i8** %[[V5]])

void test_dereference1(S1 *s) {
  S1 t = *s;
}

// CHECK-LABEL: define {{.*}} void @test_address_of0(
// CHECK: %[[T:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[V0:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V0]], i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: store %[[STRUCT_S0]]* %[[V3]], %[[STRUCT_S0]]** %{{.*}}, align 8

void test_address_of0(void) {
  S0 t = getS0();
  S0 *p = &t;
}

// CHECK-LABEL: define {{.*}} void @test_address_of1(
// CHECK: %[[V0:.*]] = call i64 @llvm.ptrauth.sign(i64 ptrtoint (%[[STRUCT_S0]]* @g0 to i64), i32 1, i64 100)
// CHECK: %[[V1:.*]] = inttoptr i64 %[[V0]] to %[[STRUCT_S0]]*
// CHECK: store %[[STRUCT_S0]]* %[[V1]], %[[STRUCT_S0]]** %{{.*}}, align 8

void test_address_of1(void) {
  S0 *p = &g0;
}

// CHECK-LABEL: define {{.*}} void @test_address_of2(
// CHECK: %[[T:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[T1:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[CALL:.*]] = call i64 @getS0()
// CHECK: %[[V0:.*]] = bitcast %[[STRUCT_S0]]* %[[T]] to i64*
// CHECK: store i64 %[[CALL]], i64* %[[V0]], align 4
// CHECK: %[[V1:.*]] = bitcast %[[STRUCT_S0]]* %[[T1]] to i8*
// CHECK: %[[V2:.*]] = bitcast %[[STRUCT_S0]]* %[[T]] to i8*
// CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %[[V1]], i8* align 4 %[[V2]], i64 8, i1 false)

void test_address_of2(void) {
  S0 t = getS0();
  S0 t1 = *&t;
}

// CHECK-LABEL: define {{.*}} void @test_conversion0(
// CHECK: %[[P_ADDR:.*]] = alloca i8*, align 8
// CHECK: %[[T:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[T2:.*]] = alloca i8*, align 8
// CHECK: %[[V0:.*]] = load i8*, i8** %[[P_ADDR]], align 8
// CHECK: %[[V1:.*]] = bitcast i8* %[[V0]] to %[[STRUCT_S0]]*

// CHECK: %[[V3:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V1]] to i64
// CHECK: %[[V4:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V3]], i32 1, i64 100)
// CHECK: %[[V5:.*]] = inttoptr i64 %[[V4]] to %[[STRUCT_S0]]*

// CHECK: %[[V6:.*]] = phi %[[STRUCT_S0]]* [ null, %{{.*}} ], [ %[[V5]], %{{.*}} ]
// CHECK: store %[[STRUCT_S0]]* %[[V6]], %[[STRUCT_S0]]** %[[T]], align 8
// CHECK: %[[V7:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[T]], align 8
// CHECK: %[[V8:.*]] = bitcast %[[STRUCT_S0]]* %[[V7]] to i8*

// CHECK: %[[V10:.*]] = ptrtoint i8* %[[V8]] to i64
// CHECK: %[[V11:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V10]], i32 1, i64 100)
// CHECK: %[[V12:.*]] = inttoptr i64 %[[V11]] to i8*

// CHECK: %[[V13:.*]] = phi i8* [ null, %{{.*}} ], [ %[[V12]], %{{.*}} ]
// CHECK: store i8* %[[V13]], i8** %[[T2]], align 8

void test_conversion0(void *p) {
  S0 *t = (S0 *)p;
  void *t2 = t;
}

// CHECK-LABEL: define {{.*}} void @test_conversion1(
// CHECK: %[[P_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[T:.*]] = alloca %[[STRUCT_S1]]*, align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[P_ADDR]], align 8
// CHECK: %[[V1:.*]] = bitcast %[[STRUCT_S0]]* %[[V0]] to %[[STRUCT_S1]]*

// CHECK: %[[V3:.*]] = ptrtoint %[[STRUCT_S1]]* %[[V1]] to i64
// CHECK: %[[V4:.*]] = call i64 @llvm.ptrauth.resign(i64 %[[V3]], i32 1, i64 100, i32 1, i64 101)
// CHECK: %[[V5:.*]] = inttoptr i64 %[[V4]] to %[[STRUCT_S1]]*

// CHECK: %[[V6:.*]] = phi %[[STRUCT_S1]]* [ null, %{{.*}} ], [ %[[V5]], %{{.*}} ]
// CHECK: store %[[STRUCT_S1]]* %[[V6]], %[[STRUCT_S1]]** %[[T]], align 8

void test_conversion1(S0 *p) {
  S1 *t = (S1 *)p;
}

// CHECK-LABEL: define {{.*}} void @test_conversion2(
// CHECK: %[[P_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[T:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[P_ADDR]], align 8
// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S0]]** %[[T]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.blend(i64 %[[V1]], i64 1000)

// CHECK: %[[V4:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V0]] to i64
// CHECK: %[[V5:.*]] = call i64 @llvm.ptrauth.resign(i64 %[[V4]], i32 1, i64 100, i32 1, i64 %[[V2]])
// CHECK: %[[V6:.*]] = inttoptr i64 %[[V5]] to %[[STRUCT_S0]]*

// CHECK: %[[V7:.*]] = phi %[[STRUCT_S0]]* [ null, %{{.*}} ], [ %[[V6]], %{{.*}} ]
// CHECK: store %[[STRUCT_S0]]* %[[V7]], %[[STRUCT_S0]]** %[[T]], align 8
// CHECK: %[[V8:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[T]], align 8
// CHECK: %[[V9:.*]] = ptrtoint %[[STRUCT_S0]]** %[[T]] to i64
// CHECK: %[[V10:.*]] = call i64 @llvm.ptrauth.blend(i64 %[[V9]], i64 1000)

// CHECK: %[[V12:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V8]] to i64
// CHECK: %[[V13:.*]] = call i64 @llvm.ptrauth.resign(i64 %[[V12]], i32 1, i64 %[[V10]], i32 1, i64 100)
// CHECK: %[[V14:.*]] = inttoptr i64 %[[V13]] to %[[STRUCT_S0]]*

// CHECK: %[[V15:.*]] = phi %[[STRUCT_S0]]* [ null, %{{.*}} ], [ %[[V14]], %{{.*}} ]
// CHECK: store %[[STRUCT_S0]]* %[[V15]], %[[STRUCT_S0]]** %{{.*}}, align 8

void test_conversion2(S0 *p) {
  S0 *__ptrauth(1, 1, 1000) t = p;
  S0 *t2 = t;
}

// CHECK-LABEL: define {{.*}} void @test_conversion3(
// CHECK: %[[S_ADDR:.*]] = alloca %[[STRUCT_S2]]*, align 8
// CHECK: %[[T:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S2]]*, %[[STRUCT_S2]]** %[[S_ADDR]], align 8
// CHECK: %[[V1:.*]] = bitcast %[[STRUCT_S2]]* %[[V0]] to %[[STRUCT_S0]]*

// CHECK: %[[V3:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V1]] to i64
// CHECK: %[[V4:.*]] = call i64 @llvm.ptrauth.resign(i64 %[[V3]], i32 1, i64 102, i32 1, i64 100)
// CHECK: %[[V5:.*]] = inttoptr i64 %[[V4]] to %[[STRUCT_S0]]*

// CHECK: %[[V6:.*]] = phi %[[STRUCT_S0]]* [ null, %{{.*}} ], [ %[[V5]], %{{.*}} ]
// CHECK: store %[[STRUCT_S0]]* %[[V6]], %[[STRUCT_S0]]** %[[T]], align 8

void test_conversion3(S2 *s) {
  S0 *t = (S0 *)s;
}

// CHECK-LABEL: define {{.*}} void @test_conversion4(
// CHECK: %[[S_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[S_ADDR]], align 8

// CHECK: %[[V2:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V0]] to i64
// CHECK: %[[V3:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V2]], i32 1, i64 100)
// CHECK: %[[V4:.*]] = inttoptr i64 %[[V3]] to %[[STRUCT_S0]]*

// CHECK: %[[V5:.*]] = phi %[[STRUCT_S0]]* [ null, %{{.*}} ], [ %[[V4]], %{{.*}} ]
// CHECK: %[[V6:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V5]] to i64
// CHECK: store i64 %[[V6]], i64* %{{.*}}, align 8

void test_conversion4(S0 *s) {
  intptr_t i = (intptr_t)s;
}

// CHECK-LABEL: define {{.*}} void @test_conversion5(
// CHECK: %[[I_ADDR:.*]] = alloca i64, align 8
// CHECK: %[[V0:.*]] = load i64, i64* %[[I_ADDR]], align 8
// CHECK: %[[V1:.*]] = inttoptr i64 %[[V0]] to %[[STRUCT_S0]]*

// CHECK: %[[V3:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V1]] to i64
// CHECK: %[[V4:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V3]], i32 1, i64 100)
// CHECK: %[[V5:.*]] = inttoptr i64 %[[V4]] to %[[STRUCT_S0]]*

// CHECK: %[[V6:.*]] = phi %[[STRUCT_S0]]* [ null, %{{.*}} ], [ %[[V5]], %{{.*}} ]
// CHECK: store %[[STRUCT_S0]]* %[[V6]], %[[STRUCT_S0]]** %{{.*}}, align 8

void test_conversion5(intptr_t i) {
  S0 *s = (S0 *)i;
}

// CHECK: define {{.*}} i32 @test_comparison0(%[[STRUCT_S0]]* %[[S:.*]])
// CHECK: %[[S_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[S_ADDR]], align 8
// CHECK: %[[V1:.*]] = call i64 @llvm.ptrauth.sign(i64 ptrtoint (%[[STRUCT_S0]]* @g0 to i64), i32 1, i64 100)
// CHECK: %[[V2:.*]] = inttoptr i64 %[[V1]] to %[[STRUCT_S0]]*
// CHECK: icmp eq %[[STRUCT_S0]]* %[[V0]], %[[V2]]

int test_comparison0(S0 *s) {
  if (s == &g0)
    return 1;
  return 2;
}

// CHECK-LABEL: define {{.*}} void @test_call0()
// CHECK: %[[T:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[P:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[V0:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T]] to i64
// CHECK: %[[V1:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V0]], i32 1, i64 100)
// CHECK: %[[V2:.*]] = inttoptr i64 %[[V1]] to %[[STRUCT_S0]]*
// CHECK: store %[[STRUCT_S0]]* %[[V2]], %[[STRUCT_S0]]** %[[P]], align 8
// CHECK: %[[V3:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[P]], align 8
// CHECK: call %[[STRUCT_S0]]* @func0(%[[STRUCT_S0]]* %[[V3]])

void test_call0(void) {
  S0 t;
  S0 *p = &t;
  func0(p);
}

// CHECK-LABEL: define {{.*}} void @test_call1()
// CHECK: %[[T0:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[T1:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[T2:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[V0:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T0]] to i64
// CHECK: %[[V1:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V0]], i32 1, i64 100)
// CHECK: %[[V2:.*]] = inttoptr i64 %[[V1]] to %[[STRUCT_S0]]*
// CHECK: %[[V3:.*]] = ptrtoint %[[STRUCT_S0]]** %[[T1]] to i64
// CHECK: %[[V4:.*]] = call i64 @llvm.ptrauth.blend(i64 %[[V3]], i64 1000)
// CHECK: %[[V5:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V2]] to i64
// CHECK: %[[V6:.*]] = call i64 @llvm.ptrauth.resign(i64 %[[V5]], i32 1, i64 100, i32 1, i64 %[[V4]])
// CHECK: %[[V7:.*]] = inttoptr i64 %[[V6]] to %[[STRUCT_S0]]*
// CHECK: store %[[STRUCT_S0]]* %[[V7]], %[[STRUCT_S0]]** %[[T1]], align 8
// CHECK: %[[V8:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[T1]], align 8
// CHECK: %[[V9:.*]] = ptrtoint %[[STRUCT_S0]]** %[[T1]] to i64
// CHECK: %[[V10:.*]] = call i64 @llvm.ptrauth.blend(i64 %[[V9]], i64 1000)

// CHECK: %[[V12:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V8]] to i64
// CHECK: %[[V13:.*]] = call i64 @llvm.ptrauth.resign(i64 %[[V12]], i32 1, i64 %[[V10]], i32 1, i64 100)
// CHECK: %[[V14:.*]] = inttoptr i64 %[[V13]] to %[[STRUCT_S0]]*

// CHECK: %[[V15:.*]] = phi %[[STRUCT_S0]]* [ null, %{{.*}} ], [ %[[V14]], %{{.*}} ]
// CHECK-NEXT: %[[CALL:.*]] = call %[[STRUCT_S0]]* @func0(%[[STRUCT_S0]]* %[[V15]])
// CHECK-NEXT: store %[[STRUCT_S0]]* %[[CALL]], %[[STRUCT_S0]]** %[[T2]], align 8
// CHECK-NEXT: ret void

void test_call1(void) {
  S0 t0;
  S0 *__ptrauth(1, 1, 1000) t1 = &t0;
  S0 *t2 = func0(t1);
}

// CHECK-LABEL: define {{.*}} void @test_call2()
// CHECK: %[[P:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[CALL:.*]] = call %[[STRUCT_S0]]* @func0(%[[STRUCT_S0]]* null)
// CHECK: %[[V0:.*]] = ptrtoint %[[STRUCT_S0]]** %[[P]] to i64
// CHECK: %[[V1:.*]] = call i64 @llvm.ptrauth.blend(i64 %[[V0:.*]], i64 1000)

// CHECK: %[[V3:.*]] = ptrtoint %[[STRUCT_S0]]* %[[CALL]] to i64
// CHECK: %[[V4:.*]] = call i64 @llvm.ptrauth.resign(i64 %[[V3]], i32 1, i64 100, i32 1, i64 %[[V1]])
// CHECK: %[[V5:.*]] = inttoptr i64 %[[V4]] to %[[STRUCT_S0]]*

// CHECK: %[[V6:.*]] = phi %[[STRUCT_S0]]* [ null, %{{.*}} ], [ %[[V5]], %{{.*}} ]
// CHECK: store %[[STRUCT_S0]]* %[[V6]], %[[STRUCT_S0]]** %[[P]], align 8

void test_call2(void) {
  S0 *__ptrauth(1, 1, 1000) p = func0(0);
}

// CHECK-LABEL: define {{.*}} i64 @test_call3(
// CHECK-NOT: @llvm.ptrauth

S0 test_call3(S0 a) {
  S0 t = a;
  S0 t1 = func1(t);
  S0 t2 = t1;
  return t2;
}

// NOTE: The aggregate temporary created to pass 't' to 'func2' isn't
//       destructed. This is a pre-existing bug.

// FIXME: Shouldn't pass raw pointers to non-trivial C struct special functions.

// CHECK: define {{.*}} void @test_call4(%[[STRUCT_S4]]* noalias sret(%struct.S4) align 16 %[[AGG_RESULT:.*]], %[[STRUCT_S4]]* %[[A:.*]])
// CHECK: %[[T:.*]] = alloca %[[STRUCT_S4]], align 16
// CHECK: %[[T1:.*]] = alloca %[[STRUCT_S4]], align 16
// CHECK: %[[AGG_TMP:.*]] = alloca %[[STRUCT_S4]], align 16
// CHECK: %[[NRVO:.*]] = alloca i1, align 1
// CHECK: %[[V0:.*]] = bitcast %[[STRUCT_S4]]* %[[T]] to i8**
// CHECK: %[[V1:.*]] = bitcast %[[STRUCT_S4]]* %[[A]] to i8**
// CHECK: %[[V2:.*]] = ptrtoint i8** %[[V1]] to i64
// CHECK: %[[V3:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V2]], i32 1, i64 104)
// CHECK: %[[V4:.*]] = inttoptr i64 %[[V3]] to i8**
// CHECK: call void @__copy_constructor_16_16_t0w8_w8_t16w16(i8** %[[V0]], i8** %[[V4]])
// CHECK: %[[V5:.*]] = bitcast %[[STRUCT_S4]]* %[[AGG_TMP]] to i8**
// CHECK: %[[V6:.*]] = bitcast %[[STRUCT_S4]]* %[[T]] to i8**
// CHECK: call void @__copy_constructor_16_16_t0w8_w8_t16w16(i8** %[[V5]], i8** %[[V6]])
// CHECK: %[[V7:.*]] = ptrtoint %[[STRUCT_S4]]* %[[T1]] to i64
// CHECK: %[[V8:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V7]], i32 1, i64 104)
// CHECK: %[[V9:.*]] = inttoptr i64 %[[V8]] to %[[STRUCT_S4]]*
// CHECK: %[[V10:.*]] = ptrtoint %[[STRUCT_S4]]* %[[AGG_TMP]] to i64
// CHECK: %[[V11:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V10]], i32 1, i64 104)
// CHECK: %[[V12:.*]] = inttoptr i64 %[[V11]] to %[[STRUCT_S4]]*
// CHECK: call void @func2(%[[STRUCT_S4]]* sret(%struct.S4) align 16 %[[V9]], %[[STRUCT_S4]]* %[[V12]])
// CHECK: store i1 false, i1* %[[NRVO]], align 1
// CHECK: %[[V13:.*]] = bitcast %[[STRUCT_S4]]* %[[AGG_RESULT]] to i8**
// CHECK: %[[V14:.*]] = bitcast %[[STRUCT_S4]]* %[[T1]] to i8**
// CHECK: %[[V15:.*]] = ptrtoint i8** %[[V13]] to i64
// CHECK: %[[V16:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V15]], i32 1, i64 104)
// CHECK: %[[V17:.*]] = inttoptr i64 %[[V16]] to i8**
// CHECK: call void @__copy_constructor_16_16_t0w8_w8_t16w16(i8** %[[V17]], i8** %[[V14]])

// CHECK: %[[V22:.*]] = bitcast %[[STRUCT_S4]]* %[[T1]] to i8**
// CHECK: call void @__destructor_16_w8(i8** %[[V22]]) #4
// CHECK: %[[V23:.*]] = bitcast %[[STRUCT_S4]]* %[[T]] to i8**
// CHECK: call void @__destructor_16_w8(i8** %[[V23]]) #4
// CHECK: %[[V24:.*]] = bitcast %[[STRUCT_S4]]* %[[A]] to i8**
// CHECK: %[[V25:.*]] = ptrtoint i8** %[[V24]] to i64
// CHECK: %[[V26:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V25]], i32 1, i64 104)
// CHECK: %[[V27:.*]] = inttoptr i64 %[[V26]] to i8**
// CHECK: call void @__destructor_16_w8(i8** %[[V27]])

S4 test_call4(S4 a) {
  S4 t = a;
  S4 t1 = func2(t);
  S4 t2 = t1;
  return t2;
}

// CHECK: define {{.*}} %[[STRUCT_S0]]* @test_return0(i8* %[[P:.*]])
// CHECK: %[[P_ADDR:.*]] = alloca i8*, align 8
// CHECK: %[[V0:.*]] = load i8*, i8** %[[P_ADDR]], align 8
// CHECK: %[[V1:.*]] = bitcast i8* %[[V0]] to %[[STRUCT_S0]]*
// CHECK: %[[V2:.*]] = icmp ne %[[STRUCT_S0]]* %[[V1]], null

// CHECK: %[[V3:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V1]] to i64
// CHECK: %[[V4:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V3]], i32 1, i64 100)
// CHECK: %[[V5:.*]] = inttoptr i64 %[[V4]] to %[[STRUCT_S0]]*

// CHECK: %[[V6:.*]] = phi %[[STRUCT_S0]]* [ null, %{{.*}} ], [ %[[V5]], %{{.*}} ]
// CHECK: ret %[[STRUCT_S0]]* %[[V6]]

S0 *test_return0(void *p) {
  return (S0 *)p;
}

// CHECK-LABEL: define {{.*}} void @test_assignment0(
// CHECK-NOT: call {{.*}}ptrauth

void test_assignment0(S0 *s) {
  S0 *t = s;
}

// CHECK-LABEL: define {{.*}} void @test_compound_literal0(
// CHECK: %[[T0:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[T1]] = alloca i32, align 4
// CHECK: %[[V0:.*]] = bitcast %[[STRUCT_S0]]* %[[T0]] to i8*
// CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %[[V0]], i8* align 4 bitcast (%[[STRUCT_S0]]* @[[CONST_TEST_COMPOUND_LITERAL0_T0]] to i8*), i64 8, i1 false)
// CHECK: %[[A0:.*]] = getelementptr inbounds %[[STRUCT_S0]], %[[STRUCT_S0]]* %[[T0]], i32 0, i32 0
// CHECK: %[[V1:.*]] = load i32, i32* %[[A0]], align 4
// CHECK: %[[A1:.*]] = getelementptr inbounds %[[STRUCT_S0]], %[[STRUCT_S0]]* %[[T0]], i32 0, i32 1
// CHECK: %[[V2:.*]] = load i32, i32* %[[A1]], align 4
// CHECK: %[[ADD:.*]] = add nsw i32 %[[V1]], %[[V2]]

void test_compound_literal0() {
  S0 t0 = (S0){1, 2};
  int t1 = t0.a0 + t0.a1;
}

// CHECK-LABEL: define {{.*}} void @test_compound_literal1(
// CHECK: %[[T0:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[_COMPOUNDLITERAL:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[V0:.*]] = ptrtoint %[[STRUCT_S0]]* %[[_COMPOUNDLITERAL]] to i64
// CHECK: %[[V1:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V0]], i32 1, i64 100)
// CHECK: %[[V2:.*]] = inttoptr i64 %[[V1]] to %[[STRUCT_S0]]*
// CHECK: store %[[STRUCT_S0]]* %[[V2]], %[[STRUCT_S0]]** %[[T0]], align 8
// CHECK: %[[V3:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[T0]], align 8
// CHECK: %[[V4:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V3]] to i64
// CHECK: %[[V5:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V4]], i32 1, i64 100)
// CHECK: %[[V6:.*]] = inttoptr i64 %[[V5]] to %[[STRUCT_S0]]*
// CHECK: %[[V7:.*]] = bitcast %[[STRUCT_S0]]* %[[V6]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V7]], i64 0
// CHECK: %[[V8:.*]] = bitcast i8* %[[RESIGNEDGEP]] to i32*
// CHECK: %[[V9:.*]] = load i32, i32* %[[V8]], align 4
// CHECK: %[[V10:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[T0]], align 8
// CHECK: %[[V11:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V10]] to i64
// CHECK: %[[V12:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V11]], i32 1, i64 100)
// CHECK: %[[V13:.*]] = inttoptr i64 %[[V12]] to %[[STRUCT_S0]]*
// CHECK: %[[V14:.*]] = bitcast %[[STRUCT_S0]]* %[[V13]] to i8*
// CHECK: %[[RESIGNEDGEP1:.*]] = getelementptr i8, i8* %[[V14]], i64 4
// CHECK: %[[V15:.*]] = bitcast i8* %[[RESIGNEDGEP1]] to i32*
// CHECK: %[[V16:.*]] = load i32, i32* %[[V15]], align 4
// CHECK: %[[ADD:.*]] = add nsw i32 %[[V9]], %[[V16]]

void test_compound_literal1() {
  S0 *t0 = &(S0){1, 2};
  int t1 = t0->a0 + t0->a1;
}

// CHECK: define {{.*}} void @test_block0(%[[STRUCT_S0]]* %[[S:.*]])
// CHECK: %[[S_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[T0:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[BLOCK:.*]] = alloca <{ i8*, i32, i32, i8*, %[[STRUCT_BLOCK_DESCRIPTOR]]*, %[[STRUCT_S0]]*, %[[STRUCT_S0]] }>, align 8
// CHECK: %[[BLOCK_CAPTURED:.*]] = getelementptr inbounds <{ i8*, i32, i32, i8*, %[[STRUCT_BLOCK_DESCRIPTOR]]*, %[[STRUCT_S0]]*, %[[STRUCT_S0]] }>, <{ i8*, i32, i32, i8*, %[[STRUCT_BLOCK_DESCRIPTOR]]*, %[[STRUCT_S0]]*, %[[STRUCT_S0]] }>* %[[BLOCK]], i32 0, i32 5
// CHECK: %[[V4:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[S_ADDR]], align 8
// CHECK: store %[[STRUCT_S0]]* %[[V4]], %[[STRUCT_S0]]** %[[BLOCK_CAPTURED]], align 8
// CHECK: %[[BLOCK_CAPTURED1:.*]] = getelementptr inbounds <{ i8*, i32, i32, i8*, %[[STRUCT_BLOCK_DESCRIPTOR]]*, %[[STRUCT_S0]]*, %[[STRUCT_S0]] }>, <{ i8*, i32, i32, i8*, %[[STRUCT_BLOCK_DESCRIPTOR]]*, %[[STRUCT_S0]]*, %[[STRUCT_S0]] }>* %[[BLOCK]], i32 0, i32 6
// CHECK: %[[V5:.*]] = bitcast %[[STRUCT_S0]]* %[[BLOCK_CAPTURED1]] to i8*
// CHECK: %[[V6:.*]] = bitcast %[[STRUCT_S0]]* %[[T0]] to i8*
// CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %[[V5]], i8* align 4 %[[V6]], i64 8, i1 false)

// CHECK: define internal i32 @__test_block0_block_invoke(i8* %[[_BLOCK_DESCRIPTOR:.*]])
// CHECK: %[[BLOCK:.*]] = bitcast i8* %[[_BLOCK_DESCRIPTOR]] to <{ i8*, i32, i32, i8*, %[[STRUCT_BLOCK_DESCRIPTOR]]*, %[[STRUCT_S0]]*, %[[STRUCT_S0]] }>*
// CHECK: %[[BLOCK_CAPTURE_ADDR:.*]] = getelementptr inbounds <{ i8*, i32, i32, i8*, %[[STRUCT_BLOCK_DESCRIPTOR]]*, %[[STRUCT_S0]]*, %[[STRUCT_S0]] }>, <{ i8*, i32, i32, i8*, %[[STRUCT_BLOCK_DESCRIPTOR]]*, %[[STRUCT_S0]]*, %[[STRUCT_S0]] }>* %[[BLOCK]], i32 0, i32 5
// CHECK: %[[V0:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[BLOCK_CAPTURE_ADDR]], align 8
// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V0]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V1]], i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: %[[V4:.*]] = bitcast %[[STRUCT_S0]]* %[[V3]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V4]], i64 4
// CHECK: %[[V5:.*]] = bitcast i8* %[[RESIGNEDGEP]] to i32*
// CHECK: %[[V6:.*]] = load i32, i32* %[[V5]], align 4
// CHECK: %[[BLOCK_CAPTURE_ADDR1:.*]] = getelementptr inbounds <{ i8*, i32, i32, i8*, %[[STRUCT_BLOCK_DESCRIPTOR]]*, %[[STRUCT_S0]]*, %[[STRUCT_S0]] }>, <{ i8*, i32, i32, i8*, %[[STRUCT_BLOCK_DESCRIPTOR]]*, %[[STRUCT_S0]]*, %[[STRUCT_S0]] }>* %[[BLOCK]], i32 0, i32 6
// CHECK: %[[A1:.*]] = getelementptr inbounds %[[STRUCT_S0]], %[[STRUCT_S0]]* %[[BLOCK_CAPTURE_ADDR1]], i32 0, i32 1
// CHECK: %[[V7:.*]] = load i32, i32* %[[A1]], align 4
// CHECK: %[[ADD:.*]] = add nsw i32 %[[V6]], %[[V7]]
// CHECK: ret i32 %[[ADD]]

void test_block0(S0 *s) {
  S0 t0 = {1, 2};
  int t1 = ^{
    return s->a1 + t0.a1;
  }();
}

// CHECK: define {{.*}} %[[STRUCT_S0]]* @test_atomic0(
// CHECK: %[[A_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[P:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[A_ADDR]], align 8
// CHECK: store %[[STRUCT_S0]]* %[[V0]], %[[STRUCT_S0]]** %[[P]], align 8
// CHECK: %[[V1:.*]] = bitcast %[[STRUCT_S0]]** %[[P]] to i64*
// CHECK: %[[ATOMIC_LOAD:.*]] = load atomic i64, i64* %[[V1]] seq_cst, align 8
// CHECK: %[[V2:.*]] = inttoptr i64 %[[ATOMIC_LOAD]] to %[[STRUCT_S0]]*

// CHECK: %[[V3:.*]] = phi %[[STRUCT_S0]]* [ %[[V2]], %{{.*}} ], [ %[[V16:.*]], %{{.*}} ]
// CHECK: %[[V4:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V3]] to i64
// CHECK: %[[V5:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V4]], i32 1, i64 100)
// CHECK: %[[V6:.*]] = inttoptr i64 %[[V5]] to %[[STRUCT_S0]]*
// CHECK: %[[INCDEC_PTR:.*]] = getelementptr inbounds %[[STRUCT_S0]], %[[STRUCT_S0]]* %[[V6]], i32 1
// CHECK: %[[V7:.*]] = ptrtoint %[[STRUCT_S0]]* %[[INCDEC_PTR]] to i64
// CHECK: %[[V8:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V7]], i32 1, i64 100)
// CHECK: %[[V9:.*]] = inttoptr i64 %[[V8]] to %[[STRUCT_S0]]*
// CHECK: %[[V10:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V3]] to i64
// CHECK: %[[V11:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V9]] to i64
// CHECK: %[[V12:.*]] = bitcast %[[STRUCT_S0]]** %[[P]] to i64*
// CHECK: %[[V13:.*]] = cmpxchg i64* %[[V12]], i64 %[[V10]], i64 %[[V11]] seq_cst seq_cst
// CHECK: %[[V14:.*]] = extractvalue { i64, i1 } %[[V13]], 0
// CHECK: %[[V16]] = inttoptr i64 %[[V14]] to %[[STRUCT_S0]]*

// CHECK: ret %[[STRUCT_S0]]* %[[V9]]

S0 *test_atomic0(S0 *a) {
  S0 * _Atomic p = a;
  return ++p;
}

// CHECK: define {{.*}} %[[STRUCT_S0]]* @test_atomic1(
// CHECK: alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[P:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[V1:.*]] = bitcast %[[STRUCT_S0]]** %[[P]] to i64*
// CHECK: %[[ATOMIC_LOAD:.*]] = load atomic i64, i64* %[[V1]] seq_cst, align 8
// CHECK: %[[V2:.*]] = inttoptr i64 %[[ATOMIC_LOAD]] to %[[STRUCT_S0]]*

// CHECK: ret %[[STRUCT_S0]]* %[[V2]]

S0 *test_atomic1(S0 *a) {
  S0 * _Atomic p = a;
  return p++;
}

// CHECK-LABEL: define {{.*}} i32 @test_address_space0(
// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S0]] addrspace(1)* %{{.*}} to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V1]], i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]] addrspace(1)*
// CHECK: %[[V4:.*]] = bitcast %[[STRUCT_S0]] addrspace(1)* %[[V3]] to i8 addrspace(1)*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8 addrspace(1)* %[[V4]], i64 4
// CHECK: %[[V5:.*]] = bitcast i8 addrspace(1)* %[[RESIGNEDGEP]] to i32 addrspace(1)*
// CHECK: load i32, i32 addrspace(1)* %[[V5]], align 4

int test_address_space0(__attribute__((address_space(1))) S0 *s) {
  return s->a1;
}

// CHECK-LABEL: define {{.*}} i32 @test_attr_annotate(
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.auth(i64 %{{.*}}, i32 1, i64 103)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S3]]*
// CHECK: %[[V4:.*]] = bitcast %[[STRUCT_S3]]* %[[V3]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V4]], i64 4
// CHECK: %[[V5:.*]] = bitcast i8* %[[RESIGNEDGEP]] to %[[STRUCT_S0]]*
// CHECK: %[[V6:.*]] = bitcast %[[STRUCT_S0]]* %[[V5]] to i8*
// CHECK: %[[V7:.*]] = call i8* @llvm.ptr.annotation.p0i8(i8* %[[V6]], i8* getelementptr inbounds ([4 x i8], [4 x i8]* @{{.*}}, i32 0, i32 0), i8* getelementptr inbounds ([{{.*}} x i8], [{{.*}} x i8]* @{{.*}}, i32 0, i32 0), i32 {{.*}})
// CHECK: %[[V8:.*]] = bitcast i8* %[[V7]] to %[[STRUCT_S0]]*
// CHECK: %[[A1:.*]] = getelementptr inbounds %[[STRUCT_S0]], %[[STRUCT_S0]]* %[[V8]], i32 0, i32 1
// CHECK: load i32, i32* %[[A1]], align 4

int test_attr_annotate(S3 *s) {
  return s->f1.a1;
}

// CHECK-LABEL: define {{.*}} void @test_ext_vector0(
// CHECK: %[[S_ADDR:.*]] = alloca %[[STRUCT_S4]]*, align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S4]]*, %[[STRUCT_S4]]** %[[S_ADDR]], align 8
// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S4]]* %[[V0]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V1]], i32 1, i64 104)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S4]]*
// CHECK: %[[V4:.*]] = bitcast %[[STRUCT_S4]]* %[[V3]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V4]], i64 24
// CHECK: %[[V5:.*]] = bitcast i8* %[[RESIGNEDGEP]] to float*
// CHECK: store float 3.000000e+00, float* %[[V5]], align 8

void test_ext_vector0(S4 *s) {
  s->extv0.hi[0] = 3.0;
}
