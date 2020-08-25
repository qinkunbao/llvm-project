; RUN: llc -mtriple arm64-apple-darwin            -asm-verbose=false -o - %s | FileCheck --check-prefix=CHECK-DAG-ISEL --check-prefix=CHECK %s
; RUN: llc -mtriple arm64-apple-darwin -fast-isel -asm-verbose=false -o - %s | FileCheck --check-prefix=CHECK-FAST-ISEL --check-prefix=CHECK %s

; CHECK-LABEL: _test_musttail:
; CHECK: b _musttail_callee

declare i32 @musttail_callee(i32)

define i32 @test_musttail(i32 %arg0) {
  %r = musttail call i32 @musttail_callee(i32 %arg0)
  ret i32 %r
}

; CHECK-LABEL: _test_musttail_variadic:
; CHECK: b _musttail_variadic_callee

declare i32 @musttail_variadic_callee(i32, ...)

define i32 @test_musttail_variadic(i32 %arg0, ...) {
  %r = musttail call i32 (i32, ...) @musttail_variadic_callee(i32 %arg0, ...)
  ret i32 %r
}

; CHECK-LABEL: _test_musttail_variadic_aggret:
; CHECK: b _musttail_variadic_aggret_callee

declare [2 x i64] @musttail_variadic_aggret_callee(i32 %arg0, ...)

define [2 x i64] @test_musttail_variadic_aggret(i32 %arg0, ...) {
  %r = musttail call [2 x i64] (i32, ...) @musttail_variadic_aggret_callee(i32 %arg0, ...)
  ret [2 x i64] %r
}

; CHECK-LABEL: _test_musttail_variadic_stack:
; CHECK-FAST-ISEL: ldp x9, x11, [sp]
; CHECK-FAST-ISEL: ldp x10, x12, [sp, #16]
; CHECK-FAST-ISEL: stp x9, x11, [sp, #16]
; CHECK-FAST-ISEL: stp x10, x12, [sp]
; CHECK-DAG-ISEL: ldp x9, x10, [sp]
; CHECK-DAG-ISEL: ldr q16, [sp, #16]
; CHECK-DAG-ISEL: stp x9, x10, [sp, #16]
; CHECK-DAG-ISEL: str q16, [sp]
; CHECK: b _musttail_variadic_stack_callee

declare i32 @musttail_variadic_stack_callee([2 x i64] %a0, [2 x i64] %a1, [2 x i64] %a2, [2 x i64] %a3, [2 x i64] %a4, [2 x i64] %a5, ...)

define i32 @test_musttail_variadic_stack([2 x i64] %a0, [2 x i64] %a1, [2 x i64] %a2, [2 x i64] %a3, [2 x i64] %a4, [2 x i64] %a5, ...) {
  %r = musttail call i32 ([2 x i64], [2 x i64], [2 x i64], [2 x i64], [2 x i64], [2 x i64], ...) @musttail_variadic_stack_callee([2 x i64] %a0, [2 x i64] %a1, [2 x i64] %a2, [2 x i64] %a3, [2 x i64] %a5, [2 x i64] %a4, ...)
  ret i32 %r
}

; We still mis-compile this test case because %arg2 is written to its argument
; slot before %arg1 is read from its slot.
;
;	ldr	q16, [sp, #16] # read %arg2
;	str	q16, [sp]      # write %arg2. this overwrites %arg1.
;	ldr	q16, [sp]      # read %arg1
;	str	q16, [sp, #16] # write %arg1
;	b	_musttail_variadic_byval_callee

%Struct = type { i64, i64 }

declare i32 @musttail_variadic_byval_callee(i32, %Struct* byval(%Struct), %Struct* byval(%Struct), ...)

define i32 @test_musttail_variadic_byval(i32 %arg0, %Struct* byval(%Struct) %arg1, %Struct* byval(%Struct) %arg2, ...) {
  %r = musttail call i32 (i32, %Struct*, %Struct*, ...) @musttail_variadic_byval_callee(i32 %arg0, %Struct* byval(%Struct) %arg2, %Struct* byval(%Struct) %arg1, ...)
  ret i32 %r
}
