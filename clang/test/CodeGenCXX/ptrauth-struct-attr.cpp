// RUN: %clang_cc1 -triple arm64-apple-ios -fptrauth-calls -fptrauth-returns -fptrauth-intrinsics -emit-llvm -std=c++17 -O1 -disable-llvm-passes -fexceptions -fcxx-exceptions -o - %s | FileCheck %s

// CHECK: %[[STRUCT_S0:.*]] = type { i32, i32, [4 x i32] }
// CHECK: %[[STRUCT_S5:.*]] = type { %[[STRUCT_S2:.*]] }
// CHECK: %[[STRUCT_S2]] = type { i32, %[[STRUCT_S0]] }
// CHECK: %[[STRUCT_S1:.*]] = type { i32, i32, [4 x i32] }
// CHECK: %[[STRUCT_S4:.*]] = type { i32, i32 }
// CHECK: %[[STRUCT_S3:.*]] = type { [2 x i32], %[[STRUCT_S0]] }
// CHECK: %[[CLASS_ANON:.*]] = type { %[[STRUCT_S0]]*, i32* }
// CHECK: %[[CLASS_ANON_0:.*]] = type { %[[STRUCT_S0]], i32 }

typedef __SIZE_TYPE__ size_t;
void* operator new(size_t count, void* ptr);

#define ATTR0 __attribute__((ptrauth_struct(1, 100)))
#define ATTR1 __attribute__((ptrauth_struct(1, 101)))
#define ATTR2 __attribute__((ptrauth_struct(1, 102)))
#define ATTR4 __attribute__((ptrauth_struct(1, 104)))

struct ATTR0 S0 {
  int f0, f1, f2[4];
  S0() {}
  S0(const S0 &);
  S0 &operator=(const S0 &);
  ~S0();
  void nonvirtual0() { f1 = 1; }
  int lambda0(int i) {
    return [&](){ return f1 + i; }();
  }
  int lambda1(int i) {
    return [*this, i](){ return f1 + i; }();
  }
};

struct ATTR1 S1 {
  int f0, f1, f2[4];
  S1();
  ~S1() noexcept(false);
};

struct ATTR2 S2 {
  int f0;
  S0 f1;
  S2(S0);
};

struct S3 {
  int f0[2];
  S0 f1;
};

struct ATTR4 S4 {
  int f0, f1;
};

struct ATTR2 S5 : S2 {
  using S2::S2;
};

// CHECK: @gs0.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (%[[STRUCT_S0]]* @gs0 to i8*), i32 1, i64 0, i64 100 },
// CHECK: @gs2 = {{.*}} constant %[[STRUCT_S0]]* bitcast ({ i8*, i32, i64, i64 }* @gs0.ptrauth to %[[STRUCT_S0]]*), align 8
// CHECK: @gs3 = {{.*}} constant %[[STRUCT_S0]]* bitcast ({ i8*, i32, i64, i64 }* @gs0.ptrauth to %[[STRUCT_S0]]*), align 8

// CHECK: define internal void @__cxx_global_var_init()
// CHECK: %[[V0:.*]] = call i64 @llvm.ptrauth.sign(i64 ptrtoint (%[[STRUCT_S0]]* @gs0 to i64), i32 1, i64 100)
// CHECK: %[[V1:.*]] = inttoptr i64 %[[V0]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0C1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V1]])
// CHECK: call i32 @__cxa_atexit(void (i8*)* bitcast ({ i8*, i32, i64, i64 }* @_ZN2S0D1Ev.ptrauth to void (i8*)*), i8* bitcast ({ i8*, i32, i64, i64 }* @gs0.ptrauth to i8*), i8* @__dso_handle)
// CHECK: ret void
// CHECK: }

// CHECK: define linkonce_odr %[[STRUCT_S0]]* @_ZN2S0C1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[THIS:.*]])
// CHECK: %[[THIS_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: store %[[STRUCT_S0]]* %[[THIS]], %[[STRUCT_S0]]** %[[THIS_ADDR]], align 8
// CHECK: %[[THIS1:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[THIS_ADDR]], align 8
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0C2Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[THIS1]])
// CHECK: ret %[[STRUCT_S0]]* %[[THIS1]]

S0 gs0;

// CHECK: define linkonce_odr %[[STRUCT_S5]]* @_ZN2S5CI12S2E2S0(%[[STRUCT_S5]]* nonnull dereferenceable(28) %[[THIS:.*]], %[[STRUCT_S0]]* %[[V0:.*]])
// CHECK: %[[THIS_ADDR:.*]] = alloca %[[STRUCT_S5]]*, align 8
// CHECK: store %[[STRUCT_S5]]* %[[THIS]], %[[STRUCT_S5]]** %[[THIS_ADDR]], align 8
// CHECK: %[[THIS1:.*]] = load %[[STRUCT_S5]]*, %[[STRUCT_S5]]** %[[THIS_ADDR]], align 8
// CHECK: call %[[STRUCT_S5]]* @_ZN2S5CI22S2E2S0(%[[STRUCT_S5]]* nonnull dereferenceable(28) %[[THIS1]], %[[STRUCT_S0]]* %[[V0]])
// CHECK: ret %[[STRUCT_S5]]* %[[THIS1]]

S5 gs1(gs0);

S0 &gs2 = gs0;
S0 &gs3 = gs2;

// CHECK: define {{.*}} void @_Z16test_nonvirtual0P2S0(%[[STRUCT_S0]]* %[[S:.*]])
// CHECK: %[[S_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[S_ADDR]], align 8
// CHECK: call void @_ZN2S011nonvirtual0Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V0]])

// CHECK: define linkonce_odr void @_ZN2S011nonvirtual0Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[THIS:.*]])
// CHECK: %[[THIS_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[THIS1:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[THIS_ADDR]], align 8
// CHECK: %[[V0:.*]] = ptrtoint %[[STRUCT_S0]]* %[[THIS1]] to i64
// CHECK: %[[V1:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V0]], i32 1, i64 100)
// CHECK: %[[V2:.*]] = inttoptr i64 %[[V1]] to %[[STRUCT_S0]]*
// CHECK: %[[V3:.*]] = bitcast %[[STRUCT_S0]]* %[[V2]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V3]], i64 4
// CHECK: %[[V4:.*]] = bitcast i8* %[[RESIGNEDGEP]] to i32*
// CHECK: store i32 1, i32* %[[V4]], align 4

void test_nonvirtual0(S0 *s) {
  s->nonvirtual0();
}

