





#ifndef mozilla_dom_voicemail_voicemail_h__
#define mozilla_dom_voicemail_voicemail_h__

#include "nsDOMEvent.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMMozVoicemail.h"
#include "nsIVoicemailProvider.h"

class nsPIDOMWindow;
class nsIDOMMozVoicemailStatus;

namespace mozilla {
namespace dom {

class Voicemail : public nsDOMEventTargetHelper,
                  public nsIDOMMozVoicemail
{
  






  class Listener;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMMOZVOICEMAIL
  NS_DECL_NSIVOICEMAILLISTENER

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  Voicemail(nsPIDOMWindow* aWindow, nsIVoicemailProvider* aProvider);
  virtual ~Voicemail();

private:
  nsCOMPtr<nsIVoicemailProvider> mProvider;
  nsRefPtr<Listener> mListener;
};

} 
} 

nsresult
NS_NewVoicemail(nsPIDOMWindow* aWindow, nsIDOMMozVoicemail** aVoicemail);

#endif 
