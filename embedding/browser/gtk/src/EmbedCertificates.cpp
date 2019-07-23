










































#include <strings.h>
#include "nsIURI.h"
#include "EmbedPrivate.h"
#include "nsServiceManagerUtils.h"
#include "nsIWebNavigationInfo.h"
#include "nsDocShellCID.h"
#include "nsCOMPtr.h"
#ifdef MOZILLA_INTERNAL_API
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#else
#include "nsStringAPI.h"
#endif
#include "nsIPrompt.h"
#include "nsIDOMWindowInternal.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIX509Cert.h"
#include "nsIX509CertDB.h"
#include "nsIDateTimeFormat.h"
#include "nsDateTimeFormatCID.h"
#include "EmbedCertificates.h"
#include "nsIKeygenThread.h"
#include "nsIX509CertValidity.h"
#include "nsICRLInfo.h"
#include "gtkmozembed.h"

#define PIPSTRING_BUNDLE_URL "chrome://pippki/locale/pippki.properties"

EmbedCertificates::EmbedCertificates(void)
{
}

EmbedCertificates::~EmbedCertificates()
{
}

NS_IMPL_THREADSAFE_ADDREF(EmbedCertificates)
NS_IMPL_THREADSAFE_RELEASE(EmbedCertificates)
NS_INTERFACE_MAP_BEGIN(EmbedCertificates)
NS_INTERFACE_MAP_ENTRY(nsITokenPasswordDialogs)
NS_INTERFACE_MAP_ENTRY(nsICertificateDialogs)
NS_INTERFACE_MAP_ENTRY(nsIClientAuthDialogs)
NS_INTERFACE_MAP_ENTRY(nsICertPickDialogs)
NS_INTERFACE_MAP_ENTRY(nsITokenDialogs)
NS_INTERFACE_MAP_ENTRY(nsIGeneratingKeypairInfoDialogs)
NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMCryptoDialogs)
NS_INTERFACE_MAP_END

nsresult
EmbedCertificates::Init(void)
{
  nsresult rv;
  nsCOMPtr<nsIStringBundleService> service =
           do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return NS_OK;
  rv = service->CreateBundle(PIPSTRING_BUNDLE_URL,
                             getter_AddRefs(mPIPStringBundle));
  return NS_OK;
}

nsresult
EmbedCertificates::SetPassword(nsIInterfaceRequestor *ctx,
                          const PRUnichar *tokenName, PRBool* _canceled)
{
  *_canceled = PR_FALSE;
  return NS_OK;
}

nsresult
EmbedCertificates::GetPassword(nsIInterfaceRequestor *ctx,
                          const PRUnichar *tokenName,
                          PRUnichar **_password,
                          PRBool* _canceled)
{
  *_canceled = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
EmbedCertificates::CrlImportStatusDialog(nsIInterfaceRequestor *ctx, nsICRLInfo *crl)
{
  return NS_OK;
}

NS_IMETHODIMP
EmbedCertificates::ConfirmDownloadCACert(nsIInterfaceRequestor *ctx,
                                    nsIX509Cert *cert,
                                    PRUint32 *_trust,
                                    PRBool *_retval)
{
  
  
  
  *_retval = PR_FALSE;
  *_trust = nsIX509CertDB::UNTRUSTED;
  return NS_OK;
}

NS_IMETHODIMP
EmbedCertificates::NotifyCACertExists(nsIInterfaceRequestor *ctx)
{
  return NS_OK;
}

NS_IMETHODIMP
EmbedCertificates::ChooseCertificate(
  nsIInterfaceRequestor *ctx,
  const PRUnichar *cn,
  const PRUnichar *organization,
  const PRUnichar *issuer,
  const PRUnichar **certNickList,
  const PRUnichar **certDetailsList,
  PRUint32 count,
  PRInt32 *selectedIndex,
  PRBool *canceled)
{
  *canceled = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
EmbedCertificates::PickCertificate(
  nsIInterfaceRequestor *ctx,
  const PRUnichar **certNickList,
  const PRUnichar **certDetailsList,
  PRUint32 count,
  PRInt32 *selectedIndex,
  PRBool *canceled)
{
  *canceled = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
EmbedCertificates::SetPKCS12FilePassword(
  nsIInterfaceRequestor *ctx,
  nsAString &_password,
  PRBool *_retval)
{
  


  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
EmbedCertificates::GetPKCS12FilePassword(
  nsIInterfaceRequestor *ctx,
  nsAString &_password,
  PRBool *_retval)
{
  


  *_retval = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP
EmbedCertificates::ViewCert(
  nsIInterfaceRequestor *ctx,
  nsIX509Cert *cert)
{
  return NS_OK;
}

NS_IMETHODIMP
EmbedCertificates::DisplayGeneratingKeypairInfo(nsIInterfaceRequestor *aCtx, nsIKeygenThread *runnable)
{
  return NS_OK;
}

NS_IMETHODIMP
EmbedCertificates::ChooseToken(
  nsIInterfaceRequestor *aCtx,
  const PRUnichar **aTokenList,
  PRUint32 aCount,
  PRUnichar **aTokenChosen,
  PRBool *aCanceled)
{
  *aCanceled = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
EmbedCertificates::DisplayProtectedAuth(
  nsIInterfaceRequestor *aCtx,
  nsIProtectedAuthThread *runnable)
{
  return NS_OK;
}


NS_IMETHODIMP
EmbedCertificates::ConfirmKeyEscrow(nsIX509Cert *escrowAuthority, PRBool *_retval)
{
  
  
  
  *_retval = PR_FALSE;
  return NS_OK;
}

