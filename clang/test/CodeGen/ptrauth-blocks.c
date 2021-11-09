// RUN: %clang_cc1 -triple arm64-apple-ios -fptrauth-calls -fptrauth-intrinsics -fblocks -emit-llvm %s  -o - | FileCheck %s

void (^blockptr)(void);

// CHECK: [[INVOCATION_1:@.*]] =  private constant { i8*, i32, i64, i64 } { i8* bitcast (void (i8*)* {{@.*}} to i8*), i32 0, i64 ptrtoint (i8** getelementptr inbounds ({ i8**, i32, i32, i8*, %struct.__block_descriptor* }, { i8**, i32, i32, i8*, %struct.__block_descriptor* }* [[GLOBAL_BLOCK_1:@.*]], i32 0, i32 3) to i64), i64 0 }, section "llvm.ptrauth"
// CHECK: [[GLOBAL_BLOCK_1]] = internal constant { i8**, i32, i32, i8*, %struct.__block_descriptor* } { i8** @_NSConcreteGlobalBlock, i32 1342177280, i32 0, i8* bitcast ({ i8*, i32, i64, i64 }* [[INVOCATION_1]] to i8*),
void (^globalblock)(void) = ^{};

// CHECK-LABEL: define {{.*}} void @test_block_call()
void test_block_call() {
  // CHECK:      [[T0:%.*]] = load void ()*, void ()** @blockptr,
  // CHECK-NEXT: [[BLOCK:%.*]] = bitcast void ()* [[T0]] to [[BLOCK_T:%.*]]*{{$}}
  // CHECK-NEXT: [[FNADDR:%.*]] = getelementptr inbounds [[BLOCK_T]], [[BLOCK_T]]* [[BLOCK]], i32 0, i32 3
  // CHECK-NEXT: [[BLOCK_OPAQUE:%.*]] = bitcast [[BLOCK_T]]* [[BLOCK]] to i8*
  // CHECK-NEXT: [[T0:%.*]] = load i8*, i8** [[FNADDR]],
  // CHECK-NEXT: [[FNPTR:%.*]] = bitcast i8* [[T0]] to void (i8*)*
  // CHECK-NEXT: [[DISC:%.*]] = ptrtoint i8** [[FNADDR]] to i64
  // CHECK-NEXT: call void [[FNPTR]](i8* [[BLOCK_OPAQUE]]) [ "ptrauth"(i32 0, i64 [[DISC]]) ]
  blockptr();
}

void use_block(int (^)(void));

// CHECK-LABEL: define {{.*}} void @test_block_literal(
void test_block_literal(int i) {
  // CHECK:      [[I:%.*]] = alloca i32,
  // CHECK-NEXT: [[BLOCK:%.*]] = alloca [[BLOCK_T:.*]], align
  // CHECK:      [[FNPTRADDR:%.*]] = getelementptr inbounds [[BLOCK_T]], [[BLOCK_T]]* [[BLOCK]], i32 0, i32 3
  // CHECK-NEXT: [[DISCRIMINATOR:%.*]] = ptrtoint i8** [[FNPTRADDR]] to i64
  // CHECK-NEXT: [[SIGNED:%.*]] = call i64 @llvm.ptrauth.sign(i64 ptrtoint (i32 (i8*)* {{@.*}} to i64), i32 0, i64 [[DISCRIMINATOR]])
  // CHECK-NEXT: [[T0:%.*]] = inttoptr i64 [[SIGNED]] to i8*
  // CHECK-NEXT: store i8* [[T0]], i8** [[FNPTRADDR]]
  use_block(^{return i;});
}

struct A {
  int value;
};
struct A *createA(void);