// CHECK: define {{.*}} %[[STRUCT_S1]]* @_Z9test_new0v()
// CHECK: entry:
// CHECK: %[[EXN_SLOT:.*]] = alloca i8*
// CHECK: %[[EHSELECTOR_SLOT:.*]] = alloca i32
// CHECK: %[[CALL:.*]] = call noalias nonnull i8* @_Znwm(i64 24)
// CHECK: %[[V0:.*]] = bitcast i8* %[[CALL]] to %[[STRUCT_S1]]*
// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S1]]* %[[V0]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V1]], i32 1, i64 101)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S1]]*
// CHECK: %[[CALL1:.*]] = invoke %[[STRUCT_S1]]* @_ZN2S1C1Ev(%[[STRUCT_S1]]* nonnull dereferenceable(24) %[[V3]])

// CHECK: ret %[[STRUCT_S1]]* %[[V0]]

// CHECK: landingpad { i8*, i32 }
// CHECK: call void @_ZdlPv(i8* %[[CALL]])

S1 *test_new0() {
  return new S1();
}

// CHECK: define {{.*}} %[[STRUCT_S1]]* @_Z9test_new1Pv(i8* %[[P:.*]])
// CHECK: %[[P_ADDR:.*]] = alloca i8*, align 8
// CHECK: store i8* %[[P]], i8** %[[P_ADDR]], align 8
// CHECK: %[[V0:.*]] = load i8*, i8** %[[P_ADDR]], align 8
// CHECK: %[[V1:.*]] = bitcast i8* %[[V0]] to %[[STRUCT_S1]]*
// CHECK: %[[V2:.*]] = ptrtoint %[[STRUCT_S1]]* %[[V1]] to i64
// CHECK: %[[V3:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V2]], i32 1, i64 101)
// CHECK: %[[V4:.*]] = inttoptr i64 %[[V3]] to %[[STRUCT_S1]]*
// CHECK: %[[CALL:.*]] = call %[[STRUCT_S1]]* @_ZN2S1C1Ev(%[[STRUCT_S1]]* nonnull dereferenceable(24) %[[V4]])
// CHECK: ret %[[STRUCT_S1]]* %[[V1]]

S1 *test_new1(void *p) {
  return new (p) S1;
}

// CHECK: define {{.*}} %[[STRUCT_S1]]* @_Z9test_new2v()
// CHECK: %[[CALL:.*]] = call noalias nonnull i8* @_Znam(i64 112)
// CHECK: %[[V2:.*]] = getelementptr inbounds i8, i8* %[[CALL]], i64 16
// CHECK: %[[V3:.*]] = bitcast i8* %[[V2]] to %[[STRUCT_S1]]*
// CHECK: %[[ARRAYCTOR_END:.*]] = getelementptr inbounds %[[STRUCT_S1]], %[[STRUCT_S1]]* %[[V3]], i64 4

// CHECK: %[[ARRAYCTOR_CUR:.*]] = phi %[[STRUCT_S1]]* [ %[[V3]], %{{.*}} ], [ %[[ARRAYCTOR_NEXT:.*]], %{{.*}} ]
// CHECK: %[[V4:.*]] = ptrtoint %[[STRUCT_S1]]* %[[ARRAYCTOR_CUR]] to i64
// CHECK: %[[V5:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V4]], i32 1, i64 101)
// CHECK: %[[V6:.*]] = inttoptr i64 %[[V5]] to %[[STRUCT_S1]]*
// CHECK: %[[CALL1:.*]] = invoke %[[STRUCT_S1]]* @_ZN2S1C1Ev(%[[STRUCT_S1]]* nonnull dereferenceable(24) %[[V6]])

// CHECK: %[[ARRAYCTOR_NEXT]] = getelementptr inbounds %[[STRUCT_S1]], %[[STRUCT_S1]]* %[[ARRAYCTOR_CUR]], i64 1
// CHECK: %[[ARRAYCTOR_DONE:.*]] = icmp eq %[[STRUCT_S1]]* %[[ARRAYCTOR_NEXT]], %[[ARRAYCTOR_END]]

// CHECK: ret %[[STRUCT_S1]]* %[[V3]]

// CHECK: landingpad { i8*, i32 }

// CHECK: %[[ARRAYDESTROY_ELEMENTPAST:.*]] = phi %[[STRUCT_S1]]* [ %[[ARRAYCTOR_CUR]], %{{.*}} ], [ %[[ARRAYDESTROY_ELEMENT:.*]], %{{.*}} ]
// CHECK: %[[ARRAYDESTROY_ELEMENT:.*]] = getelementptr inbounds %[[STRUCT_S1]], %[[STRUCT_S1]]* %[[ARRAYDESTROY_ELEMENTPAST]], i64 -1
// CHECK: %[[V10:.*]] = ptrtoint %[[STRUCT_S1]]* %[[ARRAYDESTROY_ELEMENT]] to i64
// CHECK: %[[V11:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V10]], i32 1, i64 101)
// CHECK: %[[V12:.*]] = inttoptr i64 %[[V11]] to %[[STRUCT_S1]]*
// CHECK: invoke %[[STRUCT_S1]]* @_ZN2S1D1Ev(%[[STRUCT_S1]]*  nonnull dereferenceable(24) %[[V12]])

// CHECK: icmp eq %[[STRUCT_S1]]* %[[ARRAYDESTROY_ELEMENT]], %[[V3]]

// CHECK: call void @_ZdaPv(i8* %[[CALL]])

S1 *test_new2() {
  return new S1[4];
}

// CHECK: define {{.*}} void @_Z12test_delete0P2S1(%[[STRUCT_S1]]* %{{.*}})
// CHECK: %[[A_ADDR:.*]] = alloca %[[STRUCT_S1]]*, align 8
// CHECK: %[[V0]] = load %[[STRUCT_S1]]*, %[[STRUCT_S1]]** %[[A_ADDR]], align 8

// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S1]]* %[[V0]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V1]], i32 1, i64 101)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S1]]*
// CHECK: invoke %[[STRUCT_S1]]* @_ZN2S1D1Ev(%[[STRUCT_S1]]* nonnull dereferenceable(24) %[[V0]])

// CHECK: %[[V4:.*]] = bitcast %[[STRUCT_S1]]* %[[V3]] to i8*
// CHECK: call void @_ZdlPv(i8* %[[V4]])

// CHECK: landingpad { i8*, i32 }
// CHECK: %[[V8:.*]] = bitcast %[[STRUCT_S1]]* %[[V3]] to i8*
// CHECK: call void @_ZdlPv(i8* %[[V8]])

void test_delete0(S1 *a) {
  delete a;
}

