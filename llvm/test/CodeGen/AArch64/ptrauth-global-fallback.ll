; RUN: llc -mtriple=arm64e-apple-ios %s -o - | FileCheck %s

define hidden swifttailcc void @foo() optnone noinline {
; CHECK-LABEL: foo:
; CHECK: adrp x[[TMP:[0-9]+]], _bar@GOTPAGE
; CHECK: ldr [[BAR:x[0-9]+]], [x[[TMP]], _bar@GOTPAGEOFF]
; CHECK: paciza [[BAR]]
; CHECK: str [[BAR]]

  %var = ptrtoint i8* bitcast (void ()* @bar to i8*) to i64
  %var.signed = call i64 @llvm.ptrauth.sign(i64 %var, i32 0, i64 0)
  %var.ptr = inttoptr i64 %var.signed to i8*
  store i8* %var.ptr, i8** undef, align 8
  ret void
}

declare void @bar()
declare i64 @llvm.ptrauth.sign(i64, i32 immarg, i64) #0
