





#include "Voicemail.h"

#include "mozilla/dom/MozVoicemailBinding.h"
#include "nsIDOMMozVoicemailStatus.h"
#include "nsIDOMMozVoicemailEvent.h"

#include "mozilla/Services.h"
#include "nsDOMClassInfo.h"
#include "nsServiceManagerUtils.h"
#include "GeneratedEvents.h"

#define NS_RILCONTENTHELPER_CONTRACTID "@mozilla.org/ril/content-helper;1"

using namespace mozilla::dom;

class Voicemail::Listener : public nsIVoicemailListener
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
};

NS_IMPL_ISUPPORTS1(Voicemail::Listener, nsIVoicemailListener)

Voicemail::Voicemail(nsPIDOMWindow* aWindow,
                     nsIVoicemailProvider* aProvider)
  : nsDOMEventTargetHelper(aWindow)
  , mProvider(aProvider)
{
  mListener = new Listener(this);
  DebugOnly<nsresult> rv = mProvider->RegisterVoicemailMsg(mListener);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                   "Failed registering voicemail messages with provider");
}

Voicemail::~Voicemail()
{
  MOZ_ASSERT(mProvider && mListener);

  mListener->Disconnect();
  mProvider->UnregisterVoicemailMsg(mListener);
}

JSObject*
Voicemail::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return MozVoicemailBinding::Wrap(aCx, aScope, this);
}



already_AddRefed<nsIDOMMozVoicemailStatus>
Voicemail::GetStatus(ErrorResult& aRv) const
{
  if (!mProvider) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsCOMPtr<nsIDOMMozVoicemailStatus> status;
  nsresult rv = mProvider->GetVoicemailStatus(getter_AddRefs(status));
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return nullptr;
  }

  return status.forget();
}

void
Voicemail::GetNumber(nsString& aNumber, ErrorResult& aRv) const
{
  aNumber.SetIsVoid(true);

  if (!mProvider) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  aRv = mProvider->GetVoicemailNumber(aNumber);
}

void
Voicemail::GetDisplayName(nsString& aDisplayName, ErrorResult& aRv) const
{
  aDisplayName.SetIsVoid(true);

  if (!mProvider) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  aRv = mProvider->GetVoicemailDisplayName(aDisplayName);
}



NS_IMETHODIMP
Voicemail::NotifyStatusChanged(nsIDOMMozVoicemailStatus* aStatus)
{
  nsCOMPtr<nsIDOMEvent> event;
  NS_NewDOMMozVoicemailEvent(getter_AddRefs(event), this, nullptr, nullptr);

  nsCOMPtr<nsIDOMMozVoicemailEvent> ce = do_QueryInterface(event);
  nsresult rv = ce->InitMozVoicemailEvent(NS_LITERAL_STRING("statuschanged"),
                                          false, false, aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  return DispatchTrustedEvent(ce);
}

nsresult
NS_NewVoicemail(nsPIDOMWindow* aWindow, Voicemail** aVoicemail)
{
  nsPIDOMWindow* innerWindow = aWindow->IsInnerWindow() ?
    aWindow :
    aWindow->GetCurrentInnerWindow();

  nsCOMPtr<nsIVoicemailProvider> provider =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  NS_ENSURE_STATE(provider);

  nsRefPtr<Voicemail> voicemail = new Voicemail(innerWindow, provider);
  voicemail.forget(aVoicemail);
  return NS_OK;
}
