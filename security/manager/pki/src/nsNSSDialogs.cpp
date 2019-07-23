










































#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIPrompt.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDialogParamBlock.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIX509Cert.h"
#include "nsIX509CertDB.h"
#include "nsILocaleService.h"
#include "nsIDateTimeFormat.h"
#include "nsDateTimeFormatCID.h"

#include "nsNSSDialogs.h"
#include "nsPKIParamBlock.h"
#include "nsIKeygenThread.h"
#include "nsNSSDialogHelper.h"
#include "nsIX509CertValidity.h"
#include "nsICRLInfo.h"

#define PIPSTRING_BUNDLE_URL "chrome://pippki/locale/pippki.properties"

static NS_DEFINE_CID(kCStringBundleServiceCID,  NS_STRINGBUNDLESERVICE_CID);


static NS_DEFINE_CID(kPKIParamBlockCID, NS_PKIPARAMBLOCK_CID);

nsNSSDialogs::nsNSSDialogs()
{
}

nsNSSDialogs::~nsNSSDialogs()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS8(nsNSSDialogs, nsITokenPasswordDialogs,
                                            nsIBadCertListener,
                                            nsICertificateDialogs,
                                            nsIClientAuthDialogs,
                                            nsICertPickDialogs,
                                            nsITokenDialogs,
                                            nsIDOMCryptoDialogs,
                                            nsIGeneratingKeypairInfoDialogs)

nsresult
nsNSSDialogs::Init()
{
  nsresult rv;

  nsCOMPtr<nsIStringBundleService> service = do_GetService(kCStringBundleServiceCID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  rv = service->CreateBundle(PIPSTRING_BUNDLE_URL,
                             getter_AddRefs(mPIPStringBundle));
  return rv;
}

nsresult
nsNSSDialogs::SetPassword(nsIInterfaceRequestor *ctx,
                          const PRUnichar *tokenName, PRBool* _canceled)
{
  nsresult rv;

  *_canceled = PR_FALSE;

  
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(ctx);

  nsCOMPtr<nsIDialogParamBlock> block(do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID));
  if (!block) return NS_ERROR_FAILURE;

  
  rv = block->SetString(1, tokenName);
  if (NS_FAILED(rv)) return rv;

  rv = nsNSSDialogHelper::openDialog(parent,
                                "chrome://pippki/content/changepassword.xul",
                                block);

  if (NS_FAILED(rv)) return rv;

  PRInt32 status;

  rv = block->GetInt(1, &status);
  if (NS_FAILED(rv)) return rv;

  *_canceled = (status == 0)?PR_TRUE:PR_FALSE;

  return rv;
}

nsresult
nsNSSDialogs::GetPassword(nsIInterfaceRequestor *ctx,
                          const PRUnichar *tokenName, 
                          PRUnichar **_password,
                          PRBool* _canceled)
{
  nsresult rv;
  *_canceled = PR_FALSE;
  
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(ctx);
  nsCOMPtr<nsIDialogParamBlock> block(do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID));
  if (!block) return NS_ERROR_FAILURE;
  
  rv = block->SetString(1, tokenName);
  if (NS_FAILED(rv)) return rv;
  
  rv = nsNSSDialogHelper::openDialog(parent,
                                     "chrome://pippki/content/getpassword.xul",
                                     block);
  if (NS_FAILED(rv)) return rv;
  
  PRInt32 status;
  rv = block->GetInt(1, &status);
  if (NS_FAILED(rv)) return rv;
  *_canceled = (status == 0) ? PR_TRUE : PR_FALSE;
  if (!*_canceled) {
    
    rv = block->GetString(2, _password);
  }
  return rv;
}

