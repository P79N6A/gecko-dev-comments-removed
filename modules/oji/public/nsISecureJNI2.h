




































#ifndef nsISecureJNI2_h___
#define nsISecureJNI2_h___

#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsISecurityContext.h"
#include "jni.h"

enum jni_type 
{
    jobject_type = 0,
    jboolean_type,
    jbyte_type,
    jchar_type,
    jshort_type,
    jint_type,
    jlong_type,
    jfloat_type,
    jdouble_type,
    jvoid_type
};

class nsISecureJNI2 : public nsISupports {
public:

    








    NS_IMETHOD NewObject(  jclass clazz, 
                           jmethodID methodID,
                           jvalue *args, 
                          jobject* result,
                           nsISecurityContext* ctx = NULL) = 0;
   
    









    NS_IMETHOD CallMethod(  jni_type type,
                            jobject obj, 
                            jmethodID methodID,
                            jvalue *args, 
                           jvalue* result,
                            nsISecurityContext* ctx = NULL) = 0;

    










    NS_IMETHOD CallNonvirtualMethod(  jni_type type,
                                      jobject obj, 
                                      jclass clazz,
                                      jmethodID methodID,
                                      jvalue *args, 
                                     jvalue* result,
                                      nsISecurityContext* ctx = NULL) = 0;

    








    NS_IMETHOD GetField(  jni_type type,
                          jobject obj, 
                          jfieldID fieldID,
                         jvalue* result,
                          nsISecurityContext* ctx = NULL) = 0;

    








    NS_IMETHOD SetField( jni_type type,
                         jobject obj, 
                         jfieldID fieldID,
                         jvalue val,
                         nsISecurityContext* ctx = NULL) = 0;

    









    NS_IMETHOD CallStaticMethod(  jni_type type,
                                  jclass clazz,
                                  jmethodID methodID,
                                  jvalue *args, 
                                 jvalue* result,
                                  nsISecurityContext* ctx = NULL) = 0;

    








    NS_IMETHOD GetStaticField(  jni_type type,
                                jclass clazz, 
                                jfieldID fieldID, 
                               jvalue* result,
                                nsISecurityContext* ctx = NULL) = 0;


    








    NS_IMETHOD SetStaticField( jni_type type,
                               jclass clazz, 
                               jfieldID fieldID,
                               jvalue val,
                               nsISecurityContext* ctx = NULL) = 0;


    NS_IMETHOD GetVersion( jint* version) = 0;

    NS_IMETHOD DefineClass(  const char* name,
                             jobject loader,
                             const jbyte *buf,
                             jsize len,
                            jclass* clazz) = 0;

    NS_IMETHOD FindClass(  const char* name, 
                          jclass* clazz) = 0;

    NS_IMETHOD GetSuperclass(  jclass sub,
                              jclass* super) = 0;

    NS_IMETHOD IsAssignableFrom(  jclass sub,
                                  jclass super,
                                 jboolean* result) = 0;

    NS_IMETHOD Throw(  jthrowable obj,
                      jint* result) = 0;

    NS_IMETHOD ThrowNew(  jclass clazz,
                          const char *msg,
                         jint* result) = 0;

    NS_IMETHOD ExceptionOccurred( jthrowable* result) = 0;

    NS_IMETHOD ExceptionDescribe(void) = 0;

    NS_IMETHOD ExceptionClear(void) = 0;

    NS_IMETHOD FatalError( const char* msg) = 0;

    NS_IMETHOD NewGlobalRef(  jobject lobj, 
                             jobject* result) = 0;

    NS_IMETHOD DeleteGlobalRef( jobject gref) = 0;

    NS_IMETHOD DeleteLocalRef( jobject obj) = 0;

    NS_IMETHOD IsSameObject(  jobject obj1,
                              jobject obj2,
                             jboolean* result) = 0;

    NS_IMETHOD AllocObject(  jclass clazz,
                            jobject* result) = 0;

