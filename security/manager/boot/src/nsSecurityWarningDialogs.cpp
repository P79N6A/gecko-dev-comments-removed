







































#include "nsSecurityWarningDialogs.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include "nsIPrompt.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsSecurityWarningDialogs, nsISecurityWarningDialogs)

static NS_DEFINE_CID(kCStringBundleServiceCID,  NS_STRINGBUNDLESERVICE_CID);

#define STRING_BUNDLE_URL    "chrome://pipnss/locale/security.properties"

#define ENTER_SITE_PREF      "security.warn_entering_secure"
#define WEAK_SITE_PREF       "security.warn_entering_weak"
#define LEAVE_SITE_PREF      "security.warn_leaving_secure"
#define MIXEDCONTENT_PREF    "security.warn_viewing_mixed"
#define INSECURE_SUBMIT_PREF "security.warn_submit_insecure"

nsSecurityWarningDialogs::nsSecurityWarningDialogs()
{
}

nsSecurityWarningDialogs::~nsSecurityWarningDialogs()
{
}

nsresult
nsSecurityWarningDialogs::Init()
{
  nsresult rv;

  mPrefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIStringBundleService> service = do_GetService(kCStringBundleServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  rv = service->CreateBundle(STRING_BUNDLE_URL,
                             getter_AddRefs(mStringBundle));
  return rv;
}

NS_IMETHODIMP 
nsSecurityWarningDialogs::ConfirmEnteringSecure(nsIInterfaceRequestor *ctx, PRBool *_retval)
{
  nsresult rv;

  rv = AlertDialog(ctx, ENTER_SITE_PREF, 
                   NS_LITERAL_STRING("EnterSecureMessage").get(),
                   NS_LITERAL_STRING("EnterSecureShowAgain").get());

  *_retval = PR_TRUE;
  return rv;
}

NS_IMETHODIMP 
nsSecurityWarningDialogs::ConfirmEnteringWeak(nsIInterfaceRequestor *ctx, PRBool *_retval)
{
  nsresult rv;

  rv = AlertDialog(ctx, WEAK_SITE_PREF,
                   NS_LITERAL_STRING("WeakSecureMessage").get(),
                   NS_LITERAL_STRING("WeakSecureShowAgain").get());

  *_retval = PR_TRUE;
  return rv;
}

NS_IMETHODIMP 
nsSecurityWarningDialogs::ConfirmLeavingSecure(nsIInterfaceRequestor *ctx, PRBool *_retval)
{
  nsresult rv;

  rv = AlertDialog(ctx, LEAVE_SITE_PREF, 
                   NS_LITERAL_STRING("LeaveSecureMessage").get(),
                   NS_LITERAL_STRING("LeaveSecureShowAgain").get());

  *_retval = PR_TRUE;
  return rv;
}


NS_IMETHODIMP 
nsSecurityWarningDialogs::ConfirmMixedMode(nsIInterfaceRequestor *ctx, PRBool *_retval)
{
  nsresult rv;

  rv = AlertDialog(ctx, MIXEDCONTENT_PREF, 
                   NS_LITERAL_STRING("MixedContentMessage").get(),
                   NS_LITERAL_STRING("MixedContentShowAgain").get());

  *_retval = PR_TRUE;
  return rv;
}


nsresult
nsSecurityWarningDialogs::AlertDialog(nsIInterfaceRequestor *ctx, const char *prefName,
                          const PRUnichar *dialogMessageName,
                          const PRUnichar *showAgainName)
{
  nsresult rv;

  
  PRBool prefValue;
  rv = mPrefBranch->GetBoolPref(prefName, &prefValue);
  if (NS_FAILED(rv)) prefValue = PR_TRUE;

  
  if (!prefValue) return NS_OK;

  
  
  
  

  nsCAutoString showOncePref(prefName);
  showOncePref += ".show_once";

  PRBool showOnce = PR_FALSE;
  mPrefBranch->GetBoolPref(showOncePref.get(), &showOnce);

  if (showOnce)
    prefValue = PR_FALSE;

  
  nsCOMPtr<nsIPrompt> prompt = do_GetInterface(ctx);
  if (!prompt) return NS_ERROR_FAILURE;

  
  nsXPIDLString windowTitle, message, dontShowAgain;

  mStringBundle->GetStringFromName(NS_LITERAL_STRING("Title").get(),
                                   getter_Copies(windowTitle));
  mStringBundle->GetStringFromName(dialogMessageName,
                                   getter_Copies(message));
  mStringBundle->GetStringFromName(showAgainName,
                                   getter_Copies(dontShowAgain));
  if (!windowTitle || !message || !dontShowAgain) return NS_ERROR_FAILURE;

  rv = prompt->AlertCheck(windowTitle, message, dontShowAgain, &prefValue);
  if (NS_FAILED(rv)) return rv;
      
  if (!prefValue) {
    mPrefBranch->SetBoolPref(prefName, PR_FALSE);
  } else if (showOnce) {
    mPrefBranch->SetBoolPref(showOncePref.get(), PR_FALSE);
  }

  return rv;
}

NS_IMETHODIMP 
nsSecurityWarningDialogs::ConfirmPostToInsecure(nsIInterfaceRequestor *ctx, PRBool* _result)
{
  nsresult rv;

  rv = ConfirmDialog(ctx, INSECURE_SUBMIT_PREF,
                     NS_LITERAL_STRING("PostToInsecureFromInsecureMessage").get(),
                     NS_LITERAL_STRING("PostToInsecureFromInsecureShowAgain").get(),
                     _result);

  return rv;
}

NS_IMETHODIMP 
nsSecurityWarningDialogs::ConfirmPostToInsecureFromSecure(nsIInterfaceRequestor *ctx, PRBool* _result)
{
  nsresult rv;

  rv = ConfirmDialog(ctx, nsnull, 
                     NS_LITERAL_STRING("PostToInsecureFromSecureMessage").get(),
                     nsnull, 
                     _result);

  return rv;
}

nsresult
nsSecurityWarningDialogs::ConfirmDialog(nsIInterfaceRequestor *ctx, const char *prefName,
                            const PRUnichar *messageName, 
                            const PRUnichar *showAgainName, 
                            PRBool* _result)
{
  nsresult rv;

  
  
  PRBool prefValue = PR_TRUE;
  
  if (prefName != nsnull) {
    rv = mPrefBranch->GetBoolPref(prefName, &prefValue);
    if (NS_FAILED(rv)) prefValue = PR_TRUE;
  }
  
  
  if (!prefValue) {
    *_result = PR_TRUE;
    return NS_OK;
  }
  
  
  nsCAutoString showOncePref(prefName);
  showOncePref += ".show_once";

  PRBool showOnce = PR_FALSE;
  mPrefBranch->GetBoolPref(showOncePref.get(), &showOnce);

  if (showOnce)
    prefValue = PR_FALSE;

  
  nsCOMPtr<nsIPrompt> prompt = do_GetInterface(ctx);
  if (!prompt) return NS_ERROR_FAILURE;

  
  nsXPIDLString windowTitle, message, alertMe, cont;

  mStringBundle->GetStringFromName(NS_LITERAL_STRING("Title").get(),
                                   getter_Copies(windowTitle));
  mStringBundle->GetStringFromName(messageName,
                                   getter_Copies(message));
  if (showAgainName != nsnull) {
    mStringBundle->GetStringFromName(showAgainName,
                                     getter_Copies(alertMe));
  }
  mStringBundle->GetStringFromName(NS_LITERAL_STRING("Continue").get(),
                                   getter_Copies(cont));
  
  if (!windowTitle || !message || !cont) return NS_ERROR_FAILURE;
      
  
  PRUnichar* msgchars = message.BeginWriting();
  
  PRUint32 i = 0;
  for (i = 0; msgchars[i] != '\0'; i++) {
    if (msgchars[i] == '#') {
      msgchars[i] = '\n';
    }
  }  

  PRInt32 buttonPressed;

  rv  = prompt->ConfirmEx(windowTitle, 
                          message, 
                          (nsIPrompt::BUTTON_TITLE_IS_STRING * nsIPrompt::BUTTON_POS_0) +
                          (nsIPrompt::BUTTON_TITLE_CANCEL * nsIPrompt::BUTTON_POS_1),
                          cont,
                          nsnull,
                          nsnull,
                          alertMe, 
                          &prefValue, 
                          &buttonPressed);

  if (NS_FAILED(rv)) return rv;

  *_result = (buttonPressed != 1);

  if (!prefValue && prefName != nsnull) {
    mPrefBranch->SetBoolPref(prefName, PR_FALSE);
  } else if (prefValue && showOnce) {
    mPrefBranch->SetBoolPref(showOncePref.get(), PR_FALSE);
  }

  return rv;
}

