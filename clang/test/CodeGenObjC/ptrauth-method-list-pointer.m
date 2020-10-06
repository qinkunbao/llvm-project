// RUN: %clang_cc1 -fptrauth-calls -triple arm64e-apple-ios %s -S -emit-llvm -o - | FileCheck %s

@implementation X
-(void)meth {}
@end

// By default, we should be signing the method list pointer and not emitting
// relative method lists.

// CHECK: @"_OBJC_$_INSTANCE_METHODS_X" = internal global { i32, i32, [1 x %struct._objc_method] } { i32 24, i32 1, [1 x %struct._objc_method] [%struct._objc_method { i8* getelementptr inbounds ([5 x i8], [5 x i8]* @OBJC_METH_VAR_NAME_, i32 0, i32 0), i8* getelementptr inbounds ([8 x i8], [8 x i8]* @OBJC_METH_VAR_TYPE_, i32 0, i32 0), i8* bitcast ({ i8*, i32, i64, i64 }* @"\01-[X meth].ptrauth" to i8*) }] }, section "__DATA, __objc_const", align 8

// CHECK: @"_OBJC_$_INSTANCE_METHODS_X.ptrauth" = private constant { i8*, i32, i64, i64 } { i8* bitcast ({ i32, i32, [1 x %struct._objc_method] }* @"_OBJC_$_INSTANCE_METHODS_X" to i8*), i32 2, i64 ptrtoint (%struct.__method_list_t** getelementptr inbounds (%struct._class_ro_t, %struct._class_ro_t* @"_OBJC_CLASS_RO_$_X", i32 0, i32 5) to i64), i64 49936 }, section "llvm.ptrauth", align 8

// CHECK: @"_OBJC_CLASS_RO_$_X" = internal global %struct._class_ro_t { i32 2, i32 0, i32 0, i8* null, i8* getelementptr inbounds ([2 x i8], [2 x i8]* @OBJC_CLASS_NAME_, i32 0, i32 0), %struct.__method_list_t* bitcast ({ i8*, i32, i64, i64 }* @"_OBJC_$_INSTANCE_METHODS_X.ptrauth" to %struct.__method_list_t*), %struct._objc_protocol_list* null, %struct._ivar_list_t* null, i8* null, %struct._prop_list_t* null }, section "__DATA, __objc_const", align 8
