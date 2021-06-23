// RUN: %clang_cc1 -fblocks -triple arm64-apple-ios -fptrauth-calls -emit-llvm -std=c++11 %s -o - | FileCheck %s

// CHECK: @_Z8handler2v.ptrauth = private constant { i8*, i32, i64, i64 } { i8* bitcast (void ()* @_Z8handler2v to i8*), i32 0, i64 0, i64 0 }, section "llvm.ptrauth"
// CHECK: @global_handler = dso_local global void ()* bitcast ({ i8*, i32, i64, i64 }* @_Z8handler2v.ptrauth to void ()*)

// CHECK: define dso_local void @_Z7handlerv()
__attribute__((noinline)) void handler() {
    asm volatile("");
}

// CHECK: define dso_local void @_Z8handler2v()
__attribute__((noinline)) void handler2() {
    asm volatile("");
}
void (*global_handler)() = &handler2;

// CHECK: define dso_local void @_Z11callHandlerRFvvE(void ()* nonnull %handler)
void callHandler(void (&handler)()) {
    asm volatile("");
    // Check basic usage of function reference
    ^{
        handler();
    }();
// CHECK: [[HANDLER:%.*]] = load void ()*, void ()** %handler.addr
// CHECK: store void ()* [[HANDLER]], void ()** %block.captured
    asm volatile("");

    // Check escape of function reference
    ^{
        global_handler = handler;
    }();
// CHECK: [[CAPTURE_SLOT:%.*]] = getelementptr inbounds <{ i8*, i32, i32, i8*, %struct.__block_descriptor*, void ()* }>, <{ i8*, i32, i32, i8*, %struct.__block_descriptor*, void ()* }>* %block1, i32 0, i32 5
// CHECK: [[HANDLER:%.*]] = load void ()*, void ()** %handler.addr
// CHECK: store void ()* [[HANDLER]], void ()** [[CAPTURE_SLOT]]
    asm volatile("");

    // Check return of function reference
    ^{
        return handler;
    }()();
// CHECK: [[CAPTURE_SLOT:%.*]] = getelementptr inbounds <{ i8*, i32, i32, i8*, %struct.__block_descriptor*, void ()* }>, <{ i8*, i32, i32, i8*, %struct.__block_descriptor*, void ()* }>* %block9, i32 0, i32 5
// CHECK: [[HANDLER:%.*]] = load void ()*, void ()** %handler.addr
// CHECK: store void ()* [[HANDLER]], void ()** [[CAPTURE_SLOT]]
    asm volatile("");
}
