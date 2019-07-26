




#ifndef mozilla_dom_CellBroadcast_h__
#define mozilla_dom_CellBroadcast_h__

#include "nsDOMEventTargetHelper.h"
#include "nsIDOMMozCellBroadcast.h"
#include "nsIRadioInterfaceLayer.h"
#include "mozilla/Attributes.h"

class nsPIDOMWindow;

class nsIRILContentHelper;

namespace mozilla {
namespace dom {

class CellBroadcast MOZ_FINAL : public nsDOMEventTargetHelper
                              , public nsIDOMMozCellBroadcast
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZCELLBROADCAST

  





  NS_DECL_NSIRILCELLBROADCASTCALLBACK

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(CellBroadcast,
                                           nsDOMEventTargetHelper)

  CellBroadcast() MOZ_DELETE;
  CellBroadcast(nsPIDOMWindow *aWindow,
                nsIRILContentHelper* aRIL);
  ~CellBroadcast();

private:
  nsCOMPtr<nsIRILContentHelper> mRIL;
  nsCOMPtr<nsIRILCellBroadcastCallback> mCallback;
};

} 
} 

nsresult
NS_NewCellBroadcast(nsPIDOMWindow* aWindow,
                    nsIDOMMozCellBroadcast** aCellBroadcast);

#endif 
