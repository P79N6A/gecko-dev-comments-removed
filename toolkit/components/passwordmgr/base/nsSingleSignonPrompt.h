





































#ifndef NSSINGLESIGNONPROMPT_H_
#define NSSINGLESIGNONPROMPT_H_

#include "nsIAuthPromptWrapper.h"
#include "nsCOMPtr.h"
#include "nsIPrompt.h"
#include "nsIAuthPrompt2.h"


#define NS_SINGLE_SIGNON_PROMPT_CID \
{0x1baf3398, 0xf759, 0x4a72, {0xa2, 0x1f, 0x0a, 0xbd, 0xc9, 0xcc, 0x99, 0x60}}

class nsIDOMWindow;
class nsIPromptService2;




class nsSingleSignonPrompt : public nsIAuthPromptWrapper
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUTHPROMPT
  NS_DECL_NSIAUTHPROMPTWRAPPER

  nsSingleSignonPrompt() { }
  virtual ~nsSingleSignonPrompt() { }

protected:
  void GetLocalizedString(const nsAString& aKey, nsAString& aResult);

  nsCOMPtr<nsIPrompt> mPrompt;
};





class nsSingleSignonPrompt2 : public nsIAuthPrompt2
{
  public:
    nsSingleSignonPrompt2(nsIPromptService2* aService, nsIDOMWindow* aParent);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIAUTHPROMPT2

  private:
    ~nsSingleSignonPrompt2();

    nsCOMPtr<nsIPromptService2> mService;
    nsCOMPtr<nsIDOMWindow> mParent;
};

#endif 
