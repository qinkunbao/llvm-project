// RUN: %clang_cc1 -no-opaque-pointers -I %S/Inputs -fptrauth-calls -fptrauth-objc-isa-mode=sign-and-strip -triple arm64-apple-ios -fobjc-runtime=ios-12.2 -emit-llvm -fblocks -fobjc-arc -fobjc-runtime-has-weak -O2 -disable-llvm-passes -o - %s | FileCheck %s
// RUN: %clang_cc1 -no-opaque-pointers -I %S/Inputs -fptrauth-calls -fptrauth-objc-isa-mode=sign-and-auth  -triple arm64-apple-ios -fobjc-runtime=ios-12.2 -emit-llvm -fblocks -fobjc-arc -fobjc-runtime-has-weak -O2 -disable-llvm-passes -o - %s | FileCheck %s

#include "literal-support.h"

#if __has_feature(objc_bool)
#define YES __objc_yes
#define NO __objc_no
#else
#define YES ((BOOL)1)
#define NO ((BOOL)0)
#endif

@class NSString;

// CHECK: @"OBJC_METACLASS_$_C" = global %struct._class_t { %struct._class_t* bitcast ({ i8*, i32, i64, i64 }* @"OBJC_METACLASS_$_Base.ptrauth" to %struct._class_t*), %struct._class_t* bitcast ({ i8*, i32, i64, i64 }* @"OBJC_METACLASS_$_Base.ptrauth.2" to %struct._class_t*), %struct._objc_cache* @_objc_empty_cache, i8* (i8*, i8*)** null, %struct._class_ro_t* @"_OBJC_METACLASS_RO_$_C" }, section "__DATA, __objc_data", align 8
// CHECK: @"OBJC_CLASSLIST_SUP_REFS_$_" = private global %struct._class_t* @"OBJC_METACLASS_$_C", section "__DATA,__objc_superrefs,regular,no_dead_strip", align 8
// CHECK: @"OBJC_METACLASS_$_Base" = external global %struct._class_t
// CHECK: @"\01+[C super_test].ptrauth" = private constant { i8*, i32, i64, i64 } { i8* bitcast (void (i8*, i8*)* @"\01+[C super_test]" to i8*), i32 0, i64 ptrtoint (i8** getelementptr inbounds ({ i32, i32, [1 x %struct._objc_method] }, { i32, i32, [1 x %struct._objc_method] }* @"_OBJC_$_CLASS_METHODS_C", i32 0, i32 2, i32 0, i32 2) to i64), i64 0 }, section "llvm.ptrauth", align 8
// CHECK: @"_OBJC_$_CLASS_METHODS_C" = internal global { i32, i32, [1 x %struct._objc_method] } { i32 24, i32 1, [1 x %struct._objc_method] [%struct._objc_method { i8* getelementptr inbounds ([11 x i8], [11 x i8]* @OBJC_METH_VAR_NAME_.1, i32 0, i32 0), i8* getelementptr inbounds ([8 x i8], [8 x i8]* @OBJC_METH_VAR_TYPE_, i32 0, i32 0), i8* bitcast ({ i8*, i32, i64, i64 }* @"\01+[C super_test].ptrauth" to i8*) }] }, section "__DATA, __objc_const", align 8
// CHECK: @"_OBJC_METACLASS_RO_$_C" = internal global %struct._class_ro_t { i32 129, i32 40, i32 40, i8* null, i8* getelementptr inbounds ([2 x i8], [2 x i8]* @OBJC_CLASS_NAME_, i32 0, i32 0), %struct.__method_list_t* bitcast ({ i8*, i32, i64, i64 }* @"_OBJC_$_CLASS_METHODS_C.ptrauth" to %struct.__method_list_t*), %struct._objc_protocol_list* null, %struct._ivar_list_t* null, i8* null, %struct._prop_list_t* null }, section "__DATA, __objc_const", align 8
// CHECK: @"OBJC_METACLASS_$_Base.ptrauth" = private constant { i8*, i32, i64, i64 } { i8* bitcast (%struct._class_t* @"OBJC_METACLASS_$_Base" to i8*), i32 2, i64 ptrtoint (%struct._class_t* @"OBJC_METACLASS_$_C" to i64), i64 27361 }, section "llvm.ptrauth", align 8
// CHECK: @"OBJC_METACLASS_$_Base.ptrauth.2" = private constant { i8*, i32, i64, i64 } { i8* bitcast (%struct._class_t* @"OBJC_METACLASS_$_Base" to i8*), i32 2, i64 ptrtoint (%struct._class_t** getelementptr inbounds (%struct._class_t, %struct._class_t* @"OBJC_METACLASS_$_C", i32 0, i32 1) to i64), i64 46507 }, section "llvm.ptrauth", align 8
// CHECK: @"OBJC_CLASS_$_Base" = external global %struct._class_t
// CHECK: @"_OBJC_CLASS_RO_$_C" = internal global %struct._class_ro_t { i32 128, i32 0, i32 0, i8* null, i8* getelementptr inbounds ([2 x i8], [2 x i8]* @OBJC_CLASS_NAME_, i32 0, i32 0), %struct.__method_list_t* null, %struct._objc_protocol_list* null, %struct._ivar_list_t* null, i8* null, %struct._prop_list_t* null }, section "__DATA, __objc_const", align 8
// CHECK: @"OBJC_METACLASS_$_C.ptrauth" = private constant { i8*, i32, i64, i64 } { i8* bitcast (%struct._class_t* @"OBJC_METACLASS_$_C" to i8*), i32 2, i64 ptrtoint (%struct._class_t* @"OBJC_CLASS_$_C" to i64), i64 27361 }, section "llvm.ptrauth", align 8
// CHECK: @"OBJC_CLASS_$_Base.ptrauth" = private constant { i8*, i32, i64, i64 } { i8* bitcast (%struct._class_t* @"OBJC_CLASS_$_Base" to i8*), i32 2, i64 ptrtoint (%struct._class_t** getelementptr inbounds (%struct._class_t, %struct._class_t* @"OBJC_CLASS_$_C", i32 0, i32 1) to i64), i64 46507 }, section "llvm.ptrauth", align 8
// CHECK: @"OBJC_CLASS_$_C" = global %struct._class_t { %struct._class_t* bitcast ({ i8*, i32, i64, i64 }* @"OBJC_METACLASS_$_C.ptrauth" to %struct._class_t*), %struct._class_t* bitcast ({ i8*, i32, i64, i64 }* @"OBJC_CLASS_$_Base.ptrauth" to %struct._class_t*), %struct._objc_cache* @_objc_empty_cache, i8* (i8*, i8*)** null, %struct._class_ro_t* @"_OBJC_CLASS_RO_$_C" }, section "__DATA, __objc_data", align 8

