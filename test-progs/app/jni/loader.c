#include <android/dlext.h>
#include <android/log.h>
#include <dlfcn.h>
#include <jni.h>
#include <stdlib.h>
#include <string.h>

struct partition_index_entry {
  int32_t name_relptr;
  int32_t addr_relptr;
  uint32_t size;
};

extern struct partition_index_entry __part_index_begin[], __part_index_end[];

static void *read_relptr(int32_t *relptr) { return ((char *)relptr) + *relptr; }

void *dlopen_wrapper(const char *name, int flags) {
  for (struct partition_index_entry *part = __part_index_begin;
       part != __part_index_end; ++part) {
    if (strcmp(read_relptr(&part->name_relptr), name) == 0) {
      android_dlextinfo info = {};
      info.flags = ANDROID_DLEXT_RESERVED_ADDRESS;
      info.reserved_addr = read_relptr(&part->addr_relptr);
      info.reserved_size = part->size;

      return android_dlopen_ext(name, flags, &info);
    }
  }

  return dlopen(name, flags);
}

JNIEXPORT void JNICALL Java_net_hanshq_hello_MainActivity_loadDFM(JNIEnv *env) {
  void *handle = dlopen_wrapper("libhello.so", RTLD_NOW);
  if (!handle) {
    __android_log_print(ANDROID_LOG_VERBOSE, "dlerror", "%s\n", dlerror());
    abort();
  }

  void (*init)(JNIEnv *env) = dlsym(handle, "init");
  if (!init)
    abort();

  init(env);
}