// CHECK: define {{.*}} void @_Z12test_delete1P2S1(%[[STRUCT_S1]]* %{{.*}})
// CHECK: %[[A_ADDR:.*]] = alloca %[[STRUCT_S1]]*, align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S1]]*, %[[STRUCT_S1]]** %[[A_ADDR]], align 8

// CHECK: %[[V1:.*]] = bitcast %[[STRUCT_S1]]* %[[V0]] to i8*
// CHECK: %[[V2:.*]] = ptrtoint i8* %[[V1]] to i64
// CHECK: %[[V3:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V2]], i32 1, i64 101)
// CHECK: %[[V4:.*]] = inttoptr i64 %[[V3]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V4]], i64 -16
// CHECK: %[[V5:.*]] = ptrtoint i8* %[[V1]] to i64
// CHECK: %[[V6:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V5]], i32 1, i64 101)
// CHECK: %[[V7:.*]] = inttoptr i64 %[[V6]] to i8*
// CHECK: %[[RESIGNEDGEP1:.*]] = getelementptr i8, i8* %[[V7]], i64 -8
// CHECK: %[[V8:.*]] = bitcast i8* %[[RESIGNEDGEP1]] to i64*
// CHECK: %[[V9:.*]] = load i64, i64* %[[V8]], align 4
// CHECK: %[[V10:.*]] = ptrtoint %[[STRUCT_S1]]* %[[V0]] to i64
// CHECK: %[[V11:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V10]], i32 1, i64 101)
// CHECK: %[[V12:.*]] = inttoptr i64 %[[V11]] to %[[STRUCT_S1]]*
// CHECK: %[[DELETE_END:.*]] = getelementptr inbounds %[[STRUCT_S1]], %[[STRUCT_S1]]* %[[V12]], i64 %[[V9]]
// CHECK: icmp eq %[[STRUCT_S1]]* %[[V12]], %[[DELETE_END]]

// CHECK: %[[ARRAYDESTROY_ELEMENTPAST:.*]] = phi %[[STRUCT_S1]]* [ %[[DELETE_END]], %{{.*}} ], [ %[[ARRAYDESTROY_ELEMENT]], %{{.*}} ]
// CHECK: %[[ARRAYDESTROY_ELEMENT:.*]] = getelementptr inbounds %[[STRUCT_S1]], %[[STRUCT_S1]]* %[[ARRAYDESTROY_ELEMENTPAST]], i64 -1
// CHECK: %[[V13:.*]] = ptrtoint %[[STRUCT_S1]]* %[[ARRAYDESTROY_ELEMENT]] to i64
// CHECK: %[[V14:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V13]], i32 1, i64 101)
// CHECK: %[[V15:.*]] = inttoptr i64 %[[V14]] to %[[STRUCT_S1]]*
// CHECK: invoke %[[STRUCT_S1]]* @_ZN2S1D1Ev(%[[STRUCT_S1]]* nonnull dereferenceable(24) %[[V15]])

// CHECK: icmp eq %[[STRUCT_S1]]* %[[ARRAYDESTROY_ELEMENT]], %[[V12]]

// CHECK: call void @_ZdaPv(i8* %[[RESIGNEDGEP]])

// CHECK: landingpad { i8*, i32 }

// CHECK: %[[ARRAYDESTROY_ELEMENTPAST4:.*]] = phi %[[STRUCT_S1]]* [ %[[ARRAYDESTROY_ELEMENT]], %{{.*}} ], [ %[[ARRAYDESTROY_ELEMENT5:.*]], %{{.*}} ]
// CHECK: %[[ARRAYDESTROY_ELEMENT5:.*]] = getelementptr inbounds %[[STRUCT_S1]], %[[STRUCT_S1]]* %[[ARRAYDESTROY_ELEMENTPAST4]], i64 -1
// CHECK: %[[V19:.*]] = ptrtoint %[[STRUCT_S1]]* %[[ARRAYDESTROY_ELEMENT5]] to i64
// CHECK: %[[V20:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V19]], i32 1, i64 101)
// CHECK: %[[V21:.*]] = inttoptr i64 %[[V20]] to %[[STRUCT_S1]]*
// CHECK: %[[CALL7:.*]] = invoke %[[STRUCT_S1]]* @_ZN2S1D1Ev(%[[STRUCT_S1]]* nonnull dereferenceable(24) %[[V21]])

// CHECK: icmp eq %[[STRUCT_S1]]* %[[ARRAYDESTROY_ELEMENT5]], %[[V12]]

// CHECK: call void @_ZdaPv(i8* %[[RESIGNEDGEP]])

void test_delete1(S1 *a) {
  delete [] a;
}

// CHECK: define {{.*}} void @_Z16test_assignment0P2S0S0_(%[[STRUCT_S0]]* %{{.*}}, %[[STRUCT_S0]]* %{{.*}})
// CHECK: %[[A_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[B_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[B_ADDR]], align 8
// CHECK: %[[V1:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[A_ADDR]], align 8
// CHECK: call nonnull align 4 dereferenceable(24) %[[STRUCT_S0]]* @_ZN2S0aSERKS_(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V1]], %[[STRUCT_S0]]* nonnull align 4 dereferenceable(24) %[[V0]])

void test_assignment0(S0 *a, S0 *b) {
  *a = *b;
}

// CHECK: define {{.*}} void @_Z16test_assignment1v()
// CHECK: %[[T0:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[T1:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T0]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V1]], i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0C1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V3]])
// CHECK: %[[V5:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T1]] to i64
// CHECK: %[[V6:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V5]], i32 1, i64 100)
// CHECK: %[[V7:.*]] = inttoptr i64 %[[V6]] to %[[STRUCT_S0]]*
// CHECK: invoke %[[STRUCT_S0]]* @_ZN2S0C1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V7]])

// CHECK: %[[V8:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T1]] to i64
// CHECK: %[[V9:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V8]], i32 1, i64 100)
// CHECK: %[[V10:.*]] = inttoptr i64 %[[V9]] to %[[STRUCT_S0]]*
// CHECK: %[[V11:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T0]] to i64
// CHECK: %[[V12:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V11]], i32 1, i64 100)
// CHECK: %[[V13:.*]] = inttoptr i64 %[[V12]] to %[[STRUCT_S0]]*
// CHECK: invoke nonnull align 4 dereferenceable(24) %[[STRUCT_S0]]* @_ZN2S0aSERKS_(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V13]], %[[STRUCT_S0]]* nonnull align 4 dereferenceable(24) %[[V10]])

