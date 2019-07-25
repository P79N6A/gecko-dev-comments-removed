





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
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"

#include "mozilla/Telemetry.h"
#include "nsISecurityUITelemetry.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsSecurityWarningDialogs, nsISecurityWarningDialogs)

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

  nsCOMPtr<nsIStringBundleService> service =
           do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  rv = service->CreateBundle(STRING_BUNDLE_URL,
                             getter_AddRefs(mStringBundle));
  return rv;
}

NS_IMETHODIMP 
nsSecurityWarningDialogs::ConfirmEnteringSecure(nsIInterfaceRequestor *ctx, bool *_retval)
{
  nsresult rv;

  rv = AlertDialog(ctx, ENTER_SITE_PREF, 
                   NS_LITERAL_STRING("EnterSecureMessage").get(),
                   NS_LITERAL_STRING("EnterSecureShowAgain").get(),
                   false,
                   nsISecurityUITelemetry::WARNING_ENTERING_SECURE_SITE);

  *_retval = true;
  return rv;
}

NS_IMETHODIMP 
nsSecurityWarningDialogs::ConfirmEnteringWeak(nsIInterfaceRequestor *ctx, bool *_retval)
{
  nsresult rv;

  rv = AlertDialog(ctx, WEAK_SITE_PREF,
                   NS_LITERAL_STRING("WeakSecureMessage").get(),
                   NS_LITERAL_STRING("WeakSecureShowAgain").get(),
                   false,
                   nsISecurityUITelemetry::WARNING_ENTERING_WEAK_SITE);

  *_retval = true;
  return rv;
}

NS_IMETHODIMP 
nsSecurityWarningDialogs::ConfirmLeavingSecure(nsIInterfaceRequestor *ctx, bool *_retval)
{
  nsresult rv;

  rv = AlertDialog(ctx, LEAVE_SITE_PREF, 
                   NS_LITERAL_STRING("LeaveSecureMessage").get(),
                   NS_LITERAL_STRING("LeaveSecureShowAgain").get(),
                   false,
                   nsISecurityUITelemetry::WARNING_LEAVING_SECURE_SITE);

  *_retval = true;
  return rv;
}


NS_IMETHODIMP 
nsSecurityWarningDialogs::ConfirmMixedMode(nsIInterfaceRequestor *ctx, bool *_retval)
{
  nsresult rv;

  rv = AlertDialog(ctx, MIXEDCONTENT_PREF, 
                   NS_LITERAL_STRING("MixedContentMessage").get(),
                   NS_LITERAL_STRING("MixedContentShowAgain").get(),
                   true,
                   nsISecurityUITelemetry::WARNING_MIXED_CONTENT);

  *_retval = true;
  return rv;
}

class nsAsyncAlert : public nsRunnable
{
public:
  nsAsyncAlert(nsIPrompt*       aPrompt,
               const char*      aPrefName,
               const PRUnichar* aDialogMessageName,
               const PRUnichar* aShowAgainName,
               nsIPrefBranch*   aPrefBranch,
               nsIStringBundle* aStringBundle,
               uint32_t         aBucket)
  : mPrompt(aPrompt), mPrefName(aPrefName),
    mDialogMessageName(aDialogMessageName),
    mShowAgainName(aShowAgainName), mPrefBranch(aPrefBranch),
    mStringBundle(aStringBundle),
    mBucket(aBucket) {}
  NS_IMETHOD Run();

protected:
  nsCOMPtr<nsIPrompt>       mPrompt;
  nsCString                 mPrefName;
  nsString                  mDialogMessageName;
  nsString                  mShowAgainName;
  nsCOMPtr<nsIPrefBranch>   mPrefBranch;
  nsCOMPtr<nsIStringBundle> mStringBundle;
  uint32_t                  mBucket;
};

NS_IMETHODIMP
nsAsyncAlert::Run()
{
  nsresult rv;

  
  bool prefValue;
  rv = mPrefBranch->GetBoolPref(mPrefName.get(), &prefValue);
  if (NS_FAILED(rv)) prefValue = true;

  
  if (!prefValue) return NS_OK;

  mozilla::Telemetry::Accumulate(mozilla::Telemetry::SECURITY_UI, mBucket);
  
  
  
  

  nsAutoCString showOncePref(mPrefName);
  showOncePref += ".show_once";

  bool showOnce = false;
  mPrefBranch->GetBoolPref(showOncePref.get(), &showOnce);

  if (showOnce)
    prefValue = false;

  
  nsXPIDLString windowTitle, message, dontShowAgain;

  mStringBundle->GetStringFromName(NS_LITERAL_STRING("Title").get(),
                                   getter_Copies(windowTitle));
  mStringBundle->GetStringFromName(mDialogMessageName.get(),
                                   getter_Copies(message));
  mStringBundle->GetStringFromName(mShowAgainName.get(),
                                   getter_Copies(dontShowAgain));
  if (!windowTitle || !message || !dontShowAgain) return NS_ERROR_FAILURE;

  rv = mPrompt->AlertCheck(windowTitle, message, dontShowAgain, &prefValue);
  if (NS_FAILED(rv)) return rv;
      
  if (!prefValue) {
    mPrefBranch->SetBoolPref(mPrefName.get(), false);
  } else if (showOnce) {
    mPrefBranch->SetBoolPref(showOncePref.get(), false);
  }

  return rv;
}


