





#include "Voicemail.h"

#include "mozilla/dom/MozVoicemailBinding.h"
#include "mozilla/dom/MozVoicemailEvent.h"
#include "mozilla/dom/MozVoicemailStatusBinding.h"

#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfo.h"
#include "nsServiceManagerUtils.h"

#define NS_RILCONTENTHELPER_CONTRACTID "@mozilla.org/ril/content-helper;1"
const char* kPrefRilNumRadioInterfaces = "ril.numRadioInterfaces";

using namespace mozilla::dom;

class Voicemail::Listener MOZ_FINAL : public nsIVoicemailListener
{
  Voicemail* mVoicemail;

public:
  NS_DECL_ISUPPORTS
  NS_FORWARD_SAFE_NSIVOICEMAILLISTENER(mVoicemail)

  Listener(Voicemail* aVoicemail)
    : mVoicemail(aVoicemail)
  {
    MOZ_ASSERT(mVoicemail);
  }

  void Disconnect()
  {
    MOZ_ASSERT(mVoicemail);
    mVoicemail = nullptr;
  }

private:
  ~Listener()
  {
    MOZ_ASSERT(!mVoicemail);
  }
};

NS_IMPL_ISUPPORTS(Voicemail::Listener, nsIVoicemailListener)

Voicemail::Voicemail(nsPIDOMWindow* aWindow,
                     nsIVoicemailService* aService)
  : DOMEventTargetHelper(aWindow)
  , mService(aService)
{
  mListener = new Listener(this);
  DebugOnly<nsresult> rv = mService->RegisterVoicemailMsg(mListener);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                   "Failed registering voicemail messages with service");
}

Voicemail::~Voicemail()
{
  MOZ_ASSERT(mService && mListener);

  mListener->Disconnect();
  mService->UnregisterVoicemailMsg(mListener);
}

NS_IMPL_ISUPPORTS_INHERITED0(Voicemail, DOMEventTargetHelper)

JSObject*
Voicemail::WrapObject(JSContext* aCx)
{
  return MozVoicemailBinding::Wrap(aCx, this);
}

bool
Voicemail::IsValidServiceId(uint32_t aServiceId) const
{
  uint32_t numClients = mozilla::Preferences::GetUint(kPrefRilNumRadioInterfaces, 1);

  return aServiceId < numClients;
}

bool
Voicemail::PassedOrDefaultServiceId(const Optional<uint32_t>& aServiceId,
                                    uint32_t& aResult) const
{
  if (aServiceId.WasPassed()) {
    if (!IsValidServiceId(aServiceId.Value())) {
      return false;
    }
    aResult = aServiceId.Value();
  } else {
    mService->GetVoicemailDefaultServiceId(&aResult);
  }

  return true;
}



already_AddRefed<MozVoicemailStatus>
Voicemail::GetStatus(const Optional<uint32_t>& aServiceId,
                     ErrorResult& aRv) const
{
  if (!mService) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  uint32_t id = 0;
  if (!PassedOrDefaultServiceId(aServiceId, id)) {
    aRv.Throw(NS_ERROR_INVALID_ARG);
    return nullptr;
  }
  JSContext *cx = nsContentUtils::GetCurrentJSContext();
  JS::Rooted<JS::Value> status(cx);
  nsresult rv = mService->GetVoicemailStatus(id, &status);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return nullptr;
  }
  if (!status.isObject()) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }
  JS::Rooted<JSObject*> statusObj(cx, &status.toObject());
  nsRefPtr<MozVoicemailStatus> res = new MozVoicemailStatus(statusObj, GetParentObject());
  return res.forget();
}

void
Voicemail::GetNumber(const Optional<uint32_t>& aServiceId, nsString& aNumber,
                     ErrorResult& aRv) const
{
  aNumber.SetIsVoid(true);

  if (!mService) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  uint32_t id = 0;
  if (!PassedOrDefaultServiceId(aServiceId, id)) {
    aRv.Throw(NS_ERROR_INVALID_ARG);
    return;
  }

  aRv = mService->GetVoicemailNumber(id, aNumber);
}

void
Voicemail::GetDisplayName(const Optional<uint32_t>& aServiceId, nsString& aDisplayName,
                          ErrorResult& aRv) const
{
  aDisplayName.SetIsVoid(true);

  if (!mService) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  uint32_t id = 0;
  if (!PassedOrDefaultServiceId(aServiceId, id)) {
    aRv.Throw(NS_ERROR_INVALID_ARG);
    return;
  }

  aRv = mService->GetVoicemailDisplayName(id, aDisplayName);
}



NS_IMETHODIMP
Voicemail::NotifyStatusChanged(JS::HandleValue aStatus)
{
  MozVoicemailEventInit init;
  init.mBubbles = false;
  init.mCancelable = false;
  if (aStatus.isObject()) {
    JSContext *cx = nsContentUtils::GetCurrentJSContext();
    JS::Rooted<JSObject*> statusObj(cx, &aStatus.toObject());
    init.mStatus = new MozVoicemailStatus(statusObj, GetParentObject());
  }

  nsRefPtr<MozVoicemailEvent> event =
    MozVoicemailEvent::Constructor(this, NS_LITERAL_STRING("statuschanged"), init);
  return DispatchTrustedEvent(event);
}

nsresult
NS_NewVoicemail(nsPIDOMWindow* aWindow, Voicemail** aVoicemail)
{
  nsPIDOMWindow* innerWindow = aWindow->IsInnerWindow() ?
    aWindow :
    aWindow->GetCurrentInnerWindow();

  nsCOMPtr<nsIVoicemailService> service =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_STATE(service);

  nsRefPtr<Voicemail> voicemail = new Voicemail(innerWindow, service);
  voicemail.forget(aVoicemail);
  return NS_OK;
}
