// RUN: %clang_cc1 %s -E -triple=arm64-- | FileCheck %s --check-prefixes=NOINTRIN,NORETS
// RUN: %clang_cc1 %s -E -triple=arm64-- -fptrauth-returns | FileCheck %s --check-prefixes=NOINTRIN,RETS

#if __has_feature(ptrauth_intrinsics)
// INTRIN: has_ptrauth_intrinsics
void has_ptrauth_intrinsics() {}
#else
// NOINTRIN: no_ptrauth_intrinsics
void no_ptrauth_intrinsics() {}
#endif

#if __has_feature(ptrauth_returns)
// RETS: has_ptrauth_returns
void has_ptrauth_returns() {}
#else
// NORETS: no_ptrauth_returns
void no_ptrauth_returns() {}
#endif
