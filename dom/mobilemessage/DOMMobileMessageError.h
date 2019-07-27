





#ifndef mozilla_dom_MobileMessageError_h
#define mozilla_dom_MobileMessageError_h

#include "mozilla/dom/DOMError.h"

class nsIDOMMozMmsMessage;
class nsIDOMMozSmsMessage;

namespace mozilla {
namespace dom {

class OwningMozSmsMessageOrMozMmsMessage;

class DOMMobileMessageError final : public DOMError
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DOMMobileMessageError, DOMError)

  DOMMobileMessageError(nsPIDOMWindow* aWindow, const nsAString& aName,
                        nsIDOMMozSmsMessage* aSms);

  DOMMobileMessageError(nsPIDOMWindow* aWindow, const nsAString& aName,
                        nsIDOMMozMmsMessage* aMms);

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  void GetData(OwningMozSmsMessageOrMozMmsMessage& aRetVal) const;

private:
  ~DOMMobileMessageError() {}

  nsCOMPtr<nsIDOMMozSmsMessage> mSms;
  nsCOMPtr<nsIDOMMozMmsMessage> mMms;
};

} 
} 

#endif 