// CHECK: %[[V14:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T1]] to i64
// CHECK: %[[V15:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V14]], i32 1, i64 100)
// CHECK: %[[V16:.*]] = inttoptr i64 %[[V15]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0D1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V16]])
// CHECK: %[[V18:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T0]] to i64
// CHECK: %[[V19:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V18]], i32 1, i64 100)
// CHECK: %[[V20:.*]] = inttoptr i64 %[[V19]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0D1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V20]])
// CHECK: ret void

// CHECK: landingpad { i8*, i32 }
// CHECK: landingpad { i8*, i32 }
// CHECK: %[[V28:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T1]] to i64
// CHECK: %[[V29:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V28]], i32 1, i64 100)
// CHECK: %[[V30:.*]] = inttoptr i64 %[[V29]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0D1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V30]])

// CHECK: %[[V32:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T0]] to i64
// CHECK: %[[V33:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V32]], i32 1, i64 100)
// CHECK: %[[V34:.*]] = inttoptr i64 %[[V33]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0D1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V34]])

void test_assignment1() {
  S0 t0, t1;
  t0 = t1;
}

// CHECK: define {{.*}} void @_Z16test_assignment2P2S4S0_(
// CHECK: %[[A_ADDR:.*]] = alloca %[[STRUCT_S4]]*, align 8
// CHECK: %[[B_ADDR:.*]] = alloca %[[STRUCT_S4]]*, align 8
// CHECK: %[[V0:.*]] = load %[[STRUCT_S4]]*, %[[STRUCT_S4]]** %[[B_ADDR]], align 8
// CHECK: %[[V1:.*]] = load %[[STRUCT_S4]]*, %[[STRUCT_S4]]** %[[A_ADDR]], align 8
// CHECK: %[[V2:.*]] = bitcast %[[STRUCT_S4]]* %[[V1]] to i8*
// CHECK: %[[V3:.*]] = bitcast %[[STRUCT_S4]]* %[[V0]] to i8*
// CHECK: %[[V4:.*]] = ptrtoint i8* %[[V2]] to i64
// CHECK: %[[V5:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V4]], i32 1, i64 104)
// CHECK: %[[V6:.*]] = inttoptr i64 %[[V5]] to i8*
// CHECK: %[[V7:.*]] = ptrtoint i8* %[[V3]] to i64
// CHECK: %[[V8:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V7]], i32 1, i64 104)
// CHECK: %[[V9:.*]] = inttoptr i64 %[[V8]] to i8*
// CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %[[V6]], i8* align 4 %[[V9]], i64 8, i1 false)

void test_assignment2(S4 *a, S4 *b) {
  *a = *b;
}

// CHECK: define {{.*}} void @_Z28test_constructor_destructor0v()
// CHECK: %[[T0:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[T1:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T0]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V1]], i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0C1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V3]])
// CHECK: %[[V5:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T1]] to i64
// CHECK: %[[V6:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V5]], i32 1, i64 100)
// CHECK: %[[V7:.*]] = inttoptr i64 %[[V6]] to %[[STRUCT_S0]]*
// CHECK: %[[V8:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T0]] to i64
// CHECK: %[[V9:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V8]], i32 1, i64 100)
// CHECK: %[[V10:.*]] = inttoptr i64 %[[V9]] to %[[STRUCT_S0]]*
// CHECK: invoke %[[STRUCT_S0]]* @_ZN2S0C1ERKS_(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V7]], %[[STRUCT_S0]]* nonnull align 4 dereferenceable(24) %[[V10]])

// CHECK: %[[V11:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T1]] to i64
// CHECK: %[[V12:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V11]], i32 1, i64 100)
// CHECK: %[[V13:.*]] = inttoptr i64 %[[V12]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0D1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V13]])
// CHECK: %[[V15:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T0]] to i64
// CHECK: %[[V16:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V15]], i32 1, i64 100)
// CHECK: %[[V17:.*]] = inttoptr i64 %[[V16]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0D1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24)  %[[V17]])
// CHECK: ret void

// CHECK: landingpad { i8*, i32 }
// CHECK: %[[V23:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T0]] to i64
// CHECK: %[[V24:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V23]], i32 1, i64 100)
// CHECK: %[[V25:.*]] = inttoptr i64 %[[V24]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0D1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24)  %[[V25]])

void test_constructor_destructor0() {
  S0 t0, t1 = t0;
}

// CHECK: define {{.*}} void @_Z19test_member_access1P2S0i(%[[STRUCT_S0]]* %{{.*}}, i32 %{{.*}})
// CHECK: %[[A_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[I_ADDR:.*]] = alloca i32, align 4
// CHECK: %[[V0:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[A_ADDR]], align 8
// CHECK: %[[V1:.*]] = load i32, i32* %[[I_ADDR]], align 4
// CHECK: %[[ADD:.*]] = add nsw i32 %[[V1]], 2
// CHECK: %[[IDXPROM:.*]] = sext i32 %[[ADD]] to i64
// CHECK: %[[ARRAYIDX_OFFS:.*]] = mul nsw i64 %[[IDXPROM]], 4
// CHECK: %[[ADD1:.*]] = add i64 8, %[[ARRAYIDX_OFFS]]
// CHECK: %[[V2:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V0]] to i64
// CHECK: %[[V3:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V2]], i32 1, i64 100)
// CHECK: %[[V4:.*]] = inttoptr i64 %[[V3]] to %[[STRUCT_S0]]*
// CHECK: %[[V5:.*]] = bitcast %[[STRUCT_S0]]* %[[V4]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V5]], i64 %[[ADD1]]
// CHECK: %[[V6:.*]] = bitcast i8* %[[RESIGNEDGEP]] to i32*
// CHECK: store i32 123, i32* %[[V6]], align 4

void test_member_access1(S0 *a, int i) {
  a->f2[i + 2] = 123;
}