nsresult
nsSecurityWarningDialogs::AlertDialog(nsIInterfaceRequestor* aCtx,
                                      const char* aPrefName,
                                      const PRUnichar* aDialogMessageName,
                                      const PRUnichar* aShowAgainName,
                                      bool aAsync,
                                      const uint32_t aBucket)
{
  
  nsCOMPtr<nsIPrompt> prompt = do_GetInterface(aCtx);
  if (!prompt) return NS_ERROR_FAILURE;

  nsRefPtr<nsAsyncAlert> alert = new nsAsyncAlert(prompt,
                                                  aPrefName,
                                                  aDialogMessageName,
                                                  aShowAgainName,
                                                  mPrefBranch,
                                                  mStringBundle,
                                                  aBucket);

  NS_ENSURE_TRUE(alert, NS_ERROR_OUT_OF_MEMORY);
  return aAsync ? NS_DispatchToCurrentThread(alert) : alert->Run();
}



NS_IMETHODIMP 
nsSecurityWarningDialogs::ConfirmPostToInsecure(nsIInterfaceRequestor *ctx, bool* _result)
{
  nsresult rv;

  
  rv = ConfirmDialog(ctx, INSECURE_SUBMIT_PREF,
                     NS_LITERAL_STRING("PostToInsecureFromInsecureMessage").get(),
                     NS_LITERAL_STRING("PostToInsecureFromInsecureShowAgain").get(),
                     nsISecurityUITelemetry::WARNING_CONFIRM_POST_TO_INSECURE_FROM_INSECURE,
                     _result);

  return rv;
}

NS_IMETHODIMP 
nsSecurityWarningDialogs::ConfirmPostToInsecureFromSecure(nsIInterfaceRequestor *ctx, bool* _result)
{
  nsresult rv;

  
  rv = ConfirmDialog(ctx, nullptr, 
                     NS_LITERAL_STRING("PostToInsecureFromSecureMessage").get(),
                     nullptr,
                     nsISecurityUITelemetry::WARNING_CONFIRM_POST_TO_INSECURE_FROM_SECURE,
                     _result);

  return rv;
}

nsresult
nsSecurityWarningDialogs::ConfirmDialog(nsIInterfaceRequestor *ctx, const char *prefName,
                            const PRUnichar *messageName, 
                            const PRUnichar *showAgainName, 
                            const uint32_t aBucket,
                            bool* _result)
{
  nsresult rv;

  
  
  bool prefValue = true;
  
  if (prefName != nullptr) {
    rv = mPrefBranch->GetBoolPref(prefName, &prefValue);
    if (NS_FAILED(rv)) prefValue = true;
  }
  
  
  if (!prefValue) {
    *_result = true;
    return NS_OK;
  }
  
  MOZ_ASSERT(NS_IsMainThread());
  mozilla::Telemetry::Accumulate(mozilla::Telemetry::SECURITY_UI, aBucket);
  
  nsAutoCString showOncePref(prefName);
  showOncePref += ".show_once";

  bool showOnce = false;
  mPrefBranch->GetBoolPref(showOncePref.get(), &showOnce);

  if (showOnce)
    prefValue = false;

  
  nsCOMPtr<nsIPrompt> prompt = do_GetInterface(ctx);
  if (!prompt) return NS_ERROR_FAILURE;

  
  nsXPIDLString windowTitle, message, alertMe, cont;

  mStringBundle->GetStringFromName(NS_LITERAL_STRING("Title").get(),
                                   getter_Copies(windowTitle));
  mStringBundle->GetStringFromName(messageName,
                                   getter_Copies(message));
  if (showAgainName != nullptr) {
    mStringBundle->GetStringFromName(showAgainName,
                                     getter_Copies(alertMe));
  }
  mStringBundle->GetStringFromName(NS_LITERAL_STRING("Continue").get(),
                                   getter_Copies(cont));
  
  if (!windowTitle || !message || !cont) return NS_ERROR_FAILURE;
      
  
  PRUnichar* msgchars = message.BeginWriting();
  
  uint32_t i = 0;
  for (i = 0; msgchars[i] != '\0'; i++) {
    if (msgchars[i] == '#') {
      msgchars[i] = '\n';
    }
  }  

  int32_t buttonPressed;

  rv  = prompt->ConfirmEx(windowTitle, 
                          message, 
                          (nsIPrompt::BUTTON_TITLE_IS_STRING * nsIPrompt::BUTTON_POS_0) +
                          (nsIPrompt::BUTTON_TITLE_CANCEL * nsIPrompt::BUTTON_POS_1),
                          cont,
                          nullptr,
                          nullptr,
                          alertMe, 
                          &prefValue, 
                          &buttonPressed);

  if (NS_FAILED(rv)) return rv;

  *_result = (buttonPressed != 1);
  if (*_result) {
  
  
  mozilla::Telemetry::Accumulate(mozilla::Telemetry::SECURITY_UI, aBucket + 1);
  }

  if (!prefValue && prefName != nullptr) {
    mPrefBranch->SetBoolPref(prefName, false);
  } else if (prefValue && showOnce) {
    mPrefBranch->SetBoolPref(showOncePref.get(), false);
  }

  return rv;
}

