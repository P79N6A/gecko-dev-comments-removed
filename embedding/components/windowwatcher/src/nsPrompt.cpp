






































#include "nsIServiceManager.h"
#include "nsIAuthPromptWrapper.h"
#include "nsIAuthInformation.h"
#include "nsPrompt.h"
#include "nsReadableUtils.h"
#include "nsDependentString.h"
#include "nsIStringBundle.h"
#include "nsIChannel.h"
#include "nsIURI.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsEmbedCID.h"
#include "nsNetCID.h"
#include "nsPIDOMWindow.h"
#include "nsIPromptFactory.h"
#include "nsIProxiedChannel.h"
#include "nsIProxyInfo.h"
#include "nsIIDNService.h"
#include "nsNetUtil.h"
#include "nsPromptUtils.h"

nsresult
NS_NewPrompter(nsIPrompt **result, nsIDOMWindow *aParent)
{
  nsresult rv;
  *result = 0;

  nsPrompt *prompter = new nsPrompt(aParent);
  if (!prompter)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(prompter);
  rv = prompter->Init();
  if (NS_FAILED(rv)) {
    NS_RELEASE(prompter);
    return rv;
  }

  *result = prompter;
  return NS_OK;
}

nsresult
NS_NewAuthPrompter(nsIAuthPrompt **result, nsIDOMWindow *aParent)
{

  nsresult rv;
  *result = 0;

  nsPrompt *prompter = new nsPrompt(aParent);
  if (!prompter)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(prompter);
  rv = prompter->Init();
  if (NS_FAILED(rv)) {
    NS_RELEASE(prompter);
    return rv;
  }

  *result = prompter;
  
  
  nsCOMPtr<nsIAuthPromptWrapper> siPrompt =
    do_CreateInstance("@mozilla.org/wallet/single-sign-on-prompt;1");
  if (siPrompt) {
    
    rv = siPrompt->SetPromptDialogs(prompter);
    if (NS_SUCCEEDED(rv)) {
      *result = siPrompt;
      NS_RELEASE(prompter); 
      NS_ADDREF(*result);
    }
  }
  return NS_OK;
}

nsresult
NS_NewAuthPrompter2(nsIAuthPrompt2 **result, nsIDOMWindow *aParent)
{
  nsCOMPtr<nsIPromptFactory> factory =
    do_GetService(NS_PWMGR_AUTHPROMPTFACTORY);
  if (factory) {
    
    return factory->GetPrompt(aParent,
                              NS_GET_IID(nsIAuthPrompt2),
                              reinterpret_cast<void**>(result));
  }

  nsresult rv;
  *result = 0;

  nsPrompt *prompter = new nsPrompt(aParent);
  if (!prompter)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(prompter);
  rv = prompter->Init();
  if (NS_FAILED(rv)) {
    NS_RELEASE(prompter);
    return rv;
  }

  *result = prompter;
  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsPrompt, nsIPrompt, nsIAuthPrompt, nsIAuthPrompt2)

nsPrompt::nsPrompt(nsIDOMWindow *aParent)
  : mParent(aParent)
{
#ifdef DEBUG
  {
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(aParent));

    NS_ASSERTION(!win || win->IsOuterWindow(),
                 "Inner window passed as nsPrompt parent!");
  }
#endif
}

nsresult
nsPrompt::Init()
{
  mPromptService = do_GetService(NS_PROMPTSERVICE_CONTRACTID);
  mPromptService2 = do_QueryInterface(mPromptService);
  
  
  return mPromptService ? NS_OK : NS_ERROR_FAILURE;
}





class nsAutoWindowStateHelper
{
public:
  nsAutoWindowStateHelper(nsIDOMWindow *aWindow);
  ~nsAutoWindowStateHelper();

  PRBool DefaultEnabled()
  {
    return mDefaultEnabled;
  }

protected:
  PRBool DispatchCustomEvent(const char *aEventName);

  nsIDOMWindow *mWindow;
  PRBool mDefaultEnabled;
};

nsAutoWindowStateHelper::nsAutoWindowStateHelper(nsIDOMWindow *aWindow)
  : mWindow(aWindow),
    mDefaultEnabled(DispatchCustomEvent("DOMWillOpenModalDialog"))
{
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aWindow));

  if (window) {
    window->EnterModalState();
  }
}

