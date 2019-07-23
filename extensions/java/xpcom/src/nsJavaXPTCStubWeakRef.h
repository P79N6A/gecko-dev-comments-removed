




































#ifndef _nsJavaXPTCStubWeakRef_h_
#define _nsJavaXPTCStubWeakRef_h_

#include "jni.h"
#include "nsIWeakReference.h"


class nsJavaXPTCStub;




class nsJavaXPTCStubWeakRef : public nsIWeakReference
{
public:
  nsJavaXPTCStubWeakRef(jobject aJavaObject, nsJavaXPTCStub* aXPTCStub);
  virtual ~nsJavaXPTCStubWeakRef();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIWEAKREFERENCE

protected:
  jobject         mWeakRef;
  nsJavaXPTCStub* mXPTCStub;
};

#endif 
