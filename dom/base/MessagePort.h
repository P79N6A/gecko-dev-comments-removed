





#ifndef mozilla_dom_MessagePort_h
#define mozilla_dom_MessagePort_h

#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class DispatchEventRunnable;
class PostMessageRunnable;

class MessagePortBase : public DOMEventTargetHelper
{
protected:
  explicit MessagePortBase(nsPIDOMWindow* aWindow);
  MessagePortBase();

public:

  virtual void
  PostMessageMoz(JSContext* aCx, JS::Handle<JS::Value> aMessage,
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

  
  
  
  virtual already_AddRefed<MessagePortBase>
  Clone() = 0;
};

class MessagePort final : public MessagePortBase
{
  friend class DispatchEventRunnable;
  friend class PostMessageRunnable;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MessagePort,
                                           DOMEventTargetHelper)

  explicit MessagePort(nsPIDOMWindow* aWindow);

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual void
  PostMessageMoz(JSContext* aCx, JS::Handle<JS::Value> aMessage,
                 const Optional<Sequence<JS::Value>>& aTransferable,
                 ErrorResult& aRv) override;

  virtual void
  Start() override;

  virtual void
  Close() override;

  virtual EventHandlerNonNull*
  GetOnmessage() override;

  virtual void
  SetOnmessage(EventHandlerNonNull* aCallback) override;

  

  
  
  
  void
  Entangle(MessagePort* aMessagePort);

  virtual already_AddRefed<MessagePortBase>
  Clone() override;

private:
  ~MessagePort();

  
  void Dispatch();

  nsRefPtr<DispatchEventRunnable> mDispatchRunnable;

  nsRefPtr<MessagePort> mEntangledPort;

  nsTArray<nsRefPtr<PostMessageRunnable> > mMessageQueue;
  bool mMessageQueueEnabled;
};

} 
} 

#endif 
