







































#ifndef nsAuthGSSAPI_h__
#define nsAuthGSSAPI_h__

#include "nsAuth.h"
#include "nsIAuthModule.h"
#include "nsString.h"

#define GSS_USE_FUNCTION_POINTERS 1

#include "gssapi.h"




class nsAuthGSSAPI : public nsIAuthModule
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIAUTHMODULE

    nsAuthGSSAPI(pType package);

private:
    ~nsAuthGSSAPI() { Reset(); }

    void    Reset();
    gss_OID GetOID() { return mMechOID; }

private:
    gss_ctx_id_t mCtx;
    gss_OID      mMechOID;
    nsCString    mServiceName;
    PRUint32     mServiceFlags;
    nsString     mUsername;
    PRBool       mComplete;
};

#endif 