NS_IMETHODIMP
nsNSSDialogs::ConfirmUnknownIssuer(nsIInterfaceRequestor *socketInfo,
                                   nsIX509Cert *cert, PRInt16 *outAddType,
                                   PRBool *_retval)
{
  nsresult rv;
  PRInt32 addType;
  
  *_retval = PR_FALSE;

  nsCOMPtr<nsIPKIParamBlock> block = do_CreateInstance(kPKIParamBlockCID);

  if (!block)
    return NS_ERROR_FAILURE;

  nsXPIDLString commonName;
  rv = block->SetISupportAtIndex(1, cert);
  if (NS_FAILED(rv))
    return rv;

  rv = nsNSSDialogHelper::openDialog(nsnull, 
                                     "chrome://pippki/content/newserver.xul",
                                     block);

  if (NS_FAILED(rv))
    return rv;

  PRInt32 status;
  nsCOMPtr<nsIDialogParamBlock> dialogBlock = do_QueryInterface(block);
  rv = dialogBlock->GetInt(1, &status);
  if (NS_FAILED(rv))
    return rv; 

  if (status == 0) {
    *_retval = PR_FALSE;
  } else {
    
    
    rv = dialogBlock->GetInt(2, &addType);
    switch (addType) {
      case 0:
        *outAddType = ADD_TRUSTED_PERMANENTLY;
        *_retval    = PR_TRUE;
        break;
      case 1:
        *outAddType = ADD_TRUSTED_FOR_SESSION;
        *_retval    = PR_TRUE;
        break;
      default:
        *outAddType = UNINIT_ADD_FLAG;
        *_retval    = PR_FALSE;
        break;
    } 
  }

  return NS_OK; 
}

NS_IMETHODIMP 
nsNSSDialogs::ConfirmMismatchDomain(nsIInterfaceRequestor *socketInfo, 
                                    const nsACString &targetURL, 
                                    nsIX509Cert *cert, PRBool *_retval) 
{
  nsresult rv;

  *_retval = PR_FALSE;

  nsCOMPtr<nsIPKIParamBlock> block = do_CreateInstance(kPKIParamBlockCID);

  if (!block)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDialogParamBlock> dialogBlock = do_QueryInterface(block);
  rv = dialogBlock->SetString(1, NS_ConvertUTF8toUTF16(targetURL).get());
  if (NS_FAILED(rv))
    return rv;

  rv = block->SetISupportAtIndex(1, cert);
  if (NS_FAILED(rv))
    return rv;

  rv = nsNSSDialogHelper::openDialog(nsnull,
                                 "chrome://pippki/content/domainMismatch.xul",
                                 block);
  if (NS_FAILED(rv))
    return rv;

  PRInt32 status;

  rv = dialogBlock->GetInt(1, &status);
  if (NS_FAILED(rv))
    return rv;

  *_retval = (status) ? PR_TRUE : PR_FALSE;

  return NS_OK;  
}

