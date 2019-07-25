






































#ifndef mozilla_dom_ContentChild_h
#define mozilla_dom_ContentChild_h

#include "mozilla/dom/PContentChild.h"

#include "nsIObserverService.h"
#include "nsTObserverArray.h"
#include "nsIObserver.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsServiceManagerUtils.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsWeakReference.h"

struct ChromePackage;
struct ResourceMapping;
struct OverrideMapping;

namespace mozilla {
namespace dom {

class ContentChild : public PContentChild
{
public:
    ContentChild();
    virtual ~ContentChild();

    class nsPrefObserverStorage {
    public:
        nsPrefObserverStorage(nsIObserver *aObserver, nsCString aDomain,
                              nsCString aPrefRoot, bool aHoldWeak) {
            mDomain = aDomain;
            mPrefRoot = aPrefRoot;
            mObserver = aObserver;
            if (aHoldWeak) {
                nsCOMPtr<nsISupportsWeakReference> weakRefFactory = 
                    do_QueryInterface(aObserver);
                if (weakRefFactory)
                    mWeakRef = do_GetWeakReference(aObserver);
            } else {
                mWeakRef = nsnull;
            }
        }

        ~nsPrefObserverStorage() {
        }

        bool NotifyObserver() {
            nsCOMPtr<nsIObserver> observer;
            if (mWeakRef) {
                observer = do_QueryReferent(mWeakRef);
                if (!observer) {
                    
                    
                    return false;
                }
            } else {
                observer = mObserver;
            }

            nsCOMPtr<nsIPrefBranch> prefBranch;
            nsCOMPtr<nsIPrefService> prefService = 
                                      do_GetService(NS_PREFSERVICE_CONTRACTID);
            if (prefService) {
                prefService->GetBranch(mPrefRoot.get(), 
                                       getter_AddRefs(prefBranch));
                observer->Observe(prefBranch, "nsPref:changed",
                                  NS_ConvertASCIItoUTF16(mDomain).get());
            }
            return true;
        }

        nsIObserver* GetObserver() { return mObserver; }
        const nsCString& GetDomain() { return mDomain; }
        const nsCString& GetPrefRoot() { return mPrefRoot; }

    private:
        nsCOMPtr<nsIObserver> mObserver;
        nsWeakPtr mWeakRef;
        nsCString mPrefRoot;
        nsCString mDomain;
    };

    bool Init(MessageLoop* aIOLoop,
              base::ProcessHandle aParentHandle,
              IPC::Channel* aChannel);

    static ContentChild* GetSingleton() {
        NS_ASSERTION(sSingleton, "not initialized");
        return sSingleton;
    }

    
    virtual bool RecvDummy(Shmem& foo) { return true; }

    virtual PBrowserChild* AllocPBrowser();
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

    nsresult AddRemotePrefObserver(const nsCString &aDomain, 
                                   const nsCString &aPrefRoot, 
                                   nsIObserver *aObserver, PRBool aHoldWeak);
    nsresult RemoveRemotePrefObserver(const nsCString &aDomain, 
                                      const nsCString &aPrefRoot, 
                                      nsIObserver *aObserver);
    inline void ClearPrefObservers() {
        mPrefObserverArray.Clear();
    }

    virtual bool RecvNotifyRemotePrefObserver(
            const nsCString& aDomain);
    


private:
    NS_OVERRIDE
    virtual void ActorDestroy(ActorDestroyReason why);

    static ContentChild* sSingleton;

    nsTArray< nsAutoPtr<nsPrefObserverStorage> > mPrefObserverArray;

    DISALLOW_EVIL_CONSTRUCTORS(ContentChild);
};

} 
} 

#endif
