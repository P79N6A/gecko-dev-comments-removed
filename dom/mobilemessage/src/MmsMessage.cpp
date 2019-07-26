




#include "MmsMessage.h"
#include "nsIDOMClassInfo.h"
#include "jsapi.h" 
#include "jsfriendapi.h" 
#include "nsJSUtils.h"
#include "Constants.h"
#include "nsContentUtils.h"
#include "nsIDOMFile.h"
#include "nsTArrayHelpers.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/mobilemessage/SmsTypes.h"

using namespace mozilla::idl;
using namespace mozilla::dom::mobilemessage;

DOMCI_DATA(MozMmsMessage, mozilla::dom::MmsMessage)

namespace mozilla {
namespace dom {

NS_INTERFACE_MAP_BEGIN(MmsMessage)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozMmsMessage)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozMmsMessage)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(MmsMessage)
NS_IMPL_RELEASE(MmsMessage)

MmsMessage::MmsMessage(int32_t                         aId,
                       const uint64_t                  aThreadId,
                       DeliveryState                   aDelivery,
                       const nsTArray<DeliveryStatus>& aDeliveryStatus,
                       const nsAString&                aSender,
                       const nsTArray<nsString>&       aReceivers,
                       uint64_t                        aTimestamp,
                       bool                            aRead,
                       const nsAString&                aSubject,
                       const nsAString&                aSmil,
                       const nsTArray<MmsAttachment>&  aAttachments)
  : mId(aId),
    mThreadId(aThreadId),
    mDelivery(aDelivery),
    mDeliveryStatus(aDeliveryStatus),
    mSender(aSender),
    mReceivers(aReceivers),
    mTimestamp(aTimestamp),
    mRead(aRead),
    mSubject(aSubject),
    mSmil(aSmil),
    mAttachments(aAttachments)
{
}

MmsMessage::MmsMessage(const mobilemessage::MmsMessageData& aData)
  : mId(aData.id())
  , mDelivery(aData.delivery())
  , mDeliveryStatus(aData.deliveryStatus())
  , mSender(aData.sender())
  , mReceivers(aData.receivers())
  , mTimestamp(aData.timestamp())
  , mRead(aData.read())
  , mSubject(aData.subject())
  , mSmil(aData.smil())
{
  uint32_t len = aData.attachments().Length();
  mAttachments.SetCapacity(len);
  for (uint32_t i = 0; i < len; i++) {
    MmsAttachment att;
    const MmsAttachmentData &element = aData.attachments()[i];
    att.id = element.id();
    att.location = element.location();
    if (element.contentParent()) {
      att.content = static_cast<BlobParent*>(element.contentParent())->GetBlob();
    } else if (element.contentChild()) {
      att.content = static_cast<BlobChild*>(element.contentChild())->GetBlob();
    } else {
      NS_WARNING("MmsMessage: Unable to get attachment content.");
    }
    mAttachments.AppendElement(att);
  }
}

 nsresult
