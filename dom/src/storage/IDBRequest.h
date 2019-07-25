






































#ifndef mozilla_dom_idb_request_h__
#define mozilla_dom_idb_request_h__

#include "nsIIDBRequest.h"
#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {
namespace idb {

class Request : public nsDOMEventTargetHelper
              , public nsIIDBRequest

{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBREQUEST
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(Request,
                                           nsDOMEventTargetHelper)

  Request();

protected:
  PRUint16 mReadyState;

private:
  nsRefPtr<nsDOMEventListenerWrapper> mOnSuccessListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;
};

} 
} 
} 

#endif 
