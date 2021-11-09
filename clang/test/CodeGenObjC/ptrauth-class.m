// RUN: %clang_cc1 -Wno-everything -fblocks -fptrauth-intrinsics -triple arm64-apple-ios -fobjc-runtime=ios-12.2 -emit-llvm -fobjc-arc -O2 -disable-llvm-passes -o - %s | FileCheck %s

#if __has_feature(ptrauth_objc_signable_class)
struct TestStruct {
  __ptrauth(2, 1, 1234) Class isa;
};

@interface TestClass {
@public
  __ptrauth(2, 1, 1234) Class isa;
}
@end

struct TestConstStruct {
  __ptrauth(2, 1, 1234) const Class isa;
  __ptrauth(2, 1, 1234) volatile Class visa;
};

@interface TestConstClass {
@public
  __ptrauth(2, 1, 1234) const Class isa;
  __ptrauth(2, 1, 1234) volatile Class visa;
}
@end

// CHECK-LABEL: define {{.*}} void @setTestStructIsa(%struct.TestStruct* %t, i8* %c) #0 {
void setTestStructIsa(struct TestStruct *t, Class c) {
  t->isa = c;
  // CHECK: [[C_ADDR:%.*]] = alloca i8*, align 8
  // CHECK: store i8* %c, i8** [[C_ADDR]], align 8
  // CHECK: [[ISA_SLOT:%.*]] = getelementptr inbounds %struct.TestStruct, %struct.TestStruct* %0, i32 0, i32 0
  // CHECK: [[C:%.*]] = load i8*, i8** %c.addr, align 8
  // CHECK: [[CAST_ISA_SLOT:%.*]] = ptrtoint i8** [[ISA_SLOT]] to i64
  // CHECK: [[BLENDED_VALUE:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[CAST_ISA_SLOT]], i64 1234)
  // CHECK: [[CAST_C:%.*]] = ptrtoint i8* [[C]] to i64
  // CHECK: [[AUTHENTICATED:%.*]] = call i64 @llvm.ptrauth.sign(i64 [[CAST_C]], i32 2, i64 [[BLENDED_VALUE]])
}

// CHECK-LABEL: define {{.*}} void @setTestClassIsa(%0* %t, i8* %c) #0 {
void setTestClassIsa(TestClass *t, Class c) {
  t->isa = c;
  // CHECK: [[T_ADDR:%.*]] = alloca %0*, align 8
  // CHECK: [[C_ADDR:%.*]] = alloca i8*, align 8
  // CHECK: store i8* %c, i8** [[C_ADDR]], align 8
  // CHECK: [[T:%.*]] = load %0*, %0** [[T_ADDR]], align 8
  // CHECK: [[IVAR_OFFSET32:%.*]] = load i32, i32* @"OBJC_IVAR_$_TestClass.isa", align 8
  // CHECK: [[IVAR_OFFSET64:%.*]] = sext i32 [[IVAR_OFFSET32]] to i64
  // CHECK: [[CAST_T:%.*]] = bitcast %0* [[T]] to i8*
  // CHECK: [[ADDED_PTR:%.*]] = getelementptr inbounds i8, i8* %4, i64 [[IVAR_OFFSET64]]
  // CHECK: [[ISA_SLOT:%.*]] = bitcast i8* [[ADDED_PTR]] to i8**
  // CHECK: [[C_VALUE:%.*]] = load i8*, i8** [[C_ADDR]], align 8
  // CHECK: [[CAST_ISA_SLOT:%.*]] = ptrtoint i8** [[ISA_SLOT]] to i64
  // CHECK: [[BLENDED_VALUE:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[CAST_ISA_SLOT]], i64 1234)
  // CHECK: [[CAST_C_VALUE:%.*]] = ptrtoint i8* %6 to i64
  // CHECK: [[SIGNED:%.*]] = call i64 @llvm.ptrauth.sign(i64 [[CAST_C_VALUE]], i32 2, i64 [[BLENDED_VALUE]])
}

