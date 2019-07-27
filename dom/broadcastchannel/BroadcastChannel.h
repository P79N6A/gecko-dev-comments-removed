




#ifndef mozilla_dom_BroadcastChannel_h
#define mozilla_dom_BroadcastChannel_h

#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/ipc/PBackgroundSharedTypes.h"
#include "nsIIPCBackgroundChildCreateCallback.h"
#include "nsIObserver.h"
#include "nsTArray.h"
#include "nsRefPtr.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

namespace workers {
class WorkerFeature;
}

class BroadcastChannelChild;

class BroadcastChannel MOZ_FINAL
  : public DOMEventTargetHelper
  , public nsIIPCBackgroundChildCreateCallback
  , public nsIObserver
{
  NS_DECL_NSIIPCBACKGROUNDCHILDCREATECALLBACK
  NS_DECL_NSIOBSERVER

  typedef mozilla::ipc::PrincipalInfo PrincipalInfo;

public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BroadcastChannel,
                                           DOMEventTargetHelper)

  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  static already_AddRefed<BroadcastChannel>
  Constructor(const GlobalObject& aGlobal, const nsAString& aChannel,
              ErrorResult& aRv);

  void GetName(nsAString& aName) const
  {
    aName = mChannel;
  }

  void PostMessage(const nsAString& aMessage);

  EventHandlerNonNull* GetOnmessage();
  void SetOnmessage(EventHandlerNonNull* aCallback);

  using nsIDOMEventTarget::AddEventListener;
  using nsIDOMEventTarget::RemoveEventListener;

  virtual void AddEventListener(const nsAString& aType,
                                EventListener* aCallback,
                                bool aCapture,
                                const Nullable<bool>& aWantsUntrusted,
                                ErrorResult& aRv) MOZ_OVERRIDE;
  virtual void RemoveEventListener(const nsAString& aType,
                                   EventListener* aCallback,
                                   bool aCapture,
                                   ErrorResult& aRv) MOZ_OVERRIDE;

  void Shutdown();

private:
  BroadcastChannel(nsPIDOMWindow* aWindow,
                   const PrincipalInfo& aPrincipalInfo,
                   const nsAString& aOrigin,
                   const nsAString& aChannel);

  ~BroadcastChannel();

  void UpdateMustKeepAlive();

  nsRefPtr<BroadcastChannelChild> mActor;
  nsTArray<nsString> mPendingMessages;

  workers::WorkerFeature* mWorkerFeature;

  PrincipalInfo mPrincipalInfo;
  nsString mOrigin;
  nsString mChannel;

  bool mIsKeptAlive;

  uint64_t mInnerID;
};

} 
} 

#endif 
