




#ifndef mozilla_dom_mobilemessage_MobileMessageCursorCallback_h
#define mozilla_dom_mobilemessage_MobileMessageCursorCallback_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/DOMCursor.h"
#include "nsIMobileMessageCursorCallback.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"

class nsICursorContinueCallback;

namespace mozilla {
namespace dom {

class MobileMessageManager;

namespace mobilemessage {
class MobileMessageCursorCallback;
} 

class MobileMessageCursor MOZ_FINAL : public DOMCursor
{
  friend class mobilemessage::MobileMessageCursorCallback;

public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MobileMessageCursor, DOMCursor)

  MobileMessageCursor(nsPIDOMWindow* aWindow,
                      nsICursorContinueCallback* aCallback);

  
  NS_IMETHOD
  Continue(void) MOZ_OVERRIDE;

  virtual void
  Continue(ErrorResult& aRv) MOZ_OVERRIDE;

private:
  
  ~MobileMessageCursor() {}

private:
  
  nsTArray<nsCOMPtr<nsISupports>> mPendingResults;

  nsresult
  FireSuccessWithNextPendingResult();
};

namespace mobilemessage {

class MobileMessageCursorCallback MOZ_FINAL : public nsIMobileMessageCursorCallback
{
  friend class mozilla::dom::MobileMessageManager;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIMOBILEMESSAGECURSORCALLBACK

  NS_DECL_CYCLE_COLLECTION_CLASS(MobileMessageCursorCallback)

  MobileMessageCursorCallback()
  {
    MOZ_COUNT_CTOR(MobileMessageCursorCallback);
  }

private:
  
  ~MobileMessageCursorCallback()
  {
    MOZ_COUNT_DTOR(MobileMessageCursorCallback);
  }

  nsRefPtr<MobileMessageCursor> mDOMCursor;
};

} 
} 
} 

#endif 