// CHECK-LABEL: define {{.*}} i8* @getTestStructIsa(%struct.TestStruct* %t) #0 {
Class getTestStructIsa(struct TestStruct *t) {
  return t->isa;
  // CHECK: [[T_ADDR:%.*]] = alloca %struct.TestStruct*, align 8
  // CHECK: [[T_VALUE:%.*]] = load %struct.TestStruct*, %struct.TestStruct** [[T_ADDR]], align 8
  // CHECK: [[ISA_SLOT:%.*]] = getelementptr inbounds %struct.TestStruct, %struct.TestStruct* [[T_VALUE]], i32 0, i32 0
  // CHECK: [[ISA_VALUE:%.*]] = load i8*, i8** [[ISA_SLOT]], align 8
  // CHECK: [[CAST_ISA_SLOT:%.*]] = ptrtoint i8** %isa to i64
  // CHECK: [[BLENDED_VALUE:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[CAST_ISA_SLOT]], i64 1234)
  // CHECK: [[CAST_ISA_VALUE:%.*]] = ptrtoint i8* [[ISA_VALUE]] to i64
  // CHECK: [[SIGNED_VALUE:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[CAST_ISA_VALUE]], i32 2, i64 [[BLENDED_VALUE]])
}

// CHECK-LABEL: define {{.*}} i8* @getTestClassIsa(%0* %t) #0 {
Class getTestClassIsa(TestClass *t) {
  return t->isa;
  // CHECK: [[T_ADDR:%.*]] = alloca %0*, align 8
  // CHECK: [[T_VALUE:%.*]] = bitcast %0* %t to i8*
  // CHECK: [[T:%.*]] = load %0*, %0** [[T_ADDR]], align 8
  // CHECK: [[IVAR:%.*]] = load i32, i32* @"OBJC_IVAR_$_TestClass.isa", align 8
  // CHECK: [[IVAR_CONV:%.*]] = sext i32 [[IVAR]] to i64
  // CHECK: [[CAST_T:%.*]] = bitcast %0* [[T]] to i8*
  // CHECK: [[ADD_PTR:%.*]] = getelementptr inbounds i8, i8* %4, i64 %ivar.conv
  // CHECK: [[ADD_PTR_CAST:%.*]] = bitcast i8* [[ADD_PTR]] to i8**
  // CHECK: [[LOADED_VALUE:%.*]] = load i8*, i8** [[ADD_PTR_CAST]], align 8
  // CHECK: [[INT_VALUE:%.*]] = ptrtoint i8** [[ADD_PTR_CAST]] to i64
  // CHECK: [[BLENDED_VALUE:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[INT_VALUE]], i64 1234)
  // CHECK: [[NULL_CHECK:%.*]] = icmp ne i8* [[LOADED_VALUE]], null
  // CHECK: [[CAST_VALUE:%.*]] = ptrtoint i8* [[LOADED_VALUE]] to i64
  // CHECK: [[AUTHED:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[CAST_VALUE]], i32 2, i64 [[BLENDED_VALUE]])
}

// Just enough to verify we do actually authenticate qualified Class
// CHECK: define {{.*}} i8* @getTestConstClassIsa([[T_TYPE:%.*]]* %t) #0 {
Class getTestConstClassIsa(TestConstClass *t) {
  return t->isa;
  // CHECK: [[T_ADDR:%.*]] = alloca [[T_TYPE]]*, align 8
  // CHECK: [[T_VALUE:%.*]] = bitcast [[T_TYPE]]* %t to i8*
  // CHECK: [[T:%.*]] = load [[T_TYPE]]*, [[T_TYPE]]** [[T_ADDR]], align 8
  // CHECK: [[IVAR:%.*]] = load i32, i32* @"OBJC_IVAR_$_TestConstClass.isa", align 8
  // CHECK: [[IVAR_CONV:%.*]] = sext i32 [[IVAR]] to i64
  // CHECK: [[CAST_T:%.*]] = bitcast [[T_TYPE]]* [[T]] to i8*
  // CHECK: [[ADD_PTR:%.*]] = getelementptr inbounds i8, i8* %4, i64 %ivar.conv
  // CHECK: [[ADD_PTR_CAST:%.*]] = bitcast i8* [[ADD_PTR]] to i8**
  // CHECK: [[LOADED_VALUE:%.*]] = load i8*, i8** [[ADD_PTR_CAST]], align 8
  // CHECK: [[INT_VALUE:%.*]] = ptrtoint i8** [[ADD_PTR_CAST]] to i64
  // CHECK: [[BLENDED_VALUE:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[INT_VALUE]], i64 1234)
  // CHECK: [[NULL_CHECK:%.*]] = icmp ne i8* [[LOADED_VALUE]], null
  // CHECK: [[CAST_VALUE:%.*]] = ptrtoint i8* [[LOADED_VALUE]] to i64
  // CHECK: [[AUTHED:%.*]] = call i64 @llvm.ptrauth.auth(i64 [[CAST_VALUE]], i32 2, i64 [[BLENDED_VALUE]])
}

#endif