nsAutoWindowStateHelper::~nsAutoWindowStateHelper()
{
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(mWindow));

  if (window) {
    window->LeaveModalState();
  }

  if (mDefaultEnabled) {
    DispatchCustomEvent("DOMModalDialogClosed");
  }
}

PRBool
nsAutoWindowStateHelper::DispatchCustomEvent(const char *aEventName)
{
  if (!mWindow) {
    return PR_TRUE;
  }

#ifdef DEBUG
  {
    nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(mWindow));

    NS_ASSERTION(window->GetExtantDocument() != nsnull,
                 "nsPrompt used too early on window object!");
  }
#endif

  nsCOMPtr<nsIDOMDocument> domdoc;
  mWindow->GetDocument(getter_AddRefs(domdoc));

  nsCOMPtr<nsIDOMDocumentEvent> docevent(do_QueryInterface(domdoc));
  nsCOMPtr<nsIDOMEvent> event;

  PRBool defaultActionEnabled = PR_TRUE;

  if (docevent) {
    docevent->CreateEvent(NS_LITERAL_STRING("Events"), getter_AddRefs(event));

    nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(event));
    if (privateEvent) {
      event->InitEvent(NS_ConvertASCIItoUTF16(aEventName), PR_TRUE, PR_TRUE);

      privateEvent->SetTrusted(PR_TRUE);

      nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(mWindow));

      target->DispatchEvent(event, &defaultActionEnabled);
    }
  }

  return defaultActionEnabled;
}


NS_IMETHODIMP
nsPrompt::Alert(const PRUnichar* dialogTitle, 
                const PRUnichar* text)
{
  nsAutoWindowStateHelper windowStateHelper(mParent);

  if (!windowStateHelper.DefaultEnabled()) {
    return NS_OK;
  }

  return mPromptService->Alert(mParent, dialogTitle, text);
}

NS_IMETHODIMP
nsPrompt::AlertCheck(const PRUnichar* dialogTitle, 
                     const PRUnichar* text,
                     const PRUnichar* checkMsg,
                     PRBool *checkValue)
{
  nsAutoWindowStateHelper windowStateHelper(mParent);

  if (!windowStateHelper.DefaultEnabled()) {
    
    return NS_OK;
  }

  return mPromptService->AlertCheck(mParent, dialogTitle, text, checkMsg,
                                    checkValue);
}

NS_IMETHODIMP
nsPrompt::Confirm(const PRUnichar* dialogTitle, 
                  const PRUnichar* text,
                  PRBool *_retval)
{
  nsAutoWindowStateHelper windowStateHelper(mParent);

  if (!windowStateHelper.DefaultEnabled()) {
    
    *_retval = PR_FALSE;
    return NS_OK;
  }

  return mPromptService->Confirm(mParent, dialogTitle, text, _retval);
}

NS_IMETHODIMP
nsPrompt::ConfirmCheck(const PRUnichar* dialogTitle, 
                       const PRUnichar* text,
                       const PRUnichar* checkMsg,
                       PRBool *checkValue,
                       PRBool *_retval)
{
  nsAutoWindowStateHelper windowStateHelper(mParent);

  if (!windowStateHelper.DefaultEnabled()) {
    
    *_retval = PR_FALSE;
    return NS_OK;
  }

  return mPromptService->ConfirmCheck(mParent, dialogTitle, text, checkMsg,
                                      checkValue, _retval);
}

NS_IMETHODIMP
nsPrompt::ConfirmEx(const PRUnichar *dialogTitle,
                    const PRUnichar *text,
                    PRUint32 buttonFlags,
                    const PRUnichar *button0Title,
                    const PRUnichar *button1Title,
                    const PRUnichar *button2Title,
                    const PRUnichar *checkMsg,
                    PRBool *checkValue,
                    PRInt32 *buttonPressed)
{
  nsAutoWindowStateHelper windowStateHelper(mParent);

  if (!windowStateHelper.DefaultEnabled()) {
    
    
    
    *buttonPressed = 1;
    return NS_OK;
  }

  return mPromptService->ConfirmEx(mParent, dialogTitle, text, buttonFlags,
                                   button0Title, button1Title, button2Title,
                                   checkMsg, checkValue, buttonPressed);
}

