// RUN: %clang_cc1 -no-opaque-pointers -I %S/Inputs -Wno-objc-root-class -fptrauth-intrinsics -fptrauth-calls -triple arm64-apple-ios -fobjc-runtime=ios-12.2 -emit-llvm -fblocks -fobjc-arc -fobjc-runtime-has-weak -O2 -disable-llvm-passes -o - %s -fptrauth-objc-isa-mode=strip | FileCheck --check-prefix=CHECK-STRIP %s
// RUN: %clang_cc1 -no-opaque-pointers -I %S/Inputs -Wno-objc-root-class -fptrauth-intrinsics -fptrauth-calls -triple arm64-apple-ios -fobjc-runtime=ios-12.2 -emit-llvm -fblocks -fobjc-arc -fobjc-runtime-has-weak -O2 -disable-llvm-passes -o - %s -fptrauth-objc-isa-mode=sign-and-strip | FileCheck --check-prefix=CHECK-SIGN-AND-STRIP %s
// RUN: %clang_cc1 -no-opaque-pointers -I %S/Inputs -Wno-objc-root-class -fptrauth-intrinsics -fptrauth-calls -triple arm64-apple-ios -fobjc-runtime=ios-12.2 -emit-llvm -fblocks -fobjc-arc -fobjc-runtime-has-weak -O2 -disable-llvm-passes -o - %s -fptrauth-objc-isa-mode=sign-and-auth | FileCheck --check-prefix=CHECK-SIGN-AND-AUTH %s
// RUN: %clang_cc1 -no-opaque-pointers -I %S/Inputs -Wno-objc-root-class -fptrauth-intrinsics -fptrauth-calls -triple arm64-apple-ios -fobjc-runtime=ios-12.2 -emit-llvm -fblocks -fobjc-arc -fobjc-runtime-has-weak -O2 -disable-llvm-passes -o - %s | FileCheck --check-prefix=CHECK-DISABLED %s

#include <ptrauth.h>
_Static_assert(!__has_feature(ptrauth_objc_isa_masking), "wat");
#if __has_feature(ptrauth_qualifier_authentication_mode)

@class NSString;
NSString *aString = @"foo";

// CHECK-SIGN-AND-AUTH: @__CFConstantStringClassReference.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast ([0 x i32]* @__CFConstantStringClassReference to i8*), i32 2, i64 ptrtoint (%struct.__NSConstantString_tag* @_unnamed_cfstring_ to i64), i64 27361 }, section "llvm.ptrauth", align 8
// CHECK-SIGN-AND-AUTH: @.str = private unnamed_addr constant [4 x i8] c"foo\00", section "__TEXT,__cstring,cstring_literals", align 1
// CHECK-SIGN-AND-AUTH: @_unnamed_cfstring_ = private global %struct.__NSConstantString_tag { i32* bitcast ({ i8*, i32, i64, i64 }* @__CFConstantStringClassReference.ptrauth to i32*), i32 1992, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i32 0, i32 0), i64 3 }, section "__DATA,__cfstring", align 8 #0
// CHECK-SIGN-AND-AUTH: @aString = global %0* bitcast (%struct.__NSConstantString_tag* @_unnamed_cfstring_ to %0*), align 8

// CHECK-SIGN-AND-STRIP: @__CFConstantStringClassReference.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast ([0 x i32]* @__CFConstantStringClassReference to i8*), i32 2, i64 ptrtoint (%struct.__NSConstantString_tag* @_unnamed_cfstring_ to i64), i64 27361 }, section "llvm.ptrauth", align 8
// CHECK-SIGN-AND-STRIP: @.str = private unnamed_addr constant [4 x i8] c"foo\00", section "__TEXT,__cstring,cstring_literals", align 1
// CHECK-SIGN-AND-STRIP: @_unnamed_cfstring_ = private global %struct.__NSConstantString_tag { i32* bitcast ({ i8*, i32, i64, i64 }* @__CFConstantStringClassReference.ptrauth to i32*), i32 1992, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i32 0, i32 0), i64 3 }, section "__DATA,__cfstring", align 8 #0
// CHECK-SIGN-AND-STRIP: @aString = global %0* bitcast (%struct.__NSConstantString_tag* @_unnamed_cfstring_ to %0*), align 8

