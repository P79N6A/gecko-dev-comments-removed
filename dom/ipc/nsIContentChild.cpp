





#include "nsIContentChild.h"

#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/DOMTypes.h"
#include "mozilla/dom/PermissionMessageUtils.h"
#include "mozilla/dom/StructuredCloneUtils.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/dom/ipc/BlobChild.h"
#include "mozilla/ipc/InputStreamUtils.h"

#include "JavaScriptChild.h"
#include "nsDOMFile.h"
#include "nsIJSRuntimeService.h"
#include "nsPrintfCString.h"

using namespace mozilla::ipc;
using namespace mozilla::jsipc;

namespace mozilla {
namespace dom {

PJavaScriptChild*
nsIContentChild::AllocPJavaScriptChild()
{
  nsCOMPtr<nsIJSRuntimeService> svc = do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
  NS_ENSURE_TRUE(svc, nullptr);

  JSRuntime *rt;
  svc->GetRuntime(&rt);
  NS_ENSURE_TRUE(svc, nullptr);

  nsAutoPtr<JavaScriptChild> child(new JavaScriptChild(rt));
  if (!child->init()) {
    return nullptr;
  }
  return child.forget();
}

bool
nsIContentChild::DeallocPJavaScriptChild(PJavaScriptChild* aChild)
{
  static_cast<JavaScriptChild*>(aChild)->decref();
  return true;
}

PBrowserChild*
nsIContentChild::AllocPBrowserChild(const IPCTabContext& aContext,
                                    const uint32_t& aChromeFlags,
                                    const uint64_t& aID,
                                    const bool& aIsForApp,
                                    const bool& aIsForBrowser)
{
  
  
  

  MaybeInvalidTabContext tc(aContext);
  if (!tc.IsValid()) {
    NS_ERROR(nsPrintfCString("Received an invalid TabContext from "
                             "the parent process. (%s)  Crashing...",
                             tc.GetInvalidReason()).get());
    MOZ_CRASH("Invalid TabContext received from the parent process.");
  }

  nsRefPtr<TabChild> child =
    TabChild::Create(this, tc.GetTabContext(), aChromeFlags);

  
  return child.forget().take();
}

bool
nsIContentChild::DeallocPBrowserChild(PBrowserChild* aIframe)
{
  TabChild* child = static_cast<TabChild*>(aIframe);
  NS_RELEASE(child);
  return true;
}

PBlobChild*
nsIContentChild::AllocPBlobChild(const BlobConstructorParams& aParams)
{
  return BlobChild::Create(this, aParams);
}

bool
nsIContentChild::DeallocPBlobChild(PBlobChild* aActor)
{
  BlobChild::Destroy(aActor);
  return true;
}

BlobChild*
nsIContentChild::GetOrCreateActorForBlob(nsIDOMBlob* aBlob)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aBlob);

  nsRefPtr<DOMFileImpl> blobImpl = static_cast<DOMFile*>(aBlob)->Impl();
  MOZ_ASSERT(blobImpl);

  BlobChild* actor = BlobChild::GetOrCreate(this, blobImpl);
  NS_ENSURE_TRUE(actor, nullptr);

  return actor;
}

bool
nsIContentChild::RecvAsyncMessage(const nsString& aMsg,
                                  const ClonedMessageData& aData,
                                  const InfallibleTArray<CpowEntry>& aCpows,
                                  const IPC::Principal& aPrincipal)
{
  nsRefPtr<nsFrameMessageManager> cpm = nsFrameMessageManager::sChildProcessManager;
  if (cpm) {
    StructuredCloneData cloneData = ipc::UnpackClonedMessageDataForChild(aData);
    CpowIdHolder cpows(GetCPOWManager(), aCpows);
    cpm->ReceiveMessage(static_cast<nsIContentFrameMessageManager*>(cpm.get()),
                        aMsg, false, &cloneData, &cpows, aPrincipal, nullptr);
  }
  return true;
}

} 
} 
