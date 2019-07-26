




#include "mozilla/dom/ContentChild.h"
#include "SmsIPCService.h"
#include "nsXULAppAPI.h"
#include "jsapi.h"
#include "mozilla/dom/mobilemessage/SmsChild.h"
#include "SmsMessage.h"
#include "SmsFilter.h"
#include "SmsSegmentInfo.h"
#include "DictionaryHelpers.h"
#include "nsJSUtils.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "mozilla/dom/MobileMessageManagerBinding.h"
#include "mozilla/dom/MozMmsMessageBinding.h"
#include "mozilla/dom/BindingUtils.h"

using namespace mozilla::dom;
using namespace mozilla::dom::mobilemessage;

namespace {


PSmsChild* gSmsChild;

PSmsChild*
GetSmsChild()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!gSmsChild) {
    gSmsChild = ContentChild::GetSingleton()->SendPSmsConstructor();

    NS_WARN_IF_FALSE(gSmsChild,
                     "Calling methods on SmsIPCService during shutdown!");
  }

  return gSmsChild;
}

nsresult
SendRequest(const IPCSmsRequest& aRequest,
            nsIMobileMessageCallback* aRequestReply)
{
  PSmsChild* smsChild = GetSmsChild();
  NS_ENSURE_TRUE(smsChild, NS_ERROR_FAILURE);

  SmsRequestChild* actor = new SmsRequestChild(aRequestReply);
  smsChild->SendPSmsRequestConstructor(actor, aRequest);

  return NS_OK;
}

nsresult
SendCursorRequest(const IPCMobileMessageCursor& aRequest,
                  nsIMobileMessageCursorCallback* aRequestReply,
                  nsICursorContinueCallback** aResult)
{
  PSmsChild* smsChild = GetSmsChild();
  NS_ENSURE_TRUE(smsChild, NS_ERROR_FAILURE);

  nsRefPtr<MobileMessageCursorChild> actor =
    new MobileMessageCursorChild(aRequestReply);

  
  
  actor->AddRef();

  smsChild->SendPMobileMessageCursorConstructor(actor, aRequest);

  actor.forget(aResult);
  return NS_OK;
}
} 

NS_IMPL_ISUPPORTS3(SmsIPCService,
                   nsISmsService,
                   nsIMmsService,
                   nsIMobileMessageDatabaseService)