// CHECK: define {{.*}} nonnull align 4 dereferenceable(24) %[[STRUCT_S0]]* @_Z15test_reference0R2S0(%[[STRUCT_S0]]* nonnull align 4 dereferenceable(24) %[[A:.*]])
// CHECK: %[[A_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[T0:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[T1:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[R0:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[R1:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[V11:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T0]] to i64
// CHECK: %[[V21:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V1]], i32 1, i64 100)
// CHECK: %[[V31:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: %[[V41:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[A_ADDR]], align 8
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0C1ERKS_(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V3]], %[[STRUCT_S0]]* nonnull align 4 dereferenceable(24) %[[V4]])
// CHECK: %[[V6:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[A_ADDR]], align 8
// CHECK: store %[[STRUCT_S0]]* %[[V6]], %[[STRUCT_S0]]** %[[T1]], align 8
// CHECK: %[[V8:.*]] = ptrtoint %[[STRUCT_S0]]* %[[R0]] to i64
// CHECK: %[[V9:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V8]], i32 1, i64 100)
// CHECK: %[[V10:.*]] = inttoptr i64 %[[V9]] to %[[STRUCT_S0]]*
// CHECK: %[[V11:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T0]] to i64
// CHECK: %[[V12:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V11]], i32 1, i64 100)
// CHECK: %[[V13:.*]] = inttoptr i64 %[[V12]] to %[[STRUCT_S0]]*
// CHECK: %[[CALL1:.*]] = invoke nonnull align 4 dereferenceable(24) %[[STRUCT_S0]]* @_Z5func0R2S0(%[[STRUCT_S0]]* nonnull align 4 dereferenceable(24) %[[V13]])

// CHECK: invoke %[[STRUCT_S0]]* @_ZN2S0C1ERKS_(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V10]], %[[STRUCT_S0]]* nonnull align 4 dereferenceable(24) %[[CALL1]])

// CHECK: %[[V15:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T0]] to i64
// CHECK: %[[V16:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V15]], i32 1, i64 100)
// CHECK: %[[V17:.*]] = inttoptr i64 %[[V16]] to %[[STRUCT_S0]]*
// CHECK: %[[CALL6:.*]] = invoke nonnull align 4 dereferenceable(24) %[[STRUCT_S0]]* @_Z5func0R2S0(%[[STRUCT_S0]]* nonnull align 4 dereferenceable(24) %[[V17]])

// CHECK: store %[[STRUCT_S0]]* %[[CALL6]], %[[STRUCT_S0]]** %[[R1]], align 8
// CHECK: %[[V18:.*]] = call i64 @llvm.ptrauth.sign(i64 ptrtoint (%[[STRUCT_S0]]* @gs0 to i64), i32 1, i64 100)
// CHECK: %[[V19:.*]] = inttoptr i64 %[[V18]] to %[[STRUCT_S0]]*
// CHECK: %[[V21:.*]] = ptrtoint %[[STRUCT_S0]]* %[[R0]] to i64
// CHECK: %[[V22:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V21]], i32 1, i64 100)
// CHECK: %[[V23:.*]] = inttoptr i64 %[[V22]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0D1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V23]])
// CHECK: %[[V26:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T0]] to i64
// CHECK: %[[V27:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V26]], i32 1, i64 100)
// CHECK: %[[V28:.*]] = inttoptr i64 %[[V27]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0D1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V28]])
// CHECK: ret %[[STRUCT_S0]]* %[[V19]]

// CHECK: landingpad { i8*, i32 }

// CHECK: landingpad { i8*, i32 }
// CHECK: %[[V37:.*]] = ptrtoint %[[STRUCT_S0]]* %[[R0]] to i64
// CHECK: %[[V38:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V37]], i32 1, i64 100)
// CHECK: %[[V39:.*]] = inttoptr i64 %[[V38]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0D1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V39]])

// CHECK: %[[V42:.*]] = ptrtoint %[[STRUCT_S0]]* %[[T0]] to i64
// CHECK: %[[V43:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V42]], i32 1, i64 100)
// CHECK: %[[V44:.*]] = inttoptr i64 %[[V43]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0D1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V44]])

S0 &func0(S0 &);

S0 &test_reference0(S0 &a) {
  S0 t0 = a;
  S0 &t1 = a;
  S0 r0 = func0(t0);
  S0 &r1 = func0(t0);
  return gs0;
}

// CHECK: define {{.*}} void @_Z17test_conditional0bR2S0S0_(%[[STRUCT_S0]]* noalias sret(%struct.S0) align 4 %[[AGG_RESULT:.*]], i1 %{{.*}}, %[[STRUCT_S0]]* nonnull align 4 dereferenceable(24) %{{.*}}, %[[STRUCT_S0]]* nonnull align 4 dereferenceable(24) %{{.*}})
// CHECK: %[[A_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[B_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8

// CHECK: %[[V5:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[A_ADDR]], align 8

// CHECK: %[[V6:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[B_ADDR]], align 8

// CHECK: %[[V7:.*]] = phi %[[STRUCT_S0]]* [ %[[V5]], %{{.*}} ], [ %[[V6]], %{{.*}} ]
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0C1ERKS_(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[AGG_RESULT]], %[[STRUCT_S0]]* nonnull align 4 dereferenceable(24) %[[V7]])
// CHECK: ret void

S0 test_conditional0(bool c, S0 &a, S0 &b) {
  return c ? a : b;
}

// CHECK: define {{.*}} i32 @_Z17test_conditional1b(
// CHECK: %[[A:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[B:.*]] = alloca %[[STRUCT_S0]], align 4

// CHECK: %[[V9:.*]] = phi %[[STRUCT_S0]]* [ %[[A]], %{{.*}} ], [ %[[B]], %{{.*}} ]
// CHECK: %[[F1:.*]] = getelementptr inbounds %[[STRUCT_S0]], %[[STRUCT_S0]]* %[[V9]], i32 0, i32 1
// CHECK: %[[V10:.*]] = load i32, i32* %[[F1]], align 4
// CHECK: ret i32 %[[V10]]

int test_conditional1(bool c) {
  S0 a, b;
  return (c ? a : b).f1;
}

// CHECK: define {{.*}} void @_Z17test_conditional2bR2S2R2S3(%[[STRUCT_S0]]* noalias sret(%struct.S0) align 4 %[[AGG_RESULT]], i1 %{{.*}}, %[[STRUCT_S2]]* nonnull align 4 dereferenceable(28) %{{.*}}, %[[STRUCT_S3]]* nonnull align 4 dereferenceable(32) %{{.*}})
// CHECK: %[[A_ADDR:.*]] = alloca %[[STRUCT_S2]]*, align 8
// CHECK: %[[B_ADDR:.*]] = alloca %[[STRUCT_S3]]*, align 8

// CHECK: %[[V5:.*]] = load %[[STRUCT_S2]]*, %[[STRUCT_S2]]** %[[A_ADDR]], align 8
// CHECK: %[[V6:.*]] = ptrtoint %[[STRUCT_S2]]* %[[V5]] to i64
// CHECK: %[[V7:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V6]], i32 1, i64 102)
// CHECK: %[[V8:.*]] = inttoptr i64 %[[V7]] to %[[STRUCT_S2]]*
// CHECK: %[[V9:.*]] = bitcast %[[STRUCT_S2]]* %[[V8]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V9]], i64 4
// CHECK: %[[V10:.*]] = ptrtoint i8* %[[RESIGNEDGEP]] to i64
// CHECK: %[[V11:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V10]], i32 1, i64 100)
// CHECK: %[[V12:.*]] = inttoptr i64 %[[V11]] to i8*
// CHECK: %[[V13:.*]] = bitcast i8* %[[V12]] to %[[STRUCT_S0]]*

