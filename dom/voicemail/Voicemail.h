





#ifndef mozilla_dom_voicemail_voicemail_h__
#define mozilla_dom_voicemail_voicemail_h__

#include "nsDOMEvent.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMMozVoicemail.h"
#include "nsIRadioInterfaceLayer.h"

class nsPIDOMWindow;
class nsIRILContentHelper;
class nsIDOMMozVoicemailStatus;

namespace mozilla {
namespace dom {

class Voicemail : public nsDOMEventTargetHelper,
                  public nsIDOMMozVoicemail
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMMOZVOICEMAIL
  NS_DECL_NSIRILVOICEMAILCALLBACK

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  Voicemail(nsPIDOMWindow* aWindow, nsIRILContentHelper* aRIL);
  virtual ~Voicemail();

private:
  nsCOMPtr<nsIRILContentHelper> mRIL;
  nsCOMPtr<nsIRILVoicemailCallback> mRILVoicemailCallback;

  class RILVoicemailCallback : public nsIRILVoicemailCallback
  {
    Voicemail* mVoicemail;

  public:
    NS_DECL_ISUPPORTS
    NS_FORWARD_NSIRILVOICEMAILCALLBACK(mVoicemail->)

    RILVoicemailCallback(Voicemail* aVoicemail)
    : mVoicemail(aVoicemail)
    {
      NS_ASSERTION(mVoicemail, "Null pointer!");
    }
  };
};

} 
} 

nsresult
NS_NewVoicemail(nsPIDOMWindow* aWindow, nsIDOMMozVoicemail** aVoicemail);

#endif 
