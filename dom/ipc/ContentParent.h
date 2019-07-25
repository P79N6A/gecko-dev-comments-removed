





































#ifndef mozilla_dom_ContentParent_h
#define mozilla_dom_ContentParent_h

#include "base/waitable_event_watcher.h"

#include "mozilla/dom/PContentParent.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"

#include "nsIObserver.h"
#include "nsIThreadInternal.h"
#include "mozilla/Monitor.h"
#include "nsNetUtil.h"
#include "nsIPrefService.h"
#include "nsIPermissionManager.h"

namespace mozilla {

namespace ipc {
class TestShellParent;
}

namespace dom {

class TabParent;

class ContentParent : public PContentParent
                    , public nsIObserver
                    , public nsIThreadObserver
{
private:
    typedef mozilla::ipc::GeckoChildProcessHost GeckoChildProcessHost;
    typedef mozilla::ipc::TestShellParent TestShellParent;

public:
    static ContentParent* GetSingleton(PRBool aForceNew = PR_TRUE);

#if 0
    
    static ContentParent* FreeSingleton();
#endif

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSITHREADOBSERVER

    TabParent* CreateTab(PRUint32 aChromeFlags);

    TestShellParent* CreateTestShell();
    bool DestroyTestShell(TestShellParent* aTestShell);

    void ReportChildAlreadyBlocked();
    bool RequestRunToCompletion();

    bool IsAlive();

protected:
    virtual void ActorDestroy(ActorDestroyReason why);

private:
    static ContentParent* gSingleton;

    
    
    using PContentParent::SendPBrowserConstructor;
    using PContentParent::SendPTestShellConstructor;

    ContentParent();
    virtual ~ContentParent();

    virtual PBrowserParent* AllocPBrowser(const PRUint32& aChromeFlags);
    virtual bool DeallocPBrowser(PBrowserParent* frame);

    virtual PTestShellParent* AllocPTestShell();
    virtual bool DeallocPTestShell(PTestShellParent* shell);

    virtual PNeckoParent* AllocPNecko();
    virtual bool DeallocPNecko(PNeckoParent* necko);

    virtual bool RecvGetPrefType(const nsCString& prefName,
            PRInt32* retValue, nsresult* rv);

    virtual bool RecvGetBoolPref(const nsCString& prefName,
            PRBool* retValue, nsresult* rv);

    virtual bool RecvGetIntPref(const nsCString& prefName,
            PRInt32* retValue, nsresult* rv);

    virtual bool RecvGetCharPref(const nsCString& prefName,
            nsCString* retValue, nsresult* rv);

    virtual bool RecvGetPrefLocalizedString(const nsCString& prefName,
            nsString* retValue, nsresult* rv);

    virtual bool RecvPrefHasUserValue(const nsCString& prefName,
            PRBool* retValue, nsresult* rv);

    virtual bool RecvPrefIsLocked(const nsCString& prefName,
            PRBool* retValue, nsresult* rv);

    virtual bool RecvGetChildList(const nsCString& domain,
            nsTArray<nsCString>* list, nsresult* rv);

    virtual bool RecvTestPermission(const IPC::URI&  aUri,
                                    const nsCString& aType,
                                    const PRBool&    aExact,
                                    PRUint32*        retValue);

    void EnsurePrefService();
    void EnsurePermissionService();

    virtual bool RecvStartVisitedQuery(const IPC::URI& uri);

    virtual bool RecvVisitURI(const IPC::URI& uri,
                              const IPC::URI& referrer,
                              const PRUint32& flags);

    virtual bool RecvSetURITitle(const IPC::URI& uri,
                                 const nsString& title);
    
    virtual bool RecvNotifyIME(const int&, const int&);

    virtual bool RecvNotifyIMEChange(const nsString&, const PRUint32&, const int&, 
                               const int&, const int&)
;


    mozilla::Monitor mMonitor;

    GeckoChildProcessHost* mSubprocess;

    int mRunToCompletionDepth;
    bool mShouldCallUnblockChild;
    nsCOMPtr<nsIThreadObserver> mOldObserver;

    bool mIsAlive;
    nsCOMPtr<nsIPrefBranch> mPrefService; 
    nsCOMPtr<nsIPermissionManager> mPermissionService; 
};

} 
} 

#endif
