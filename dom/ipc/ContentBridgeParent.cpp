





#include "mozilla/dom/ContentBridgeParent.h"
#include "mozilla/dom/TabParent.h"
#include "JavaScriptParent.h"
#include "nsXULAppAPI.h"

using namespace base;
using namespace mozilla::ipc;
using namespace mozilla::jsipc;

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS(ContentBridgeParent, nsIContentParent)

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
  MessageLoop::current()->PostTask(
    FROM_HERE,
    NewRunnableMethod(this, &ContentBridgeParent::DeferredDestroy));
}

 ContentBridgeParent*
ContentBridgeParent::Create(Transport* aTransport, ProcessId aOtherProcess)
{
  nsRefPtr<ContentBridgeParent> bridge =
    new ContentBridgeParent(aTransport);
  ProcessHandle handle;
  if (!base::OpenProcessHandle(aOtherProcess, &handle)) {
    
    return nullptr;
  }
  bridge->mSelfRef = bridge;

  DebugOnly<bool> ok = bridge->Open(aTransport, handle, XRE_GetIOMessageLoop());
  MOZ_ASSERT(ok);
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
                                     const InfallibleTArray<jsipc::CpowEntry>& aCpows,
                                     const IPC::Principal& aPrincipal,
                                     InfallibleTArray<nsString>* aRetvals)
{
  return nsIContentParent::RecvSyncMessage(aMsg, aData, aCpows, aPrincipal, aRetvals);
}

bool
ContentBridgeParent::RecvAsyncMessage(const nsString& aMsg,
                                      const ClonedMessageData& aData,
                                      const InfallibleTArray<jsipc::CpowEntry>& aCpows,
                                      const IPC::Principal& aPrincipal)
{
  return nsIContentParent::RecvAsyncMessage(aMsg, aData, aCpows, aPrincipal);
}

PBlobParent*
ContentBridgeParent::SendPBlobConstructor(PBlobParent* actor,
                                          const BlobConstructorParams& params)
{
  return PContentBridgeParent::SendPBlobConstructor(actor, params);
}

PBrowserParent*
ContentBridgeParent::SendPBrowserConstructor(PBrowserParent* aActor,
                                             const IPCTabContext& aContext,
                                             const uint32_t& aChromeFlags,
                                             const ContentParentId& aCpID,
                                             const bool& aIsForApp,
                                             const bool& aIsForBrowser)
{
  return PContentBridgeParent::SendPBrowserConstructor(aActor,
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
ContentBridgeParent::AllocPBrowserParent(const IPCTabContext &aContext,
                                         const uint32_t& aChromeFlags,
                                         const ContentParentId& aCpID,
                                         const bool& aIsForApp,
                                         const bool& aIsForBrowser)
{
  return nsIContentParent::AllocPBrowserParent(aContext,
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




jsipc::JavaScriptShared*
ContentBridgeParent::GetCPOWManager()
{
  if (ManagedPJavaScriptParent().Length()) {
    return static_cast<JavaScriptParent*>(ManagedPJavaScriptParent()[0]);
  }
  JavaScriptParent* actor = static_cast<JavaScriptParent*>(SendPJavaScriptConstructor());
  return actor;
}

} 
} 
