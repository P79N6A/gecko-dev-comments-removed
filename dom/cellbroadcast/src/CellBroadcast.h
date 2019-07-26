




#ifndef mozilla_dom_CellBroadcast_h__
#define mozilla_dom_CellBroadcast_h__

#include "nsDOMEventTargetHelper.h"
#include "nsIDOMMozCellBroadcast.h"
#include "mozilla/Attributes.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class CellBroadcast MOZ_FINAL : public nsDOMEventTargetHelper
                              , public nsIDOMMozCellBroadcast
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZCELLBROADCAST

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(CellBroadcast,
                                           nsDOMEventTargetHelper)

  CellBroadcast() MOZ_DELETE;
  CellBroadcast(nsPIDOMWindow *aWindow);
};

} 
} 

nsresult
NS_NewCellBroadcast(nsPIDOMWindow* aWindow,
                    nsIDOMMozCellBroadcast** aCellBroadcast);

#endif 
