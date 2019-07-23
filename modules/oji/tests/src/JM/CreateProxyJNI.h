



































#include <nsISecureEnv.h>

class nsDummySecureEnv : public nsISecureEnv {
public:
    nsDummySecureEnv() {}

	

    NS_METHOD QueryInterface(const nsIID & uuid, void * *result) { return NS_OK; }

    NS_METHOD_(nsrefcnt) AddRef(void) { return NS_OK; }

    NS_METHOD_(nsrefcnt) Release(void) { return NS_OK; }


	

    NS_METHOD NewObject(  jclass clazz, 
                           jmethodID methodID,
                           jvalue *args, 
                          jobject* result,
                           nsISecurityContext* ctx = NULL) { return NS_OK; }
   
    NS_METHOD CallMethod(  jni_type type,
                            jobject obj, 
                            jmethodID methodID,
                            jvalue *args, 
                           jvalue* result,
                            nsISecurityContext* ctx = NULL) { return NS_OK; }

    NS_METHOD CallNonvirtualMethod(  jni_type type,
                                      jobject obj, 
                                      jclass clazz,
                                      jmethodID methodID,
                                      jvalue *args, 
                                     jvalue* result,
                                      nsISecurityContext* ctx = NULL) { return NS_OK; }

    NS_METHOD GetField(  jni_type type,
                          jobject obj, 
                          jfieldID fieldID,
                         jvalue* result,
                          nsISecurityContext* ctx = NULL) { return NS_OK; }

    NS_METHOD SetField( jni_type type,
                         jobject obj, 
                         jfieldID fieldID,
                         jvalue val,
                         nsISecurityContext* ctx = NULL) { return NS_OK; }

    NS_METHOD CallStaticMethod(  jni_type type,
                                  jclass clazz,
                                  jmethodID methodID,
                                  jvalue *args, 
                                 jvalue* result,
                                  nsISecurityContext* ctx = NULL) { return NS_OK; }

    NS_METHOD GetStaticField(  jni_type type,
                                jclass clazz, 
                                jfieldID fieldID, 
                               jvalue* result,
                                nsISecurityContext* ctx = NULL) { return NS_OK; }


    NS_METHOD SetStaticField( jni_type type,
                               jclass clazz, 
                               jfieldID fieldID,
                               jvalue val,
                               nsISecurityContext* ctx = NULL) { return NS_OK; }


    NS_METHOD GetVersion( jint* version) { return NS_OK; }

    NS_METHOD DefineClass(  const char* name,
                             jobject loader,
                             const jbyte *buf,
                             jsize len,
                            jclass* clazz) { return NS_OK; }

    NS_METHOD FindClass(  const char* name, 
                          jclass* clazz) { return NS_OK; }

    NS_METHOD GetSuperclass(  jclass sub,
                              jclass* super) { return NS_OK; }

    NS_METHOD IsAssignableFrom(  jclass sub,
                                  jclass super,
                                 jboolean* result) { return NS_OK; }

    NS_METHOD Throw(  jthrowable obj,
                      jint* result) { return NS_OK; }

    NS_METHOD ThrowNew(  jclass clazz,
                          const char *msg,
                         jint* result) { return NS_OK; }

    NS_METHOD ExceptionOccurred( jthrowable* result) { return NS_OK; }

    NS_METHOD ExceptionDescribe(void) { return NS_OK; }

    NS_METHOD ExceptionClear(void) { return NS_OK; }

    NS_METHOD FatalError( const char* msg) { return NS_OK; }

    NS_METHOD NewGlobalRef(  jobject lobj, 
                             jobject* result) { return NS_OK; }

    NS_METHOD DeleteGlobalRef( jobject gref) { return NS_OK; }

    NS_METHOD DeleteLocalRef( jobject obj) { return NS_OK; }

    NS_METHOD IsSameObject(  jobject obj1,
                              jobject obj2,
                             jboolean* result) { return NS_OK; }

