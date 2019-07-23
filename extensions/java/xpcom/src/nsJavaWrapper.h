




































#ifndef _nsJavaWrapper_h_
#define _nsJavaWrapper_h_

#include "jni.h"
#include "nsISupports.h"















nsresult GetNewOrUsedJavaWrapper(JNIEnv* env, nsISupports* aXPCOMObject,
                                 const nsIID& aIID, jobject aObjectLoader,
                                 jobject* aResult);











nsresult GetXPCOMInstFromProxy(JNIEnv* env, jobject aJavaObject,
                               void** aResult);

#endif 
