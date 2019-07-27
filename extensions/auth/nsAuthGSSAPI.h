




#ifndef nsAuthGSSAPI_h__
#define nsAuthGSSAPI_h__

#include "nsAuth.h"
#include "nsIAuthModule.h"
#include "nsString.h"
#include "mozilla/Attributes.h"

#define GSS_USE_FUNCTION_POINTERS 1

#include "gssapi.h"





















class nsAuthGSSAPI MOZ_FINAL : public nsIAuthModule
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIAUTHMODULE

    explicit nsAuthGSSAPI(pType package);

    static void Shutdown();

private:
    ~nsAuthGSSAPI() { Reset(); }

    void    Reset();
    gss_OID GetOID() { return mMechOID; }

private:
    gss_ctx_id_t mCtx;
    gss_OID      mMechOID;
    nsCString    mServiceName;
    uint32_t     mServiceFlags;
    nsString     mUsername;
    bool         mComplete;
};

#endif 
