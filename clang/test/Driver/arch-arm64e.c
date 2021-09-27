// Check that we can manually enable specific ptrauth features.

// RUN: %clang -target arm64-apple-ios -c %s -### 2>&1 | FileCheck %s --check-prefix NONE
// NONE: "-cc1"
// NONE-NOT: "-fptrauth-intrinsics"
// NONE-NOT: "-fptrauth-returns"
// NONE-NOT: "-fptrauth-auth-traps"

// RUN: %clang -target arm64-apple-ios -fptrauth-intrinsics -c %s -### 2>&1 | FileCheck %s --check-prefix INTRIN
// INTRIN: "-cc1"{{.*}} {{.*}} "-fptrauth-intrinsics"

// RUN: %clang -target arm64-apple-ios -fptrauth-returns -c %s -### 2>&1 | FileCheck %s --check-prefix RETURN
// RETURN: "-cc1"{{.*}} {{.*}} "-fptrauth-returns"

// RUN: %clang -target arm64-apple-ios -fptrauth-auth-traps -c %s -### 2>&1 | FileCheck %s --check-prefix TRAPS
// TRAPS: "-cc1"{{.*}} {{.*}} "-fptrauth-auth-traps"


// Check the arm64e defaults.

// RUN: %clang -target arm64e-apple-ios -c %s -### 2>&1 | FileCheck %s --check-prefix DEFAULT
// RUN: %clang -mkernel -target arm64e-apple-ios -c %s -### 2>&1 | FileCheck %s --check-prefix DEFAULT-KERN
// RUN: %clang -fapple-kext -target arm64e-apple-ios -c %s -### 2>&1 | FileCheck %s --check-prefix DEFAULT-KERN
// DEFAULT: "-fptrauth-returns" "-fptrauth-intrinsics" "-fptrauth-auth-traps" "-target-cpu" "apple-a12"{{.*}}
// DEFAULT-KERN: "-fptrauth-returns" "-fptrauth-intrinsics" "-fptrauth-auth-traps" "-target-cpu" "apple-a12"{{.*}}

// RUN: %clang -target arm64e-apple-ios -fno-ptrauth-returns -c %s -### 2>&1 | FileCheck %s --check-prefix NORET

// NORET-NOT: "-fptrauth-returns"
// NORET: "-fptrauth-intrinsics" "-fptrauth-auth-traps" "-target-cpu" "apple-a12"

// RUN: %clang -target arm64e-apple-ios -fno-ptrauth-intrinsics -c %s -### 2>&1 | FileCheck %s --check-prefix NOINTRIN

// NOINTRIN: "-fptrauth-returns"
// NOINTRIN-NOT: "-fptrauth-intrinsics"
// NOINTRIN: "-fptrauth-auth-traps" "-target-cpu" "apple-a12"{{.*}}


// RUN: %clang -target arm64e-apple-ios -fno-ptrauth-auth-traps -c %s -### 2>&1 | FileCheck %s --check-prefix NOTRAP
// NOTRAP: "-fptrauth-returns" "-fptrauth-intrinsics" "-target-cpu" "apple-a12"
