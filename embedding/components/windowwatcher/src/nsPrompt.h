




































#include "nsCOMPtr.h"
#include "nsIDOMWindow.h"
#include "nsIPrompt.h"
#include "nsIAuthPrompt.h"
#include "nsIAuthPrompt2.h"
#include "nsIPromptService.h"
#include "nsIPromptService2.h"

class nsPrompt : public nsIPrompt,
                 public nsIAuthPrompt,
                 public nsIAuthPrompt2 {

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROMPT
  NS_DECL_NSIAUTHPROMPT
  NS_DECL_NSIAUTHPROMPT2

  nsPrompt(nsIDOMWindow *window);
  virtual ~nsPrompt() {}

  nsresult Init();

  




  static nsresult PromptPasswordAdapter(nsIPromptService* aService,
                                        nsIDOMWindow* aParent,
                                        nsIChannel* aChannel,
                                        PRUint32 aLevel,
                                        nsIAuthInformation* aAuthInfo,
                                        const PRUnichar* aCheckLabel,
                                        PRBool* aCheckValue,
                                        PRBool* retval);

protected:
  nsCOMPtr<nsIDOMWindow>        mParent;
  nsCOMPtr<nsIPromptService>    mPromptService;
  
  nsCOMPtr<nsIPromptService2>   mPromptService2;
};





class AuthPromptWrapper : public nsIAuthPrompt2
{
  public:
    AuthPromptWrapper(nsIAuthPrompt* aAuthPrompt) :
      mAuthPrompt(aAuthPrompt) {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIAUTHPROMPT2

  private:
    ~AuthPromptWrapper() {}

    nsCOMPtr<nsIAuthPrompt> mAuthPrompt;
};

nsresult
NS_NewPrompter(nsIPrompt **result, nsIDOMWindow *aParent);

nsresult
NS_NewAuthPrompter(nsIAuthPrompt **result, nsIDOMWindow *aParent);

nsresult
NS_NewAuthPrompter2(nsIAuthPrompt2 **result, nsIDOMWindow *aParent);