// CHECK: %[[V14:.*]] = load %[[STRUCT_S3]]*, %[[STRUCT_S3]]** %[[B_ADDR]], align 8
// CHECK: %[[F1:.*]] = getelementptr inbounds %[[STRUCT_S3]], %[[STRUCT_S3]]* %[[V14]], i32 0, i32 1
// CHECK: %[[V15:.*]] = ptrtoint %[[STRUCT_S0]]* %[[F1]] to i64
// CHECK: %[[V16:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V15]], i32 1, i64 100)
// CHECK: %[[V17:.*]] = inttoptr i64 %[[V16]] to %[[STRUCT_S0]]*

// CHECK: %[[V18:.*]] = phi %[[STRUCT_S0]]* [ %[[V13]], %{{.*}} ], [ %[[V17]], %{{.*}} ]
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0C1ERKS_(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[AGG_RESULT]], %[[STRUCT_S0]]* nonnull align 4 dereferenceable(24) %[[V18]])
// CHECK: ret void

S0 test_conditional2(bool c, S2 &a, S3 &b) {
  return c ? a.f1 : b.f1;
}

// CHECK: define {{.*}} void @_Z15test_exception0v()
// CHECK: %[[S0:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[I:.*]] = alloca i32, align 4
// CHECK: %[[EXCEPTION:.*]] = call i8* @__cxa_allocate_exception(i64 24)
// CHECK: %[[V0:.*]] = bitcast i8* %[[EXCEPTION]] to %[[STRUCT_S0]]*
// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V0]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V1]], i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: invoke %[[STRUCT_S0]]* @_ZN2S0C1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V3]])

// CHECK: invoke void @__cxa_throw(i8* %[[EXCEPTION]], i8* bitcast ({ i8*, i8* }* @_ZTI2S0 to i8*), i8* bitcast ({ i8*, i32, i64, i64 }* @_ZN2S0D1Ev.ptrauth to i8*))

// CHECK: landingpad { i8*, i32 }
// CHECK: call void @__cxa_free_exception(i8* %[[EXCEPTION]])

// CHECK: %[[V12:.*]] = call i8* @__cxa_begin_catch(
// CHECK: %[[V13:.*]] = ptrtoint i8* %[[V12]] to i64
// CHECK: %[[V14:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V13]], i32 1, i64 100)
// CHECK: %[[V15:.*]] = inttoptr i64 %[[V14]] to i8*
// CHECK: %[[EXN_BYREF:.*]] = bitcast i8* %[[V15]] to %[[STRUCT_S0]]*
// CHECK: store %[[STRUCT_S0]]* %[[EXN_BYREF]], %[[STRUCT_S0]]** %[[S0]], align 8
// CHECK: %[[V17:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[S0]], align 8
// CHECK: %[[V18:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V17]] to i64
// CHECK: %[[V19:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V18]], i32 1, i64 100)
// CHECK: %[[V20:.*]] = inttoptr i64 %[[V19]] to %[[STRUCT_S0]]*
// CHECK: %[[V21:.*]] = bitcast %[[STRUCT_S0]]* %[[V20]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V21]], i64 4
// CHECK: %[[V22:.*]] = bitcast i8* %[[RESIGNEDGEP]] to i32*
// CHECK: %[[V23:.*]] = load i32, i32* %[[V22]], align 4
// CHECK: store i32 %[[V23]], i32* %[[I]], align 4

void test_exception0() {
  try {
    throw S0();
  } catch (const S0 &s0) {
    int i = s0.f1;
  }
}

// CHECK: define {{.*}} void @_Z15test_exception1v()
// CHECK: %[[S0:.*]] = alloca %[[STRUCT_S0]], align 4
// CHECK: %[[I:.*]] = alloca i32, align 4
// CHECK: %[[EXCEPTION:.*]] = call i8* @__cxa_allocate_exception(i64 24)
// CHECK: %[[V0:.*]] = bitcast i8* %[[EXCEPTION]] to %[[STRUCT_S0]]*
// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V0]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V1]], i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: invoke %[[STRUCT_S0]]* @_ZN2S0C1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24)  %[[V3]])

// CHECK: %[[V12:.*]] = call i8* @__cxa_get_exception_ptr(
// CHECK: %[[V13:.*]] = bitcast i8* %[[V12]] to %[[STRUCT_S0]]*
// CHECK: %[[V14:.*]] = ptrtoint %[[STRUCT_S0]]* %[[S0]] to i64
// CHECK: %[[V15:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V14]], i32 1, i64 100)
// CHECK: %[[V16:.*]] = inttoptr i64 %[[V15]] to %[[STRUCT_S0]]*
// CHECK: %[[V17:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V13]] to i64
// CHECK: %[[V18:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V17]], i32 1, i64 100)
// CHECK: %[[V19:.*]] = inttoptr i64 %[[V18]] to %[[STRUCT_S0]]*
// CHECK: invoke %[[STRUCT_S0]]* @_ZN2S0C1ERKS_(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V16]], %[[STRUCT_S0]]* nonnull align 4 dereferenceable(24) %[[V19]])

// CHECK: %[[V20:.*]] = call i8* @__cxa_begin_catch(
// CHECK: %[[F1:.*]] = getelementptr inbounds %[[STRUCT_S0]], %[[STRUCT_S0]]* %[[S0]], i32 0, i32 1
// CHECK: %[[V22:.*]] = load i32, i32* %[[F1]], align 4
// CHECK: store i32 %[[V22]], i32* %[[I]], align 4
// CHECK: %[[V24:.*]] = ptrtoint %[[STRUCT_S0]]* %[[S0]] to i64
// CHECK: %[[V25:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V24]], i32 1, i64 100)
// CHECK: %[[V26:.*]] = inttoptr i64 %[[V25]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0D1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V26]])

void test_exception1() {
  try {
    throw S0();
  } catch (S0 s0) {
    int i = s0.f1;
  }
}

