// RUN: %clang_cc1 -fobjc-arc -fblocks -fptrauth-calls -fptrauth-block-descriptor-pointers -triple arm64e-apple-ios  -emit-llvm -o - %s | FileCheck %s

void a() {
  // Test out a global block.
  void (^blk)(void) = ^{};
}

// CHECK: @"__block_descriptor_32_e5_v8\01?0l" = linkonce_odr hidden unnamed_addr constant

// CHECK: @"__block_descriptor_32_e5_v8\01?0l.ptrauth" = private constant { i8*, i32, i64, i64 } {
// CHECK-SAME: i8* bitcast ({ {{.*}} }* @"__block_descriptor_32_e5_v8\01?0l" to i8*),
// CHECK-SAME: i32 2,
// CHECK-SAME: i64 ptrtoint (%struct.__block_descriptor** getelementptr inbounds ({ {{.*}} }, { {{.*}} }* @__block_literal_global, i32 0, i32 4) to i64),
// CHECK-SAME: i64 49339 }

// CHECK: @__block_literal_global = internal constant { i8**, i32, i32, i8*, %struct.__block_descriptor* } {
// CHECK-SAME: i8** @_NSConcreteGlobalBlock,
// CHECK-SAME: i32 1342177280
// CHECK-SAME: i32 0,
// CHECK-SAME: i8* bitcast ({ {{.*}} }* @__a_block_invoke.ptrauth to i8*),
// CHECK-SAME: %struct.__block_descriptor* bitcast ({ {{.*}} }* @"__block_descriptor_32_e5_v8\01?0l.ptrauth" to %struct.__block_descriptor*) }

void b(int p) {
  // CHECK-LABEL: define {{.*}} void @b

  // Test out a stack block.
  void (^blk)(void) = ^{(void)p;};

  // CHECK: [[BLOCK:%.*]] = alloca <{ i8*, i32, i32, i8*, %struct.__block_descriptor*, i32 }>
  // CHECK: [[BLOCK_DESCRIPTOR_REF:%.*]] = getelementptr inbounds <{ {{.*}} }>, <{ i8*, i32, i32, i8*, %struct.__block_descriptor*, i32 }>* [[BLOCK]], i32 0, i32 4
  // CHECK: [[BLOCK_DESCRIPTOR_REF_INT:%.*]] = ptrtoint %struct.__block_descriptor** [[BLOCK_DESCRIPTOR_REF]] to i64
  // CHECK: [[BLENDED:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[BLOCK_DESCRIPTOR_REF_INT]], i64 49339)
  // CHECK: [[SIGNED_REF:%.*]] = call i64 @llvm.ptrauth.sign(i64 ptrtoint ({ {{.*}} }* @"__block_descriptor_36_e5_v8\01?0l" to i64), i32 2, i64 [[BLENDED]])
  // CHECK: [[SIGNED_REF_PTR:%.*]] = inttoptr i64 [[SIGNED_REF]] to %struct.__block_descriptor*
  // CHECK: store %struct.__block_descriptor* [[SIGNED_REF_PTR]], %struct.__block_descriptor** [[BLOCK_DESCRIPTOR_REF]]
}