NS_IMETHODIMP 
nsNSSDialogs::ConfirmCertExpired(nsIInterfaceRequestor *socketInfo, 
                                 nsIX509Cert *cert, PRBool *_retval)
{
  nsresult rv;
  PRTime now = PR_Now();
  PRTime notAfter, notBefore, timeToUse;
  nsCOMPtr<nsIX509CertValidity> validity;
  const char *key;
  const char *titleKey;

  *_retval = PR_FALSE;

  nsCOMPtr<nsIPKIParamBlock> block = do_CreateInstance(kPKIParamBlockCID);

  if (!block)
    return NS_ERROR_FAILURE; 
  rv = cert->GetValidity(getter_AddRefs(validity));
  if (NS_FAILED(rv))
    return rv;

  rv = validity->GetNotAfter(&notAfter);
  if (NS_FAILED(rv))
    return rv;

  rv = validity->GetNotBefore(&notBefore);
  if (NS_FAILED(rv))
    return rv;

  if (LL_CMP(now, >, notAfter)) {
    key       = "serverCertExpiredMsg1"; 
    titleKey  = "serverCertExpiredTitle";
    timeToUse = notAfter; 
  } else {
    key = "serverCertNotYetValedMsg1";
    titleKey  = "serverCertNotYetValidTitle";
    timeToUse = notBefore;
  }

  nsXPIDLString message1;
  nsXPIDLString title;
  nsAutoString commonName;
  nsAutoString formattedDate;

  rv = cert->GetCommonName(commonName);

  nsIDateTimeFormat *aDateTimeFormat;
  rv = CallCreateInstance(NS_DATETIMEFORMAT_CONTRACTID, &aDateTimeFormat);

  aDateTimeFormat->FormatPRTime(nsnull, kDateFormatShort, 
                                kTimeFormatNoSeconds, timeToUse, 
                                formattedDate);
  const PRUnichar *formatStrings[2] = { commonName.get(), formattedDate.get() }; 
  NS_ConvertASCIItoUTF16 keyString(key);
  NS_ConvertASCIItoUTF16 titleKeyString(titleKey);
  mPIPStringBundle->FormatStringFromName(keyString.get(), formatStrings, 
                                         2, getter_Copies(message1));
  mPIPStringBundle->FormatStringFromName(titleKeyString.get(), formatStrings,
                                         2, getter_Copies(title));
  
  nsCOMPtr<nsIDialogParamBlock> dialogBlock = do_QueryInterface(block);
  rv = dialogBlock->SetString(1,message1); 
  rv = dialogBlock->SetString(2,title);

  if (NS_FAILED(rv))
    return rv;

  rv = block->SetISupportAtIndex(1, cert);
  if (NS_FAILED(rv))
    return rv;

  rv = nsNSSDialogHelper::openDialog(nsnull,
                             "chrome://pippki/content/serverCertExpired.xul",
                             block);

  PRInt32 status;
  rv = dialogBlock->GetInt(1, &status);
  if (NS_FAILED(rv))
    return rv; 

  *_retval = (status) ? PR_TRUE : PR_FALSE;
  
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSDialogs::NotifyCrlNextupdate(nsIInterfaceRequestor *socketInfo, 
                                  const nsACString &targetURL, nsIX509Cert *cert)
{
  nsresult rv;

  nsCOMPtr<nsIPKIParamBlock> block = do_CreateInstance(kPKIParamBlockCID);
  nsCOMPtr<nsIDialogParamBlock> dialogBlock = do_QueryInterface(block);

  rv = dialogBlock->SetString(1, NS_ConvertUTF8toUTF16(targetURL).get());
  if (NS_FAILED(rv))
    return rv;

  rv = block->SetISupportAtIndex(1, cert);
  if (NS_FAILED(rv))
    return rv;

  rv = nsNSSDialogHelper::openDialog(nsnull,
                             "chrome://pippki/content/serverCrlNextupdate.xul",
                             block);
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSDialogs::CrlImportStatusDialog(nsIInterfaceRequestor *ctx, nsICRLInfo *crl)
{
  nsresult rv;

  nsCOMPtr<nsIPKIParamBlock> block = do_CreateInstance(kPKIParamBlockCID,&rv);
  if (NS_FAILED(rv))
    return rv;
  
  rv = block->SetISupportAtIndex(1, crl);
  if (NS_FAILED(rv))
    return rv;

  rv = nsNSSDialogHelper::openDialog(nsnull,
                             "chrome://pippki/content/crlImportDialog.xul",
                             block);
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSDialogs::ConfirmDownloadCACert(nsIInterfaceRequestor *ctx, 
                                    nsIX509Cert *cert,
                                    PRUint32 *_trust,
                                    PRBool *_retval)
{
  nsresult rv;

  *_retval = PR_TRUE;

  
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(ctx);

  nsCOMPtr<nsIPKIParamBlock> block = do_CreateInstance(kPKIParamBlockCID);
  if (!block)
    return NS_ERROR_FAILURE;

  rv = block->SetISupportAtIndex(1, cert);
  if (NS_FAILED(rv))
    return rv;

  rv = nsNSSDialogHelper::openDialog(parent, 
                                   "chrome://pippki/content/downloadcert.xul",
                                     block);
  if (NS_FAILED(rv)) return rv;

  PRInt32 status;
  PRInt32 ssl, email, objsign;

  nsCOMPtr<nsIDialogParamBlock> dlgParamBlock = do_QueryInterface(block);
  
  rv = dlgParamBlock->GetInt(1, &status);
  if (NS_FAILED(rv)) return rv;
  rv = dlgParamBlock->GetInt(2, &ssl);
  if (NS_FAILED(rv)) return rv;
  rv = dlgParamBlock->GetInt(3, &email);
  if (NS_FAILED(rv)) return rv;
  rv = dlgParamBlock->GetInt(4, &objsign);
  if (NS_FAILED(rv)) return rv;
 
  *_trust = nsIX509CertDB::UNTRUSTED;
  *_trust |= (ssl) ? nsIX509CertDB::TRUSTED_SSL : 0;
  *_trust |= (email) ? nsIX509CertDB::TRUSTED_EMAIL : 0;
  *_trust |= (objsign) ? nsIX509CertDB::TRUSTED_OBJSIGN : 0;

  *_retval = (status == 0)?PR_FALSE:PR_TRUE;

  return rv;
}


NS_IMETHODIMP 
nsNSSDialogs::NotifyCACertExists(nsIInterfaceRequestor *ctx)
{
  nsresult rv;

  
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(ctx);

  nsCOMPtr<nsIDialogParamBlock> block(do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID));
  if (!block) return NS_ERROR_FAILURE;

  
  rv = nsNSSDialogHelper::openDialog(parent, 
                                     "chrome://pippki/content/cacertexists.xul",
                                     block);

  return rv;
}


NS_IMETHODIMP
nsNSSDialogs::ChooseCertificate(nsIInterfaceRequestor *ctx, const PRUnichar *cn, const PRUnichar *organization, const PRUnichar *issuer, const PRUnichar **certNickList, const PRUnichar **certDetailsList, PRUint32 count, PRInt32 *selectedIndex, PRBool *canceled) 
{
  nsresult rv;
  PRUint32 i;

  *canceled = PR_FALSE;

  
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(ctx);

  nsCOMPtr<nsIDialogParamBlock> block(do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID));
  if (!block) return NS_ERROR_FAILURE;

  block->SetNumberStrings(4+count*2);

  rv = block->SetString(0, cn);
  if (NS_FAILED(rv)) return rv;

  rv = block->SetString(1, organization);
  if (NS_FAILED(rv)) return rv;

  rv = block->SetString(2, issuer);
  if (NS_FAILED(rv)) return rv;

  for (i = 0; i < count; i++) {
    rv = block->SetString(i+3, certNickList[i]);
    if (NS_FAILED(rv)) return rv;
  }

  for (i = 0; i < count; i++) {
    rv = block->SetString(i+count+3, certDetailsList[i]);
    if (NS_FAILED(rv)) return rv;
  }

  rv = block->SetInt(0, count);
  if (NS_FAILED(rv)) return rv;

  rv = nsNSSDialogHelper::openDialog(nsnull,
                                "chrome://pippki/content/clientauthask.xul",
                                block);
  if (NS_FAILED(rv)) return rv;

  PRInt32 status;

  rv = block->GetInt(0, &status);
  if (NS_FAILED(rv)) return rv;

  *canceled = (status == 0)?PR_TRUE:PR_FALSE;
  if (!*canceled) {
    
    rv = block->GetInt(1, selectedIndex);
  }
  return rv;
}


NS_IMETHODIMP
nsNSSDialogs::PickCertificate(nsIInterfaceRequestor *ctx, 
                              const PRUnichar **certNickList, 
                              const PRUnichar **certDetailsList, 
                              PRUint32 count, 
                              PRInt32 *selectedIndex, 
                              PRBool *canceled) 
{
  nsresult rv;
  PRUint32 i;

  *canceled = PR_FALSE;

  
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(ctx);

  nsCOMPtr<nsIDialogParamBlock> block(do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID));
  if (!block) return NS_ERROR_FAILURE;

  block->SetNumberStrings(1+count*2);

  for (i = 0; i < count; i++) {
    rv = block->SetString(i, certNickList[i]);
    if (NS_FAILED(rv)) return rv;
  }

  for (i = 0; i < count; i++) {
    rv = block->SetString(i+count, certDetailsList[i]);
    if (NS_FAILED(rv)) return rv;
  }

  rv = block->SetInt(0, count);
  if (NS_FAILED(rv)) return rv;

  rv = block->SetInt(1, *selectedIndex);
  if (NS_FAILED(rv)) return rv;

  rv = nsNSSDialogHelper::openDialog(nsnull,
                                "chrome://pippki/content/certpicker.xul",
                                block);
  if (NS_FAILED(rv)) return rv;

  PRInt32 status;

  rv = block->GetInt(0, &status);
  if (NS_FAILED(rv)) return rv;

  *canceled = (status == 0)?PR_TRUE:PR_FALSE;
  if (!*canceled) {
    rv = block->GetInt(1, selectedIndex);
  }
  return rv;
}


