; RUN: opt -coro-early -coro-split -coro-cleanup -S %s | FileCheck %s
target datalayout = "E-p:64:64"

%swift.type = type { i64 }
%swift.opaque = type opaque
%T4red215EmptyCollectionV = type opaque
%TSi = type <{ i64 }>

@prototype.signed = private constant { i8*, i32, i64, i64 } { i8* bitcast (i8* (i8*, i1)* @prototype to i8*), i32 2, i64 0, i64 12867 }, section "llvm.ptrauth"

define i8* @f(i8* %buffer, i32 %n) {
entry:
  %id = call token @llvm.coro.id.retcon(i32 8, i32 4, i8* %buffer, i8* bitcast ({ i8*, i32, i64, i64 }* @prototype.signed to i8*), i8* bitcast (i8* (i32)* @allocate to i8*), i8* bitcast (void (i8*)* @deallocate to i8*))
  %hdl = call i8* @llvm.coro.begin(token %id, i8* null)
  br label %loop

loop:
  %n.val = phi i32 [ %n, %entry ], [ %inc, %resume ]
  call void @print(i32 %n.val)
  %unwind0 = call i1 (...) @llvm.coro.suspend.retcon.i1()
  br i1 %unwind0, label %cleanup, label %resume

resume:
  %inc = add i32 %n.val, 1
  br label %loop

cleanup:
  call i1 @llvm.coro.end(i8* %hdl, i1 0)
  unreachable
}

; CHECK:       @prototype.signed = private constant
; CHECK:       [[GLOBAL:@.*]] = private constant { i8*, i32, i64, i64 } { i8* bitcast (i8* (i8*, i1)* [[RESUME:@.*]] to i8*), i32 2, i64 0, i64 12867 }, section "llvm.ptrauth"

; CHECK-LABEL: define i8* @f(i8* %buffer, i32 %n)
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[T0:%.*]] = bitcast i8* %buffer to [[FRAME_T:%.*]]*
; CHECK-NEXT:    [[T1:%.*]] = getelementptr inbounds [[FRAME_T]], [[FRAME_T]]* [[T0]], i32 0, i32 0
; CHECK-NEXT:    store i32 %n, i32* [[T1]]
; CHECK-NEXT:    call void @print(i32 %n)
; CHECK-NEXT:    ret i8* bitcast ({ i8*, i32, i64, i64 }* [[GLOBAL]] to i8*)

; CHECK:      define internal i8* [[RESUME]](i8* noalias nonnull align 4 dereferenceable(8) %0, i1 zeroext %1) {
; CHECK:         bitcast ({ i8*, i32, i64, i64 }* [[GLOBAL]] to

@g.prototype.signed = private constant { i8*, i32, i64, i64 } { i8* bitcast (i8* (i8*, i1)* @prototype to i8*), i32 2, i64 1, i64 8723 }, section "llvm.ptrauth"

define i8* @g(i8* %buffer, i32 %n) {
entry:
  %id = call token @llvm.coro.id.retcon(i32 8, i32 4, i8* %buffer, i8* bitcast ({ i8*, i32, i64, i64 }* @g.prototype.signed to i8*), i8* bitcast (i8* (i32)* @allocate to i8*), i8* bitcast (void (i8*)* @deallocate to i8*))
  %hdl = call i8* @llvm.coro.begin(token %id, i8* null)
  br label %loop

loop:
  %n.val = phi i32 [ %n, %entry ], [ %inc, %resume ]
  call void @print(i32 %n.val)
  %unwind0 = call i1 (...) @llvm.coro.suspend.retcon.i1()
  br i1 %unwind0, label %cleanup, label %resume

resume:
  %inc = add i32 %n.val, 1
  br label %loop

cleanup:
  call i1 @llvm.coro.end(i8* %hdl, i1 0)
  unreachable
}

; CHECK-LABEL: define i8* @g(i8* %buffer, i32 %n)
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[T0:%.*]] = bitcast i8* %buffer to [[FRAME_T:%.*]]*
; CHECK-NEXT:    [[T1:%.*]] = getelementptr inbounds [[FRAME_T]], [[FRAME_T]]* [[T0]], i32 0, i32 0
; CHECK-NEXT:    store i32 %n, i32* [[T1]]
; CHECK-NEXT:    call void @print(i32 %n)
; CHECK-NEXT:    [[T0:%.*]] = ptrtoint i8* %buffer to i64
; CHECK-NEXT:    [[T1:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T0]], i64 8723)
; CHECK-NEXT:    [[T2:%.*]] = call i64 @llvm.ptrauth.sign(i64 ptrtoint (i8* (i8*, i1)* [[RESUME:@.*]] to i64), i32 2, i64 [[T1]])
; CHECK-NEXT:    [[T3:%.*]] = inttoptr i64 [[T2]] to i8* (i8*, i1)*
; CHECK-NEXT:    [[T4:%.*]] = bitcast i8* (i8*, i1)* [[T3]] to i8*
; CHECK-NEXT:    ret i8* [[T4]]

; CHECK:      define internal i8* [[RESUME]](i8* noalias nonnull align 4 dereferenceable(8) %0, i1 zeroext %1) {
; CHECK:         call void @print(i32 %inc)
; CHECK-NEXT:    [[T0:%.*]] = ptrtoint i8* %0 to i64
; CHECK-NEXT:    [[T1:%.*]] = call i64 @llvm.ptrauth.blend(i64 [[T0]], i64 8723)
; CHECK-NEXT:    [[T2:%.*]] = call i64 @llvm.ptrauth.sign(i64 ptrtoint (i8* (i8*, i1)* [[RESUME]] to i64), i32 2, i64 [[T1]])
; CHECK-NEXT:    [[T3:%.*]] = inttoptr i64 [[T2]] to i8* (i8*, i1)*
; CHECK-NEXT:    [[T4:%.*]] = bitcast i8* (i8*, i1)* [[T3]] to i8*
; CHECK-NEXT:    ret i8* [[T4]]

declare noalias i8* @malloc(i64) #5
declare void @free(i8* nocapture) #5

declare token @llvm.coro.id.retcon(i32, i32, i8*, i8*, i8*, i8*)
declare i8* @llvm.coro.begin(token, i8*)
declare i1 @llvm.coro.suspend.retcon.i1(...)
declare i1 @llvm.coro.end(i8*, i1)
declare i8* @llvm.coro.prepare.retcon(i8*)

declare i8* @prototype(i8*, i1 zeroext)

declare noalias i8* @allocate(i32 %size)
declare void @deallocate(i8* %ptr)

declare void @print(i32)