    NS_IMETHOD GetObjectClass(  jobject obj,
                               jclass* result) = 0;

    NS_IMETHOD IsInstanceOf(  jobject obj,
                              jclass clazz,
                             jboolean* result) = 0;

    NS_IMETHOD GetMethodID(  jclass clazz, 
                             const char* name,
                             const char* sig,
                            jmethodID* id) = 0;

    NS_IMETHOD GetFieldID(  jclass clazz, 
                            const char* name,
                            const char* sig,
                           jfieldID* id) = 0;

    NS_IMETHOD GetStaticMethodID(  jclass clazz, 
                                   const char* name,
                                   const char* sig,
                                  jmethodID* id) = 0;

    NS_IMETHOD GetStaticFieldID(  jclass clazz, 
                                  const char* name,
                                  const char* sig,
                                 jfieldID* id) = 0;

    NS_IMETHOD NewString(  const jchar* unicode,
                           jsize len,
                          jstring* result) = 0;

    NS_IMETHOD GetStringLength(  jstring str,
                                jsize* result) = 0;
    
    NS_IMETHOD GetStringChars(  jstring str,
                                jboolean *isCopy,
                               const jchar** result) = 0;

    NS_IMETHOD ReleaseStringChars(  jstring str,
                                    const jchar *chars) = 0;

    NS_IMETHOD NewStringUTF(  const char *utf,
                             jstring* result) = 0;

    NS_IMETHOD GetStringUTFLength(  jstring str,
                                   jsize* result) = 0;
    
    NS_IMETHOD GetStringUTFChars(  jstring str,
                                   jboolean *isCopy,
                                  const char** result) = 0;

    NS_IMETHOD ReleaseStringUTFChars(  jstring str,
                                       const char *chars) = 0;

    NS_IMETHOD GetArrayLength(  jarray array,
                               jsize* result) = 0;

    NS_IMETHOD NewObjectArray(  jsize len,
    					  jclass clazz,
                          jobject init,
                         jobjectArray* result) = 0;

    NS_IMETHOD GetObjectArrayElement(  jobjectArray array,
                                       jsize index,
                                      jobject* result) = 0;

    NS_IMETHOD SetObjectArrayElement(  jobjectArray array,
                                       jsize index,
                                       jobject val) = 0;

    NS_IMETHOD NewArray( jni_type element_type,
                          jsize len,
                         jarray* result) = 0;

    NS_IMETHOD GetArrayElements(  jni_type type,
                                  jarray array,
                                  jboolean *isCopy,
                                 void* result) = 0;

    NS_IMETHOD ReleaseArrayElements( jni_type type,
                                     jarray array,
                                     void *elems,
                                     jint mode) = 0;

    NS_IMETHOD GetArrayRegion(  jni_type type,
                                jarray array,
                                jsize start,
                                jsize len,
                               void* buf) = 0;

    NS_IMETHOD SetArrayRegion(  jni_type type,
                                jarray array,
                                jsize start,
                                jsize len,
                                void* buf) = 0;

    NS_IMETHOD RegisterNatives(  jclass clazz,
                                 const JNINativeMethod *methods,
                                 jint nMethods,
                                jint* result) = 0;

    NS_IMETHOD UnregisterNatives(  jclass clazz,
                                  jint* result) = 0;

    NS_IMETHOD MonitorEnter(  jobject obj,
                             jint* result) = 0;

    NS_IMETHOD MonitorExit(  jobject obj,
                            jint* result) = 0;

    NS_IMETHOD GetJavaVM(  JavaVM **vm,
                          jint* result) = 0;
};

#define NS_ISECUREJNI2_IID                              \
{   /* ca9148d0-598a-11d2-a1d4-00805f8f694d */          \
    0xca9148d0,                                         \
    0x598a,                                             \
    0x11d2,                                             \
    {0xa1, 0xd4, 0x00, 0x80, 0x5f, 0x8f, 0x69, 0x4d }   \
}

#endif 