// CHECK-STRIP: @.str = private unnamed_addr constant [4 x i8] c"foo\00", section "__TEXT,__cstring,cstring_literals", align 1
// CHECK-STRIP: @_unnamed_cfstring_ = private global %struct.__NSConstantString_tag { i32* getelementptr inbounds ([0 x i32], [0 x i32]* @__CFConstantStringClassReference, i32 0, i32 0), i32 1992, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i32 0, i32 0), i64 3 }, section "__DATA,__cfstring", align 8 #0
// CHECK-STRIP: @aString = global %0* bitcast (%struct.__NSConstantString_tag* @_unnamed_cfstring_ to %0*), align 8

// CHECK-DISABLED: @.str = private unnamed_addr constant [4 x i8] c"foo\00", section "__TEXT,__cstring,cstring_literals", align 1
// CHECK-DISABLED: @_unnamed_cfstring_ = private global %struct.__NSConstantString_tag { i32* getelementptr inbounds ([0 x i32], [0 x i32]* @__CFConstantStringClassReference, i32 0, i32 0), i32 1992, i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str, i32 0, i32 0), i64 3 }, section "__DATA,__cfstring", align 8 #0
// CHECK-DISABLED: @aString = global %0* bitcast (%struct.__NSConstantString_tag* @_unnamed_cfstring_ to %0*), align 8

#if __has_feature(ptrauth_objc_isa_signs)
int ptrauth_objc_isa_signs_global = 0; // Verifying compilation path
// CHECK-SIGN-AND-AUTH: @ptrauth_objc_isa_signs_global = global i32 0, align 4
// CHECK-SIGN-AND-STRIP: @ptrauth_objc_isa_signs_global = global i32 0, align 4
#if __has_feature(ptrauth_objc_isa_authenticates)
int ptrauth_objc_isa_signs_and_auths_global = 0; // Verifying compilation path
// CHECK-SIGN-AND-AUTH: @ptrauth_objc_isa_signs_and_auths_global = global i32 0, align 4
#elif __has_feature(ptrauth_objc_isa_strips)
int ptrauth_objc_isa_signs_and_strips_global = 0; // Verifying compilation path
                                                  // CHECK-SIGN-AND-STRIP: @ptrauth_objc_isa_signs_and_strips_global = global i32 0, align 4
#else
_Static_assert(false, "none of the tests should hit this path");
#endif
#else
_Static_assert(!__has_feature(ptrauth_objc_isa_authenticates), "Strip and auth is an invalid mode");
#if __has_feature(ptrauth_objc_isa_strips)
int ptrauth_objc_isa_strips_global = 0; // Verifying compilation path
                                        // CHECK-STRIP: @ptrauth_objc_isa_strips_global = global i32 0, align 4
#else
// Make sure that the isa features don't lie when objc isa signing is completely disabled
int ptrauth_objc_isa_disabled_global = 0; // Verifying compilation path
                                          // CHECK-DISABLED: @ptrauth_objc_isa_disabled_global = global i32 0, align 4
#endif
#endif

typedef struct {
  void *__ptrauth_objc_isa_pointer ptr_isa;
  __UINT64_TYPE__ __ptrauth_objc_isa_uintptr int_isa;
} TestStruct;

void testModes(TestStruct *instruct, TestStruct *outstruct);
void testModes(TestStruct *instruct, TestStruct *outstruct) {
  instruct->ptr_isa = *(void **)outstruct->ptr_isa;
  instruct->int_isa = *(__UINT64_TYPE__ *)outstruct->int_isa;
  // CHECK-STRIP: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK-STRIP: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK-STRIP: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK-STRIP: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK-STRIP: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK-STRIP: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK-SIGN-AND-STRIP: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK-SIGN-AND-STRIP: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK-SIGN-AND-STRIP: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK-SIGN-AND-STRIP: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
  // CHECK-SIGN-AND-STRIP: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK-SIGN-AND-STRIP: [[TMP:%.*]] = call i64 @llvm.ptrauth.strip
  // CHECK-SIGN-AND-STRIP: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK-SIGN-AND-STRIP: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
  // CHECK-SIGN-AND-AUTH: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK-SIGN-AND-AUTH: [[TMP:%.*]] = call i64 @llvm.ptrauth.auth
  // CHECK-SIGN-AND-AUTH: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK-SIGN-AND-AUTH: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
  // CHECK-SIGN-AND-AUTH: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK-SIGN-AND-AUTH: [[TMP:%.*]] = call i64 @llvm.ptrauth.auth
  // CHECK-SIGN-AND-AUTH: [[TMP:%.*]] = call i64 @llvm.ptrauth.blend
  // CHECK-SIGN-AND-AUTH: [[TMP:%.*]] = call i64 @llvm.ptrauth.sign
}

#endif