// CHECK-LABEL: define {{.*}} void @test_block_nonaddress_capture(
void test_block_nonaddress_capture() {
  // CHECK: [[VAR:%.*]] = alloca %struct.A*,
  // CHECK: [[BLOCK:%.*]] = alloca
  //   flags - no copy/dispose required
  // CHECK: store i32 1073741824, i32*
  // CHECK: [[CAPTURE:%.*]] = getelementptr inbounds {{.*}} [[BLOCK]], i32 0, i32 5
  // CHECK: [[LOAD:%.*]] = load %struct.A*, %struct.A** [[VAR]],
  // CHECK: store %struct.A* [[LOAD]], %struct.A** [[CAPTURE]]
  struct A * __ptrauth(1, 0, 15) ptr = createA();
  use_block(^{ return ptr->value; });
}
// CHECK-LABEL: define internal i32 @__test_block_nonaddress_capture_block_invoke
// CHECK: call i64 @llvm.ptrauth.auth(i64 {{%.*}}, i32 1, i64 15)

// CHECK-LABEL: define {{.*}} void @test_block_address_capture(
void test_block_address_capture() {
  // CHECK: [[VAR:%.*]] = alloca %struct.A*,
  // CHECK: [[BLOCK:%.*]] = alloca
  //   flags - copy/dispose required
  // CHECK: store i32 1107296256, i32*
  // CHECK: [[CAPTURE:%.*]] = getelementptr inbounds {{.*}} [[BLOCK]], i32 0, i32 5
  // CHECK: [[LOAD:%.*]] = load %struct.A*, %struct.A** [[VAR]],
  // CHECK: [[T0:%.*]] = ptrtoint %struct.A** [[VAR]] to i64
  // CHECK: [[OLDDISC:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T0]], i64 30)
  // CHECK: [[T0:%.*]] = ptrtoint %struct.A** [[CAPTURE]] to i64
  // CHECK: [[NEWDISC:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T0]], i64 30)
  // CHECK: [[T0:%.*]] = icmp ne %struct.A* [[LOAD]], null
  // CHECK: br i1 [[T0]]
  // CHECK: [[T0:%.*]] = ptrtoint %struct.A* [[LOAD]] to i64
  // CHECK: [[T1:%.*]] = call i64 @llvm.ptrauth.resign(i64 [[T0]], i32 1, i64 [[OLDDISC]], i32 1, i64 [[NEWDISC]])
  // CHECK: [[T2:%.*]] = inttoptr i64 [[T1]] to %struct.A*
  // CHECK: [[T0:%.*]] = phi
  // CHECK: store %struct.A* [[T0]], %struct.A** [[CAPTURE]]
  struct A * __ptrauth(1, 1, 30) ptr = createA();
  use_block(^{ return ptr->value; });
}
// CHECK-LABEL: define internal i32 @__test_block_address_capture_block_invoke
// CHECK: call i64 @llvm.ptrauth.auth(i64 {{%.*}}, i32 1, i64 {{%.*}})

// CHECK: linkonce_odr hidden void @__copy_helper_block_8_32p1d30(
// CHECK: [[OLDSLOT:%.*]] = getelementptr inbounds {{.*}} {{.*}}, i32 0, i32 5
// CHECK: [[NEWSLOT:%.*]] = getelementptr inbounds {{.*}} {{.*}}, i32 0, i32 5
// CHECK: [[LOAD:%.*]] = load %struct.A*, %struct.A** [[OLDSLOT]],
// CHECK: [[T0:%.*]] = ptrtoint %struct.A** [[OLDSLOT]] to i64
// CHECK: [[OLDDISC:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T0]], i64 30)
// CHECK: [[T0:%.*]] = ptrtoint %struct.A** [[NEWSLOT]] to i64
// CHECK: [[NEWDISC:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T0]], i64 30)
// CHECK: [[T0:%.*]] = icmp ne %struct.A* [[LOAD]], null
// CHECK: br i1 [[T0]]
// CHECK: [[T0:%.*]] = ptrtoint %struct.A* [[LOAD]] to i64
// CHECK: [[T1:%.*]] = call i64 @llvm.ptrauth.resign(i64 [[T0]], i32 1, i64 [[OLDDISC]], i32 1, i64 [[NEWDISC]])
// CHECK: [[T2:%.*]] = inttoptr i64 [[T1]] to %struct.A*
// CHECK: [[T0:%.*]] = phi
// CHECK: store %struct.A* [[T0]], %struct.A** [[NEWSLOT]]

