





#ifndef mozilla_dom_PostMessageEvent_h
#define mozilla_dom_PostMessageEvent_h

#include "js/StructuredClone.h"
#include "nsCOMPtr.h"
#include "nsRefPtr.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"

class nsGlobalWindow;
class nsIPrincipal;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class MessagePortBase;
class MessagePortIdentifier;





class PostMessageEvent final : public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  PostMessageEvent(nsGlobalWindow* aSource,
                   const nsAString& aCallerOrigin,
                   nsGlobalWindow* aTargetWindow,
                   nsIPrincipal* aProvidedPrincipal,
                   bool aTrustedCaller);

  bool Write(JSContext* aCx, JS::Handle<JS::Value> aMessage,
             JS::Handle<JS::Value> aTransfer, bool aSubsumes,
             nsPIDOMWindow* aWindow);

private:
  ~PostMessageEvent();

  const MessagePortIdentifier& GetPortIdentifier(uint64_t aId);

  MessagePortIdentifier* NewPortIdentifier(uint64_t* aPosition);

  bool StoreISupports(nsISupports* aSupports)
  {
    mSupportsArray.AppendElement(aSupports);
    return true;
  }

  static JSObject*
  ReadStructuredClone(JSContext* cx,
                      JSStructuredCloneReader* reader,
                      uint32_t tag,
                      uint32_t data,
                      void* closure);

  static bool
  WriteStructuredClone(JSContext* cx,
                       JSStructuredCloneWriter* writer,
                       JS::Handle<JSObject*> obj,
                       void *closure);

  static bool
  ReadTransferStructuredClone(JSContext* aCx,
                              JSStructuredCloneReader* reader,
                              uint32_t tag, void* aData,
                              uint64_t aExtraData,
                              void* aClosure,
                              JS::MutableHandle<JSObject*> returnObject);

  static bool
  TransferStructuredClone(JSContext* aCx,
                          JS::Handle<JSObject*> aObj,
                          void* aClosure,
                          uint32_t* aTag,
                          JS::TransferableOwnership* aOwnership,
                          void** aContent,
                          uint64_t* aExtraData);

  static void
  FreeTransferStructuredClone(uint32_t aTag,
                              JS::TransferableOwnership aOwnership,
                              void *aContent,
                              uint64_t aExtraData,
                              void* aClosure);

  static const JSStructuredCloneCallbacks sPostMessageCallbacks;

  JSAutoStructuredCloneBuffer mBuffer;
  nsRefPtr<nsGlobalWindow> mSource;
  nsString mCallerOrigin;
  nsRefPtr<nsGlobalWindow> mTargetWindow;
  nsCOMPtr<nsIPrincipal> mProvidedPrincipal;
  bool mTrustedCaller;
  nsTArray<nsCOMPtr<nsISupports>> mSupportsArray;
  nsTArray<MessagePortIdentifier> mPortIdentifiers;
};

} 
} 

#endif 
