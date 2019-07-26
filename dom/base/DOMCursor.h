





#ifndef mozilla_dom_domcursor_h__
#define mozilla_dom_domcursor_h__

#include "nsIDOMDOMCursor.h"
#include "DOMRequest.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {

class DOMCursor : public DOMRequest
                , public nsIDOMDOMCursor
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMDOMCURSOR
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(DOMCursor,
                                                         DOMRequest)

  DOMCursor(nsIDOMWindow* aWindow, nsICursorContinueCallback *aCallback);

  virtual JSObject*
  WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  bool Done() const
  {
    return mFinished;
  }
  virtual void Continue(ErrorResult& aRv);

  void Reset();
  void FireDone();

private:
  DOMCursor() MOZ_DELETE;

  nsCOMPtr<nsICursorContinueCallback> mCallback;
  bool mFinished;
};

} 
} 

#endif 
