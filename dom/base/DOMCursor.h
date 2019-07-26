





#ifndef mozilla_dom_domcursor_h__
#define mozilla_dom_domcursor_h__

#include "nsIDOMDOMCursor.h"
#include "DOMRequest.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {

class DOMCursor : public nsIDOMDOMCursor
                , public DOMRequest
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMDOMCURSOR
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(DOMCursor,
                                                         DOMRequest)

  DOMCursor() MOZ_DELETE;
  DOMCursor(nsIDOMWindow* aWindow, nsICursorContinueCallback *aCallback);

  void Reset();
  void FireDone();

private:
  nsCOMPtr<nsICursorContinueCallback> mCallback;
  bool mFinished;
};

} 
} 

#endif 
