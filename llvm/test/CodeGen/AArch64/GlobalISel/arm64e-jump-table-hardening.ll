; RUN: llc -verify-machineinstrs -o - %s -mtriple=arm64-apple-ios -global-isel -global-isel-abort=1 -aarch64-enable-atomic-cfg-tidy=0 | FileCheck %s

; x16 and x17 are hard-coded into the pseudo-instruction because they have
; better security guarantees.

; CHECK-LABEL: test_jumptable:
; CHECK: ubfx  x16, x0, #0, #32
; CHECK: cmp   x16, #5
; CHECK: csel  x16, x16, xzr, ls
; CHECK: adrp  x17, LJTI0_0@PAGE
; CHECK: add   x17, x17, LJTI0_0@PAGEOFF
; CHECK: ldrsw x16, [x17, x16, lsl #2]
; CHECK: Ltmp0:
; CHECK: adr   x17, Ltmp0
; CHECK: add   x16, x17, x16
; CHECK: br    x16

define i32 @test_jumptable(i32 %in) "jump-table-hardening" {

  switch i32 %in, label %def [
    i32 0, label %lbl1
    i32 1, label %lbl2
    i32 2, label %lbl3
    i32 4, label %lbl4
    i32 5, label %lbl5
  ]

def:
  ret i32 0

lbl1:
  ret i32 1

lbl2:
  ret i32 2

lbl3:
  ret i32 4

lbl4:
  ret i32 8

lbl5:
  ret i32 10

}

; CHECK: LJTI0_0:
; CHECK-NEXT: .long LBB{{[0-9_]+}}-Ltmp0
; CHECK-NEXT: .long LBB{{[0-9_]+}}-Ltmp0
; CHECK-NEXT: .long LBB{{[0-9_]+}}-Ltmp0
; CHECK-NEXT: .long LBB{{[0-9_]+}}-Ltmp0
; CHECK-NEXT: .long LBB{{[0-9_]+}}-Ltmp0
; CHECK-NEXT: .long LBB{{[0-9_]+}}-Ltmp0
