




#ifndef mozilla_dom_CellBroadcast_h__
#define mozilla_dom_CellBroadcast_h__

#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/ErrorResult.h"
#include "nsICellBroadcastProvider.h"
#include "js/TypeDecls.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class CellBroadcast MOZ_FINAL : public DOMEventTargetHelper,
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

  CellBroadcast() MOZ_DELETE;
  CellBroadcast(nsPIDOMWindow *aWindow,
                nsICellBroadcastProvider* aProvider);

  nsPIDOMWindow*
  GetParentObject() const { return GetOwner(); }

  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  IMPL_EVENT_HANDLER(received)

private:
  nsCOMPtr<nsICellBroadcastProvider> mProvider;
  nsRefPtr<Listener> mListener;
};

} 
} 

#endif 
