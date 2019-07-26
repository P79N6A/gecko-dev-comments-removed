





#ifndef mozilla_dom_nsIContentChild_h
#define mozilla_dom_nsIContentChild_h

#include "mozilla/dom/ipc/Blob.h"

#include "nsISupports.h"

#define NS_ICONTENTCHILD_IID                                    \
  { 0x4eed2e73, 0x94ba, 0x48a8,                                 \
    { 0xa2, 0xd1, 0xa5, 0xed, 0x86, 0xd7, 0xbb, 0xe4 } }

class PBrowserChild;

namespace IPC {
class Principal;
} 

namespace mozilla {

namespace jsipc {
class PJavaScriptChild;
class JavaScriptChild;
class CpowEntry;
} 

namespace dom {
class IPCTabContext;

class nsIContentChild : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENTCHILD_IID)

  BlobChild* GetOrCreateActorForBlob(nsIDOMBlob* aBlob);

  virtual PBlobChild*
  SendPBlobConstructor(PBlobChild* aActor,
                       const BlobConstructorParams& params) = 0;
  virtual bool
  SendPBrowserConstructor(PBrowserChild* aActor,
                          const IPCTabContext& aContext,
                          const uint32_t& aChromeFlags,
                          const uint64_t& aID,
                          const bool& aIsForApp,
                          const bool& aIsForBrowser) = 0;
  virtual jsipc::JavaScriptChild* GetCPOWManager() = 0;
protected:
  virtual jsipc::PJavaScriptChild* AllocPJavaScriptChild();
  virtual bool DeallocPJavaScriptChild(jsipc::PJavaScriptChild*);

  virtual PBrowserChild* AllocPBrowserChild(const IPCTabContext& aContext,
                                            const uint32_t& aChromeFlags,
                                            const uint64_t& aID,
                                            const bool& aIsForApp,
                                            const bool& aIsForBrowser);
  virtual bool DeallocPBrowserChild(PBrowserChild*);

  virtual PBlobChild* AllocPBlobChild(const BlobConstructorParams& aParams);
  virtual bool DeallocPBlobChild(PBlobChild*);

  virtual bool RecvAsyncMessage(const nsString& aMsg,
                                const ClonedMessageData& aData,
                                const InfallibleTArray<jsipc::CpowEntry>& aCpows,
                                const IPC::Principal& aPrincipal);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContentChild, NS_ICONTENTCHILD_IID)

} 
} 
#endif 