    NS_METHOD AllocObject(  jclass clazz,
                            jobject* result) { return NS_OK; }

    NS_METHOD GetObjectClass(  jobject obj,
                               jclass* result) { return NS_OK; }

    NS_METHOD IsInstanceOf(  jobject obj,
                              jclass clazz,
                             jboolean* result) { return NS_OK; }

    NS_METHOD GetMethodID(  jclass clazz, 
                             const char* name,
                             const char* sig,
                            jmethodID* id) { return NS_OK; }

    NS_METHOD GetFieldID(  jclass clazz, 
                            const char* name,
                            const char* sig,
                           jfieldID* id) { return NS_OK; }

    NS_METHOD GetStaticMethodID(  jclass clazz, 
                                   const char* name,
                                   const char* sig,
                                  jmethodID* id) { return NS_OK; }

    NS_METHOD GetStaticFieldID(  jclass clazz, 
                                  const char* name,
                                  const char* sig,
                                 jfieldID* id) { return NS_OK; }

    NS_METHOD NewString(  const jchar* unicode,
                           jsize len,
                          jstring* result) { return NS_OK; }

    NS_METHOD GetStringLength(  jstring str,
                                jsize* result) { return NS_OK; }
    
    NS_METHOD GetStringChars(  jstring str,
                                jboolean *isCopy,
                               const jchar** result) { return NS_OK; }

    NS_METHOD ReleaseStringChars(  jstring str,
                                    const jchar *chars) { return NS_OK; }

    NS_METHOD NewStringUTF(  const char *utf,
                             jstring* result) { return NS_OK; }

    NS_METHOD GetStringUTFLength(  jstring str,
                                   jsize* result) { return NS_OK; }
    
    NS_METHOD GetStringUTFChars(  jstring str,
                                   jboolean *isCopy,
                                  const char** result) { return NS_OK; }

    NS_METHOD ReleaseStringUTFChars(  jstring str,
                                       const char *chars) { return NS_OK; }

    NS_METHOD GetArrayLength(  jarray array,
                               jsize* result) { return NS_OK; }

    NS_METHOD NewObjectArray(  jsize len,
    					  jclass clazz,
                          jobject init,
                         jobjectArray* result) { return NS_OK; }

    NS_METHOD GetObjectArrayElement(  jobjectArray array,
                                       jsize index,
                                      jobject* result) { return NS_OK; }

    NS_METHOD SetObjectArrayElement(  jobjectArray array,
                                       jsize index,
                                       jobject val) { return NS_OK; }

    NS_METHOD NewArray( jni_type element_type,
                          jsize len,
                         jarray* result) { return NS_OK; }

    NS_METHOD GetArrayElements(  jni_type type,
                                  jarray array,
                                  jboolean *isCopy,
                                 void* result) { return NS_OK; }

    NS_METHOD ReleaseArrayElements( jni_type type,
                                     jarray array,
                                     void *elems,
                                     jint mode) { return NS_OK; }


    NS_METHOD GetArrayRegion(  jni_type type,
                                jarray array,
                                jsize start,
                                jsize len,
                               void* buf) { return NS_OK; }

    NS_METHOD SetArrayRegion(  jni_type type,
                                jarray array,
                                jsize start,
                                jsize len,
                                void* buf) { return NS_OK; }

    NS_METHOD RegisterNatives(  jclass clazz,
                                 const JNINativeMethod *methods,
                                 jint nMethods,
                                jint* result) { return NS_OK; }

    NS_METHOD UnregisterNatives(  jclass clazz,
                                  jint* result) { return NS_OK; }

    NS_METHOD MonitorEnter(  jobject obj,
                             jint* result) { return NS_OK; }

    NS_METHOD MonitorExit(  jobject obj,
                            jint* result) { return NS_OK; }

    NS_METHOD GetJavaVM(  JavaVM **vm,
                          jint* result) { return NS_OK; }

};

