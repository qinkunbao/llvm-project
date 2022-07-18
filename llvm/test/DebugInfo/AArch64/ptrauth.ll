; RUN: llc %s -filetype=obj -mtriple arm64e-apple-darwin -o - \
; RUN:   | llvm-dwarfdump - | FileCheck %s

; CHECK: DW_AT_type	(0x{{0+}}[[TY:.*]] "void *__ptrauth(4, 1, 0x04d2)")
; CHECK: 0x{{0+}}[[TY]]: DW_TAG_LLVM_ptrauth_type
; CHECK-NEXT: DW_AT_type {{.*}}"void *"
; CHECK-NEXT: DW_AT_LLVM_ptrauth_key (0x04)
; CHECK-NEXT: DW_AT_LLVM_ptrauth_address_discriminated (true)
; CHECK-NEXT: DW_AT_LLVM_ptrauth_extra_discriminator (0x04d2)

; CHECK: DW_AT_type	(0x{{0+}}[[TY:.*]] "void *__ptrauth(4, 1, 0x04d3, "isa-pointer")")
; CHECK: 0x{{0+}}[[TY]]: DW_TAG_LLVM_ptrauth_type
; CHECK-NEXT: DW_AT_type {{.*}}"void *"
; CHECK-NEXT: DW_AT_LLVM_ptrauth_key (0x04)
; CHECK-NEXT: DW_AT_LLVM_ptrauth_address_discriminated (true)
; CHECK-NEXT: DW_AT_LLVM_ptrauth_extra_discriminator (0x04d3)
; CHECK-NEXT: DW_AT_LLVM_ptrauth_isa_pointer	(true)

; CHECK: DW_AT_type	(0x{{0+}}[[TY:.*]] "void *__ptrauth(4, 1, 0x04d4, "authenticates-null-values")")
; CHECK: 0x{{0+}}[[TY]]: DW_TAG_LLVM_ptrauth_type
; CHECK-NEXT: DW_AT_type {{.*}}"void *"
; CHECK-NEXT: DW_AT_LLVM_ptrauth_key (0x04)
; CHECK-NEXT: DW_AT_LLVM_ptrauth_address_discriminated (true)
; CHECK-NEXT: DW_AT_LLVM_ptrauth_extra_discriminator (0x04d4)
; CHECK-NEXT: DW_AT_LLVM_ptrauth_authenticates_null_values	(true)

; CHECK: DW_AT_type	(0x{{0+}}[[TY:.*]] "void *__ptrauth(4, 1, 0x04d5, "isa-pointer,authenticates-null-values")")
; CHECK: 0x{{0+}}[[TY]]: DW_TAG_LLVM_ptrauth_type
; CHECK-NEXT: DW_AT_type {{.*}}"void *"
; CHECK-NEXT: DW_AT_LLVM_ptrauth_key (0x04)
; CHECK-NEXT: DW_AT_LLVM_ptrauth_address_discriminated (true)
; CHECK-NEXT: DW_AT_LLVM_ptrauth_extra_discriminator (0x04d5)
; CHECK-NEXT: DW_AT_LLVM_ptrauth_isa_pointer	(true)
; CHECK-NEXT: DW_AT_LLVM_ptrauth_authenticates_null_values	(true)

target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"

@p = common global i8* null, align 8, !dbg !0

!llvm.dbg.cu = !{!8}
!llvm.module.flags = !{!16, !17}

!0 = !DIGlobalVariableExpression(var: !4, expr: !DIExpression())
!1 = !DIGlobalVariableExpression(var: !5, expr: !DIExpression())
!2 = !DIGlobalVariableExpression(var: !6, expr: !DIExpression())
!3 = !DIGlobalVariableExpression(var: !7, expr: !DIExpression())
!4 = distinct !DIGlobalVariable(name: "p1", scope: !8, file: !9, line: 1, type: !12, isLocal: false, isDefinition: true)
!5 = distinct !DIGlobalVariable(name: "p2", scope: !8, file: !9, line: 1, type: !13, isLocal: false, isDefinition: true)
!6 = distinct !DIGlobalVariable(name: "p3", scope: !8, file: !9, line: 1, type: !14, isLocal: false, isDefinition: true)
!7 = distinct !DIGlobalVariable(name: "p4", scope: !8, file: !9, line: 1, type: !15, isLocal: false, isDefinition: true)
!8 = distinct !DICompileUnit(language: DW_LANG_C99, file: !9, emissionKind: FullDebug, globals: !11)
!9 = !DIFile(filename: "/tmp/p.c", directory: "/")
!10 = !{}
!11 = !{!0,!1,!2,!3}
!12 = !DIDerivedType(tag: DW_TAG_LLVM_ptrauth_type, baseType: !18, ptrAuthKey: 4, ptrAuthIsAddressDiscriminated: true, ptrAuthExtraDiscriminator: 1234)
!13 = !DIDerivedType(tag: DW_TAG_LLVM_ptrauth_type, baseType: !18, ptrAuthKey: 4, ptrAuthIsAddressDiscriminated: true, ptrAuthExtraDiscriminator: 1235, ptrAuthIsaPointer: true)
!14 = !DIDerivedType(tag: DW_TAG_LLVM_ptrauth_type, baseType: !18, ptrAuthKey: 4, ptrAuthIsAddressDiscriminated: true, ptrAuthExtraDiscriminator: 1236, ptrAuthAuthenticatesNullValues: true)
!15 = !DIDerivedType(tag: DW_TAG_LLVM_ptrauth_type, baseType: !18, ptrAuthKey: 4, ptrAuthIsAddressDiscriminated: true, ptrAuthExtraDiscriminator: 1237, ptrAuthIsaPointer: true, ptrAuthAuthenticatesNullValues: true)
!16 = !{i32 2, !"Dwarf Version", i32 4}
!17 = !{i32 2, !"Debug Info Version", i32 3}
!18 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null)