NS_IMETHODIMP
nsPrompt::Prompt(const PRUnichar *dialogTitle,
                 const PRUnichar *text,
                 PRUnichar **answer,
                 const PRUnichar *checkMsg,
                 PRBool *checkValue,
                 PRBool *_retval)
{
  nsAutoWindowStateHelper windowStateHelper(mParent);

  if (!windowStateHelper.DefaultEnabled()) {
    
    
    *_retval = PR_FALSE;
    return NS_OK;
  }

  return mPromptService->Prompt(mParent, dialogTitle, text, answer, checkMsg,
                                checkValue, _retval);
}

NS_IMETHODIMP
nsPrompt::PromptUsernameAndPassword(const PRUnichar *dialogTitle,
                                    const PRUnichar *text,
                                    PRUnichar **username,
                                    PRUnichar **password,
                                    const PRUnichar *checkMsg,
                                    PRBool *checkValue,
                                    PRBool *_retval)
{
  nsAutoWindowStateHelper windowStateHelper(mParent);

  if (!windowStateHelper.DefaultEnabled()) {
    
    
    *_retval = PR_FALSE;
    return NS_OK;
  }

  return mPromptService->PromptUsernameAndPassword(mParent, dialogTitle, text,
                                                   username, password,
                                                   checkMsg, checkValue,
                                                   _retval);
}

NS_IMETHODIMP
nsPrompt::PromptPassword(const PRUnichar *dialogTitle,
                         const PRUnichar *text,
                         PRUnichar **password,
                         const PRUnichar *checkMsg,
                         PRBool *checkValue,
                         PRBool *_retval)
{
  nsAutoWindowStateHelper windowStateHelper(mParent);

  if (!windowStateHelper.DefaultEnabled()) {
    
    
    *_retval = PR_FALSE;
    return NS_OK;
  }

  return mPromptService->PromptPassword(mParent, dialogTitle, text, password,
                                        checkMsg, checkValue, _retval);
}

NS_IMETHODIMP
nsPrompt::Select(const PRUnichar *dialogTitle,
                 const PRUnichar* inMsg,
                 PRUint32 inCount, 
                 const PRUnichar **inList,
                 PRInt32 *outSelection,
                 PRBool *_retval)
{
  nsAutoWindowStateHelper windowStateHelper(mParent);

  if (!windowStateHelper.DefaultEnabled()) {
    
    *outSelection = 0;
    *_retval = PR_FALSE;
    return NS_OK;
  }

  return mPromptService->Select(mParent, dialogTitle, inMsg, inCount, inList,
                                outSelection, _retval);
}







