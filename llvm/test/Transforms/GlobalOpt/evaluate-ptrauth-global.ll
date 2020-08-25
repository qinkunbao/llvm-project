; RUN: opt -globalopt %s -S -o - | FileCheck %s

target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64e-apple-ios13"

; Check that simple references to llvm.ptrauth globals get optimized into
; constant initializers, but more complex ones don't.
; CHECK: @llvm.global_ctors = appending global [2 x { i32, void ()*, i8* }] [{ i32, void ()*, i8* } { i32 65535, void ()* @test_init_ptrauth_offset, i8* null }, { i32, void ()*, i8* } { i32 65535, void ()* @test_init_ptrauth_nested, i8* null }]
@llvm.global_ctors = appending global [3 x { i32, void ()*, i8* }] [{ i32, void ()*, i8* } { i32 65535, void ()* @test_init_ptrauth, i8* null }, { i32, void ()*, i8* } { i32 65535, void ()* @test_init_ptrauth_offset, i8* null }, { i32, void ()*, i8* } { i32 65535, void ()* @test_init_ptrauth_nested, i8* null }]

@f.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (void ()* @f to i8*), i32 0, i64 0, i64 0 }, section "llvm.ptrauth", align 8

; CHECK: @fp = local_unnamed_addr global i8* bitcast ({ i8*, i32, i64, i64 }* @f.ptrauth to i8*), align 8
@fp = global i8* null, align 8

; CHECK-NOT: @test_init_ptrauth
define internal void @test_init_ptrauth() {
  store i8* bitcast ({ i8*, i32, i64, i64 }* @f.ptrauth to i8*), i8** @fp, align 8
  ret void
}

; CHECK: @fp_offset = local_unnamed_addr global i8* null, align 8
@fp_offset = global i8* null, align 8

; CHECK: @fp_nested = local_unnamed_addr global i8* null, align 8
@fp_nested = global i8* null, align 8

; CHECK: @fp_nested_offset = local_unnamed_addr global i8* null, align 8
@fp_nested_offset = global i8* null, align 8

; CHECK: define internal void @test_init_ptrauth_offset()
; CHECK:   store i8* getelementptr inbounds (i8, i8* bitcast ({ i8*, i32, i64, i64 }* @f.ptrauth to i8*), i64 1), i8** @fp_offset, align 8
define internal void @test_init_ptrauth_offset() {
  store i8* getelementptr inbounds (i8, i8* bitcast ({ i8*, i32, i64, i64 }* @f.ptrauth to i8*), i64 1), i8** @fp_offset, align 8
  ret void
}

; CHECK: define internal void @test_init_ptrauth_nested()
; CHECK:   store i8* bitcast ({ i8*, i32, i64, i64 }* @f.ptrauth to i8*), i8** @fp_nested, align 8
; CHECK:   store i8* getelementptr inbounds (i8, i8* bitcast ({ i8*, i32, i64, i64 }* @f.ptrauth to i8*), i64 1), i8** @fp_nested_offset, align 8
define internal void @test_init_ptrauth_nested() {
  store i8* bitcast ({ i8*, i32, i64, i64 }* @f.ptrauth to i8*), i8** @fp_nested, align 8
  store i8* getelementptr inbounds (i8, i8* bitcast ({ i8*, i32, i64, i64 }* @f.ptrauth to i8*), i64 1), i8** @fp_nested_offset, align 8
  ret void
}

declare void @f()
