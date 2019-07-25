




































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

