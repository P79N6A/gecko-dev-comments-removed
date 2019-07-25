





































#include "SmsMessage.h"
#include "nsIDOMClassInfo.h"
#include "jsapi.h" 
#include "jsfriendapi.h" 
#include "Constants.h"

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

SmsMessage::SmsMessage(const SmsMessageData& aData)
  : mData(aData)
{
}

 nsresult
SmsMessage::Create(PRInt32 aId,
                   const nsAString& aDelivery,
                   const nsAString& aSender,
                   const nsAString& aReceiver,
                   const nsAString& aBody,
                   const jsval& aTimestamp,
                   JSContext* aCx,
                   nsIDOMMozSmsMessage** aMessage)
{
  *aMessage = nsnull;

  
  
  SmsMessageData data;
  data.id() = aId;
  data.sender() = nsString(aSender);
  data.receiver() = nsString(aReceiver);
  data.body() = nsString(aBody);

  if (aDelivery.Equals(DELIVERY_RECEIVED)) {
    data.delivery() = eDeliveryState_Received;
  } else if (aDelivery.Equals(DELIVERY_SENT)) {
    data.delivery() = eDeliveryState_Sent;
  } else {
    return NS_ERROR_INVALID_ARG;
  }

  
  if (aTimestamp.isObject()) {
    JSObject& obj = aTimestamp.toObject();
    if (!JS_ObjectIsDate(aCx, &obj)) {
      return NS_ERROR_INVALID_ARG;
    }
    data.timestamp() = js_DateGetMsecSinceEpoch(aCx, &obj);
  } else {
    if (!aTimestamp.isNumber()) {
      return NS_ERROR_INVALID_ARG;
    }
    jsdouble number = aTimestamp.toNumber();
    if (static_cast<PRUint64>(number) != number) {
      return NS_ERROR_INVALID_ARG;
    }
    data.timestamp() = static_cast<PRUint64>(number);
  }

  nsCOMPtr<nsIDOMMozSmsMessage> message = new SmsMessage(data);
  message.swap(*aMessage);
  return NS_OK;
}

const SmsMessageData&
SmsMessage::GetData() const
{
  return mData;
}

NS_IMETHODIMP
SmsMessage::GetId(PRInt32* aId)
{
  *aId = mData.id();
  return NS_OK;
}

NS_IMETHODIMP
SmsMessage::GetDelivery(nsAString& aDelivery)
{
  switch (mData.delivery()) {
    case eDeliveryState_Received:
      aDelivery.AssignLiteral("received");
      break;
    case eDeliveryState_Sent:
      aDelivery.AssignLiteral("sent");
      break;
    case eDeliveryState_Unknown:
    default:
      NS_ASSERTION(true, "We shouldn't get an unknown delivery state!");
      return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

NS_IMETHODIMP
SmsMessage::GetSender(nsAString& aSender)
{
  aSender = mData.sender();
  return NS_OK;
}

NS_IMETHODIMP
SmsMessage::GetReceiver(nsAString& aReceiver)
{
  aReceiver = mData.receiver();
  return NS_OK;
}

NS_IMETHODIMP
SmsMessage::GetBody(nsAString& aBody)
{
  aBody = mData.body();
  return NS_OK;
}

NS_IMETHODIMP
SmsMessage::GetTimestamp(JSContext* cx, jsval* aDate)
{
  *aDate = OBJECT_TO_JSVAL(JS_NewDateObjectMsec(cx, mData.timestamp()));
  return NS_OK;
}

} 
} 
} 
