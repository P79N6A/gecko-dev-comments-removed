






































#include "ContentProcessParent.h"

#include "TabParent.h"
#include "History.h"
#include "mozilla/ipc/TestShellParent.h"
#include "mozilla/net/NeckoParent.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsIPrefLocalizedString.h"
#include "nsIObserverService.h"
#include "nsContentUtils.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsChromeRegistryChrome.h"

using namespace mozilla::ipc;
using namespace mozilla::net;
using namespace mozilla::places;
using mozilla::MonitorAutoEnter;

namespace mozilla {
namespace dom {

#define NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC "ipc:network:set-offline"

ContentProcessParent* ContentProcessParent::gSingleton;

ContentProcessParent*
ContentProcessParent::GetSingleton(PRBool aForceNew)
{
    if (gSingleton && !gSingleton->IsAlive())
        gSingleton = nsnull;
    
    if (!gSingleton && aForceNew) {
        nsRefPtr<ContentProcessParent> parent = new ContentProcessParent();
        if (parent) {
            nsCOMPtr<nsIObserverService> obs =
                do_GetService("@mozilla.org/observer-service;1");
            if (obs) {
                if (NS_SUCCEEDED(obs->AddObserver(parent, "xpcom-shutdown",
                                                  PR_FALSE))) {
                    gSingleton = parent;
                    nsCOMPtr<nsIPrefBranch2> prefs 
                        (do_GetService(NS_PREFSERVICE_CONTRACTID));
                    if (prefs) {  
                        prefs->AddObserver("", parent, PR_FALSE);
                    }
                }
                obs->AddObserver(
                  parent, NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC, PR_FALSE); 
            }
            nsCOMPtr<nsIThreadInternal>
                threadInt(do_QueryInterface(NS_GetCurrentThread()));
            if (threadInt) {
                threadInt->GetObserver(getter_AddRefs(parent->mOldObserver));
                threadInt->SetObserver(parent);
            }
        }
    }
    return gSingleton;
}

void
ContentProcessParent::ActorDestroy(ActorDestroyReason why)
{
    nsCOMPtr<nsIThreadObserver>
        kungFuDeathGrip(static_cast<nsIThreadObserver*>(this));
    nsCOMPtr<nsIObserverService>
        obs(do_GetService("@mozilla.org/observer-service;1"));
    if (obs)
        obs->RemoveObserver(static_cast<nsIObserver*>(this), "xpcom-shutdown");
    nsCOMPtr<nsIThreadInternal>
        threadInt(do_QueryInterface(NS_GetCurrentThread()));
    if (threadInt)
        threadInt->SetObserver(mOldObserver);
    if (mRunToCompletionDepth)
        mRunToCompletionDepth = 0;

    mIsAlive = false;
}

TabParent*
ContentProcessParent::CreateTab()
{
  return static_cast<TabParent*>(SendPIFrameEmbeddingConstructor());
}

TestShellParent*
ContentProcessParent::CreateTestShell()
{
  return static_cast<TestShellParent*>(SendPTestShellConstructor());
}

bool
ContentProcessParent::DestroyTestShell(TestShellParent* aTestShell)
{
    return PTestShellParent::Send__delete__(aTestShell);
}

ContentProcessParent::ContentProcessParent()
    : mMonitor("ContentProcessParent::mMonitor")
    , mRunToCompletionDepth(0)
    , mShouldCallUnblockChild(false)
    , mIsAlive(true)
{
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    mSubprocess = new GeckoChildProcessHost(GeckoProcessType_Content);
    mSubprocess->AsyncLaunch();
    Open(mSubprocess->GetChannel(), mSubprocess->GetChildProcessHandle());

    nsCOMPtr<nsIChromeRegistry> registrySvc = nsChromeRegistry::GetService();
    nsChromeRegistryChrome* chromeRegistry =
        static_cast<nsChromeRegistryChrome*>(registrySvc.get());
    chromeRegistry->SendRegisteredChrome(this);
}

ContentProcessParent::~ContentProcessParent()
{
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    
    
    if (gSingleton == this)
        gSingleton = nsnull;
}

bool
ContentProcessParent::IsAlive()
{
    return mIsAlive;
}

bool
ContentProcessParent::RecvGetPrefType(const nsCString& prefName,
                                      PRInt32* retValue, nsresult* rv)
{
    *retValue = 0;

    EnsurePrefService();
    *rv = mPrefService->GetPrefType(prefName.get(), retValue);
    return true;
}

bool
ContentProcessParent::RecvGetBoolPref(const nsCString& prefName,
                                      PRBool* retValue, nsresult* rv)
{
    *retValue = PR_FALSE;

    EnsurePrefService();
    *rv = mPrefService->GetBoolPref(prefName.get(), retValue);
    return true;
}

bool
ContentProcessParent::RecvGetIntPref(const nsCString& prefName,
                                     PRInt32* retValue, nsresult* rv)
{
    *retValue = 0;

    EnsurePrefService();
    *rv = mPrefService->GetIntPref(prefName.get(), retValue);
    return true;
}

bool
ContentProcessParent::RecvGetCharPref(const nsCString& prefName,
                                      nsCString* retValue, nsresult* rv)
{
    EnsurePrefService();
    *rv = mPrefService->GetCharPref(prefName.get(), getter_Copies(*retValue));
    return true;
}

bool
ContentProcessParent::RecvGetPrefLocalizedString(const nsCString& prefName,
                                                 nsString* retValue, nsresult* rv)
{
    EnsurePrefService();
    nsCOMPtr<nsIPrefLocalizedString> string;
    *rv = mPrefService->GetComplexValue(prefName.get(),
            NS_GET_IID(nsIPrefLocalizedString), getter_AddRefs(string));

    if (NS_SUCCEEDED(*rv))
      string->GetData(getter_Copies(*retValue));

    return true;
}

bool
ContentProcessParent::RecvPrefHasUserValue(const nsCString& prefName,
                                           PRBool* retValue, nsresult* rv)
{
    *retValue = PR_FALSE;

    EnsurePrefService();
    *rv = mPrefService->PrefHasUserValue(prefName.get(), retValue);
    return true;
}

bool
ContentProcessParent::RecvPrefIsLocked(const nsCString& prefName,
                                       PRBool* retValue, nsresult* rv)
{
    *retValue = PR_FALSE;

    EnsurePrefService();
    *rv = mPrefService->PrefIsLocked(prefName.get(), retValue);
        
    return true;
}

bool
ContentProcessParent::RecvGetChildList(const nsCString& domain,
                                       nsTArray<nsCString>* list, nsresult* rv)
{
    EnsurePrefService();

    PRUint32 count;
    char **childArray;
    *rv = mPrefService->GetChildList(domain.get(), &count, &childArray);

    if (NS_SUCCEEDED(*rv)) {
      list->SetCapacity(count);
      for (PRUint32 i = 0; i < count; ++i)
        *(list->AppendElement()) = childArray[i];
    }
        
    return true;
}

void
ContentProcessParent::EnsurePrefService()
{
    nsresult rv;
    if (!mPrefService) {
        mPrefService = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
        NS_ASSERTION(NS_SUCCEEDED(rv), 
                     "We lost prefService in the Chrome process !");
    }
}

NS_IMPL_THREADSAFE_ISUPPORTS2(ContentProcessParent,
                              nsIObserver,
                              nsIThreadObserver)

namespace {
void
DeleteSubprocess(GeckoChildProcessHost* aSubprocess)
{
    delete aSubprocess;
}
}

NS_IMETHODIMP
ContentProcessParent::Observe(nsISupports* aSubject,
                              const char* aTopic,
                              const PRUnichar* aData)
{
    if (!strcmp(aTopic, "xpcom-shutdown") && mSubprocess) {
        
        nsCOMPtr<nsIPrefBranch2> prefs 
            (do_GetService(NS_PREFSERVICE_CONTRACTID));
        if (prefs) { 
            if (gSingleton) {
                prefs->RemoveObserver("", this);
            }
        }

        Close();
        XRE_GetIOMessageLoop()->PostTask(
            FROM_HERE,
            NewRunnableFunction(DeleteSubprocess, mSubprocess));
        mSubprocess = nsnull;
    }

    
    if (!strcmp(aTopic, "nsPref:changed")) {
        
        NS_LossyConvertUTF16toASCII strData(aData);
        SendNotifyRemotePrefObserver(strData);
    }

    if (!strcmp(aTopic, NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC) && mSubprocess) {
      NS_ConvertUTF16toUTF8 dataStr(aData);
      const char *offline = dataStr.get();
      SendSetOffline(!strcmp(offline, "true") ? true : false);
    }
    return NS_OK;
}

PIFrameEmbeddingParent*
ContentProcessParent::AllocPIFrameEmbedding()
{
  TabParent* parent = new TabParent();
  if (parent){
    NS_ADDREF(parent);
  }
  return parent;
}

bool
ContentProcessParent::DeallocPIFrameEmbedding(PIFrameEmbeddingParent* frame)
{
  TabParent* parent = static_cast<TabParent*>(frame);
  NS_RELEASE(parent);
  return true;
}

PTestShellParent*
ContentProcessParent::AllocPTestShell()
{
  return new TestShellParent();
}

bool
ContentProcessParent::DeallocPTestShell(PTestShellParent* shell)
{
  delete shell;
  return true;
}

PNeckoParent* 
ContentProcessParent::AllocPNecko()
{
    return new NeckoParent();
}

bool 
ContentProcessParent::DeallocPNecko(PNeckoParent* necko)
{
    delete necko;
    return true;
}

void
ContentProcessParent::ReportChildAlreadyBlocked()
{
    if (!mRunToCompletionDepth) {
#ifdef DEBUG
        printf("Running to completion...\n");
#endif
        mRunToCompletionDepth = 1;
        mShouldCallUnblockChild = false;
    }
}
    
bool
ContentProcessParent::RequestRunToCompletion()
{
    if (!mRunToCompletionDepth &&
        BlockChild()) {
#ifdef DEBUG
        printf("Running to completion...\n");
#endif
        mRunToCompletionDepth = 1;
        mShouldCallUnblockChild = true;
    }
    return !!mRunToCompletionDepth;
}


bool
ContentProcessParent::RecvStartVisitedQuery(const IPC::URI& aURI)
{
    nsCOMPtr<nsIURI> newURI = aURI;
    IHistory *history = nsContentUtils::GetHistory(); 
    history->RegisterVisitedCallback(newURI, nsnull);
    return true;
}

bool
ContentProcessParent::RecvVisitURI(const IPC::URI& uri,
                                   const IPC::URI& referrer,
                                   const PRUint32& flags)
{
    nsCOMPtr<nsIURI> ourURI = uri;
    nsCOMPtr<nsIURI> ourReferrer = referrer;
    IHistory *history = nsContentUtils::GetHistory(); 
    history->VisitURI(ourURI, ourReferrer, flags);
    return true;
}


bool
ContentProcessParent::RecvSetURITitle(const IPC::URI& uri,
                                      const nsString& title)
{
    nsCOMPtr<nsIURI> ourURI = uri;
    IHistory *history = nsContentUtils::GetHistory(); 
    history->SetURITitle(ourURI, title);
    return true;
}


NS_IMETHODIMP
ContentProcessParent::OnDispatchedEvent(nsIThreadInternal *thread)
{
    if (mOldObserver)
        return mOldObserver->OnDispatchedEvent(thread);

    return NS_OK;
}


NS_IMETHODIMP
ContentProcessParent::OnProcessNextEvent(nsIThreadInternal *thread,
                                         PRBool mayWait,
                                         PRUint32 recursionDepth)
{
    if (mRunToCompletionDepth)
        ++mRunToCompletionDepth;

    if (mOldObserver)
        return mOldObserver->OnProcessNextEvent(thread, mayWait, recursionDepth);

    return NS_OK;
}


NS_IMETHODIMP
ContentProcessParent::AfterProcessNextEvent(nsIThreadInternal *thread,
                                            PRUint32 recursionDepth)
{
    if (mRunToCompletionDepth &&
        !--mRunToCompletionDepth) {
#ifdef DEBUG
            printf("... ran to completion.\n");
#endif
            if (mShouldCallUnblockChild) {
                mShouldCallUnblockChild = false;
                UnblockChild();
            }
    }

    if (mOldObserver)
        return mOldObserver->AfterProcessNextEvent(thread, recursionDepth);

    return NS_OK;
}
    
} 
} 