// CHECK: define {{.*}} void @_Z15test_exception2v()
// CHECK: %[[S0:.*]] = alloca %[[STRUCT_S0]]**, align 8
// CHECK: %[[EXN_BYREF_TMP:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[I:.*]] = alloca i32, align 4
// CHECK: %[[EXCEPTION:.*]] = call i8* @__cxa_allocate_exception(i64 24)
// CHECK: %[[V0:.*]] = bitcast i8* %[[EXCEPTION]] to %[[STRUCT_S0]]*
// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V0]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V1]], i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: invoke %[[STRUCT_S0]]* @_ZN2S0C1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V3]])

// CHECK: %[[V12:.*]] = call i8* @__cxa_begin_catch(
// CHECK: %[[V13:.*]] = ptrtoint i8* %[[V12]] to i64
// CHECK: %[[V14:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V13]], i32 1, i64 100)
// CHECK: %[[V15:.*]] = inttoptr i64 %[[V14]] to i8*
// CHECK: %[[V16:.*]] = bitcast i8* %[[V15]] to %[[STRUCT_S0]]*
// CHECK: store %[[STRUCT_S0]]* %[[V16]], %[[STRUCT_S0]]** %[[EXN_BYREF_TMP]], align 8
// CHECK: store %[[STRUCT_S0]]** %[[EXN_BYREF_TMP]], %[[STRUCT_S0]]*** %[[S0]], align 8
// CHECK: %[[V18:.*]] = load %[[STRUCT_S0]]**, %[[STRUCT_S0]]*** %[[S0]], align 8
// CHECK: %[[V19:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[V18]], align 8
// CHECK: %[[V20:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V19]] to i64
// CHECK: %[[V21:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V20]], i32 1, i64 100)
// CHECK: %[[V22:.*]] = inttoptr i64 %[[V21]] to %[[STRUCT_S0]]*
// CHECK: %[[V23:.*]] = bitcast %[[STRUCT_S0]]* %[[V22]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V23]], i64 4
// CHECK: %[[V24:.*]] = bitcast i8* %[[RESIGNEDGEP]] to i32*
// CHECK: %[[V25:.*]] = load i32, i32* %[[V24]], align 4
// CHECK: store i32 %[[V25]], i32* %[[I]], align 4

void test_exception2() {
  try {
    throw S0();
  } catch (S0 *&s0) {
    int i = s0->f1;
  }
}

// CHECK: define linkonce_odr i32 @_ZN2S07lambda0Ei(
// CHECK: %[[THIS_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[I_ADDR:.*]] = alloca i32, align 4
// CHECK: %[[REF_TMP:.*]] = alloca %[[CLASS_ANON]], align 8
// CHECK: %[[THIS1:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[THIS_ADDR]], align 8
// CHECK: %[[V1:.*]] = getelementptr inbounds %[[CLASS_ANON]], %[[CLASS_ANON]]* %[[REF_TMP]], i32 0, i32 0
// CHECK: store %[[STRUCT_S0]]* %[[THIS1]], %[[STRUCT_S0]]** %[[V1]], align 8
// CHECK: %[[V2:.*]] = getelementptr inbounds %[[CLASS_ANON]], %[[CLASS_ANON]]* %[[REF_TMP]], i32 0, i32 1
// CHECK: store i32* %[[I_ADDR]], i32** %[[V2]], align 8
// CHECK: call i32 @_ZZN2S07lambda0EiENKUlvE_clEv(%[[CLASS_ANON]]* nonnull dereferenceable(16) %[[REF_TMP]])

void test_lambda0(S0 *a) {
  a->lambda0(1);
}

// CHECK: define linkonce_odr i32 @_ZN2S07lambda1Ei(
// CHECK: %[[THIS_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: %[[I_ADDR:.*]] = alloca i32, align 4
// CHECK: %[[REF_TMP:.*]] = alloca %[[CLASS_ANON_0]], align 4
// CHECK: %[[THIS1:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[THIS_ADDR]], align 8
// CHECK: %[[V1:.*]] = getelementptr inbounds %[[CLASS_ANON_0]], %[[CLASS_ANON_0]]* %[[REF_TMP]], i32 0, i32 0
// CHECK: %[[V2:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V1]] to i64
// CHECK: %[[V3:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V2]], i32 1, i64 100)
// CHECK: %[[V4:.*]] = inttoptr i64 %[[V3]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0C1ERKS_(%[[STRUCT_S0]]* nonnull dereferenceable(24)  %[[V4]], %[[STRUCT_S0]]* nonnull align 4 dereferenceable(24) %[[THIS1]])
// CHECK: %[[V5:.*]] = getelementptr inbounds %[[CLASS_ANON_0]], %[[CLASS_ANON_0]]* %[[REF_TMP]], i32 0, i32 1
// CHECK: %[[V6:.*]] = load i32, i32* %[[I_ADDR]], align 4
// CHECK: store i32 %[[V6]], i32* %[[V5]], align 4
// CHECK: %[[CALL2:.*]] = invoke i32 @_ZZN2S07lambda1EiENKUlvE_clEv(%[[CLASS_ANON_0]]* nonnull dereferenceable(28) %[[REF_TMP]])

// CHECK: call %[[CLASS_ANON_0]]* @_ZZN2S07lambda1EiENUlvE_D1Ev(%[[CLASS_ANON_0]]* nonnull dereferenceable(28) %[[REF_TMP]])
// CHECK: ret i32 %[[CALL2]]

// CHECK: landingpad { i8*, i32 }
// CHECK: call %[[CLASS_ANON_0]]* @_ZZN2S07lambda1EiENUlvE_D1Ev(%[[CLASS_ANON_0]]* nonnull dereferenceable(28) %[[REF_TMP]])

void test_lambda1(S0 *a) {
  a->lambda1(1);
}

// CHECK: define linkonce_odr %[[STRUCT_S0]]* @_ZN2S0C2Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[THIS:.*]])
// CHECK: %[[THIS_ADDR:.*]] = alloca %[[STRUCT_S0]]*, align 8
// CHECK: store %[[STRUCT_S0]]* %[[THIS]], %[[STRUCT_S0]]** %[[THIS_ADDR]], align 8
// CHECK: %[[THIS1:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[THIS_ADDR]], align 8
// CHECK: ret %[[STRUCT_S0]]* %[[THIS1]]

