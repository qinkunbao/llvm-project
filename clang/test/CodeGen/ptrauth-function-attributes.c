// RUN: %clang_cc1 -triple arm64-apple-ios                    -emit-llvm %s  -o - | FileCheck %s --check-prefixes=ALL,OFF

// RUN: %clang_cc1 -triple arm64-apple-ios  -fptrauth-returns -emit-llvm %s  -o - | FileCheck %s --check-prefixes=ALL,RETS
// RUN: %clang_cc1 -triple arm64e-apple-ios -fptrauth-returns -emit-llvm %s  -o - | FileCheck %s --check-prefixes=ALL,RETS
// RUN: %clang_cc1 -triple arm64e-apple-ios                   -emit-llvm %s  -o - | FileCheck %s --check-prefixes=ALL,OFF

// RUN: %clang_cc1 -triple arm64-apple-ios  -fptrauth-auth-traps -emit-llvm %s -o - | FileCheck %s --check-prefixes=ALL,TRAPS
// RUN: %clang_cc1 -triple arm64e-apple-ios -fptrauth-auth-traps -emit-llvm %s -o - | FileCheck %s --check-prefixes=ALL,TRAPS
// RUN: %clang_cc1 -triple arm64e-apple-ios                      -emit-llvm %s -o - | FileCheck %s --check-prefixes=ALL,OFF

// ALL-LABEL: define void @test() #0
void test() {
}

// RETS: attributes #0 = {{{.*}} "ptrauth-returns" {{.*}}}
// TRAPS: attributes #0 = {{{.*}} "ptrauth-auth-traps" {{.*}}}
// OFF-NOT: attributes {{.*}} "ptrauth-
