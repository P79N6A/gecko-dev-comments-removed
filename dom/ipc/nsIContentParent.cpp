





#include "nsIContentParent.h"

#include "mozilla/AppProcessChecker.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/PTabContext.h"
#include "mozilla/dom/PermissionMessageUtils.h"
#include "mozilla/dom/StructuredCloneUtils.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/dom/ipc/BlobParent.h"
#include "mozilla/jsipc/CrossProcessObjectWrappers.h"
#include "mozilla/unused.h"

#include "nsFrameMessageManager.h"
#include "nsIJSRuntimeService.h"
#include "nsPrintfCString.h"

using namespace mozilla::jsipc;


#ifdef DISABLE_ASSERTS_FOR_FUZZING
#define ASSERT_UNLESS_FUZZING(...) do { } while (0)
#else
#define ASSERT_UNLESS_FUZZING(...) MOZ_ASSERT(false, __VA_ARGS__)
#endif

namespace mozilla {
namespace dom {

nsIContentParent::nsIContentParent()
{
  mMessageManager = nsFrameMessageManager::NewProcessMessageManager(true);
}

ContentParent*
nsIContentParent::AsContentParent()
{
  MOZ_ASSERT(IsContentParent());
  return static_cast<ContentParent*>(this);
}

PJavaScriptParent*
nsIContentParent::AllocPJavaScriptParent()
{
  nsCOMPtr<nsIJSRuntimeService> svc =
    do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
  NS_ENSURE_TRUE(svc, nullptr);

  JSRuntime *rt;
  svc->GetRuntime(&rt);
  NS_ENSURE_TRUE(svc, nullptr);

  return NewJavaScriptParent(rt);
}

bool
nsIContentParent::DeallocPJavaScriptParent(PJavaScriptParent* aParent)
{
  ReleaseJavaScriptParent(aParent);
  return true;
}

bool
nsIContentParent::CanOpenBrowser(const IPCTabContext& aContext)
{
  const IPCTabAppBrowserContext& appBrowser = aContext.appBrowserContext();

  
  
  
  
  if (appBrowser.type() != IPCTabAppBrowserContext::TPopupIPCTabContext) {
    ASSERT_UNLESS_FUZZING("Unexpected IPCTabContext type.  Aborting AllocPBrowserParent.");
    return false;
  }

  const PopupIPCTabContext& popupContext = appBrowser.get_PopupIPCTabContext();
  if (popupContext.opener().type() != PBrowserOrId::TPBrowserParent) {
    ASSERT_UNLESS_FUZZING("Unexpected PopupIPCTabContext type.  Aborting AllocPBrowserParent.");
    return false;
  }

  auto opener = TabParent::GetFrom(popupContext.opener().get_PBrowserParent());
  if (!opener) {
    ASSERT_UNLESS_FUZZING("Got null opener from child; aborting AllocPBrowserParent.");
    return false;
  }

  
  
  
  if (!popupContext.isBrowserElement() && opener->IsBrowserElement()) {
    ASSERT_UNLESS_FUZZING("Child trying to escalate privileges!  Aborting AllocPBrowserParent.");
    return false;
  }

  MaybeInvalidTabContext tc(aContext);
  if (!tc.IsValid()) {
    NS_ERROR(nsPrintfCString("Child passed us an invalid TabContext.  (%s)  "
                             "Aborting AllocPBrowserParent.",
                             tc.GetInvalidReason()).get());
    return false;
  }

  return true;
}

PBrowserParent*
nsIContentParent::AllocPBrowserParent(const TabId& aTabId,
                                      const IPCTabContext& aContext,
                                      const uint32_t& aChromeFlags,
                                      const ContentParentId& aCpId,
                                      const bool& aIsForApp,
                                      const bool& aIsForBrowser)
{
  unused << aCpId;
  unused << aIsForApp;
  unused << aIsForBrowser;

  if (!CanOpenBrowser(aContext)) {
    return nullptr;
  }

  MaybeInvalidTabContext tc(aContext);
  MOZ_ASSERT(tc.IsValid());
  TabParent* parent = new TabParent(this, aTabId, tc.GetTabContext(), aChromeFlags);

  
  NS_ADDREF(parent);
  return parent;
}

bool
nsIContentParent::DeallocPBrowserParent(PBrowserParent* aFrame)
{
  TabParent* parent = TabParent::GetFrom(aFrame);
  NS_RELEASE(parent);
  return true;
}

PBlobParent*
nsIContentParent::AllocPBlobParent(const BlobConstructorParams& aParams)
{
  return BlobParent::Create(this, aParams);
}

bool
nsIContentParent::DeallocPBlobParent(PBlobParent* aActor)
{
  BlobParent::Destroy(aActor);
  return true;
}

BlobParent*
nsIContentParent::GetOrCreateActorForBlob(File* aBlob)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aBlob);

