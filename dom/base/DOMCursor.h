





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
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DOMCursor,
                                           DOMRequest)

  DOMCursor(nsPIDOMWindow* aWindow, nsICursorContinueCallback *aCallback);
  DOMCursor(nsIGlobalObject* aGlobal, nsICursorContinueCallback *aCallback);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  bool Done() const
  {
    return mFinished;
  }
  virtual void Continue(ErrorResult& aRv);

  void Reset();
  void FireDone();

protected:
  ~DOMCursor() {}

private:
  DOMCursor() = delete;
  
  
  already_AddRefed<mozilla::dom::Promise>
  Then(JSContext* aCx, AnyCallback* aResolveCallback,
       AnyCallback* aRejectCallback, ErrorResult& aRv) = delete;

  nsCOMPtr<nsICursorContinueCallback> mCallback;
  bool mFinished;
};

} 
} 

#endif 
