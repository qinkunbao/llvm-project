#include <android/dlext.h>
#include <android/log.h>
#include <dlfcn.h>
#include <jni.h>
#include <stdlib.h>

extern char __mod0_ehdr_start[];
extern char __mod_end[];

JNIEXPORT void JNICALL Java_net_hanshq_hello_MainActivity_loadDFM(JNIEnv *env) {
  android_dlextinfo info = {};
  info.flags = ANDROID_DLEXT_RESERVED_ADDRESS;
  info.reserved_addr = __mod0_ehdr_start;
  info.reserved_size = __mod_end - __mod0_ehdr_start;

  void *handle = android_dlopen_ext("libhello.so", RTLD_NOW, &info);
  if (!handle) {
    __android_log_print(ANDROID_LOG_VERBOSE, "dlerror", "%s\n", dlerror());
    abort();
  }

  void (*init)(JNIEnv *env) = dlsym(handle, "init");
  if (!init)
    abort();

  init(env);
}
