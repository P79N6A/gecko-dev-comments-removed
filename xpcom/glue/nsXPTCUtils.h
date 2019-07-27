





#ifndef nsXPTCUtils_h__
#define nsXPTCUtils_h__

#include "xptcall.h"
#include "mozilla/MemoryReporting.h"





class nsAutoXPTCStub : protected nsIXPTCProxy
{
public:
  nsISomeInterface* mXPTCStub;

protected:
  nsAutoXPTCStub() : mXPTCStub(nullptr) {}

  nsresult
  InitStub(const nsIID& aIID)
  {
    return NS_GetXPTCallStub(aIID, this, &mXPTCStub);
  }

  ~nsAutoXPTCStub()
  {
    if (mXPTCStub) {
      NS_DestroyXPTCallStub(mXPTCStub);
    }
  }

  size_t
  SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return mXPTCStub ? NS_SizeOfIncludingThisXPTCallStub(mXPTCStub, aMallocSizeOf) : 0;
  }
};

#endif 
