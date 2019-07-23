





































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
ContentProcessParent::CreateTab(const MagicWindowHandle& hwnd)
{
  return static_cast<TabParent*>(SendPIFrameEmbeddingConstructor(hwnd));
}

TestShellParent*
ContentProcessParent::CreateTestShell()
{
  return static_cast<TestShellParent*>(SendPTestShellConstructor());
}

ContentProcessParent::ContentProcessParent()
    : mMonitor("ContentProcessParent::mMonitor")
{
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    
    mSubprocess = new GeckoChildProcessHost(GeckoProcessType_Content, this);
    mSubprocess->SyncLaunch();
    Open(mSubprocess->GetChannel());
}

ContentProcessParent::~ContentProcessParent()
{
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    NS_ASSERTION(gSingleton == this, "More than one singleton?!");
    gSingletonDied = PR_TRUE;
    gSingleton = nsnull;
}

NS_IMPL_ISUPPORTS1(ContentProcessParent, nsIObserver)

NS_IMETHODIMP
ContentProcessParent::Observe(nsISupports* aSubject,
                              const char* aTopic,
                              const PRUnichar* aData)
{
    if (!strcmp(aTopic, "xpcom-shutdown") && mSubprocess) {
        SendQuit();
#ifdef OS_WIN
        MonitorAutoEnter mon(mMonitor);
        while (mSubprocess) {
            mon.Wait();
        }
#endif
    }
    return NS_OK;
}

void
ContentProcessParent::OnWaitableEventSignaled(base::WaitableEvent *event)
{
    
    NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
    MonitorAutoEnter mon(mMonitor);
    mSubprocess = nsnull;
    mon.Notify();
}

PIFrameEmbeddingParent*
ContentProcessParent::PIFrameEmbeddingConstructor(
        const MagicWindowHandle& parentWidget)
{
    return new TabParent();
}

bool
ContentProcessParent::PIFrameEmbeddingDestructor(PIFrameEmbeddingParent* frame)
{
  delete frame;
  return true;
}

PTestShellParent*
ContentProcessParent::PTestShellConstructor()
{
  return new TestShellParent();
}

bool
ContentProcessParent::PTestShellDestructor(PTestShellParent* shell)
{
  delete shell;
  return true;
}

PNeckoParent* 
ContentProcessParent::PNeckoConstructor()
{
    return new NeckoParent();
}

bool 
ContentProcessParent::PNeckoDestructor(PNeckoParent* necko)
{
    delete necko;
    return true;
}

} 
} 