  nsRefPtr<FileImpl> blobImpl = aBlob->Impl();
  MOZ_ASSERT(blobImpl);

  BlobParent* actor = BlobParent::GetOrCreate(this, blobImpl);
  NS_ENSURE_TRUE(actor, nullptr);

  return actor;
}

bool
nsIContentParent::RecvSyncMessage(const nsString& aMsg,
                                  const ClonedMessageData& aData,
                                  InfallibleTArray<CpowEntry>&& aCpows,
                                  const IPC::Principal& aPrincipal,
                                  InfallibleTArray<nsString>* aRetvals)
{
  
  nsIPrincipal* principal = aPrincipal;
  if (IsContentParent()) {
    ContentParent* parent = AsContentParent();
    if (!ContentParent::IgnoreIPCPrincipal() &&
        parent && principal && !AssertAppPrincipal(parent, principal)) {
      return false;
    }
  }

  nsRefPtr<nsFrameMessageManager> ppm = mMessageManager;
  if (ppm) {
    StructuredCloneData cloneData = ipc::UnpackClonedMessageDataForParent(aData);
    CrossProcessCpowHolder cpows(this, aCpows);
    ppm->ReceiveMessage(static_cast<nsIContentFrameMessageManager*>(ppm.get()), nullptr,
                        aMsg, true, &cloneData, &cpows, aPrincipal, aRetvals);
  }
  return true;
}

bool
nsIContentParent::RecvRpcMessage(const nsString& aMsg,
                                 const ClonedMessageData& aData,
                                 InfallibleTArray<CpowEntry>&& aCpows,
                                 const IPC::Principal& aPrincipal,
                                 InfallibleTArray<nsString>* aRetvals)
{
  
  nsIPrincipal* principal = aPrincipal;
  if (IsContentParent()) {
    ContentParent* parent = AsContentParent();
    if (!ContentParent::IgnoreIPCPrincipal() &&
        parent && principal && !AssertAppPrincipal(parent, principal)) {
      return false;
    }
  }

  nsRefPtr<nsFrameMessageManager> ppm = mMessageManager;
  if (ppm) {
    StructuredCloneData cloneData = ipc::UnpackClonedMessageDataForParent(aData);
    CrossProcessCpowHolder cpows(this, aCpows);
    ppm->ReceiveMessage(static_cast<nsIContentFrameMessageManager*>(ppm.get()), nullptr,
                        aMsg, true, &cloneData, &cpows, aPrincipal, aRetvals);
  }
  return true;
}

bool
nsIContentParent::RecvAsyncMessage(const nsString& aMsg,
                                   const ClonedMessageData& aData,
                                   InfallibleTArray<CpowEntry>&& aCpows,
                                   const IPC::Principal& aPrincipal)
{
  
  nsIPrincipal* principal = aPrincipal;
  if (IsContentParent()) {
    ContentParent* parent = AsContentParent();
    if (!ContentParent::IgnoreIPCPrincipal() &&
        parent && principal && !AssertAppPrincipal(parent, principal)) {
      return false;
    }
  }

  nsRefPtr<nsFrameMessageManager> ppm = mMessageManager;
  if (ppm) {
    StructuredCloneData cloneData = ipc::UnpackClonedMessageDataForParent(aData);
    CrossProcessCpowHolder cpows(this, aCpows);
    ppm->ReceiveMessage(static_cast<nsIContentFrameMessageManager*>(ppm.get()), nullptr,
                        aMsg, false, &cloneData, &cpows, aPrincipal, nullptr);
  }
  return true;
}

} 
} 
