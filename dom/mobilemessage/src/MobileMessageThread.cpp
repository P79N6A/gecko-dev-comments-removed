




#include "MobileMessageThread.h"
#include "nsIDOMClassInfo.h"
#include "jsapi.h"           
#include "jsfriendapi.h"     
#include "nsJSUtils.h"       
#include "nsContentUtils.h"  
#include "nsTArrayHelpers.h" 
#include "Constants.h"       


using namespace mozilla::dom::mobilemessage;

DOMCI_DATA(MozMobileMessageThread, mozilla::dom::MobileMessageThread)

namespace mozilla {
namespace dom {

NS_INTERFACE_MAP_BEGIN(MobileMessageThread)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozMobileMessageThread)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozMobileMessageThread)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(MobileMessageThread)
NS_IMPL_RELEASE(MobileMessageThread)

 nsresult
MobileMessageThread::Create(const uint64_t aId,
                            const JS::Value& aParticipants,
                            const JS::Value& aTimestamp,
                            const nsAString& aBody,
                            const uint64_t aUnreadCount,
                            const nsAString& aLastMessageType,
                            JSContext* aCx,
                            nsIDOMMozMobileMessageThread** aThread)
{
  *aThread = nullptr;

  
  
  ThreadData data;
  data.id() = aId;
  data.body().Assign(aBody);
  data.unreadCount() = aUnreadCount;

  
  {
    if (!aParticipants.isObject()) {
      return NS_ERROR_INVALID_ARG;
    }

    JS::Rooted<JSObject*> obj(aCx, &aParticipants.toObject());
    if (!JS_IsArrayObject(aCx, obj)) {
      return NS_ERROR_INVALID_ARG;
    }

    uint32_t length;
    JS_ALWAYS_TRUE(JS_GetArrayLength(aCx, obj, &length));
    NS_ENSURE_TRUE(length, NS_ERROR_INVALID_ARG);

    for (uint32_t i = 0; i < length; ++i) {
      JS::Rooted<JS::Value> val(aCx);

      if (!JS_GetElement(aCx, obj, i, val.address()) || !val.isString()) {
        return NS_ERROR_INVALID_ARG;
      }

      nsDependentJSString str;
      str.init(aCx, val.toString());
      data.participants().AppendElement(str);
    }
  }

  
  if (aTimestamp.isObject()) {
    JS::Rooted<JSObject*> obj(aCx, &aTimestamp.toObject());
    if (!JS_ObjectIsDate(aCx, obj)) {
      return NS_ERROR_INVALID_ARG;
    }
    data.timestamp() = js_DateGetMsecSinceEpoch(obj);
  } else {
    if (!aTimestamp.isNumber()) {
      return NS_ERROR_INVALID_ARG;
    }
    double number = aTimestamp.toNumber();
    if (static_cast<uint64_t>(number) != number) {
      return NS_ERROR_INVALID_ARG;
    }
    data.timestamp() = static_cast<uint64_t>(number);
  }

  
  {
    MessageType lastMessageType;
    if (aLastMessageType.Equals(MESSAGE_TYPE_SMS)) {
      lastMessageType = eMessageType_SMS;
    } else if (aLastMessageType.Equals(MESSAGE_TYPE_MMS)) {
      lastMessageType = eMessageType_MMS;
    } else {
      return NS_ERROR_INVALID_ARG;
    }
    data.lastMessageType() = lastMessageType;
  }

  nsCOMPtr<nsIDOMMozMobileMessageThread> thread = new MobileMessageThread(data);
  thread.forget(aThread);
  return NS_OK;
}

MobileMessageThread::MobileMessageThread(const uint64_t aId,
                                         const nsTArray<nsString>& aParticipants,
                                         const uint64_t aTimestamp,
                                         const nsString& aBody,
                                         const uint64_t aUnreadCount,
                                         MessageType aLastMessageType)
  : mData(aId, aParticipants, aTimestamp, aBody, aUnreadCount, aLastMessageType)
{
  MOZ_ASSERT(aParticipants.Length());
}

MobileMessageThread::MobileMessageThread(const ThreadData& aData)
  : mData(aData)
{
  MOZ_ASSERT(aData.participants().Length());
}

NS_IMETHODIMP
MobileMessageThread::GetId(uint64_t* aId)
{
  *aId = mData.id();
  return NS_OK;
}

NS_IMETHODIMP
MobileMessageThread::GetBody(nsAString& aBody)
{
  aBody = mData.body();
  return NS_OK;
}

NS_IMETHODIMP
MobileMessageThread::GetUnreadCount(uint64_t* aUnreadCount)
{
  *aUnreadCount = mData.unreadCount();
  return NS_OK;
}

NS_IMETHODIMP
MobileMessageThread::GetParticipants(JSContext* aCx,
                                     JS::Value* aParticipants)
{
  JS::Rooted<JSObject*> obj(aCx);

  nsresult rv = nsTArrayToJSArray(aCx, mData.participants(), obj.address());
  NS_ENSURE_SUCCESS(rv, rv);

  aParticipants->setObject(*obj);
  return NS_OK;
}

NS_IMETHODIMP
MobileMessageThread::GetTimestamp(JSContext* aCx,
                                  JS::Value* aDate)
{
  JSObject *obj = JS_NewDateObjectMsec(aCx, mData.timestamp());
  NS_ENSURE_TRUE(obj, NS_ERROR_FAILURE);

  *aDate = OBJECT_TO_JSVAL(obj);
  return NS_OK;
}

NS_IMETHODIMP
MobileMessageThread::GetLastMessageType(nsAString& aLastMessageType)
{
  switch (mData.lastMessageType()) {
    case eMessageType_SMS:
      aLastMessageType = MESSAGE_TYPE_SMS;
      break;
    case eMessageType_MMS:
      aLastMessageType = MESSAGE_TYPE_MMS;
      break;
    case eMessageType_EndGuard:
    default:
      MOZ_NOT_REACHED("We shouldn't get any other message type!");
      return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

} 
} 
