





































#include "ContentProcessParent.h"

#include "TabParent.h"
#include "mozilla/ipc/TestShellParent.h"
#include "mozilla/net/NeckoParent.h"

#include "nsIObserverService.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsChromeRegistryChrome.h"

using namespace mozilla::ipc;
using namespace mozilla::net;
using mozilla::MonitorAutoEnter;

namespace {
PRBool gSingletonDied = PR_FALSE;
}

namespace mozilla {
namespace dom {

ContentProcessParent* ContentProcessParent::gSingleton;

ContentProcessParent*
ContentProcessParent::GetSingleton()
{
    if (!gSingleton && !gSingletonDied) {
        nsRefPtr<ContentProcessParent> parent = new ContentProcessParent();
        if (parent) {
            nsCOMPtr<nsIObserverService> obs =
                do_GetService("@mozilla.org/observer-service;1");
            if (obs) {
                if (NS_SUCCEEDED(obs->AddObserver(parent, "xpcom-shutdown",
                                                  PR_FALSE))) {
                    gSingleton = parent;
                }
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
    NS_ASSERTION(gSingleton == this, "More than one singleton?!");
    gSingletonDied = PR_TRUE;
    gSingleton = nsnull;
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
        Close();
        XRE_GetIOMessageLoop()->PostTask(
            FROM_HERE,
            NewRunnableFunction(DeleteSubprocess, mSubprocess));
        mSubprocess = nsnull;
    }
    return NS_OK;
}

PIFrameEmbeddingParent*
ContentProcessParent::AllocPIFrameEmbedding()
{
    return new TabParent();
}

bool
ContentProcessParent::DeallocPIFrameEmbedding(PIFrameEmbeddingParent* frame)
{
  delete frame;
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

bool
ContentProcessParent::RequestRunToCompletion()
{
    if (!mRunToCompletionDepth &&
        BlockChild()) {
#ifdef DEBUG
        printf("Running to completion...\n");
#endif
        mRunToCompletionDepth = 1;
    }

    return !!mRunToCompletionDepth;
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
            UnblockChild();
    }

    if (mOldObserver)
        return mOldObserver->AfterProcessNextEvent(thread, recursionDepth);

    return NS_OK;
}
    
} 
} 