MmsMessage::Create(int32_t               aId,
                   const uint64_t        aThreadId,
                   const nsAString&      aDelivery,
                   const JS::Value&      aDeliveryStatus,
                   const nsAString&      aSender,
                   const JS::Value&      aReceivers,
                   const JS::Value&      aTimestamp,
                   bool                  aRead,
                   const nsAString&      aSubject,
                   const nsAString&      aSmil,
                   const JS::Value&      aAttachments,
                   JSContext*            aCx,
                   nsIDOMMozMmsMessage** aMessage)
{
  *aMessage = nullptr;

  
  DeliveryState delivery;
  if (aDelivery.Equals(DELIVERY_SENT)) {
    delivery = eDeliveryState_Sent;
  } else if (aDelivery.Equals(DELIVERY_RECEIVED)) {
    delivery = eDeliveryState_Received;
  } else if (aDelivery.Equals(DELIVERY_SENDING)) {
    delivery = eDeliveryState_Sending;
  } else if (aDelivery.Equals(DELIVERY_NOT_DOWNLOADED)) {
    delivery = eDeliveryState_NotDownloaded;
  } else if (aDelivery.Equals(DELIVERY_ERROR)) {
    delivery = eDeliveryState_Error;
  } else {
    return NS_ERROR_INVALID_ARG;
  }

  
  if (!aDeliveryStatus.isObject()) {
    return NS_ERROR_INVALID_ARG;
  }
  JSObject* deliveryStatusObj = &aDeliveryStatus.toObject();
  if (!JS_IsArrayObject(aCx, deliveryStatusObj)) {
    return NS_ERROR_INVALID_ARG;
  }

  uint32_t length;
  JS_ALWAYS_TRUE(JS_GetArrayLength(aCx, deliveryStatusObj, &length));

  nsTArray<DeliveryStatus> deliveryStatus;
  for (uint32_t i = 0; i < length; ++i) {
    JS::Value statusJsVal;
    if (!JS_GetElement(aCx, deliveryStatusObj, i, &statusJsVal) ||
        !statusJsVal.isString()) {
      return NS_ERROR_INVALID_ARG;
    }

    nsDependentJSString statusStr;
    statusStr.init(aCx, statusJsVal.toString());

    DeliveryStatus status;
    if (statusStr.Equals(DELIVERY_STATUS_NOT_APPLICABLE)) {
      status = eDeliveryStatus_NotApplicable;
    } else if (statusStr.Equals(DELIVERY_STATUS_SUCCESS)) {
      status = eDeliveryStatus_Success;
    } else if (statusStr.Equals(DELIVERY_STATUS_PENDING)) {
      status = eDeliveryStatus_Pending;
    } else if (statusStr.Equals(DELIVERY_STATUS_ERROR)) {
      status = eDeliveryStatus_Error;
    } else {
      return NS_ERROR_INVALID_ARG;
    }

    deliveryStatus.AppendElement(status);
  }

  
  if (!aReceivers.isObject()) {
    return NS_ERROR_INVALID_ARG;
  }
  JSObject* receiversObj = &aReceivers.toObject();
  if (!JS_IsArrayObject(aCx, receiversObj)) {
    return NS_ERROR_INVALID_ARG;
  }

  JS_ALWAYS_TRUE(JS_GetArrayLength(aCx, receiversObj, &length));

  nsTArray<nsString> receivers;
  for (uint32_t i = 0; i < length; ++i) {
    JS::Value receiverJsVal;
    if (!JS_GetElement(aCx, receiversObj, i, &receiverJsVal) ||
        !receiverJsVal.isString()) {
      return NS_ERROR_INVALID_ARG;
    }

    nsDependentJSString receiverStr;
    receiverStr.init(aCx, receiverJsVal.toString());
    receivers.AppendElement(receiverStr);
  }

  
  uint64_t timestamp;
  if (aTimestamp.isObject()) {
    JSObject* timestampObj = &aTimestamp.toObject();
    if (!JS_ObjectIsDate(aCx, timestampObj)) {
      return NS_ERROR_INVALID_ARG;
    }
    timestamp = js_DateGetMsecSinceEpoch(timestampObj);
  } else {
    if (!aTimestamp.isNumber()) {
      return NS_ERROR_INVALID_ARG;
    }
    double number = aTimestamp.toNumber();
    if (static_cast<uint64_t>(number) != number) {
      return NS_ERROR_INVALID_ARG;
    }
    timestamp = static_cast<uint64_t>(number);
  }

  
  if (!aAttachments.isObject()) {
    return NS_ERROR_INVALID_ARG;
  }
  JSObject* attachmentsObj = &aAttachments.toObject();
  if (!JS_IsArrayObject(aCx, attachmentsObj)) {
    return NS_ERROR_INVALID_ARG;
  }

  nsTArray<MmsAttachment> attachments;
  JS_ALWAYS_TRUE(JS_GetArrayLength(aCx, attachmentsObj, &length));

  for (uint32_t i = 0; i < length; ++i) {
    JS::Value attachmentJsVal;
    if (!JS_GetElement(aCx, attachmentsObj, i, &attachmentJsVal)) {
      return NS_ERROR_INVALID_ARG;
    }

    MmsAttachment attachment;
    nsresult rv = attachment.Init(aCx, &attachmentJsVal);
    NS_ENSURE_SUCCESS(rv, rv);

    attachments.AppendElement(attachment);
  }

  nsCOMPtr<nsIDOMMozMmsMessage> message = new MmsMessage(aId,
                                                         aThreadId,
                                                         delivery,
                                                         deliveryStatus,
                                                         aSender,
                                                         receivers,
                                                         timestamp,
                                                         aRead,
                                                         aSubject,
                                                         aSmil,
                                                         attachments);
  message.forget(aMessage);
  return NS_OK;
}

