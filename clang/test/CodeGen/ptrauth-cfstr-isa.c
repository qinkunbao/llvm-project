// RUN: %clang_cc1 -cc1 -no-opaque-pointers -nostdsysteminc -fptrauth-calls -fptrauth-objc-isa-mode=sign-and-strip -triple arm64-apple-ios -emit-llvm -O2 -disable-llvm-passes -o - %s | FileCheck %s
// RUN: %clang_cc1 -cc1 -no-opaque-pointers -nostdsysteminc -fptrauth-calls -fptrauth-objc-isa-mode=sign-and-auth  -triple arm64-apple-ios -emit-llvm -O2 -disable-llvm-passes -o - %s | FileCheck %s

#define CFSTR __builtin___CFStringMakeConstantString

void f() {
  CFSTR("Hello, World!");
}

const void *G = CFSTR("yo joe");

void h() {
  static const void *h = CFSTR("Goodbye, World!");
}

// CHECK: @__CFConstantStringClassReference = external global [0 x i32]
// CHECK: @__CFConstantStringClassReference.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast ([0 x i32]* @__CFConstantStringClassReference to i8*), i32 2, i64 ptrtoint (%struct.__NSConstantString_tag* @_unnamed_cfstring_ to i64), i64 27361 }, section "llvm.ptrauth", align 8
// CHECK: @.str = private unnamed_addr constant [14 x i8] c"Hello, World!\00", section "__TEXT,__cstring,cstring_literals", align 1
// CHECK: @_unnamed_cfstring_ = private global %struct.__NSConstantString_tag { i32* bitcast ({ i8*, i32, i64, i64 }* @__CFConstantStringClassReference.ptrauth to i32*), i32 1992, i8* getelementptr inbounds ([14 x i8], [14 x i8]* @.str, i32 0, i32 0), i64 13 }, section "__DATA,__cfstring", align 8 #0
// CHECK: @__CFConstantStringClassReference.ptrauth.1 = private constant { i8*, i32, i64, i64 } { i8* bitcast ([0 x i32]* @__CFConstantStringClassReference to i8*), i32 2, i64 ptrtoint (%struct.__NSConstantString_tag* @_unnamed_cfstring_.3 to i64), i64 27361 }, section "llvm.ptrauth", align 8
// CHECK: @.str.2 = private unnamed_addr constant [7 x i8] c"yo joe\00", section "__TEXT,__cstring,cstring_literals", align 1
// CHECK: @_unnamed_cfstring_.3 = private global %struct.__NSConstantString_tag { i32* bitcast ({ i8*, i32, i64, i64 }* @__CFConstantStringClassReference.ptrauth.1 to i32*), i32 1992, i8* getelementptr inbounds ([7 x i8], [7 x i8]* @.str.2, i32 0, i32 0), i64 6 }, section "__DATA,__cfstring", align 8 #0
// CHECK: @G = global i8* bitcast (%struct.__NSConstantString_tag* @_unnamed_cfstring_.3 to i8*), align 8
// CHECK: @h.h = internal global i8* bitcast (%struct.__NSConstantString_tag* @_unnamed_cfstring_.6 to i8*), align 8
// CHECK: @__CFConstantStringClassReference.ptrauth.4 = private constant { i8*, i32, i64, i64 } { i8* bitcast ([0 x i32]* @__CFConstantStringClassReference to i8*), i32 2, i64 ptrtoint (%struct.__NSConstantString_tag* @_unnamed_cfstring_.6 to i64), i64 27361 }, section "llvm.ptrauth", align 8
// CHECK: @.str.5 = private unnamed_addr constant [16 x i8] c"Goodbye, World!\00", section "__TEXT,__cstring,cstring_literals", align 1
// CHECK: @_unnamed_cfstring_.6 = private global %struct.__NSConstantString_tag { i32* bitcast ({ i8*, i32, i64, i64 }* @__CFConstantStringClassReference.ptrauth.4 to i32*), i32 1992, i8* getelementptr inbounds ([16 x i8], [16 x i8]* @.str.5, i32 0, i32 0), i64 15 }, section "__DATA,__cfstring", align 8 #0
