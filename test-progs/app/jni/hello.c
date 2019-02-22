#include <stdlib.h>
#include <jni.h>

static const char *const messages[] = {
        "Hello, world!",
        "Hej världen!",
        "Bonjour, monde!",
        "Hallo Welt!"
};

int (*rand_ptr)() = rand;

jstring JNICALL
Java_net_hanshq_hello_MainActivity_getMessage(JNIEnv *env, jobject obj)
{
        int i;

        i = rand_ptr() % (sizeof(messages) / sizeof(messages[0]));

        return (*env)->NewStringUTF(env, messages[i]);
}

JNIEXPORT void init(JNIEnv *env) {
  jclass cls = (*env)->FindClass(env, "net/hanshq/hello/MainActivity");
  if (!cls)
    abort();

  JNINativeMethod natives[] = {
      {"getMessage", "()Ljava/lang/String;", Java_net_hanshq_hello_MainActivity_getMessage},
  };
  if ((*env)->RegisterNatives(env, cls, natives,
                              sizeof(natives) / sizeof(natives[0])) != 0)
    abort();
}
