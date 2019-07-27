





#ifndef mozilla_dom_nsIContentChild_h
#define mozilla_dom_nsIContentChild_h

#include "mozilla/dom/ipc/IdType.h"

#include "nsISupports.h"
#include "nsTArrayForwardDeclare.h"
#include "mozilla/dom/CPOWManagerGetter.h"

#define NS_ICONTENTCHILD_IID                                    \
  { 0x4eed2e73, 0x94ba, 0x48a8,                                 \
    { 0xa2, 0xd1, 0xa5, 0xed, 0x86, 0xd7, 0xbb, 0xe4 } }

class nsString;

namespace IPC {
class Principal;
} 

namespace mozilla {

namespace jsipc {
class PJavaScriptChild;
class CpowEntry;
} 

namespace dom {

class Blob;
class BlobChild;
class BlobImpl;
class BlobConstructorParams;
class ClonedMessageData;
class IPCTabContext;
class PBlobChild;
class PBrowserChild;

class nsIContentChild : public nsISupports
                      , public CPOWManagerGetter
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENTCHILD_IID)

  BlobChild* GetOrCreateActorForBlob(Blob* aBlob);
  BlobChild* GetOrCreateActorForBlobImpl(BlobImpl* aImpl);

  virtual PBlobChild* SendPBlobConstructor(
    PBlobChild* aActor,
    const BlobConstructorParams& aParams) = 0;

  virtual bool
  SendPBrowserConstructor(PBrowserChild* aActor,
                          const TabId& aTabId,
                          const IPCTabContext& aContext,
                          const uint32_t& aChromeFlags,
                          const ContentParentId& aCpID,
                          const bool& aIsForApp,
                          const bool& aIsForBrowser) = 0;
protected:
  virtual jsipc::PJavaScriptChild* AllocPJavaScriptChild();
  virtual bool DeallocPJavaScriptChild(jsipc::PJavaScriptChild*);

  virtual PBrowserChild* AllocPBrowserChild(const TabId& aTabId,
                                            const IPCTabContext& aContext,
                                            const uint32_t& aChromeFlags,
                                            const ContentParentId& aCpId,
                                            const bool& aIsForApp,
                                            const bool& aIsForBrowser);
  virtual bool DeallocPBrowserChild(PBrowserChild*);

  virtual PBlobChild* AllocPBlobChild(const BlobConstructorParams& aParams);

  virtual bool DeallocPBlobChild(PBlobChild* aActor);

  virtual bool RecvAsyncMessage(const nsString& aMsg,
                                const ClonedMessageData& aData,
                                InfallibleTArray<jsipc::CpowEntry>&& aCpows,
                                const IPC::Principal& aPrincipal);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContentChild, NS_ICONTENTCHILD_IID)

} 
} 

#endif 