NS_IMETHODIMP 
nsNSSDialogs::SetPKCS12FilePassword(nsIInterfaceRequestor *ctx, 
                                    nsAString &_password,
                                    PRBool *_retval)
{
  nsresult rv;
  *_retval = PR_TRUE;
  
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(ctx);
  nsCOMPtr<nsIDialogParamBlock> block(do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID));
  if (!block) return NS_ERROR_FAILURE;
  
  rv = nsNSSDialogHelper::openDialog(parent,
                                  "chrome://pippki/content/setp12password.xul",
                                  block);
  if (NS_FAILED(rv)) return rv;
  
  PRInt32 status;
  rv = block->GetInt(1, &status);
  if (NS_FAILED(rv)) return rv;
  *_retval = (status == 0) ? PR_FALSE : PR_TRUE;
  if (*_retval) {
    
    PRUnichar *pw;
    rv = block->GetString(2, &pw);
    if (NS_SUCCEEDED(rv)) {
      _password = pw;
      nsMemory::Free(pw);
    }
  }
  return rv;
}

NS_IMETHODIMP 
nsNSSDialogs::GetPKCS12FilePassword(nsIInterfaceRequestor *ctx, 
                                    nsAString &_password,
                                    PRBool *_retval)
{
  nsresult rv;
  *_retval = PR_TRUE;
  
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(ctx);
  nsCOMPtr<nsIDialogParamBlock> block(do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID));
  if (!block) return NS_ERROR_FAILURE;
  
  rv = nsNSSDialogHelper::openDialog(parent,
                                  "chrome://pippki/content/getp12password.xul",
                                  block);
  if (NS_FAILED(rv)) return rv;
  
  PRInt32 status;
  rv = block->GetInt(1, &status);
  if (NS_FAILED(rv)) return rv;
  *_retval = (status == 0) ? PR_FALSE : PR_TRUE;
  if (*_retval) {
    
    PRUnichar *pw;
    rv = block->GetString(2, &pw);
    if (NS_SUCCEEDED(rv)) {
      _password = pw;
      nsMemory::Free(pw);
    }
  }
  return rv;
}


