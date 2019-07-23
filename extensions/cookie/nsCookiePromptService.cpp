




































#include "nsCookiePromptService.h"
#include "nsICookie.h"
#include "nsICookieAcceptDialog.h"
#include "nsIDOMWindow.h"
#include "nsIWindowWatcher.h"
#include "nsIServiceManager.h"
#include "nsString.h"
#include "nsIDialogParamBlock.h"
#include "nsIMutableArray.h"





NS_IMPL_ISUPPORTS1(nsCookiePromptService, nsICookiePromptService)

nsCookiePromptService::nsCookiePromptService() {
}

nsCookiePromptService::~nsCookiePromptService() {
}

NS_IMETHODIMP
nsCookiePromptService::CookieDialog(nsIDOMWindow *aParent,
                                    nsICookie *aCookie,
                                    const nsACString &aHostname,
                                    PRInt32 aCookiesFromHost,
                                    PRBool aChangingCookie,
                                    PRBool *aRememberDecision,
                                    PRInt32 *aAccept)
{
  nsresult rv;

  nsCOMPtr<nsIDialogParamBlock> block = do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID,&rv);
  if (NS_FAILED(rv)) return rv;

  
  
  block->SetInt(nsICookieAcceptDialog::ACCEPT_COOKIE, 1);
  block->SetString(nsICookieAcceptDialog::HOSTNAME, NS_ConvertUTF8toUTF16(aHostname).get());
  block->SetInt(nsICookieAcceptDialog::COOKIESFROMHOST, aCookiesFromHost);
  block->SetInt(nsICookieAcceptDialog::CHANGINGCOOKIE, aChangingCookie != PR_FALSE);
  
  nsCOMPtr<nsIMutableArray> objects =
    do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = objects->AppendElement(aCookie, PR_FALSE);
  if (NS_FAILED(rv)) return rv;

  block->SetObjects(objects);

  nsCOMPtr<nsIWindowWatcher> wwatcher = do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDOMWindow> parent = aParent;
  if (!parent) {
    wwatcher->GetActiveWindow(getter_AddRefs(parent));
  }

  nsCOMPtr<nsISupports> arguments = do_QueryInterface(block);
  nsCOMPtr<nsIDOMWindow> dialog;
  rv = wwatcher->OpenWindow(parent, "chrome://cookie/content/cookieAcceptDialog.xul", "_blank",
                             "centerscreen,chrome,modal,titlebar", arguments,
                             getter_AddRefs(dialog));

  if (NS_FAILED(rv)) return rv;

  
  PRBool tempValue;
  block->GetInt(nsICookieAcceptDialog::ACCEPT_COOKIE, &tempValue);
  *aAccept = tempValue;
  
  
  block->GetInt(nsICookieAcceptDialog::REMEMBER_DECISION, &tempValue);
  *aRememberDecision = (tempValue == 1);

  return rv;
}

