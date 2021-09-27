; RUN: opt < %s -instcombine -S | FileCheck %s

declare i32 @f(i32)

@f.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (i32 (i32)* @f to i8*), i32 0, i64 0, i64 0 }, section "llvm.ptrauth"

define i32 @test_ptrauth_call(i32 %a0) {
; CHECK-LABEL: @test_ptrauth_call(
; CHECK-NEXT:    [[TMP0:%.*]] = call i32 @f(i32 [[A0:%.*]]){{$}}
; CHECK-NEXT:    ret i32 [[TMP0]]
;
  %tmp0 = call i32 bitcast ({ i8*, i32, i64, i64 }* @f.ptrauth to i32 (i32)*)(i32 %a0) [ "ptrauth"(i32 0, i64 0) ]
  ret i32 %tmp0
}

@f_disc.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (i32 (i32)* @f to i8*), i32 1, i64 0, i64 5678 }, section "llvm.ptrauth"

define i32 @test_ptrauth_call_disc(i32 %a0) {
; CHECK-LABEL: @test_ptrauth_call_disc(
; CHECK-NEXT:    [[TMP0:%.*]] = call i32 @f(i32 [[A0:%.*]]){{$}}
; CHECK-NEXT:    ret i32 [[TMP0]]
;
  %tmp0 = call i32 bitcast ({ i8*, i32, i64, i64 }* @f_disc.ptrauth to i32 (i32)*)(i32 %a0) [ "ptrauth"(i32 1, i64 5678) ]
  ret i32 %tmp0
}

@f_addr_disc.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (i32 (i32)* @f to i8*), i32 1, i64 ptrtoint (i8** @f_addr_disc.ref to i64), i64 0 }, section "llvm.ptrauth"

@f_addr_disc.ref = constant i8* bitcast ({ i8*, i32, i64, i64 }* @f_addr_disc.ptrauth to i8*)

define i32 @test_ptrauth_call_addr_disc(i32 %a0) {
; CHECK-LABEL: @test_ptrauth_call_addr_disc(
; CHECK-NEXT:    [[TMP0:%.*]] = call i32 @f(i32 [[A0:%.*]]){{$}}
; CHECK-NEXT:    ret i32 [[TMP0]]
;
  %tmp0 = call i32 bitcast ({ i8*, i32, i64, i64 }* @f_addr_disc.ptrauth to i32 (i32)*)(i32 %a0) [ "ptrauth"(i32 1, i64 ptrtoint (i8** @f_addr_disc.ref to i64)) ]
  ret i32 %tmp0
}

@f_both_disc.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (i32 (i32)* @f to i8*), i32 1, i64 ptrtoint (i8** @f_both_disc.ref to i64), i64 1234 }, section "llvm.ptrauth"

@f_both_disc.ref = constant i8* bitcast ({ i8*, i32, i64, i64 }* @f_both_disc.ptrauth to i8*)

define i32 @test_ptrauth_call_blend(i32 %a0) {
; CHECK-LABEL: @test_ptrauth_call_blend(
; CHECK-NEXT:    [[TMP0:%.*]] = call i32 @f(i32 [[A0:%.*]]){{$}}
; CHECK-NEXT:    ret i32 [[TMP0]]
;
  %tmp = call i64 @llvm.ptrauth.blend(i64 ptrtoint (i8** @f_both_disc.ref to i64), i64 1234)
  %tmp0 = call i32 bitcast ({ i8*, i32, i64, i64 }* @f_both_disc.ptrauth to i32 (i32)*)(i32 %a0) [ "ptrauth"(i32 1, i64 %tmp) ]
  ret i32 %tmp0
}

define i32 @test_ptrauth_call_mismatch_key(i32 %a0) {
; CHECK-LABEL: @test_ptrauth_call_mismatch_key(
; CHECK-NEXT:    [[TMP0:%.*]] = call i32 bitcast ({ i8*, i32, i64, i64 }* @f_disc.ptrauth to i32 (i32)*)(i32 [[A0:%.*]]) [ "ptrauth"(i32 0, i64 5678) ]
; CHECK-NEXT:    ret i32 [[TMP0]]
;
  %tmp0 = call i32 bitcast ({ i8*, i32, i64, i64 }* @f_disc.ptrauth to i32 (i32)*)(i32 %a0) [ "ptrauth"(i32 0, i64 5678) ]
  ret i32 %tmp0
}

define i32 @test_ptrauth_call_mismatch_disc(i32 %a0) {
; CHECK-LABEL: @test_ptrauth_call_mismatch_disc(
; CHECK-NEXT:    [[TMP0:%.*]] = call i32 bitcast ({ i8*, i32, i64, i64 }* @f_disc.ptrauth to i32 (i32)*)(i32 [[A0:%.*]]) [ "ptrauth"(i32 1, i64 0) ]
; CHECK-NEXT:    ret i32 [[TMP0]]
;
  %tmp0 = call i32 bitcast ({ i8*, i32, i64, i64 }* @f_disc.ptrauth to i32 (i32)*)(i32 %a0) [ "ptrauth"(i32 1, i64 0) ]
  ret i32 %tmp0
}

define i32 @test_ptrauth_call_mismatch_blend(i32 %a0) {
; CHECK-LABEL: @test_ptrauth_call_mismatch_blend(
; CHECK-NEXT:    [[TMP:%.*]] = call i64 @llvm.ptrauth.blend(i64 ptrtoint (i8** @f_both_disc.ref to i64), i64 0)
; CHECK-NEXT:    [[TMP0:%.*]] = call i32 bitcast ({ i8*, i32, i64, i64 }* @f_both_disc.ptrauth to i32 (i32)*)(i32 [[A0:%.*]]) [ "ptrauth"(i32 1, i64 [[TMP]]) ]
; CHECK-NEXT:    ret i32 [[TMP0]]
;
  %tmp = call i64 @llvm.ptrauth.blend(i64 ptrtoint (i8** @f_both_disc.ref to i64), i64 0)
  %tmp0 = call i32 bitcast ({ i8*, i32, i64, i64 }* @f_both_disc.ptrauth to i32 (i32)*)(i32 %a0) [ "ptrauth"(i32 1, i64 %tmp) ]
  ret i32 %tmp0
}

declare i64 @llvm.ptrauth.blend(i64, i64)
