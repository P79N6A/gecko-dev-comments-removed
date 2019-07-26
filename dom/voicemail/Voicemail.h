





#ifndef mozilla_dom_voicemail_voicemail_h__
#define mozilla_dom_voicemail_voicemail_h__

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsDOMEvent.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIVoicemailProvider.h"

class JSObject;
struct JSContext;

class nsPIDOMWindow;
class nsIDOMMozVoicemailStatus;

namespace mozilla {
namespace dom {

class Voicemail MOZ_FINAL : public nsDOMEventTargetHelper
{
  






  class Listener;

public:
  NS_DECL_NSIVOICEMAILLISTENER

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper)

  Voicemail(nsPIDOMWindow* aWindow, nsIVoicemailProvider* aProvider);

  virtual ~Voicemail();

  nsPIDOMWindow*
  GetParentObject() const
  {
    return GetOwner();
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  already_AddRefed<nsIDOMMozVoicemailStatus>
  GetStatus(ErrorResult& aRv) const;

  void
  GetNumber(nsString& aNumber, ErrorResult& aRv) const;

  void
  GetDisplayName(nsString& aDisplayName, ErrorResult& aRv) const;

  IMPL_EVENT_HANDLER(statuschanged)

private:
  nsCOMPtr<nsIVoicemailProvider> mProvider;
  nsRefPtr<Listener> mListener;
};

} 
} 

nsresult
NS_NewVoicemail(nsPIDOMWindow* aWindow,
                mozilla::dom::Voicemail** aVoicemail);

#endif 
