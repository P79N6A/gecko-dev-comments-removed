




































#include "jni.h"
#include "nsJavaXPTCStubWeakRef.h"
#include "nsJavaXPTCStub.h"
#include "nsJavaXPCOMBindingUtils.h"
#include "nsIInterfaceInfoManager.h"
















nsJavaXPTCStubWeakRef::nsJavaXPTCStubWeakRef(jobject aJavaObject,
                                             nsJavaXPTCStub* aXPTCStub)
  : mXPTCStub(aXPTCStub)
{
  JNIEnv* env = GetJNIEnv();
  jobject weakref = env->NewObject(weakReferenceClass,
                                   weakReferenceConstructorMID, aJavaObject);
  mWeakRef = env->NewGlobalRef(weakref);
}

nsJavaXPTCStubWeakRef::~nsJavaXPTCStubWeakRef()
{
  JNIEnv* env = GetJNIEnv();
  env->CallVoidMethod(mWeakRef, clearReferentMID);
  env->DeleteGlobalRef(mWeakRef);
  mXPTCStub->ReleaseWeakRef();
}

NS_IMPL_ADDREF(nsJavaXPTCStubWeakRef)
NS_IMPL_RELEASE(nsJavaXPTCStubWeakRef)

NS_IMPL_QUERY_INTERFACE1(nsJavaXPTCStubWeakRef, nsIWeakReference)

NS_IMETHODIMP
nsJavaXPTCStubWeakRef::QueryReferent(const nsIID& aIID, void** aInstancePtr)
{
  LOG(("nsJavaXPTCStubWeakRef::QueryReferent()\n"));

  
  
  
  JNIEnv* env = GetJNIEnv();
  jobject javaObject = env->CallObjectMethod(mWeakRef, getReferentMID);
  if (env->IsSameObject(javaObject, NULL))
    return NS_ERROR_NULL_POINTER;

  
  return mXPTCStub->QueryInterface(aIID, aInstancePtr);
}

