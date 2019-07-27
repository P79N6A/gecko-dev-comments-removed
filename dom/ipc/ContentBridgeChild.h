





#ifndef mozilla_dom_ContentBridgeChild_h
#define mozilla_dom_ContentBridgeChild_h

#include "mozilla/dom/PContentBridgeChild.h"
#include "mozilla/dom/nsIContentChild.h"
#include "nsIObserver.h"

namespace mozilla {
namespace dom {

class ContentBridgeChild final : public PContentBridgeChild
                               , public nsIContentChild
                               , public nsIObserver
{
public:
  explicit ContentBridgeChild(Transport* aTransport);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  static ContentBridgeChild*
  Create(Transport* aTransport, ProcessId aOtherProcess);

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;
  void DeferredDestroy();

  virtual bool RecvAsyncMessage(const nsString& aMsg,
                                const ClonedMessageData& aData,
                                InfallibleTArray<jsipc::CpowEntry>&& aCpows,
                                const IPC::Principal& aPrincipal) override;

  virtual PBlobChild*
  SendPBlobConstructor(PBlobChild* actor,
                       const BlobConstructorParams& aParams) override;

  jsipc::CPOWManager* GetCPOWManager() override;

  virtual bool SendPBrowserConstructor(PBrowserChild* aActor,
                                       const TabId& aTabId,
                                       const IPCTabContext& aContext,
                                       const uint32_t& aChromeFlags,
                                       const ContentParentId& aCpID,
                                       const bool& aIsForApp,
                                       const bool& aIsForBrowser) override;

protected:
  virtual ~ContentBridgeChild();

  virtual PBrowserChild* AllocPBrowserChild(const TabId& aTabId,
                                            const IPCTabContext& aContext,
                                            const uint32_t& aChromeFlags,
                                            const ContentParentId& aCpID,
                                            const bool& aIsForApp,
                                            const bool& aIsForBrowser) override;
  virtual bool DeallocPBrowserChild(PBrowserChild*) override;
  virtual bool RecvPBrowserConstructor(PBrowserChild* aCctor,
                                       const TabId& aTabId,
                                       const IPCTabContext& aContext,
                                       const uint32_t& aChromeFlags,
                                       const ContentParentId& aCpID,
                                       const bool& aIsForApp,
                                       const bool& aIsForBrowser) override;

  virtual mozilla::jsipc::PJavaScriptChild* AllocPJavaScriptChild() override;
  virtual bool DeallocPJavaScriptChild(mozilla::jsipc::PJavaScriptChild*) override;

  virtual PBlobChild* AllocPBlobChild(const BlobConstructorParams& aParams) override;
  virtual bool DeallocPBlobChild(PBlobChild*) override;

  DISALLOW_EVIL_CONSTRUCTORS(ContentBridgeChild);

protected: 
  nsRefPtr<ContentBridgeChild> mSelfRef;
  Transport* mTransport; 
};

} 
} 

#endif 
