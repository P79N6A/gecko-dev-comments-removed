




































#include "StdAfx.h"

#include <Wininet.h>

#include "npapi.h"
#include "nsServiceManagerUtils.h"
#include "nsISupportsUtils.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsWeakReference.h"
#include "nsIObserver.h"
#include "nsStringAPI.h"

#include "XPConnect.h"



const PRUint32 kDefaultHostingFlags =
#ifdef XPC_IDISPATCH_SUPPORT
    nsIActiveXSecurityPolicy::HOSTING_FLAGS_HOST_NOTHING;
#else
    nsIActiveXSecurityPolicy::HOSTING_FLAGS_HOST_SAFE_OBJECTS |
    nsIActiveXSecurityPolicy::HOSTING_FLAGS_DOWNLOAD_CONTROLS |
    nsIActiveXSecurityPolicy::HOSTING_FLAGS_SCRIPT_SAFE_OBJECTS;
#endif

class PrefObserver :
    public nsSupportsWeakReference,
    public nsIObserver
{
public:
    PrefObserver();

protected:
    virtual ~PrefObserver();

    void Sync(nsIPrefBranch *aPrefBranch);
    
    PRUint32 mHostingFlags;
    nsCOMPtr<nsIPrefBranch2> mPrefBranch;
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    static PrefObserver *sPrefObserver;

    nsresult Subscribe();
    nsresult Unsubscribe();
    PRUint32 GetHostingFlags() const;
};

const char *kActiveXHostingFlags = "security.xpconnect.activex.";
const char *kUserAgentPref = "general.useragent.";
const char *kProxyPref = "network.http.";

PrefObserver *PrefObserver::sPrefObserver = nsnull;

PrefObserver::PrefObserver() :
    mHostingFlags(kDefaultHostingFlags)
{
    nsresult rv = NS_OK;
    mPrefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ASSERTION(mPrefBranch, "where is the pref service?");
}

PrefObserver::~PrefObserver()
{
}

NS_IMPL_ADDREF(PrefObserver)
NS_IMPL_RELEASE(PrefObserver)

NS_INTERFACE_MAP_BEGIN(PrefObserver)
    NS_INTERFACE_MAP_ENTRY(nsIObserver)
    NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END


NS_IMETHODIMP PrefObserver::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
    if (strcmp(NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, aTopic) != 0)
    {
        return S_OK;
    }

    nsresult rv;
    nsCOMPtr<nsIPrefBranch> prefBranch = do_QueryInterface(aSubject, &rv);
    if (NS_FAILED(rv))
        return rv;

    NS_ConvertUTF16toUTF8 pref(aData);
    if (strcmp(kActiveXHostingFlags, pref.get()) == 0 ||
        strcmp(kUserAgentPref, pref.get()) == 0 ||
        strcmp(kProxyPref, pref.get()) == 0)
    {
        Sync(prefBranch);
    }
    return NS_OK;
}

void PrefObserver::Sync(nsIPrefBranch *aPrefBranch)
{
    NS_ASSERTION(aPrefBranch, "no pref branch");
    if (!aPrefBranch)
    {
        return;
    }

    
    
	

    
    
    

    nsCOMPtr<nsIDispatchSupport> dispSupport = do_GetService(NS_IDISPATCH_SUPPORT_CONTRACTID);
    if (!dispSupport)
        mHostingFlags = kDefaultHostingFlags;
    else
        dispSupport->GetHostingFlags(nsnull, &mHostingFlags);
}

nsresult
PrefObserver::Subscribe()
{
    NS_ENSURE_TRUE(mPrefBranch, NS_ERROR_FAILURE);

    mPrefBranch->AddObserver(kProxyPref, this, PR_TRUE);
    mPrefBranch->AddObserver(kUserAgentPref, this, PR_TRUE);
    mPrefBranch->AddObserver(kActiveXHostingFlags, this, PR_TRUE);

    Sync(mPrefBranch);

    return S_OK;
}

nsresult
PrefObserver::Unsubscribe()
{
    NS_ENSURE_TRUE(mPrefBranch, NS_ERROR_FAILURE);

    mPrefBranch->RemoveObserver(kProxyPref, this);
    mPrefBranch->RemoveObserver(kUserAgentPref, this);
    mPrefBranch->RemoveObserver(kActiveXHostingFlags, this);

    return NS_OK;
}

PRUint32 PrefObserver::GetHostingFlags() const
{
    return mHostingFlags;
}



PRUint32 MozAxPlugin::PrefGetHostingFlags()
{
    if (!PrefObserver::sPrefObserver)
    {
        PrefObserver::sPrefObserver = new PrefObserver();
        if (!PrefObserver::sPrefObserver)
        {
            return nsIActiveXSecurityPolicy::HOSTING_FLAGS_HOST_NOTHING;
        }
        PrefObserver::sPrefObserver->AddRef();
        PrefObserver::sPrefObserver->Subscribe();
    }
    return PrefObserver::sPrefObserver->GetHostingFlags();
}

void MozAxPlugin::ReleasePrefObserver()
{
    if (PrefObserver::sPrefObserver)
    {
        PrefObserver::sPrefObserver->Unsubscribe();
        PrefObserver::sPrefObserver->Release();
        PrefObserver::sPrefObserver = nsnull;
    }
}

