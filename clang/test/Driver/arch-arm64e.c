// Check that we can manually enable specific ptrauth features.

// RUN: %clang -target arm64-apple-ios -c %s -### 2>&1 | FileCheck %s --check-prefix NONE
// NONE: "-cc1"
// NONE-NOT: "-fptrauth-intrinsics"

// RUN: %clang -target arm64-apple-ios -fptrauth-intrinsics -c %s -### 2>&1 | FileCheck %s --check-prefix INTRIN
// INTRIN: "-cc1"{{.*}} {{.*}} "-fptrauth-intrinsics"


// Check the arm64e defaults.

// RUN: %clang -target arm64e-apple-ios -c %s -### 2>&1 | FileCheck %s --check-prefix DEFAULT
// RUN: %clang -mkernel -target arm64e-apple-ios -c %s -### 2>&1 | FileCheck %s --check-prefix DEFAULT-KERN
// RUN: %clang -fapple-kext -target arm64e-apple-ios -c %s -### 2>&1 | FileCheck %s --check-prefix DEFAULT-KERN
// DEFAULT: "-fptrauth-intrinsics" "-target-cpu" "apple-a12"{{.*}}
// DEFAULT-KERN: "-fptrauth-intrinsics" "-target-cpu" "apple-a12"{{.*}}

// RUN: %clang -target arm64e-apple-ios -fno-ptrauth-intrinsics -c %s -### 2>&1 | FileCheck %s --check-prefix NOINTRIN

// NOINTRIN-NOT: "-fptrauth-intrinsics"
// NOINTRIN: "-target-cpu" "apple-a12"{{.*}}
