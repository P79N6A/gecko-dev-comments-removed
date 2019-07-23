




































#ifndef nsXPTCUtils_h__
#define nsXPTCUtils_h__

#include "xptcall.h"





class nsAutoXPTCStub : protected nsIXPTCProxy
{
public:
  nsISomeInterface* mXPTCStub;

protected:
  nsAutoXPTCStub() : mXPTCStub(nsnull) { }

  nsresult
  InitStub(const nsIID& aIID)
  {
    return NS_GetXPTCallStub(aIID, this, &mXPTCStub);
  }

  ~nsAutoXPTCStub()
  {
    if (mXPTCStub)
      NS_DestroyXPTCallStub(mXPTCStub);
  }
};

#endif 