NS_IMETHODIMP 
nsNSSDialogs::ViewCert(nsIInterfaceRequestor *ctx, 
                       nsIX509Cert *cert)
{
  nsresult rv;

  nsCOMPtr<nsIPKIParamBlock> block = do_CreateInstance(kPKIParamBlockCID);
  if (!block)
    return NS_ERROR_FAILURE;

  rv = block->SetISupportAtIndex(1, cert);
  if (NS_FAILED(rv))
    return rv;

  
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(ctx);

  rv = nsNSSDialogHelper::openDialog(parent,
                                     "chrome://pippki/content/certViewer.xul",
                                     block);
  return rv;
}

NS_IMETHODIMP
nsNSSDialogs::DisplayGeneratingKeypairInfo(nsIInterfaceRequestor *aCtx, nsIKeygenThread *runnable) 
{
  nsresult rv;

  
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(aCtx);

  rv = nsNSSDialogHelper::openDialog(parent,
                                     "chrome://pippki/content/createCertInfo.xul",
                                     runnable);
  return rv;
}

NS_IMETHODIMP
nsNSSDialogs::ChooseToken(nsIInterfaceRequestor *aCtx, const PRUnichar **aTokenList, PRUint32 aCount, PRUnichar **aTokenChosen, PRBool *aCanceled) {
  nsresult rv;
  PRUint32 i;

  *aCanceled = PR_FALSE;

  
  nsCOMPtr<nsIDOMWindowInternal> parent = do_GetInterface(aCtx);

  nsCOMPtr<nsIDialogParamBlock> block(do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID));
  if (!block) return NS_ERROR_FAILURE;

  block->SetNumberStrings(aCount);

  for (i = 0; i < aCount; i++) {
    rv = block->SetString(i, aTokenList[i]);
    if (NS_FAILED(rv)) return rv;
  }

  rv = block->SetInt(0, aCount);
  if (NS_FAILED(rv)) return rv;

  rv = nsNSSDialogHelper::openDialog(nsnull,
                                "chrome://pippki/content/choosetoken.xul",
                                block);
  if (NS_FAILED(rv)) return rv;

  PRInt32 status;

  rv = block->GetInt(0, &status);
  if (NS_FAILED(rv)) return rv;

  *aCanceled = (status == 0)?PR_TRUE:PR_FALSE;
  if (!*aCanceled) {
    
    rv = block->GetString(0, aTokenChosen);
  }
  return rv;
}


NS_IMETHODIMP 
nsNSSDialogs::ConfirmKeyEscrow(nsIX509Cert *escrowAuthority, PRBool *_retval)
                                     
{
  *_retval = PR_FALSE;

  nsresult rv;

  nsCOMPtr<nsIPKIParamBlock> block = do_CreateInstance(kPKIParamBlockCID);
  if (!block)
    return NS_ERROR_FAILURE;

  rv = block->SetISupportAtIndex(1, escrowAuthority);
  if (NS_FAILED(rv))
    return rv;

  rv = nsNSSDialogHelper::openDialog(nsnull,
                                     "chrome://pippki/content/escrowWarn.xul",
                                     block);

  if (NS_FAILED(rv))
    return rv;

  PRInt32 status=0;
  nsCOMPtr<nsIDialogParamBlock> dlgParamBlock = do_QueryInterface(block);
  rv = dlgParamBlock->GetInt(1, &status);
 
  if (status) {
    *_retval = PR_TRUE;
  } 
  return rv;
}