bool
MmsMessage::GetData(ContentParent* aParent,
                    mobilemessage::MmsMessageData& aData)
{
  NS_ASSERTION(aParent, "aParent is null");

  aData.id() = mId;
  aData.delivery() = mDelivery;
  aData.deliveryStatus() = mDeliveryStatus;
  aData.sender().Assign(mSender);
  aData.receivers() = mReceivers;
  aData.timestamp() = mTimestamp;
  aData.read() = mRead;
  aData.subject() = mSubject;
  aData.smil() = mSmil;

  aData.attachments().SetCapacity(mAttachments.Length());
  for (uint32_t i = 0; i < mAttachments.Length(); i++) {
    MmsAttachmentData mma;
    const MmsAttachment &element = mAttachments[i];
    mma.id().Assign(element.id);
    mma.location().Assign(element.location);
    mma.contentParent() = aParent->GetOrCreateActorForBlob(element.content);
    if (!mma.contentParent()) {
      return false;
    }
    aData.attachments().AppendElement(mma);
  }

  return true;
}

NS_IMETHODIMP
MmsMessage::GetType(nsAString& aType)
{
  aType = NS_LITERAL_STRING("mms");
  return NS_OK;
}

NS_IMETHODIMP
MmsMessage::GetId(int32_t* aId)
{
  *aId = mId;
  return NS_OK;
}

NS_IMETHODIMP
MmsMessage::GetThreadId(uint64_t* aThreadId)
{
  *aThreadId = mThreadId;
  return NS_OK;
}

