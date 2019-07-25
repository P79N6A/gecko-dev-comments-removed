





































#ifndef mozilla_dom_ContentProcessParent_h
#define mozilla_dom_ContentProcessParent_h

#include "base/waitable_event_watcher.h"

#include "mozilla/dom/PContentProcessParent.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"

#include "nsIObserver.h"
#include "nsIThreadInternal.h"
#include "mozilla/Monitor.h"
#include "nsNetUtil.h"
#include "nsIPrefService.h"

namespace mozilla {

namespace ipc {
class TestShellParent;
}

namespace dom {

class TabParent;

class ContentProcessParent : public PContentProcessParent
                           , public nsIObserver
                           , public nsIThreadObserver
{
private:
    typedef mozilla::ipc::GeckoChildProcessHost GeckoChildProcessHost;
    typedef mozilla::ipc::TestShellParent TestShellParent;

public:
    static ContentProcessParent* GetSingleton(PRBool aForceNew = PR_TRUE);

#if 0
    
    static ContentProcessParent* FreeSingleton();
#endif

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSITHREADOBSERVER

    TabParent* CreateTab();

    TestShellParent* CreateTestShell();
    bool DestroyTestShell(TestShellParent* aTestShell);

    void ReportChildAlreadyBlocked();
    bool RequestRunToCompletion();

    bool IsAlive();

protected:
    virtual void ActorDestroy(ActorDestroyReason why);

private:
    static ContentProcessParent* gSingleton;

    
    
    using PContentProcessParent::SendPIFrameEmbeddingConstructor;
    using PContentProcessParent::SendPTestShellConstructor;

    ContentProcessParent();
    virtual ~ContentProcessParent();

    virtual PIFrameEmbeddingParent* AllocPIFrameEmbedding();
    virtual bool DeallocPIFrameEmbedding(PIFrameEmbeddingParent* frame);

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

    void EnsurePrefService();

    virtual bool RecvVisitURI(const IPC::URI& uri,
                              const IPC::URI& referrer,
                              const PRUint32& flags);

    mozilla::Monitor mMonitor;

    GeckoChildProcessHost* mSubprocess;

    int mRunToCompletionDepth;
    bool mShouldCallUnblockChild;
    nsCOMPtr<nsIThreadObserver> mOldObserver;

    bool mIsAlive;
    nsCOMPtr<nsIPrefBranch> mPrefService; 
};

} 
} 

#endif
