// RUN: %clang_cc1 %s -E -triple=arm64-- | FileCheck %s --check-prefixes=NOCALLS,NOINTRIN,NORETS,NOQUAL
// RUN: %clang_cc1 %s -E -triple=arm64-- -fptrauth-calls | FileCheck %s --check-prefixes=CALLS,NOINTRIN,NORETS,NOQUAL,NOFUNC
// RUN: %clang_cc1 %s -E -triple=arm64-- -fptrauth-returns | FileCheck %s --check-prefixes=NOCALLS,NOINTRIN,RETS,NOQUAL,NOFUNC
// RUN: %clang_cc1 %s -E -triple=arm64-- -fptrauth-intrinsics | FileCheck %s --check-prefixes=NOCALLS,INTRIN,NORETS,QUAL,NOFUNC
// RUN: %clang_cc1 %s -E -triple=arm64e-apple-ios6.0 -fptrauth-intrinsics -fptrauth-function-pointer-type-discrimination | FileCheck %s --check-prefixes=NOCALLS,INTRIN,NORETS,QUAL,FUNC

#if __has_feature(ptrauth_calls)
// CALLS: has_ptrauth_calls
void has_ptrauth_calls() {}
#else
// NOCALLS: no_ptrauth_calls
void no_ptrauth_calls() {}
#endif

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

#if __has_feature(ptrauth_qualifier)
// QUAL: has_ptrauth_qualifier
void has_ptrauth_qualifier() {}
#else
// NOQUAL: no_ptrauth_qualifier
void no_ptrauth_qualifier() {}
#endif

// This is always enabled when ptrauth_calls is enabled, on new enough clangs.
#if __has_feature(ptrauth_member_function_pointer_type_discrimination)
// CALLS: has_ptrauth_member_function_pointer_type_discrimination
void has_ptrauth_member_function_pointer_type_discrimination() {}
#else
// NOCALLS: no_ptrauth_member_function_pointer_type_discrimination
void no_ptrauth_member_function_pointer_type_discrimination() {}
#endif

#include <ptrauth.h>

#if __has_feature(ptrauth_function_pointer_type_discrimination)
// FUNC: has_ptrauth_function_pointer_type_discrimination
int has_ptrauth_function_pointer_type_discrimination() {
// FUNC: return __builtin_ptrauth_type_discriminator(void (*)(void))
  return ptrauth_function_pointer_type_discriminator(void (*)(void));
}
#else
// NOFUNC: no_ptrauth_function_pointer_type_discrimination
int no_ptrauth_function_pointer_type_discrimination() {
// NOFUNC: return ((ptrauth_extra_data_t)0)
  return ptrauth_function_pointer_type_discriminator(void(*)(void));
}
#endif
