





































#ifdef MOZ_WIDGET_QT
#include <QApplication>
#endif

#include "ContentProcessChild.h"
#include "TabChild.h"

#include "mozilla/ipc/TestShellChild.h"
#include "mozilla/net/NeckoChild.h"
#include "mozilla/ipc/XPCShellEnvironment.h"
#include "mozilla/jsipc/PContextWrapperChild.h"

#include "nsXULAppAPI.h"

#include "base/message_loop.h"
#include "base/task.h"

#include "nsChromeRegistryContent.h"
#include "mozilla/chrome/RegistryMessageUtils.h"

using namespace mozilla::ipc;
using namespace mozilla::net;

#ifdef MOZ_WIDGET_QT
extern int    gArgc;
extern char **gArgv;
#endif

namespace mozilla {
namespace dom {

ContentProcessChild* ContentProcessChild::sSingleton;

ContentProcessChild::ContentProcessChild()
    : mQuit(PR_FALSE)
{
}

ContentProcessChild::~ContentProcessChild()
{
}

bool
ContentProcessChild::Init(MessageLoop* aIOLoop,
                          base::ProcessHandle aParentHandle,
                          IPC::Channel* aChannel)
{
    NS_ASSERTION(!sSingleton, "only one ContentProcessChild per child");
  
    Open(aChannel, aParentHandle, aIOLoop);
    sSingleton = this;

#ifdef MOZ_WIDGET_QT
    NS_ASSERTION(!qApp, "QApplication created too early?");
    mQApp = new QApplication(gArgc, (char**)gArgv);
#endif

    return true;
}

PIFrameEmbeddingChild*
ContentProcessChild::AllocPIFrameEmbedding()
{
  nsRefPtr<TabChild> iframe = new TabChild();
  NS_ENSURE_TRUE(iframe && NS_SUCCEEDED(iframe->Init()) &&
                 mIFrames.AppendElement(iframe),
                 nsnull);
  return iframe.forget().get();
}

bool
ContentProcessChild::DeallocPIFrameEmbedding(PIFrameEmbeddingChild* iframe)
{
    if (mIFrames.RemoveElement(iframe)) {
      TabChild* child = static_cast<TabChild*>(iframe);
      NS_RELEASE(child);
    }
    return true;
}

PTestShellChild*
ContentProcessChild::AllocPTestShell()
{
    PTestShellChild* testshell = new TestShellChild();
    if (testshell && mTestShells.AppendElement(testshell)) {
        return testshell;
    }
    delete testshell;
    return nsnull;
}

bool
ContentProcessChild::DeallocPTestShell(PTestShellChild* shell)
{
    mTestShells.RemoveElement(shell);
    return true;
}

bool
ContentProcessChild::RecvPTestShellConstructor(PTestShellChild* actor)
{
    actor->SendPContextWrapperConstructor()->SendPObjectWrapperConstructor(true);
    return true;
}

PNeckoChild* 
ContentProcessChild::AllocPNecko()
{
    return new NeckoChild();
}

bool 
ContentProcessChild::DeallocPNecko(PNeckoChild* necko)
{
    delete necko;
    return true;
}

bool
ContentProcessChild::RecvRegisterChrome(const nsTArray<ChromePackage>& packages,
                                        const nsTArray<ResourceMapping>& resources,
                                        const nsTArray<OverrideMapping>& overrides)
{
    nsCOMPtr<nsIChromeRegistry> registrySvc = nsChromeRegistry::GetService();
    nsChromeRegistryContent* chromeRegistry =
        static_cast<nsChromeRegistryContent*>(registrySvc.get());
    chromeRegistry->RegisterRemoteChrome(packages, resources, overrides);
    return true;
}

void
ContentProcessChild::Quit()
{
    NS_ASSERTION(mQuit, "Exiting uncleanly!");
    mIFrames.Clear();
    mTestShells.Clear();
}

void
ContentProcessChild::ActorDestroy(ActorDestroyReason why)
{
    if (AbnormalShutdown == why)
        NS_WARNING("shutting down because of crash!");

    mQuit = PR_TRUE;
    Quit();

    XRE_ShutdownChildProcess();
}

} 
} 
