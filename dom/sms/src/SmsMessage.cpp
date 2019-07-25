




































#include "SmsMessage.h"
#include "nsIDOMClassInfo.h"
#include "jsapi.h" 

DOMCI_DATA(MozSmsMessage, mozilla::dom::sms::SmsMessage)

namespace mozilla {
namespace dom {
namespace sms {

NS_INTERFACE_MAP_BEGIN(SmsMessage)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozSmsMessage)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozSmsMessage)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(SmsMessage)
NS_IMPL_RELEASE(SmsMessage)

SmsMessage::SmsMessage(PRInt32 aId, DeliveryState aDelivery,
                       const nsAString& aSender, const nsAString& aReceiver,
                       const nsAString& aBody, PRUint64 aTimestamp)
  : mId(aId)
  , mDelivery(aDelivery)
  , mSender(aSender)
  , mReceiver(aReceiver)
  , mBody(aBody)
  , mTimestamp(aTimestamp)
{
}

NS_IMETHODIMP
SmsMessage::GetId(PRInt32* aId)
{
  *aId = mId;
  return NS_OK;
}

NS_IMETHODIMP
SmsMessage::GetDelivery(nsAString& aDelivery)
{
  switch (mDelivery) {
    case eDeliveryState_Sent:
      aDelivery.AssignLiteral("sent");
      break;
    case eDeliveryState_Received:
      aDelivery.AssignLiteral("received");
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP
SmsMessage::GetSender(nsAString& aSender)
{
  aSender = mSender;
  return NS_OK;
}

NS_IMETHODIMP
SmsMessage::GetReceiver(nsAString& aReceiver)
{
  aReceiver = mReceiver;
  return NS_OK;
}

NS_IMETHODIMP
SmsMessage::GetBody(nsAString& aBody)
{
  aBody = mBody;
  return NS_OK;
}

NS_IMETHODIMP
SmsMessage::GetTimestamp(JSContext* cx, jsval* aDate)
{
  *aDate = OBJECT_TO_JSVAL(JS_NewDateObjectMsec(cx, mTimestamp));
  return NS_OK;
}

} 
} 
} 
