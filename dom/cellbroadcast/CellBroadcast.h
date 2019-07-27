





#ifndef mozilla_dom_CellBroadcast_h__
#define mozilla_dom_CellBroadcast_h__

#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/ErrorResult.h"
#include "nsICellBroadcastService.h"
#include "js/TypeDecls.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class CellBroadcast final : public DOMEventTargetHelper,
                                private nsICellBroadcastListener
{
  






  class Listener;

  
  ~CellBroadcast();

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSICELLBROADCASTLISTENER

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(DOMEventTargetHelper)

  static already_AddRefed<CellBroadcast>
  Create(nsPIDOMWindow* aOwner, ErrorResult& aRv);

  CellBroadcast() = delete;
  CellBroadcast(nsPIDOMWindow *aWindow,
                nsICellBroadcastService* aService);

  nsPIDOMWindow*
  GetParentObject() const { return GetOwner(); }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  IMPL_EVENT_HANDLER(received)

private:
  nsRefPtr<Listener> mListener;
};

} 
} 

#endif 
