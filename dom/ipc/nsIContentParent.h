





#ifndef mozilla_dom_nsIContentParent_h
#define mozilla_dom_nsIContentParent_h

#include "mozilla/dom/ipc/Blob.h"

#include "nsFrameMessageManager.h"
#include "nsISupports.h"

#define NS_ICONTENTPARENT_IID                                   \
  { 0xeeec9ebf, 0x8ecf, 0x4e38,                                 \
    { 0x81, 0xda, 0xb7, 0x34, 0x13, 0x7e, 0xac, 0xf3 } }

class nsFrameMessageManager;

namespace IPC {
class Principal;
} 

namespace mozilla {

namespace jsipc {
class PJavaScriptParent;
class JavaScriptParent;
class CpowEntry;
} 

namespace dom {
struct IPCTabContext;
class ContentParent;

class nsIContentParent : public nsISupports
                       , public mozilla::dom::ipc::MessageManagerCallback
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENTPARENT_IID)

  nsIContentParent();
  BlobParent* GetOrCreateActorForBlob(nsIDOMBlob* aBlob);

  virtual uint64_t ChildID() = 0;
  virtual bool IsForApp() = 0;
  virtual bool IsForBrowser() = 0;

  virtual PBlobParent* SendPBlobConstructor(
    PBlobParent* actor,
    const BlobConstructorParams& params) NS_WARN_UNUSED_RESULT = 0;

  virtual PBrowserParent* SendPBrowserConstructor(
    PBrowserParent* actor,
    const IPCTabContext& context,
    const uint32_t& chromeFlags,
    const uint64_t& aId,
    const bool& aIsForApp,
    const bool& aIsForBrowser) NS_WARN_UNUSED_RESULT = 0;

  virtual jsipc::JavaScriptParent *GetCPOWManager() = 0;

  virtual bool IsContentParent() { return false; }
  ContentParent* AsContentParent();

protected: 
  bool CanOpenBrowser(const IPCTabContext& aContext);

protected: 
  virtual mozilla::jsipc::PJavaScriptParent* AllocPJavaScriptParent();
  virtual bool DeallocPJavaScriptParent(mozilla::jsipc::PJavaScriptParent*);

  virtual PBrowserParent* AllocPBrowserParent(const IPCTabContext& aContext,
                                              const uint32_t& aChromeFlags,
                                              const uint64_t& aId,
                                              const bool& aIsForApp,
                                              const bool& aIsForBrowser);
  virtual bool DeallocPBrowserParent(PBrowserParent* frame);

  virtual PBlobParent* AllocPBlobParent(const BlobConstructorParams& aParams);
  virtual bool DeallocPBlobParent(PBlobParent*);

  virtual bool RecvSyncMessage(const nsString& aMsg,
                               const ClonedMessageData& aData,
                               const InfallibleTArray<jsipc::CpowEntry>& aCpows,
                               const IPC::Principal& aPrincipal,
                               InfallibleTArray<nsString>* aRetvals);
  virtual bool AnswerRpcMessage(const nsString& aMsg,
                                const ClonedMessageData& aData,
                                const InfallibleTArray<jsipc::CpowEntry>& aCpows,
                                const IPC::Principal& aPrincipal,
                                InfallibleTArray<nsString>* aRetvals);
  virtual bool RecvAsyncMessage(const nsString& aMsg,
                                const ClonedMessageData& aData,
                                const InfallibleTArray<jsipc::CpowEntry>& aCpows,
                                const IPC::Principal& aPrincipal);

protected: 
  nsRefPtr<nsFrameMessageManager> mMessageManager;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContentParent, NS_ICONTENTPARENT_IID)

} 
} 

#endif 
