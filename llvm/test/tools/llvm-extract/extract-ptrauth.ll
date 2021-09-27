; RUN: llvm-extract -S -func test_fn %s | FileCheck %s

@test_gv = external global i32
@test_gv.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (i32* @test_gv to i8*), i32 2, i64 0, i64 0 }, section "llvm.ptrauth"

; CHECK: @test_gv = external global i32
; CHECK: @test_gv.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (i32* @test_gv to i8*), i32 2, i64 0, i64 0 }, section "llvm.ptrauth"

define i8* @test_fn() {
  ret i8* bitcast ({ i8*, i32, i64, i64 }* @test_gv.ptrauth to i8*)
}
