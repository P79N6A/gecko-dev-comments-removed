














































#include <jni.h>


#ifndef _Included_netscape_javascript_JSObject
#define _Included_netscape_javascript_JSObject
#ifdef __cplusplus
extern "C" {
#endif





JNIEXPORT void JNICALL Java_netscape_javascript_JSObject_initClass
  (JNIEnv *, jclass);






JNIEXPORT jobject JNICALL Java_netscape_javascript_JSObject_getMember
  (JNIEnv *, jobject, jstring);






JNIEXPORT jobject JNICALL Java_netscape_javascript_JSObject_getSlot
  (JNIEnv *, jobject, jint);






JNIEXPORT void JNICALL Java_netscape_javascript_JSObject_setMember
  (JNIEnv *, jobject, jstring, jobject);






JNIEXPORT void JNICALL Java_netscape_javascript_JSObject_setSlot
  (JNIEnv *, jobject, jint, jobject);






JNIEXPORT void JNICALL Java_netscape_javascript_JSObject_removeMember
  (JNIEnv *, jobject, jstring);






JNIEXPORT jobject JNICALL Java_netscape_javascript_JSObject_call
  (JNIEnv *, jobject, jstring, jobjectArray);






JNIEXPORT jobject JNICALL Java_netscape_javascript_JSObject_eval
  (JNIEnv *, jobject, jstring);






JNIEXPORT jstring JNICALL Java_netscape_javascript_JSObject_toString
  (JNIEnv *, jobject);






JNIEXPORT jobject JNICALL Java_netscape_javascript_JSObject_getWindow
  (JNIEnv *, jclass, jobject);






JNIEXPORT void JNICALL Java_netscape_javascript_JSObject_finalize
  (JNIEnv *, jobject);






JNIEXPORT jboolean JNICALL Java_netscape_javascript_JSObject_equals
  (JNIEnv *, jobject, jobject);

#ifdef __cplusplus
}
#endif
#endif