@interface Base
+ (void)test;
@end

@interface C : Base
@end

@implementation C
// CHECK-LABEL: define internal void @"\01+[C super_test]"(i8* %self, i8* %_cmd) #1 {
+ (void)super_test {
  return [super test];
  // CHECK: [[SELF_ADDR:%.*]] = alloca i8*, align 8
  // CHECK: [[CMD_ADDR:%.*]] = alloca i8*, align 8
  // CHECK: [[SUPER_STRUCT:%.*]] = alloca %struct._objc_super, align 8
  // CHECK: store i8* %self, i8** [[SELF_ADDR]], align 8, !tbaa !11
  // CHECK: store i8* %_cmd, i8** [[CMD_ADDR]], align 8, !tbaa !14
  // CHECK: [[TARGET:%.*]] = load i8*, i8** [[SELF_ADDR]], align 8, !tbaa !11
  // CHECK: [[OBJC_SUPER_TARGET:%.*]] = getelementptr inbounds %struct._objc_super, %struct._objc_super* [[SUPER_STRUCT]], i32 0, i32 0
  // CHECK: store i8* [[TARGET]], i8** [[OBJC_SUPER_TARGET]], align 8
  // CHECK: [[SUPER_REFERENCES:%.*]] = load %struct._class_t*, %struct._class_t** @"OBJC_CLASSLIST_SUP_REFS_$_", align 8
  // CHECK: [[CAST_SR:%.*]] = bitcast %struct._class_t* [[SUPER_REFERENCES]] to i8*
  // CHECK: [[OBJC_SUPER_SUPER:%.*]] = getelementptr inbounds %struct._objc_super, %struct._objc_super* [[SUPER_STRUCT]], i32 0, i32 1
  // CHECK: store i8* [[CAST_SR]], i8** [[OBJC_SUPER_SUPER:%.*]], align 8
  // CHECK: call void bitcast (i8* (%struct._objc_super*, i8*, ...)* @objc_msgSendSuper2 to void (%struct._objc_super*, i8*)*)(%struct._objc_super* [[SUPER_STRUCT]], i8* %5), !clang.arc.no_objc_arc_exceptions !16
}
@end

id str = @"";
