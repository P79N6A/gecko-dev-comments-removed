




#ifndef nsAuthSASL_h__
#define nsAuthSASL_h__

#include "nsIAuthModule.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"






class nsAuthSASL MOZ_FINAL : public nsIAuthModule
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
    bool           mSASLReady;
};

#endif 

