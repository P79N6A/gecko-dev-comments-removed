






































#ifndef mozilla_dom_ContentChild_h
#define mozilla_dom_ContentChild_h

#include "mozilla/dom/PContentChild.h"

#include "nsTArray.h"

struct ChromePackage;
class nsIObserver;
struct ResourceMapping;
struct OverrideMapping;

namespace mozilla {
namespace dom {

class AlertObserver;
class PrefObserver;

class ContentChild : public PContentChild
{
public:
    ContentChild();
    virtual ~ContentChild();

    bool Init(MessageLoop* aIOLoop,
              base::ProcessHandle aParentHandle,
              IPC::Channel* aChannel);

    static ContentChild* GetSingleton() {
        NS_ASSERTION(sSingleton, "not initialized");
        return sSingleton;
    }

    
    virtual bool RecvDummy(Shmem& foo) { return true; }

    virtual PBrowserChild* AllocPBrowser(const PRUint32& aChromeFlags);
    virtual bool DeallocPBrowser(PBrowserChild*);

    virtual PTestShellChild* AllocPTestShell();
    virtual bool DeallocPTestShell(PTestShellChild*);
    virtual bool RecvPTestShellConstructor(PTestShellChild*);

    virtual PNeckoChild* AllocPNecko();
    virtual bool DeallocPNecko(PNeckoChild*);

    virtual bool RecvRegisterChrome(const nsTArray<ChromePackage>& packages,
                                    const nsTArray<ResourceMapping>& resources,
                                    const nsTArray<OverrideMapping>& overrides);

    virtual bool RecvSetOffline(const PRBool& offline);

    virtual bool RecvNotifyVisited(const IPC::URI& aURI);

    



    nsresult AddRemotePrefObserver(const nsCString& aDomain, 
                                   const nsCString& aPrefRoot, 
                                   nsIObserver* aObserver, PRBool aHoldWeak);
    nsresult RemoveRemotePrefObserver(const nsCString& aDomain, 
                                      const nsCString& aPrefRoot, 
                                      nsIObserver* aObserver);

    
    nsresult AddRemoteAlertObserver(const nsString& aData, nsIObserver* aObserver);

    virtual bool RecvNotifyRemotePrefObserver(const nsCString& aDomain);
    
    virtual bool RecvNotifyAlertsObserver(const nsCString& aType, const nsString& aData);

    virtual bool RecvAsyncMessage(const nsString& aMsg, const nsString& aJSON);

private:
    NS_OVERRIDE
    virtual void ActorDestroy(ActorDestroyReason why);

    nsTArray<nsAutoPtr<AlertObserver> > mAlertObservers;
    nsTArray<nsAutoPtr<PrefObserver> > mPrefObservers;
    bool mDead;

    static ContentChild* sSingleton;

    DISALLOW_EVIL_CONSTRUCTORS(ContentChild);
};

} 
} 

#endif
