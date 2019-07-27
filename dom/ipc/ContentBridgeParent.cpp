





#include "mozilla/dom/ContentBridgeParent.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/jsipc/CrossProcessObjectWrappers.h"
#include "nsXULAppAPI.h"
#include "nsIObserverService.h"

using namespace mozilla::ipc;
using namespace mozilla::jsipc;

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS(ContentBridgeParent,
                  nsIContentParent,
                  nsIObserver)

ContentBridgeParent::ContentBridgeParent(Transport* aTransport)
  : mTransport(aTransport)
{}

ContentBridgeParent::~ContentBridgeParent()
{
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE, new DeleteTask<Transport>(mTransport));
}

void
ContentBridgeParent::ActorDestroy(ActorDestroyReason aWhy)
{
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os) {
    os->RemoveObserver(this, "content-child-shutdown");
  }
  MessageLoop::current()->PostTask(
    FROM_HERE,
    NewRunnableMethod(this, &ContentBridgeParent::DeferredDestroy));
}

 ContentBridgeParent*
ContentBridgeParent::Create(Transport* aTransport, ProcessId aOtherPid)
{
  nsRefPtr<ContentBridgeParent> bridge =
    new ContentBridgeParent(aTransport);
  bridge->mSelfRef = bridge;

  DebugOnly<bool> ok = bridge->Open(aTransport, aOtherPid,
                                    XRE_GetIOMessageLoop());
  MOZ_ASSERT(ok);

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os) {
    os->AddObserver(bridge, "content-child-shutdown", false);
  }

  
  
  bridge->mMessageManager->InitWithCallback(bridge);

  return bridge.get();
}

void
ContentBridgeParent::DeferredDestroy()
{
  mSelfRef = nullptr;
  
}

bool
ContentBridgeParent::RecvSyncMessage(const nsString& aMsg,
                                     const ClonedMessageData& aData,
                                     InfallibleTArray<jsipc::CpowEntry>&& aCpows,
                                     const IPC::Principal& aPrincipal,
                                     InfallibleTArray<nsString>* aRetvals)
{
  return nsIContentParent::RecvSyncMessage(aMsg, aData, Move(aCpows),
                                           aPrincipal, aRetvals);
}

bool
ContentBridgeParent::RecvAsyncMessage(const nsString& aMsg,
                                      const ClonedMessageData& aData,
                                      InfallibleTArray<jsipc::CpowEntry>&& aCpows,
                                      const IPC::Principal& aPrincipal)
{
  return nsIContentParent::RecvAsyncMessage(aMsg, aData, Move(aCpows),
                                            aPrincipal);
}

PBlobParent*
ContentBridgeParent::SendPBlobConstructor(PBlobParent* actor,
                                          const BlobConstructorParams& params)
{
  return PContentBridgeParent::SendPBlobConstructor(actor, params);
}

PBrowserParent*
ContentBridgeParent::SendPBrowserConstructor(PBrowserParent* aActor,
                                             const TabId& aTabId,
                                             const IPCTabContext& aContext,
                                             const uint32_t& aChromeFlags,
                                             const ContentParentId& aCpID,
                                             const bool& aIsForApp,
                                             const bool& aIsForBrowser)
{
  return PContentBridgeParent::SendPBrowserConstructor(aActor,
                                                       aTabId,
                                                       aContext,
                                                       aChromeFlags,
                                                       aCpID,
                                                       aIsForApp,
                                                       aIsForBrowser);
}

PBlobParent*
ContentBridgeParent::AllocPBlobParent(const BlobConstructorParams& aParams)
{
  return nsIContentParent::AllocPBlobParent(aParams);
}

bool
ContentBridgeParent::DeallocPBlobParent(PBlobParent* aActor)
{
  return nsIContentParent::DeallocPBlobParent(aActor);
}

mozilla::jsipc::PJavaScriptParent *
ContentBridgeParent::AllocPJavaScriptParent()
{
  return nsIContentParent::AllocPJavaScriptParent();
}

bool
ContentBridgeParent::DeallocPJavaScriptParent(PJavaScriptParent *parent)
{
  return nsIContentParent::DeallocPJavaScriptParent(parent);
}

PBrowserParent*
ContentBridgeParent::AllocPBrowserParent(const TabId& aTabId,
                                         const IPCTabContext &aContext,
                                         const uint32_t& aChromeFlags,
                                         const ContentParentId& aCpID,
                                         const bool& aIsForApp,
                                         const bool& aIsForBrowser)
{
  return nsIContentParent::AllocPBrowserParent(aTabId,
                                               aContext,
                                               aChromeFlags,
                                               aCpID,
                                               aIsForApp,
                                               aIsForBrowser);
}

bool
ContentBridgeParent::DeallocPBrowserParent(PBrowserParent* aParent)
{
  return nsIContentParent::DeallocPBrowserParent(aParent);
}




jsipc::CPOWManager*
ContentBridgeParent::GetCPOWManager()
{
  if (ManagedPJavaScriptParent().Length()) {
    return CPOWManagerFor(ManagedPJavaScriptParent()[0]);
  }
  return CPOWManagerFor(SendPJavaScriptConstructor());
}

NS_IMETHODIMP
ContentBridgeParent::Observe(nsISupports* aSubject,
                             const char* aTopic,
                             const char16_t* aData)
{
  if (!strcmp(aTopic, "content-child-shutdown")) {
    Close();
  }
  return NS_OK;
}

} 
} 
