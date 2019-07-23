







































#ifndef _Included_Test1
#define _Included_Test1
#ifdef __cplusplus
extern "C" {
#endif



#undef Test1_static_final_int
#define Test1_static_final_int 11L



















JNIEXPORT void JNICALL Java_Test1_mprint
  (JNIEnv *, jobject, jint);






JNIEXPORT void JNICALL Java_Test1_mprint_1static
  (JNIEnv *, jclass, jint);






JNIEXPORT jint JNICALL Java_Test1_Test1_1method3_1native
  (JNIEnv *, jobject, jboolean, jbyte, jchar, jshort, jint, jlong, jfloat, jdouble, jstring, jobjectArray);






JNIEXPORT jint JNICALL Java_Test1_Test1_1method3_1native_1static
  (JNIEnv *, jobject, jboolean, jbyte, jchar, jshort, jint, jlong, jfloat, jdouble, jstring, jobjectArray);

#ifdef __cplusplus
}
#endif
#endif