// CHECK-LABEL: define {{.*}} void @test_block_nonaddress_byref_capture(
void test_block_nonaddress_byref_capture() {
  //   flags - no copy/dispose required for byref
  // CHECK: store i32 0,
  // CHECK: call %struct.A* @createA()
  //   flags - copy/dispose required for block (because it captures byref)
  // CHECK: store i32 1107296256,
  __block struct A * __ptrauth(1, 0, 45) ptr = createA();
  use_block(^{ return ptr->value; });
}

// CHECK-LABEL: define {{.*}} void @test_block_address_byref_capture(
void test_block_address_byref_capture() {
  // CHECK: [[BYREF:%.*]] = alloca [[BYREF_T:.*]], align
  // CHECK: [[BLOCK:%.*]] = alloca
  //   flags - byref requires copy/dispose
  // CHECK: store i32 33554432,
  // CHECK: store i32 48,
  // CHECK: [[COPY_HELPER_FIELD:%.*]] = getelementptr inbounds [[BYREF_T]], [[BYREF_T]]* [[BYREF]], i32 0, i32 4
  // CHECK: [[T0:%.*]] = ptrtoint i8** [[COPY_HELPER_FIELD]] to i64
  // CHECK: [[T1:%.*]] = call i64 @llvm.ptrauth.sign(i64 ptrtoint (void (i8*, i8*)* @__Block_byref_object_copy_ to i64), i32 0, i64 [[T0]])
  // CHECK: [[T2:%.*]] = inttoptr i64 [[T1]] to i8*
  // CHECK: store i8* [[T2]], i8** [[COPY_HELPER_FIELD]], align
  // CHECK: [[DISPOSE_HELPER_FIELD:%.*]] = getelementptr inbounds [[BYREF_T]], [[BYREF_T]]* [[BYREF]], i32 0, i32 5
  // CHECK: [[T0:%.*]] = ptrtoint i8** [[DISPOSE_HELPER_FIELD]] to i64
  // CHECK: [[T1:%.*]] = call i64 @llvm.ptrauth.sign(i64 ptrtoint (void (i8*)* @__Block_byref_object_dispose_ to i64), i32 0, i64 [[T0]])
  // CHECK: [[T2:%.*]] = inttoptr i64 [[T1]] to i8*
  // CHECK: store i8* [[T2]], i8** [[DISPOSE_HELPER_FIELD]], align
  //   flags - copy/dispose required
  // CHECK: store i32 1107296256, i32*
  __block struct A * __ptrauth(1, 1, 60) ptr = createA();
  use_block(^{ return ptr->value; });
}
// CHECK-LABEL: define internal void @__Block_byref_object_copy_
// CHECK: [[NEWSLOT:%.*]] = getelementptr inbounds {{.*}} {{.*}}, i32 0, i32 6
// CHECK: [[OLDSLOT:%.*]] = getelementptr inbounds {{.*}} {{.*}}, i32 0, i32 6
// CHECK: [[LOAD:%.*]] = load %struct.A*, %struct.A** [[OLDSLOT]],
// CHECK: [[T0:%.*]] = ptrtoint %struct.A** [[OLDSLOT]] to i64
// CHECK: [[OLDDISC:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T0]], i64 60)
// CHECK: [[T0:%.*]] = ptrtoint %struct.A** [[NEWSLOT]] to i64
// CHECK: [[NEWDISC:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T0]], i64 60)
// CHECK: [[T0:%.*]] = icmp ne %struct.A* [[LOAD]], null
// CHECK: br i1 [[T0]]
// CHECK: [[T0:%.*]] = ptrtoint %struct.A* [[LOAD]] to i64
// CHECK: [[T1:%.*]] = call i64 @llvm.ptrauth.resign(i64 [[T0]], i32 1, i64 [[OLDDISC]], i32 1, i64 [[NEWDISC]])
// CHECK: [[T2:%.*]] = inttoptr i64 [[T1]] to %struct.A*
// CHECK: [[T0:%.*]] = phi
// CHECK: store %struct.A* [[T0]], %struct.A** [[NEWSLOT]]
