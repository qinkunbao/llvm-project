#!/bin/bash

# From http://www.hanshq.net/command-line-android.html
#
# Note: this expects $JAVA_HOME to be set and the JDK tools to be in $PATH.

set -eu

CHROMIUM="${HOME}/c/src"

SDK="${CHROMIUM}/third_party/android_tools/sdk"
NDK="${CHROMIUM}/third_party/android_ndk"

BUILD_TOOLS="${SDK}/build-tools/27.0.3"
PLATFORM="${SDK}/platforms/android-28"

LLVM="${HOME}/l/ra"

mkdir -p build/gen build/obj
mkdir -p build/apk build/apk/lib/arm64-v8a build/apk/lib/x86

"${BUILD_TOOLS}/aapt" package -f -m -J build/gen/ -S res \
    -M AndroidManifest.xml -I "${PLATFORM}/android.jar"

javac -source 1.7 -target 1.7 -bootclasspath "${JAVA_HOME}/jre/lib/rt.jar" \
    -classpath "${PLATFORM}/android.jar" -d build/obj \
    build/gen/net/hanshq/hello/R.java java/net/hanshq/hello/MainActivity.java

"${BUILD_TOOLS}/dx" --dex --output=build/apk/classes.dex build/obj/

function clang() {
  "${LLVM}"/bin/clang --target=aarch64-linux-android \
      --sysroot="${NDK}/platforms/android-27/arch-arm64" \
      -I"${NDK}/sysroot/usr/include" \
      -I"${NDK}/sysroot/usr/include/aarch64-linux-android" \
      -L"${NDK}/toolchains/aarch64-linux-android-4.9/prebuilt/linux-x86_64/lib/gcc/aarch64-linux-android/4.9.x" \
      -fuse-ld=lld -z defs \
      -fPIC -shared "$@" -llog
}

clang -o build/libloader.so \
  -g \
  -ffunction-sections \
  -fdata-sections \
  -fvisibility=hidden \
  -Wl,--gc-sections \
  -Wl,--module-symbol,init \
  -Wl,-soname,libloader.so \
  jni/hello.c jni/loader.c

"${LLVM}"/bin/llvm-objcopy build/libloader.so build/apk/lib/arm64-v8a/libloader.so --extract-module=1 --strip-all
"${LLVM}"/bin/llvm-objcopy build/libloader.so build/apk/lib/arm64-v8a/libhello.so --extract-module=2 --strip-all

"${BUILD_TOOLS}/aapt" package -f -M AndroidManifest.xml -S res/ -0 .so \
    -I "${PLATFORM}/android.jar" \
    -F build/Hello.unsigned.apk build/apk/

"${BUILD_TOOLS}/zipalign" -f -p 4 \
    build/Hello.unsigned.apk build/Hello.aligned.apk

# keytool -genkeypair -keystore keystore.jks -alias androidkey \
#     -validity 10000 -keyalg RSA -keysize 2048 \
#     -storepass android -keypass android

"${BUILD_TOOLS}/apksigner" sign --ks keystore.jks \
    --ks-key-alias androidkey --ks-pass pass:android \
    --key-pass pass:android --out build/Hello.apk \
    build/Hello.aligned.apk
