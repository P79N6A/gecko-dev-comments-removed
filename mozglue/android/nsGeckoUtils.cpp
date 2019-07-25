




































#include <jni.h>
#include <stdlib.h>

extern "C"
__attribute__ ((visibility("default")))
void JNICALL
Java_org_mozilla_gecko_GeckoAppShell_putenv(JNIEnv *jenv, jclass, jstring map)
{
    const char* str;
    
    
    str = jenv->GetStringUTFChars(map, NULL);
    if (str == NULL)
        return;
    putenv(strdup(str));
    jenv->ReleaseStringUTFChars(map, str);
}

extern "C"
__attribute__ ((visibility("default")))
jobject JNICALL
Java_org_mozilla_gecko_GeckoAppShell_allocateDirectBuffer(JNIEnv *jenv, jclass, jlong size)
{
    return jenv->NewDirectByteBuffer(malloc(size), size);
}

extern "C"
__attribute__ ((visibility("default")))
void JNICALL
Java_org_mozilla_gecko_GeckoAppShell_freeDirectBuffer(JNIEnv *jenv, jclass, jobject buf)
{
    free(jenv->GetDirectBufferAddress(buf));
}

