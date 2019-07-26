




#ifndef mozilla_dom_CellBroadcast_h__
#define mozilla_dom_CellBroadcast_h__

#include "nsDOMEventTargetHelper.h"
#include "nsIDOMMozCellBroadcast.h"
#include "nsICellBroadcastProvider.h"
#include "mozilla/Attributes.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class CellBroadcast MOZ_FINAL : public nsDOMEventTargetHelper
                              , public nsIDOMMozCellBroadcast
{
  






  class Listener;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMMOZCELLBROADCAST
  NS_DECL_NSICELLBROADCASTLISTENER

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  CellBroadcast() MOZ_DELETE;
  CellBroadcast(nsPIDOMWindow *aWindow,
                nsICellBroadcastProvider* aProvider);
  ~CellBroadcast();

private:
  nsCOMPtr<nsICellBroadcastProvider> mProvider;
  nsRefPtr<Listener> mListener;
};

} 
} 

nsresult
NS_NewCellBroadcast(nsPIDOMWindow* aWindow,
                    nsIDOMMozCellBroadcast** aCellBroadcast);

#endif 