// CHECK: define linkonce_odr %[[STRUCT_S5]]* @_ZN2S5CI22S2E2S0(%[[STRUCT_S5]]* nonnull dereferenceable(28) %[[THIS:.*]], %[[STRUCT_S0]]* %[[V0:.*]])
// CHECK: %[[THIS_ADDR:.*]] = alloca %[[STRUCT_S5]]*, align 8
// CHECK: store %[[STRUCT_S5]]* %[[THIS]], %[[STRUCT_S5]]** %[[THIS_ADDR]], align 8
// CHECK: %[[THIS1:.*]] = load %[[STRUCT_S5]]*, %[[STRUCT_S5]]** %[[THIS_ADDR]], align 8
// CHECK: %[[V1:.*]] = bitcast %[[STRUCT_S5]]* %[[THIS1]] to %[[STRUCT_S2]]*
// CHECK: call %[[STRUCT_S2]]* @_ZN2S2C2E2S0(%[[STRUCT_S2]]* nonnull dereferenceable(28) %[[V1]], %[[STRUCT_S0]]* %[[V0]])
// CHECK: ret %[[STRUCT_S5]]* %[[THIS1]]

// CHECK: define linkonce_odr i32 @_ZZN2S07lambda0EiENKUlvE_clEv(
// CHECK: %[[THIS_ADDR:.*]] = alloca %[[CLASS_ANON]]*, align 8
// CHECK: store %[[CLASS_ANON]]* %[[THIS]], %[[CLASS_ANON]]** %[[THIS_ADDR]], align 8
// CHECK: %[[THIS1:.*]] = load %[[CLASS_ANON]]*, %[[CLASS_ANON]]** %[[THIS_ADDR]], align 8
// CHECK: %[[V0:.*]] = getelementptr inbounds %[[CLASS_ANON]], %[[CLASS_ANON]]* %[[THIS1]], i32 0, i32 0
// CHECK: %[[V1:.*]] = load %[[STRUCT_S0]]*, %[[STRUCT_S0]]** %[[V0]], align 8
// CHECK: %[[V2:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V1]] to i64
// CHECK: %[[V3:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V2]], i32 1, i64 100)
// CHECK: %[[V4:.*]] = inttoptr i64 %[[V3]] to %[[STRUCT_S0]]*
// CHECK: %[[V5:.*]] = bitcast %[[STRUCT_S0]]* %[[V4]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V5]], i64 4
// CHECK: %[[V6:.*]] = bitcast i8* %[[RESIGNEDGEP]] to i32*
// CHECK: %[[V7:.*]] = load i32, i32* %[[V6]], align 4
// CHECK: %[[V8:.*]] = getelementptr inbounds %[[CLASS_ANON]], %[[CLASS_ANON]]* %[[THIS1]], i32 0, i32 1
// CHECK: %[[V9:.*]] = load i32*, i32** %[[V8]], align 8
// CHECK: %[[V10:.*]] = load i32, i32* %[[V9]], align 4
// CHECK: %[[ADD:.*]] = add nsw i32 %[[V7]], %[[V10]]
// CHECK: ret i32 %[[ADD]]

// CHECK: define linkonce_odr i32 @_ZZN2S07lambda1EiENKUlvE_clEv(
// CHECK: %[[THIS_ADDR:.*]] = alloca %[[CLASS_ANON_0]]*, align 8
// CHECK: store %[[CLASS_ANON_0]]* %[[THIS]], %[[CLASS_ANON_0]]** %[[THIS_ADDR]], align 8
// CHECK: %[[THIS1:.*]] = load %[[CLASS_ANON_0]]*, %[[CLASS_ANON_0]]** %[[THIS_ADDR]], align 8
// CHECK: %[[V0:.*]] = getelementptr inbounds %[[CLASS_ANON_0]], %[[CLASS_ANON_0]]* %[[THIS1]], i32 0, i32 0
// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V0]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V1]], i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: %[[V4:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V3]] to i64
// CHECK: %[[V5:.*]] = call i64 @llvm.ptrauth.auth(i64 %[[V4]], i32 1, i64 100)
// CHECK: %[[V6:.*]] = inttoptr i64 %[[V5]] to %[[STRUCT_S0]]*
// CHECK: %[[V7:.*]] = bitcast %[[STRUCT_S0]]* %[[V6]] to i8*
// CHECK: %[[RESIGNEDGEP:.*]] = getelementptr i8, i8* %[[V7]], i64 4
// CHECK: %[[V8:.*]] = bitcast i8* %[[RESIGNEDGEP]] to i32*
// CHECK: %[[V9:.*]] = load i32, i32* %[[V8]], align 4
// CHECK: %[[V10:.*]] = getelementptr inbounds %[[CLASS_ANON_0]], %[[CLASS_ANON_0]]* %[[THIS1]], i32 0, i32 1
// CHECK: %[[V11:.*]] = load i32, i32* %[[V10]], align 4
// CHECK: %[[ADD:.*]] = add nsw i32 %[[V9]], %[[V11]]
// CHECK: ret i32 %[[ADD]]

// CHECK: define linkonce_odr %[[CLASS_ANON_0]]* @_ZZN2S07lambda1EiENUlvE_D1Ev(%[[CLASS_ANON_0]]* nonnull dereferenceable(28) %[[THIS]]) unnamed_addr
// CHECK: %[[THIS_ADDR:.*]] = alloca %[[CLASS_ANON_0]]*, align 8
// CHECK: %[[THIS1:.*]] = load %[[CLASS_ANON_0]]*, %[[CLASS_ANON_0]]** %[[THIS_ADDR]], align 8
// CHECK: [[CLASS_ANON_0]]* @_ZZN2S07lambda1EiENUlvE_D2Ev(%[[CLASS_ANON_0]]* nonnull dereferenceable(28) %[[THIS1]])

// CHECK: define linkonce_odr %[[CLASS_ANON_0]]* @_ZZN2S07lambda1EiENUlvE_D2Ev(%[[CLASS_ANON_0]]* nonnull dereferenceable(28) %[[THIS]]) unnamed_addr
// CHECK: %[[THIS_ADDR:.*]] = alloca %[[CLASS_ANON_0]]*, align 8
// CHECK: %[[THIS1:.*]] = load %[[CLASS_ANON_0]]*, %[[CLASS_ANON_0]]** %[[THIS_ADDR]], align 8
// CHECK: %[[V0:.*]] = getelementptr inbounds %[[CLASS_ANON_0]], %[[CLASS_ANON_0]]* %[[THIS1]], i32 0, i32 0
// CHECK: %[[V1:.*]] = ptrtoint %[[STRUCT_S0]]* %[[V0]] to i64
// CHECK: %[[V2:.*]] = call i64 @llvm.ptrauth.sign(i64 %[[V1]], i32 1, i64 100)
// CHECK: %[[V3:.*]] = inttoptr i64 %[[V2]] to %[[STRUCT_S0]]*
// CHECK: call %[[STRUCT_S0]]* @_ZN2S0D1Ev(%[[STRUCT_S0]]* nonnull dereferenceable(24) %[[V3]])
