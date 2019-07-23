




































#ifndef nsISecureJNI_h___
#define nsISecureJNI_h___

#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsISecurityContext.h"
#include "jni.h"

class nsISecureJNI : public nsISupports {
public:

    










    NS_IMETHOD NewObject(  JNIEnv *env, 
                           jclass clazz, 
                           const char *name, 
                           const char *sig, 
                           jvalue *args, 
                           nsISecurityContext* ctx,
                          jvalue* result) = 0;
   
    










    NS_IMETHOD CallMethod(  JNIEnv *env, 
                            jobject obj, 
                            const char *name, 
                            const char *sig, 
                            jvalue *args, 
                            nsISecurityContext* ctx,
                           jvalue* result) = 0;

    










    NS_IMETHOD CallNonvirtualMethod(  JNIEnv *env, 
                                      jobject obj, 
                                      jclass clazz,
                                      const char *name, 
                                      const char *sig, 
                                      jvalue *args, 
                                      nsISecurityContext* ctx,
                                     jvalue* result) = 0;

    









    NS_IMETHOD GetField(  JNIEnv *env, 
                          jobject obj, 
                          const char *name, 
                          const char *sig, 
                          nsISecurityContext* ctx,
                         jvalue* result) = 0;

    









    NS_IMETHOD SetField( JNIEnv *env, 
                         jobject obj, 
                         const char *name, 
                         const char *sig, 
                         jvalue val,
                         nsISecurityContext* ctx) = 0;

    










    NS_IMETHOD CallStaticMethod(  JNIEnv *env, 
                                  jclass clazz,
                                  const char *name, 
                                  const char *sig, 
                                  jvalue *args, 
                                  nsISecurityContext* ctx,
                                 jvalue* result) = 0;

    









    NS_IMETHOD GetStaticField(  JNIEnv *env, 
                                jclass clazz, 
                                const char *name, 
                                const char *sig, 
                                nsISecurityContext* ctx,
                               jvalue* result) = 0;


    









    NS_IMETHOD SetStaticField( JNIEnv *env, 
                               jclass clazz, 
                               const char *name, 
                               const char *sig, 
                               jvalue val,
                               nsISecurityContext* ctx) = 0;
};

#define NS_ISECUREJNI_IID                         \
{ /* {7C968050-4C4B-11d2-A1CB-00805F8F694D} */         \
    0x7c968050,                                        \
    0x4c4b,                                            \
    0x11d2,                                            \
    { 0xa1, 0xcb, 0x0, 0x80, 0x5f, 0x8f, 0x69, 0x4d }  \
};

#endif 
