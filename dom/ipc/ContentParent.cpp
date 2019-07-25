






































#include "ContentParent.h"

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
#include "nsExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"

#ifdef ANDROID
#include "AndroidBridge.h"
using namespace mozilla;
#endif

using namespace mozilla::ipc;
using namespace mozilla::net;
using namespace mozilla::places;
using mozilla::MonitorAutoEnter;

namespace mozilla {
namespace dom {

#define NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC "ipc:network:set-offline"

ContentParent* ContentParent::gSingleton;

ContentParent*
ContentParent::GetSingleton(PRBool aForceNew)
{
    if (gSingleton && !gSingleton->IsAlive())
        gSingleton = nsnull;
    
    if (!gSingleton && aForceNew) {
        nsRefPtr<ContentParent> parent = new ContentParent();
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
ContentParent::ActorDestroy(ActorDestroyReason why)
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
ContentParent::CreateTab(PRUint32 aChromeFlags)
{
  return static_cast<TabParent*>(SendPBrowserConstructor(aChromeFlags));
}

TestShellParent*
ContentParent::CreateTestShell()
{
  return static_cast<TestShellParent*>(SendPTestShellConstructor());
}

bool
ContentParent::DestroyTestShell(TestShellParent* aTestShell)
{
    return PTestShellParent::Send__delete__(aTestShell);
}

ContentParent::ContentParent()
    : mMonitor("ContentParent::mMonitor")
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

ContentParent::~ContentParent()
{
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    
    
    if (gSingleton == this)
        gSingleton = nsnull;
}

bool
ContentParent::IsAlive()
{
    return mIsAlive;
}

bool
ContentParent::RecvGetPrefType(const nsCString& prefName,
                               PRInt32* retValue, nsresult* rv)
{
    *retValue = 0;

    EnsurePrefService();
    *rv = mPrefService->GetPrefType(prefName.get(), retValue);
    return true;
}

bool
ContentParent::RecvGetBoolPref(const nsCString& prefName,
                               PRBool* retValue, nsresult* rv)
{
    *retValue = PR_FALSE;

    EnsurePrefService();
    *rv = mPrefService->GetBoolPref(prefName.get(), retValue);
    return true;
}

bool
ContentParent::RecvGetIntPref(const nsCString& prefName,
                              PRInt32* retValue, nsresult* rv)
{
    *retValue = 0;

    EnsurePrefService();
    *rv = mPrefService->GetIntPref(prefName.get(), retValue);
    return true;
}

bool
ContentParent::RecvGetCharPref(const nsCString& prefName,
                               nsCString* retValue, nsresult* rv)
{
    EnsurePrefService();
    *rv = mPrefService->GetCharPref(prefName.get(), getter_Copies(*retValue));
    return true;
}

bool
ContentParent::RecvGetPrefLocalizedString(const nsCString& prefName,
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
ContentParent::RecvPrefHasUserValue(const nsCString& prefName,
                                    PRBool* retValue, nsresult* rv)
{
    *retValue = PR_FALSE;

    EnsurePrefService();
    *rv = mPrefService->PrefHasUserValue(prefName.get(), retValue);
    return true;
}

bool
ContentParent::RecvPrefIsLocked(const nsCString& prefName,
                                PRBool* retValue, nsresult* rv)
{
    *retValue = PR_FALSE;

    EnsurePrefService();
    *rv = mPrefService->PrefIsLocked(prefName.get(), retValue);
        
    return true;
}

bool
ContentParent::RecvGetChildList(const nsCString& domain,
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

bool
ContentParent::RecvTestPermission(const IPC::URI&  aUri,
                                   const nsCString& aType,
                                   const PRBool&    aExact,
                                   PRUint32*        retValue)
{
    EnsurePermissionService();

    nsCOMPtr<nsIURI> uri(aUri);
    if (aExact) {
        mPermissionService->TestExactPermission(uri, aType.get(), retValue);
    } else {
        mPermissionService->TestPermission(uri, aType.get(), retValue);
    }
    return true;
}

void
ContentParent::EnsurePrefService()
{
    nsresult rv;
    if (!mPrefService) {
        mPrefService = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
        NS_ASSERTION(NS_SUCCEEDED(rv), 
                     "We lost prefService in the Chrome process !");
    }
}

void
ContentParent::EnsurePermissionService()
{
    nsresult rv;
    if (!mPermissionService) {
        mPermissionService = do_GetService(
            NS_PERMISSIONMANAGER_CONTRACTID, &rv);
        NS_ASSERTION(NS_SUCCEEDED(rv), 
                     "We lost permissionService in the Chrome process !");
    }
}

NS_IMPL_THREADSAFE_ISUPPORTS2(ContentParent,
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
ContentParent::Observe(nsISupports* aSubject,
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

    if (!mIsAlive || !mSubprocess)
        return NS_OK;

    
    if (!strcmp(aTopic, "nsPref:changed")) {
        
        NS_LossyConvertUTF16toASCII strData(aData);
        if (!SendNotifyRemotePrefObserver(strData))
            return NS_ERROR_NOT_AVAILABLE;
    }
    else if (!strcmp(aTopic, NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC)) {
      NS_ConvertUTF16toUTF8 dataStr(aData);
      const char *offline = dataStr.get();
      if (!SendSetOffline(!strcmp(offline, "true") ? true : false))
          return NS_ERROR_NOT_AVAILABLE;
    }
    return NS_OK;
}

PBrowserParent*
ContentParent::AllocPBrowser(const PRUint32& aChromeFlags)
{
  TabParent* parent = new TabParent();
  if (parent){
    NS_ADDREF(parent);
  }
  return parent;
}

bool
ContentParent::DeallocPBrowser(PBrowserParent* frame)
{
  TabParent* parent = static_cast<TabParent*>(frame);
  NS_RELEASE(parent);
  return true;
}

PTestShellParent*
ContentParent::AllocPTestShell()
{
  return new TestShellParent();
}

bool
ContentParent::DeallocPTestShell(PTestShellParent* shell)
{
  delete shell;
  return true;
}

PNeckoParent* 
ContentParent::AllocPNecko()
{
    return new NeckoParent();
}

bool 
ContentParent::DeallocPNecko(PNeckoParent* necko)
{
    delete necko;
    return true;
}

void
ContentParent::ReportChildAlreadyBlocked()
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
ContentParent::RequestRunToCompletion()
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
ContentParent::RecvStartVisitedQuery(const IPC::URI& aURI)
{
    nsCOMPtr<nsIURI> newURI(aURI);
    IHistory *history = nsContentUtils::GetHistory(); 
    history->RegisterVisitedCallback(newURI, nsnull);
    return true;
}


bool
ContentParent::RecvVisitURI(const IPC::URI& uri,
                                   const IPC::URI& referrer,
                                   const PRUint32& flags)
{
    nsCOMPtr<nsIURI> ourURI(uri);
    nsCOMPtr<nsIURI> ourReferrer(referrer);
    IHistory *history = nsContentUtils::GetHistory(); 
    history->VisitURI(ourURI, ourReferrer, flags);
    return true;
}


bool
ContentParent::RecvSetURITitle(const IPC::URI& uri,
                                      const nsString& title)
{
    nsCOMPtr<nsIURI> ourURI(uri);
    IHistory *history = nsContentUtils::GetHistory(); 
    history->SetURITitle(ourURI, title);
    return true;
}

bool
ContentParent::RecvLoadURIExteneral(const URI& uri)
{
    nsCOMPtr<nsIExternalProtocolService> extProtService (do_GetService(NS_EXTERNALPROTOCOLSERVICE_CONTRACTID));
    nsCOMPtr<nsIURI> ourURI = uri;
    extProtService->LoadURI(ourURI, nsnull);
    return true;
}



NS_IMETHODIMP
ContentParent::OnDispatchedEvent(nsIThreadInternal *thread)
{
    if (mOldObserver)
        return mOldObserver->OnDispatchedEvent(thread);

    return NS_OK;
}


NS_IMETHODIMP
ContentParent::OnProcessNextEvent(nsIThreadInternal *thread,
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
ContentParent::AfterProcessNextEvent(nsIThreadInternal *thread,
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


bool 
ContentParent::RecvNotifyIMEChange(const nsString& aText, 
                                   const PRUint32& aTextLen, 
                                   const int& aStart, const int& aEnd, 
                                   const int& aNewEnd)
{
#ifdef ANDROID
    AndroidBridge::Bridge()->NotifyIMEChange(aText.get(), aTextLen,
                                             aStart, aEnd, aNewEnd);
    return true;
#else
    return false;
#endif
}

bool 
ContentParent::RecvNotifyIME(const int& aType, const int& aStatus)
{
#ifdef ANDROID
    AndroidBridge::Bridge()->NotifyIME(aType, aStatus);
    return true;
#else
    return false;
#endif
}
    
} 
} 
