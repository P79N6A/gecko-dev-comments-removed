




































#ifndef nsAuthSASL_h__
#define nsAuthSASL_h__

#include "nsIAuthModule.h"
#include "nsString.h"
#include "nsCOMPtr.h"

class nsAuthSASL : public nsIAuthModule
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIAUTHMODULE

    nsAuthSASL();

private:
    ~nsAuthSASL() { Reset(); }

    void Reset();

    nsCOMPtr<nsIAuthModule> mInnerModule;
    nsString       mUsername;
    PRBool         mSASLReady;
};

#endif 

