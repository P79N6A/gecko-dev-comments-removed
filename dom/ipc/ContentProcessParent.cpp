





































#include "ContentProcessParent.h"

#include "mozilla/ipc/GeckoThread.h"

#include "TabParent.h"
#include "mozilla/ipc/TestShellParent.h"
#include "mozilla/net/NeckoParent.h"

#include "nsIObserverService.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"

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
        }
    }
    return gSingleton;
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
{
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    mSubprocess = new GeckoChildProcessHost(GeckoProcessType_Content);
    mSubprocess->AsyncLaunch();
    Open(mSubprocess->GetChannel(), mSubprocess->GetChildProcessHandle());
}

ContentProcessParent::~ContentProcessParent()
{
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    NS_ASSERTION(gSingleton == this, "More than one singleton?!");
    gSingletonDied = PR_TRUE;
    gSingleton = nsnull;
}

NS_IMPL_ISUPPORTS1(ContentProcessParent, nsIObserver)

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

} 
} 