NS_IMETHODIMP
MmsMessage::GetDelivery(nsAString& aDelivery)
{
  switch (mDelivery) {
    case eDeliveryState_Received:
      aDelivery = DELIVERY_RECEIVED;
      break;
    case eDeliveryState_Sending:
      aDelivery = DELIVERY_SENDING;
      break;
    case eDeliveryState_Sent:
      aDelivery = DELIVERY_SENT;
      break;
    case eDeliveryState_Error:
      aDelivery = DELIVERY_ERROR;
      break;
    case eDeliveryState_NotDownloaded:
      aDelivery = DELIVERY_NOT_DOWNLOADED;
      break;
    case eDeliveryState_Unknown:
    case eDeliveryState_EndGuard:
    default:
      MOZ_NOT_REACHED("We shouldn't get any other delivery state!");
      return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

NS_IMETHODIMP
MmsMessage::GetDeliveryStatus(JSContext* aCx, JS::Value* aDeliveryStatus)
{
  
  
  
  uint32_t length = mDeliveryStatus.Length();
  if (length == 0) {
    *aDeliveryStatus = JSVAL_NULL;
    return NS_OK;
  }

  nsTArray<nsString> tempStrArray;
  for (uint32_t i = 0; i < length; ++i) {
    nsString statusStr;
    switch (mDeliveryStatus[i]) {
      case eDeliveryStatus_NotApplicable:
        statusStr = DELIVERY_STATUS_NOT_APPLICABLE;
        break;
      case eDeliveryStatus_Success:
        statusStr = DELIVERY_STATUS_SUCCESS;
        break;
      case eDeliveryStatus_Pending:
        statusStr = DELIVERY_STATUS_PENDING;
        break;
      case eDeliveryStatus_Error:
        statusStr = DELIVERY_STATUS_ERROR;
        break;
      case eDeliveryStatus_EndGuard:
      default:
        MOZ_NOT_REACHED("We shouldn't get any other delivery status!");
        return NS_ERROR_UNEXPECTED;
    }
    tempStrArray.AppendElement(statusStr);
  }

  JSObject* deliveryStatusObj = nullptr;
  nsresult rv = nsTArrayToJSArray(aCx, tempStrArray, &deliveryStatusObj);
  NS_ENSURE_SUCCESS(rv, rv);

  aDeliveryStatus->setObject(*deliveryStatusObj);
  return NS_OK;
}

NS_IMETHODIMP
MmsMessage::GetSender(nsAString& aSender)
{
  aSender = mSender;
  return NS_OK;
}

NS_IMETHODIMP
MmsMessage::GetReceivers(JSContext* aCx, JS::Value* aReceivers)
{
  uint32_t length = mReceivers.Length();
  if (length == 0) {
    return NS_ERROR_UNEXPECTED;
  }

  JSObject* reveiversObj = nullptr;
  nsresult rv = nsTArrayToJSArray(aCx, mReceivers, &reveiversObj);
  NS_ENSURE_SUCCESS(rv, rv);

  aReceivers->setObject(*reveiversObj);
  return NS_OK;
}

NS_IMETHODIMP
MmsMessage::GetTimestamp(JSContext* cx, JS::Value* aDate)
{
  JSObject *obj = JS_NewDateObjectMsec(cx, mTimestamp);
  NS_ENSURE_TRUE(obj, NS_ERROR_FAILURE);

  *aDate = OBJECT_TO_JSVAL(obj);
  return NS_OK;
}

NS_IMETHODIMP
MmsMessage::GetRead(bool* aRead)
{
  *aRead = mRead;
  return NS_OK;
}

NS_IMETHODIMP
MmsMessage::GetSubject(nsAString& aSubject)
{
  aSubject = mSubject;
  return NS_OK;
}

NS_IMETHODIMP
MmsMessage::GetSmil(nsAString& aSmil)
{
  aSmil = mSmil;
  return NS_OK;
}

NS_IMETHODIMP
MmsMessage::GetAttachments(JSContext* aCx, JS::Value* aAttachments)
{
  
  
  uint32_t length = mAttachments.Length();
  if (length == 0) {
    *aAttachments = JSVAL_NULL;
    return NS_OK;
  }

  JSObject* attachments = JS_NewArrayObject(aCx, length, nullptr);
  NS_ENSURE_TRUE(attachments, NS_ERROR_OUT_OF_MEMORY);

  for (uint32_t i = 0; i < length; ++i) {
    const MmsAttachment &attachment = mAttachments[i];

    JSObject* attachmentObj = JS_NewObject(aCx, nullptr, nullptr, nullptr);
    NS_ENSURE_TRUE(attachmentObj, NS_ERROR_OUT_OF_MEMORY);

    JS::Value tmpJsVal;
    JSString* tmpJsStr;

    
    tmpJsStr = JS_NewUCStringCopyN(aCx,
                                   attachment.id.get(),
                                   attachment.id.Length());
    NS_ENSURE_TRUE(tmpJsStr, NS_ERROR_OUT_OF_MEMORY);

    tmpJsVal.setString(tmpJsStr);
    if (!JS_DefineProperty(aCx, attachmentObj, "id", tmpJsVal,
                           NULL, NULL, JSPROP_ENUMERATE)) {
      return NS_ERROR_FAILURE;
    }

    
    tmpJsStr = JS_NewUCStringCopyN(aCx,
                                   attachment.location.get(),
                                   attachment.location.Length());
    NS_ENSURE_TRUE(tmpJsStr, NS_ERROR_OUT_OF_MEMORY);

    tmpJsVal.setString(tmpJsStr);
    if (!JS_DefineProperty(aCx, attachmentObj, "location", tmpJsVal,
                           NULL, NULL, JSPROP_ENUMERATE)) {
      return NS_ERROR_FAILURE;
    }

    
    JS::Rooted<JSObject*> global(aCx, JS_GetGlobalForScopeChain(aCx));
    nsresult rv = nsContentUtils::WrapNative(aCx,
                                             global,
                                             attachment.content,
                                             &NS_GET_IID(nsIDOMBlob),
                                             &tmpJsVal);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!JS_DefineProperty(aCx, attachmentObj, "content", tmpJsVal,
                           NULL, NULL, JSPROP_ENUMERATE)) {
      return NS_ERROR_FAILURE;
    }

    tmpJsVal = OBJECT_TO_JSVAL(attachmentObj);
    if (!JS_SetElement(aCx, attachments, i, &tmpJsVal)) {
      return NS_ERROR_FAILURE;
    }
  }

  aAttachments->setObject(*attachments);
  return NS_OK;
}

} 
} 