NS_IMETHODIMP
nsPrompt::Prompt(const PRUnichar* dialogTitle,
                 const PRUnichar* text,
                 const PRUnichar* passwordRealm,
                 PRUint32 savePassword,
                 const PRUnichar* defaultText,
                 PRUnichar* *result,
                 PRBool *_retval)
{
  nsAutoWindowStateHelper windowStateHelper(mParent);

  if (!windowStateHelper.DefaultEnabled()) {
    
    *result = nsnull;
    *_retval = PR_FALSE;
    return NS_OK;
  }

  
  if (defaultText) {
    *result = ToNewUnicode(nsDependentString(defaultText));

    if (!*result) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return mPromptService->Prompt(mParent, dialogTitle, text, result, nsnull,
                                nsnull, _retval);
}

NS_IMETHODIMP
nsPrompt::PromptUsernameAndPassword(const PRUnichar* dialogTitle, 
                                    const PRUnichar* text,
                                    const PRUnichar* passwordRealm,
                                    PRUint32 savePassword,
                                    PRUnichar* *user,
                                    PRUnichar* *pwd,
                                    PRBool *_retval)
{
  nsAutoWindowStateHelper windowStateHelper(mParent);

  if (!windowStateHelper.DefaultEnabled()) {
    
    *user = nsnull;
    *pwd = nsnull;
    *_retval = PR_FALSE;
    return NS_OK;
  }

  
  return mPromptService->PromptUsernameAndPassword(mParent, dialogTitle, text,
                                                   user, pwd, nsnull, nsnull,
                                                   _retval);
}

NS_IMETHODIMP
nsPrompt::PromptPassword(const PRUnichar* dialogTitle, 
                         const PRUnichar* text,
                         const PRUnichar* passwordRealm,
                         PRUint32 savePassword,
                         PRUnichar* *pwd,
                         PRBool *_retval)
{
  nsAutoWindowStateHelper windowStateHelper(mParent);

  if (!windowStateHelper.DefaultEnabled()) {
    
    *pwd = nsnull;
    *_retval = PR_FALSE;
    return NS_OK;
  }

  
  return mPromptService->PromptPassword(mParent, dialogTitle, text, pwd,
                                        nsnull, nsnull, _retval);
}
NS_IMETHODIMP
nsPrompt::PromptAuth(nsIChannel* aChannel,
                     PRUint32 aLevel,
                     nsIAuthInformation* aAuthInfo,
                     PRBool* retval)
{
  nsAutoWindowStateHelper windowStateHelper(mParent);

  if (!windowStateHelper.DefaultEnabled()) {
    
    *retval = PR_FALSE;
    return NS_OK;
  }

  if (mPromptService2) {
    return mPromptService2->PromptAuth(mParent, aChannel,
                                       aLevel, aAuthInfo,
                                       nsnull, nsnull, retval);
  }

  return PromptPasswordAdapter(mPromptService, mParent, aChannel,
                               aLevel, aAuthInfo, nsnull, nsnull, retval);
}

NS_IMETHODIMP
nsPrompt::AsyncPromptAuth(nsIChannel* aChannel,
                          nsIAuthPromptCallback* aCallback,
                          nsISupports* aContext,
                          PRUint32 aLevel,
                          nsIAuthInformation* aAuthInfo,
                          nsICancelable** retval)
{
  nsAutoWindowStateHelper windowStateHelper(mParent);

  if (!windowStateHelper.DefaultEnabled()) {
    
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  if (mPromptService2) {
    return mPromptService2->AsyncPromptAuth(mParent, aChannel,
                                            aCallback, aContext,
                                            aLevel, aAuthInfo,
                                            nsnull, nsnull, retval);
  }

  
  return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult
MakeDialogText(nsIChannel* aChannel, nsIAuthInformation* aAuthInfo,
               nsXPIDLString& message)
{
  nsresult rv;
  nsCOMPtr<nsIStringBundleService> bundleSvc =
    do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringBundle> bundle;
  rv = bundleSvc->CreateBundle("chrome://global/locale/prompts.properties",
                               getter_AddRefs(bundle));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCAutoString host;
  PRInt32 port;
  NS_GetAuthHostPort(aChannel, aAuthInfo, PR_FALSE, host, &port);

  nsAutoString displayHost;
  CopyUTF8toUTF16(host, displayHost);

  nsCOMPtr<nsIURI> uri;
  aChannel->GetURI(getter_AddRefs(uri));

  nsCAutoString scheme;
  uri->GetScheme(scheme);

  nsAutoString username;
  aAuthInfo->GetUsername(username);

  PRUint32 flags;
  aAuthInfo->GetFlags(&flags);
  PRBool proxyAuth = (flags & nsIAuthInformation::AUTH_PROXY) != 0;

  nsAutoString realm;
  aAuthInfo->GetRealm(realm);

  
  if (port != -1) {
    displayHost.Append(PRUnichar(':'));
    displayHost.AppendInt(port);
  }

  NS_NAMED_LITERAL_STRING(proxyText, "EnterUserPasswordForProxy");
  NS_NAMED_LITERAL_STRING(originText, "EnterUserPasswordForRealm");
  NS_NAMED_LITERAL_STRING(noRealmText, "EnterUserPasswordFor");
  NS_NAMED_LITERAL_STRING(passwordText, "EnterPasswordFor");

  const PRUnichar *text;
  if (proxyAuth) {
    text = proxyText.get();
  } else {
    text = originText.get();

    
    nsAutoString schemeU; 
    CopyASCIItoUTF16(scheme, schemeU);
    schemeU.AppendLiteral("://");
    displayHost.Insert(schemeU, 0);
  }

  const PRUnichar *strings[] = { realm.get(), displayHost.get() };
  PRUint32 count = NS_ARRAY_LENGTH(strings);

  if (flags & nsIAuthInformation::ONLY_PASSWORD) {
    text = passwordText.get();
    strings[0] = username.get();
  } else if (!proxyAuth && realm.IsEmpty()) {
    text = noRealmText.get();
    count--;
    strings[0] = strings[1];
  }

  rv = bundle->FormatStringFromName(text, strings, count, getter_Copies(message));
  return rv;
}

 nsresult
nsPrompt::PromptPasswordAdapter(nsIPromptService* aService,
                                nsIDOMWindow* aParent,
                                nsIChannel* aChannel,
                                PRUint32 aLevel,
                                nsIAuthInformation* aAuthInfo,
                                const PRUnichar* aCheckLabel,
                                PRBool* aCheckValue,
                                PRBool* retval)
{
  
  nsXPIDLString message;
  MakeDialogText(aChannel, aAuthInfo, message);

  nsAutoString defaultUser, defaultDomain, defaultPass;
  aAuthInfo->GetUsername(defaultUser);
  aAuthInfo->GetDomain(defaultDomain);
  aAuthInfo->GetPassword(defaultPass);

  PRUint32 flags;
  aAuthInfo->GetFlags(&flags);

  if ((flags & nsIAuthInformation::NEED_DOMAIN) && !defaultDomain.IsEmpty()) {
    defaultDomain.Append(PRUnichar('\\'));
    defaultUser.Insert(defaultDomain, 0);
  }

  
  
  PRUnichar *user = ToNewUnicode(defaultUser),
            *pass = ToNewUnicode(defaultPass);
  nsresult rv;
  if (flags & nsIAuthInformation::ONLY_PASSWORD)
    rv = aService->PromptPassword(aParent, nsnull, message.get(),
                                  &pass, aCheckLabel,
                                  aCheckValue, retval);
  else
    rv = aService->PromptUsernameAndPassword(aParent, nsnull, message.get(),
                                             &user, &pass, aCheckLabel,
                                             aCheckValue, retval);

  nsAdoptingString userStr(user);
  nsAdoptingString passStr(pass);
  NS_SetAuthInfo(aAuthInfo, userStr, passStr);

  return rv;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(AuthPromptWrapper, nsIAuthPrompt2)

NS_IMETHODIMP
AuthPromptWrapper::PromptAuth(nsIChannel* aChannel,
                              PRUint32 aLevel,
                              nsIAuthInformation* aAuthInfo,
                              PRBool* retval)
{
  nsCAutoString keyUTF8;
  NS_GetAuthKey(aChannel, aAuthInfo, keyUTF8);

  NS_ConvertUTF8toUTF16 key(keyUTF8);

  nsXPIDLString text;
  MakeDialogText(aChannel, aAuthInfo, text);

  PRUint32 flags;
  aAuthInfo->GetFlags(&flags);

  nsresult rv;
  nsXPIDLString user, password;
  if (flags & nsIAuthInformation::ONLY_PASSWORD) {
    rv = mAuthPrompt->PromptPassword(nsnull, text.get(), key.get(),
                                     nsIAuthPrompt::SAVE_PASSWORD_PERMANENTLY,
                                     getter_Copies(password), retval);
    if (NS_SUCCEEDED(rv) && *retval) {
      NS_ASSERTION(password, "password must not be null if retval is true");
      aAuthInfo->SetPassword(password);
    }
  } else {
    rv = mAuthPrompt->PromptUsernameAndPassword(nsnull, text.get(), key.get(),
                                                nsIAuthPrompt::SAVE_PASSWORD_PERMANENTLY,
                                                getter_Copies(user),
                                                getter_Copies(password),
                                                retval);
    if (NS_SUCCEEDED(rv) && *retval) {
      NS_ASSERTION(user && password, "out params must be nonnull");
      NS_SetAuthInfo(aAuthInfo, user, password);
    }
  }
  return rv;
}

NS_IMETHODIMP
AuthPromptWrapper::AsyncPromptAuth(nsIChannel*,
                                   nsIAuthPromptCallback*,
                                   nsISupports*,
                                   PRUint32,
                                   nsIAuthInformation*,
                                   nsICancelable**)
{
  
  
  return NS_ERROR_NOT_IMPLEMENTED;
}



