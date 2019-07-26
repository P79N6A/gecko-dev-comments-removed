




#ifndef mozilla_dom_MessagePort_h
#define mozilla_dom_MessagePort_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsDOMEventTargetHelper.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class DispatchEventRunnable;
class PostMessageRunnable;

class MessagePort MOZ_FINAL : public nsDOMEventTargetHelper
{
  friend class DispatchEventRunnable;
  friend class PostMessageRunnable;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper)
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MessagePort,
                                           nsDOMEventTargetHelper)

  MessagePort(nsPIDOMWindow* aWindow);
  ~MessagePort();

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  void
  PostMessage(JSContext* aCx, JS::Handle<JS::Value> aMessage,
              const Optional<JS::Handle<JS::Value> >& aTransfer,
              ErrorResult& aRv);

  void
  Start();

  void
  Close();

  
  
  EventHandlerNonNull*
  GetOnmessage();

  void
  SetOnmessage(EventHandlerNonNull* aCallback);

  

  
  
  
  void
  Entangle(MessagePort* aMessagePort);

  
  
  
  already_AddRefed<MessagePort>
  Clone();

private:
  
  void Dispatch();

  nsRefPtr<DispatchEventRunnable> mDispatchRunnable;

  nsRefPtr<MessagePort> mEntangledPort;

  nsTArray<nsRefPtr<PostMessageRunnable> > mMessageQueue;
  bool mMessageQueueEnabled;
};

} 
} 

#endif 
