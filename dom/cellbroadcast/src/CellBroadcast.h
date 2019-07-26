




#ifndef mozilla_dom_CellBroadcast_h__
#define mozilla_dom_CellBroadcast_h__

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsDOMEventTargetHelper.h"
#include "nsICellBroadcastProvider.h"

class JSObject;
struct JSContext;

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class CellBroadcast MOZ_FINAL : public nsDOMEventTargetHelper
{
  






  class Listener;

public:
  NS_DECL_NSICELLBROADCASTLISTENER

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper)

  static already_AddRefed<CellBroadcast>
  Create(nsPIDOMWindow* aOwner, ErrorResult& aRv);

  CellBroadcast() MOZ_DELETE;
  CellBroadcast(nsPIDOMWindow *aWindow,
                nsICellBroadcastProvider* aProvider);
  
  ~CellBroadcast();

  nsPIDOMWindow*
  GetParentObject() const { return GetOwner(); }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  IMPL_EVENT_HANDLER(received)

private:
  nsCOMPtr<nsICellBroadcastProvider> mProvider;
  nsRefPtr<Listener> mListener;
};

} 
} 

#endif 
