




#ifndef mozilla_dom_MessagePort_h
#define mozilla_dom_MessagePort_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsDOMEventTargetHelper.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class MessagePort MOZ_FINAL : public nsDOMEventTargetHelper
{
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
              const Optional<JS::Handle<JS::Value> >& aTransfer);

  void
  Start();

  void
  Close();

  IMPL_EVENT_HANDLER(message)

  

  
  
  
  void
  Entangle(MessagePort* aMessagePort);

private:
  nsRefPtr<MessagePort> mEntangledPort;
};

} 
} 

#endif 
