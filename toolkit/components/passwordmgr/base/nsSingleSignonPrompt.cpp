





































#include "nsSingleSignonPrompt.h"
#include "nsPasswordManager.h"
#include "nsIServiceManager.h"

#include "nsIAuthInformation.h"
#include "nsIDOMWindow.h"
#include "nsIChannel.h"
#include "nsIIDNService.h"
#include "nsIPromptService2.h"
#include "nsIProxiedChannel.h"
#include "nsIProxyInfo.h"
#include "nsIURI.h"

#include "nsNetUtil.h"
#include "nsPromptUtils.h"

NS_IMPL_ISUPPORTS2(nsSingleSignonPrompt,
                   nsIAuthPromptWrapper,
                   nsIAuthPrompt)


NS_IMETHODIMP
nsSingleSignonPrompt::Prompt(const PRUnichar* aDialogTitle,
                             const PRUnichar* aText,
                             const PRUnichar* aPasswordRealm,
                             PRUint32 aSavePassword,
                             const PRUnichar* aDefaultText,
                             PRUnichar** aResult,
                             PRBool* aConfirm)
{
  nsAutoString checkMsg;
  nsString emptyString;
  PRBool checkValue = PR_FALSE;
  PRBool *checkPtr = nsnull;
  PRUnichar* value = nsnull;
  nsCOMPtr<nsIPasswordManagerInternal> mgrInternal;

  if (nsPasswordManager::SingleSignonEnabled() && aPasswordRealm) {
    if (aSavePassword == SAVE_PASSWORD_PERMANENTLY) {
      nsPasswordManager::GetLocalizedString(NS_LITERAL_STRING("rememberValue"),
                                            checkMsg);
      checkPtr = &checkValue;
    }

    mgrInternal = do_GetService(NS_PASSWORDMANAGER_CONTRACTID);
    nsCAutoString outHost;
    nsAutoString outUser, outPassword;

    mgrInternal->FindPasswordEntry(NS_ConvertUTF16toUTF8(aPasswordRealm),
                                   emptyString,
                                   emptyString,
                                   outHost,
                                   outUser,
                                   outPassword);

    if (!outUser.IsEmpty()) {
      value = ToNewUnicode(outUser);
      checkValue = PR_TRUE;
    }
  }

  if (!value && aDefaultText)
    value = ToNewUnicode(nsDependentString(aDefaultText));

  mPrompt->Prompt(aDialogTitle,
                  aText,
                  &value,
                  checkMsg.get(),
                  checkPtr,
                  aConfirm);

  if (*aConfirm) {
    if (checkValue && value && value[0] != '\0') {
      
      

      nsCOMPtr<nsIPasswordManager> manager = do_QueryInterface(mgrInternal);

      manager->AddUser(NS_ConvertUTF16toUTF8(aPasswordRealm),
                       nsDependentString(value),
                       emptyString);
    }

    *aResult = value;
  } else {
    if (value)
      nsMemory::Free(value);
    *aResult = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSingleSignonPrompt::PromptUsernameAndPassword(const PRUnichar* aDialogTitle,
                                                const PRUnichar* aText,
                                                const PRUnichar* aPasswordRealm,
                                                PRUint32 aSavePassword,
                                                PRUnichar** aUser,
                                                PRUnichar** aPassword,
                                                PRBool* aConfirm)
{
  nsAutoString checkMsg;
  nsString emptyString;
  PRBool checkValue = PR_FALSE;
  PRBool *checkPtr = nsnull;
  PRUnichar *user = nsnull, *password = nsnull;
  nsCOMPtr<nsIPasswordManagerInternal> mgrInternal;

  if (nsPasswordManager::SingleSignonEnabled() && aPasswordRealm) {
    if (aSavePassword == SAVE_PASSWORD_PERMANENTLY) {
      nsPasswordManager::GetLocalizedString(NS_LITERAL_STRING("rememberPassword"),
                                            checkMsg);
      checkPtr = &checkValue;
    }

    mgrInternal = do_GetService(NS_PASSWORDMANAGER_CONTRACTID);
    nsCAutoString outHost;
    nsAutoString outUser, outPassword;

    mgrInternal->FindPasswordEntry(NS_ConvertUTF16toUTF8(aPasswordRealm),
                                   emptyString,
                                   emptyString,
                                   outHost,
                                   outUser,
                                   outPassword);

    user = ToNewUnicode(outUser);
    password = ToNewUnicode(outPassword);
    if (!outUser.IsEmpty() || !outPassword.IsEmpty())
      checkValue = PR_TRUE;
  }

  mPrompt->PromptUsernameAndPassword(aDialogTitle,
                                     aText,
                                     &user,
                                     &password,
                                     checkMsg.get(),
                                     checkPtr,
                                     aConfirm);

  if (*aConfirm) {
    if (checkValue && user && password && (user[0] != '\0' || password[0] != '\0')) {
      
      

      nsCOMPtr<nsIPasswordManager> manager = do_QueryInterface(mgrInternal);

      manager->AddUser(NS_ConvertUTF16toUTF8(aPasswordRealm),
                       nsDependentString(user),
                       nsDependentString(password));
    }

    *aUser = user;
    *aPassword = password;

  } else {
    if (user)
      nsMemory::Free(user);
    if (password)
      nsMemory::Free(password);

    *aUser = *aPassword = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSingleSignonPrompt::PromptPassword(const PRUnichar* aDialogTitle,
                                     const PRUnichar* aText,
                                     const PRUnichar* aPasswordRealm,
                                     PRUint32 aSavePassword,
                                     PRUnichar** aPassword,
                                     PRBool* aConfirm)
{
  nsAutoString checkMsg;
  nsString emptyString;
  PRBool checkValue = PR_FALSE;
  PRBool *checkPtr = nsnull;
  PRUnichar* password = nsnull;
  nsCOMPtr<nsIPasswordManagerInternal> mgrInternal;

  if (nsPasswordManager::SingleSignonEnabled() && aPasswordRealm) {
    if (aSavePassword == SAVE_PASSWORD_PERMANENTLY) {
      nsPasswordManager::GetLocalizedString(NS_LITERAL_STRING("rememberPassword"),
                                            checkMsg);
      checkPtr = &checkValue;
    }

    mgrInternal = do_GetService(NS_PASSWORDMANAGER_CONTRACTID);
    nsCAutoString outHost;
    nsAutoString outUser, outPassword;

    mgrInternal->FindPasswordEntry(NS_ConvertUTF16toUTF8(aPasswordRealm),
                                   emptyString,
                                   emptyString,
                                   outHost,
                                   outUser,
                                   outPassword);

    password = ToNewUnicode(outPassword);
    if (!outPassword.IsEmpty())
      checkValue = PR_TRUE;
  }

  mPrompt->PromptPassword(aDialogTitle,
                          aText,
                          &password,
                          checkMsg.get(),
                          checkPtr,
                          aConfirm);

  if (*aConfirm) {
    if (checkValue && password && password[0] != '\0') {
      
      

      nsCOMPtr<nsIPasswordManager> manager = do_QueryInterface(mgrInternal);

      manager->AddUser(NS_ConvertUTF16toUTF8(aPasswordRealm),
                       emptyString,
                       nsDependentString(password));
    }

    *aPassword = password;

  } else {
    if (password)
      nsMemory::Free(password);
    *aPassword = nsnull;
  }

  return NS_OK;
}



NS_IMETHODIMP
nsSingleSignonPrompt::SetPromptDialogs(nsIPrompt* aDialogs)
{
  mPrompt = aDialogs;
  return NS_OK;
}




nsSingleSignonPrompt2::nsSingleSignonPrompt2(nsIPromptService2* aService,
                                             nsIDOMWindow* aParent)
  : mService(aService), mParent(aParent)
{
}

nsSingleSignonPrompt2::~nsSingleSignonPrompt2()
{
}

NS_IMPL_ISUPPORTS1(nsSingleSignonPrompt2, nsIAuthPrompt2)



NS_IMETHODIMP
nsSingleSignonPrompt2::PromptAuth(nsIChannel* aChannel,
                                  PRUint32 aLevel,
                                  nsIAuthInformation* aAuthInfo,
                                  PRBool* aConfirm)
{
  nsCAutoString key;
  NS_GetAuthKey(aChannel, aAuthInfo, key);

  nsAutoString checkMsg;
  PRBool checkValue = PR_FALSE;
  PRBool *checkPtr = nsnull;
  nsCOMPtr<nsIPasswordManagerInternal> mgrInternal;

  if (nsPasswordManager::SingleSignonEnabled()) {
    nsPasswordManager::GetLocalizedString(NS_LITERAL_STRING("rememberPassword"),
                                          checkMsg);
    checkPtr = &checkValue;

    mgrInternal = do_GetService(NS_PASSWORDMANAGER_CONTRACTID);
    nsCAutoString outHost;
    nsAutoString outUser, outPassword;

    const nsString& emptyString = EmptyString();
    mgrInternal->FindPasswordEntry(key,
                                   emptyString,
                                   emptyString,
                                   outHost,
                                   outUser,
                                   outPassword);

    NS_SetAuthInfo(aAuthInfo, outUser, outPassword);

    if (!outUser.IsEmpty() || !outPassword.IsEmpty())
      checkValue = PR_TRUE;
  }

  mService->PromptAuth(mParent, aChannel, aLevel, aAuthInfo,
                       checkMsg.get(), checkPtr, aConfirm);

  if (*aConfirm) {
    
    nsAutoString user, password;
    aAuthInfo->GetUsername(user);
    aAuthInfo->GetPassword(password);
    if (checkValue && (!user.IsEmpty() || !password.IsEmpty())) {
      

      nsCOMPtr<nsIPasswordManager> manager = do_QueryInterface(mgrInternal);

      manager->AddUser(key, user, password);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSingleSignonPrompt2::AsyncPromptAuth(nsIChannel*,
                                       nsIAuthPromptCallback*,
                                       nsISupports*,
                                       PRUint32,
                                       nsIAuthInformation*,
                                       nsICancelable**)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}



