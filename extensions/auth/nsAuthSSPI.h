




































#ifndef nsAuthSSPI_h__
#define nsAuthSSPI_h__

#include "nsAuth.h"
#include "nsIAuthModule.h"
#include "nsString.h"

#include <windows.h>

#define SECURITY_WIN32 1
#include <ntsecapi.h>
#include <security.h>
#include <rpc.h>










class nsAuthSSPI : public nsIAuthModule
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIAUTHMODULE

    nsAuthSSPI(pType package = PACKAGE_TYPE_NEGOTIATE);

private:
    ~nsAuthSSPI();

    void Reset();

private:
    CredHandle   mCred;
    CtxtHandle   mCtxt;
    nsCString    mServiceName;
    PRUint32     mServiceFlags;
    PRUint32     mMaxTokenLen;
    pType        mPackage;
    nsString     mDomain;
    nsString     mUsername;
    nsString     mPassword;
};

#endif 
