




































#ifndef _nsJavaXPTCStub_h_
#define _nsJavaXPTCStub_h_

#include "nsXPTCUtils.h"
#include "jni.h"
#include "nsVoidArray.h"
#include "nsIInterfaceInfo.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"
#include "nsJavaXPTCStubWeakRef.h"


#define NS_JAVAXPTCSTUB_IID \
{0x88dd8130, 0xebe6, 0x4431, {0x9d, 0xa7, 0xe6, 0xb7, 0x54, 0x74, 0xfb, 0x21}}

class nsJavaXPTCStub : protected nsAutoXPTCStub,
                       public nsSupportsWeakReference
{
  friend class nsJavaXPTCStubWeakRef;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISUPPORTSWEAKREFERENCE
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_JAVAXPTCSTUB_IID)

  nsJavaXPTCStub(jobject aJavaObject, nsIInterfaceInfo *aIInfo,
                 nsresult *rv);

  virtual ~nsJavaXPTCStub();

  
  NS_IMETHOD CallMethod(PRUint16 aMethodIndex,
                        const XPTMethodDescriptor *aInfo,
                        nsXPTCMiniVariant *aParams);

  nsISomeInterface* GetStub() { return mXPTCStub; }

  
  jobject GetJavaObject();

  
  
  void DeleteStrongRef();

private:
  NS_IMETHOD_(nsrefcnt) AddRefInternal();
  NS_IMETHOD_(nsrefcnt) ReleaseInternal();

  
  
  void Destroy();

  
  
  
  
  void ReleaseWeakRef();

  
  nsJavaXPTCStub * FindStubSupportingIID(const nsID &aIID);

  
  PRBool SupportsIID(const nsID &aIID);

  nsresult SetupJavaParams(const nsXPTParamInfo &aParamInfo,
                           const XPTMethodDescriptor* aMethodInfo,
                           PRUint16 aMethodIndex,
                           nsXPTCMiniVariant* aDispatchParams,
                           nsXPTCMiniVariant &aVariant,
                           jvalue &aJValue, nsACString &aMethodSig);
  nsresult GetRetvalSig(const nsXPTParamInfo* aParamInfo,
                        const XPTMethodDescriptor* aMethodInfo,
                        PRUint16 aMethodIndex,
                        nsXPTCMiniVariant* aDispatchParams,
                        nsACString &aRetvalSig);
  nsresult FinalizeJavaParams(const nsXPTParamInfo &aParamInfo,
                              const XPTMethodDescriptor* aMethodInfo,
                              PRUint16 aMethodIndex,
                              nsXPTCMiniVariant* aDispatchParams,
                              nsXPTCMiniVariant &aVariant,
                              jvalue &aJValue);
  nsresult SetXPCOMRetval();

  jobject                     mJavaWeakRef;
  jobject                     mJavaStrongRef;
  jint                        mJavaRefHashCode;
  nsCOMPtr<nsIInterfaceInfo>  mIInfo;

  nsVoidArray     mChildren; 
  nsJavaXPTCStub *mMaster;   

  nsAutoRefCnt    mWeakRefCnt;  
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsJavaXPTCStub, NS_JAVAXPTCSTUB_IID)

#endif 
