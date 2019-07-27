





#ifndef mozilla_dom_voicemail_voicemail_h__
#define mozilla_dom_voicemail_voicemail_h__

#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/ErrorResult.h"
#include "nsIVoicemailProvider.h"

class JSObject;
struct JSContext;

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class MozVoicemailStatus;

class Voicemail MOZ_FINAL : public DOMEventTargetHelper,
                            private nsIVoicemailListener
{
  






  class Listener;

  virtual ~Voicemail();

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIVOICEMAILLISTENER

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(DOMEventTargetHelper)

  Voicemail(nsPIDOMWindow* aWindow, nsIVoicemailProvider* aProvider);

  nsPIDOMWindow*
  GetParentObject() const
  {
    return GetOwner();
  }

  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  already_AddRefed<MozVoicemailStatus>
  GetStatus(const Optional<uint32_t>& aServiceId, ErrorResult& aRv) const;

  void
  GetNumber(const Optional<uint32_t>& aServiceId, nsString& aNumber,
            ErrorResult& aRv) const;

  void
  GetDisplayName(const Optional<uint32_t>& aServiceId, nsString& aDisplayName,
                 ErrorResult& aRv) const;

  IMPL_EVENT_HANDLER(statuschanged)

private:
  nsCOMPtr<nsIVoicemailProvider> mProvider;
  nsRefPtr<Listener> mListener;

  bool
  IsValidServiceId(uint32_t aServiceId) const;

  bool
  PassedOrDefaultServiceId(const Optional<uint32_t>& aServiceId,
                           uint32_t& aResult) const;
};

} 
} 

nsresult
NS_NewVoicemail(nsPIDOMWindow* aWindow,
                mozilla::dom::Voicemail** aVoicemail);

#endif 
