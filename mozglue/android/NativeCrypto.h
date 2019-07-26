
#include <jni.h>


#ifndef _Included_org_mozilla_gecko_background_nativecode_NativeCrypto
#define _Included_org_mozilla_gecko_background_nativecode_NativeCrypto
#ifdef __cplusplus
extern "C" {
#endif





JNIEXPORT jbyteArray JNICALL Java_org_mozilla_gecko_background_nativecode_NativeCrypto_pbkdf2SHA256
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jint, jint);






JNIEXPORT jbyteArray JNICALL Java_org_mozilla_gecko_background_nativecode_NativeCrypto_sha1
  (JNIEnv *, jclass, jbyteArray);

#ifdef __cplusplus
}
#endif
#endif