NS_IMETHODIMP
SmsIPCService::HasSupport(bool* aHasSupport)
{
  PSmsChild* smsChild = GetSmsChild();
  NS_ENSURE_TRUE(smsChild, NS_ERROR_FAILURE);

  smsChild->SendHasSupport(aHasSupport);

  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::GetSegmentInfoForText(const nsAString & aText,
                                     nsIDOMMozSmsSegmentInfo** aResult)
{
  PSmsChild* smsChild = GetSmsChild();
  NS_ENSURE_TRUE(smsChild, NS_ERROR_FAILURE);

  SmsSegmentInfoData data;
  bool ok = smsChild->SendGetSegmentInfoForText(nsString(aText), &data);
  NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMMozSmsSegmentInfo> info = new SmsSegmentInfo(data);
  info.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::Send(const nsAString& aNumber,
                    const nsAString& aMessage,
                    const bool aSilent,
                    nsIMobileMessageCallback* aRequest)
{
  return SendRequest(SendMessageRequest(SendSmsMessageRequest(nsString(aNumber),
                                                              nsString(aMessage),
                                                              aSilent)),
                     aRequest);
}

NS_IMETHODIMP
SmsIPCService::IsSilentNumber(const nsAString& aNumber,
                              bool*            aIsSilent)
{
  NS_ERROR("We should not be here!");
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
SmsIPCService::AddSilentNumber(const nsAString& aNumber)
{
  PSmsChild* smsChild = GetSmsChild();
  NS_ENSURE_TRUE(smsChild, NS_ERROR_FAILURE);

  smsChild->SendAddSilentNumber(nsString(aNumber));
  return NS_OK;
}

NS_IMETHODIMP
SmsIPCService::RemoveSilentNumber(const nsAString& aNumber)
{
  PSmsChild* smsChild = GetSmsChild();
  NS_ENSURE_TRUE(smsChild, NS_ERROR_FAILURE);

  smsChild->SendRemoveSilentNumber(nsString(aNumber));
  return NS_OK;
}




NS_IMETHODIMP
SmsIPCService::GetMessageMoz(int32_t aMessageId,
                             nsIMobileMessageCallback* aRequest)
{
  return SendRequest(GetMessageRequest(aMessageId), aRequest);
}

NS_IMETHODIMP
SmsIPCService::DeleteMessage(int32_t *aMessageIds, uint32_t aSize,
                             nsIMobileMessageCallback* aRequest)
{
  DeleteMessageRequest data;
  data.messageIds().AppendElements(aMessageIds, aSize);
  return SendRequest(data, aRequest);
}

NS_IMETHODIMP
SmsIPCService::CreateMessageCursor(nsIDOMMozSmsFilter* aFilter,
                                   bool aReverse,
                                   nsIMobileMessageCursorCallback* aCursorCallback,
                                   nsICursorContinueCallback** aResult)
{
  const SmsFilterData& data =
    SmsFilterData(static_cast<SmsFilter*>(aFilter)->GetData());

  return SendCursorRequest(CreateMessageCursorRequest(data, aReverse),
                           aCursorCallback, aResult);
}

NS_IMETHODIMP
SmsIPCService::MarkMessageRead(int32_t aMessageId,
                               bool aValue,
                               nsIMobileMessageCallback* aRequest)
{
  return SendRequest(MarkMessageReadRequest(aMessageId, aValue), aRequest);
}

NS_IMETHODIMP
SmsIPCService::CreateThreadCursor(nsIMobileMessageCursorCallback* aCursorCallback,
                                  nsICursorContinueCallback** aResult)
{
  return SendCursorRequest(CreateThreadCursorRequest(), aCursorCallback,
                           aResult);
}

bool
GetSendMmsMessageRequestFromParams(const JS::Value& aParam,
                                   SendMmsMessageRequest& request) {
  if (aParam.isUndefined() || aParam.isNull() || !aParam.isObject()) {
    return false;
  }

  mozilla::AutoJSContext cx;
  JS::Rooted<JS::Value> param(cx, aParam);
  RootedDictionary<MmsParameters> params(cx);
  if (!params.Init(cx, param)) {
    return false;
  }

  
  if (!params.mReceivers.WasPassed()) {
    return false;
  }
  request.receivers().AppendElements(params.mReceivers.Value());

  
  mozilla::dom::ContentChild* cc = mozilla::dom::ContentChild::GetSingleton();

  if (!params.mAttachments.WasPassed()) {
    return false;
  }

  for (uint32_t i = 0; i < params.mAttachments.Value().Length(); i++) {
    MmsAttachment& attachment = params.mAttachments.Value()[i];
    MmsAttachmentData mmsAttachment;
    mmsAttachment.id().Assign(attachment.mId);
    mmsAttachment.location().Assign(attachment.mLocation);
    mmsAttachment.contentChild() = cc->GetOrCreateActorForBlob(attachment.mContent);
    if (!mmsAttachment.contentChild()) {
      return false;
    }
    request.attachments().AppendElement(mmsAttachment);
  }

  request.smil() = params.mSmil;
  request.subject() = params.mSubject;

  return true;
}

NS_IMETHODIMP
SmsIPCService::Send(const JS::Value& aParameters,
                    nsIMobileMessageCallback *aRequest)
{
  SendMmsMessageRequest req;
  if (!GetSendMmsMessageRequestFromParams(aParameters, req)) {
    return NS_ERROR_INVALID_ARG;
  }
  return SendRequest(SendMessageRequest(req), aRequest);
}

NS_IMETHODIMP
SmsIPCService::Retrieve(int32_t aId, nsIMobileMessageCallback *aRequest)
{
  return SendRequest(RetrieveMessageRequest(aId), aRequest);
}
