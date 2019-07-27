





#ifndef mozilla_dom_MessagePort_h
#define mozilla_dom_MessagePort_h

#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "nsIIPCBackgroundChildCreateCallback.h"
#include "nsTArray.h"

#ifdef XP_WIN
#undef PostMessage
#endif

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class DispatchEventRunnable;
class MessagePortChild;
class MessagePortIdentifier;
class MessagePortMessage;
class SharedMessagePortMessage;

namespace workers {
class WorkerFeature;
}

class MessagePortBase : public DOMEventTargetHelper
{
protected:
  explicit MessagePortBase(nsPIDOMWindow* aWindow);
  MessagePortBase();

public:

  virtual void
  PostMessage(JSContext* aCx, JS::Handle<JS::Value> aMessage,
              const Optional<Sequence<JS::Value>>& aTransferable,
              ErrorResult& aRv) = 0;

  virtual void
  Start() = 0;

  virtual void
  Close() = 0;

  
  
  virtual EventHandlerNonNull*
  GetOnmessage() = 0;

  virtual void
  SetOnmessage(EventHandlerNonNull* aCallback) = 0;

  
  
  
  virtual bool
  CloneAndDisentangle(MessagePortIdentifier& aIdentifier) = 0;
};

class MessagePort final : public MessagePortBase
                        , public nsIIPCBackgroundChildCreateCallback
                        , public nsIObserver
{
  friend class DispatchEventRunnable;

public:
  NS_DECL_NSIIPCBACKGROUNDCHILDCREATECALLBACK
  NS_DECL_NSIOBSERVER
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MessagePort,
                                           DOMEventTargetHelper)

  static already_AddRefed<MessagePort>
  Create(nsPIDOMWindow* aWindow, const nsID& aUUID,
         const nsID& aDestinationUUID, ErrorResult& aRv);

  static already_AddRefed<MessagePort>
  Create(nsPIDOMWindow* aWindow, const MessagePortIdentifier& aIdentifier,
         ErrorResult& aRv);

  static void
  ForceClose(const MessagePortIdentifier& aIdentifier);

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual void
  PostMessage(JSContext* aCx, JS::Handle<JS::Value> aMessage,
              const Optional<Sequence<JS::Value>>& aTransferable,
              ErrorResult& aRv) override;

  virtual void Start() override;

  virtual void Close() override;

  virtual EventHandlerNonNull* GetOnmessage() override;

  virtual void SetOnmessage(EventHandlerNonNull* aCallback) override;

  

  void UnshippedEntangle(MessagePort* aEntangledPort);

  virtual bool CloneAndDisentangle(MessagePortIdentifier& aIdentifier) override;

  

  void Entangled(nsTArray<MessagePortMessage>& aMessages);
  void MessagesReceived(nsTArray<MessagePortMessage>& aMessages);
  void StopSendingDataConfirmed();
  void Closed();

private:
  explicit MessagePort(nsPIDOMWindow* aWindow);
  ~MessagePort();

  enum State {
    
    
    
    
    
    eStateUnshippedEntangled,

    
    
    
    eStateEntangling,

    
    
    
    
    
    
    eStateEntangled,

    
    
    
    
    
    eStateDisentangling,

    
    
    
    
    
    eStateDisentangled
  };

  void Initialize(const nsID& aUUID, const nsID& aDestinationUUID,
                  uint32_t aSequenceID, bool mNeutered, State aState,
                  ErrorResult& aRv);

  void ConnectToPBackground();

  
  void Dispatch();

  void StartDisentangling();
  void Disentangle();

  void RemoveDocFromBFCache();

  
  
  
  void UpdateMustKeepAlive();

  nsAutoPtr<workers::WorkerFeature> mWorkerFeature;

  nsRefPtr<DispatchEventRunnable> mDispatchRunnable;

  nsRefPtr<MessagePortChild> mActor;

  nsRefPtr<MessagePort> mUnshippedEntangledPort;

  nsTArray<nsRefPtr<SharedMessagePortMessage>> mMessages;
  nsTArray<nsRefPtr<SharedMessagePortMessage>> mMessagesForTheOtherPort;

  nsAutoPtr<MessagePortIdentifier> mIdentifier;

  uint64_t mInnerID;

  State mState;

  
  
  enum {
    eNextStepNone,
    eNextStepDisentangle,
    eNextStepClose
  } mNextStep;

  bool mMessageQueueEnabled;

  bool mIsKeptAlive;
};

} 
} 

#endif 
