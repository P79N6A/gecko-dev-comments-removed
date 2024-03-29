




#include "nsCertPicker.h"
#include "pkix/pkixtypes.h"
#include "nsMemory.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsIServiceManager.h"
#include "nsNSSComponent.h"
#include "nsNSSCertificate.h"
#include "nsReadableUtils.h"
#include "nsICertPickDialogs.h"
#include "nsNSSShutDown.h"
#include "nsNSSCertHelper.h"
#include "ScopedNSSTypes.h"

#include "cert.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS(nsCertPicker, nsIUserCertPicker)

nsCertPicker::nsCertPicker()
{
}

nsCertPicker::~nsCertPicker()
{
}

NS_IMETHODIMP nsCertPicker::PickByUsage(nsIInterfaceRequestor *ctx, 
                                        const char16_t *selectedNickname, 
                                        int32_t certUsage, 
                                        bool allowInvalid, 
                                        bool allowDuplicateNicknames, 
                                        bool *canceled, 
                                        nsIX509Cert **_retval)
{
  nsNSSShutDownPreventionLock locker;
  int32_t selectedIndex = -1;
  bool selectionFound = false;
  char16_t **certNicknameList = nullptr;
  char16_t **certDetailsList = nullptr;
  CERTCertListNode* node = nullptr;
  nsresult rv = NS_OK;

  {
    
    nsCOMPtr<nsIInterfaceRequestor> ctx = new PipUIContext();
    ScopedCERTCertList allcerts(PK11_ListCerts(PK11CertListUnique, ctx));
  }

  
  

  ScopedCERTCertList certList(
    CERT_FindUserCertsByUsage(CERT_GetDefaultCertDB(), 
                              (SECCertUsage)certUsage,
                              !allowDuplicateNicknames,
                              !allowInvalid,
                              ctx));
  if (!certList) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  ScopedCERTCertNicknames nicknames(getNSSCertNicknamesFromCertList(certList.get()));
  if (!nicknames) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  certNicknameList = (char16_t **)moz_xmalloc(sizeof(char16_t *) * nicknames->numnicknames);
  certDetailsList = (char16_t **)moz_xmalloc(sizeof(char16_t *) * nicknames->numnicknames);

  if (!certNicknameList || !certDetailsList) {
    free(certNicknameList);
    free(certDetailsList);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  int32_t CertsToUse;

  for (CertsToUse = 0, node = CERT_LIST_HEAD(certList.get());
       !CERT_LIST_END(node, certList.get()) &&
         CertsToUse < nicknames->numnicknames;
       node = CERT_LIST_NEXT(node)
      )
  {
    nsNSSCertificate *tempCert = nsNSSCertificate::Create(node->cert);

    if (tempCert) {

      
      

      NS_ADDREF(tempCert);

      nsAutoString i_nickname(NS_ConvertUTF8toUTF16(nicknames->nicknames[CertsToUse]));
      nsAutoString nickWithSerial;
      nsAutoString details;

      if (!selectionFound) {
        if (i_nickname == nsDependentString(selectedNickname)) {
          selectedIndex = CertsToUse;
          selectionFound = true;
        }
      }

      if (NS_SUCCEEDED(tempCert->FormatUIStrings(i_nickname, nickWithSerial, details))) {
        certNicknameList[CertsToUse] = ToNewUnicode(nickWithSerial);
        certDetailsList[CertsToUse] = ToNewUnicode(details);
      }
      else {
        certNicknameList[CertsToUse] = nullptr;
        certDetailsList[CertsToUse] = nullptr;
      }

      NS_RELEASE(tempCert);

      ++CertsToUse;
    }
  }

  if (CertsToUse) {
    nsICertPickDialogs *dialogs = nullptr;
    rv = getNSSDialogs((void**)&dialogs, 
      NS_GET_IID(nsICertPickDialogs), 
      NS_CERTPICKDIALOGS_CONTRACTID);

    if (NS_SUCCEEDED(rv)) {
      nsPSMUITracker tracker;
      if (tracker.isUIForbidden()) {
        rv = NS_ERROR_NOT_AVAILABLE;
      }
      else {
        
        rv = dialogs->PickCertificate(ctx,
          (const char16_t**)certNicknameList, (const char16_t**)certDetailsList,
          CertsToUse, &selectedIndex, canceled);
      }

      NS_RELEASE(dialogs);
    }
  }

  int32_t i;
  for (i = 0; i < CertsToUse; ++i) {
    free(certNicknameList[i]);
    free(certDetailsList[i]);
  }
  free(certNicknameList);
  free(certDetailsList);
  
  if (!CertsToUse) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (NS_SUCCEEDED(rv) && !*canceled) {
    for (i = 0, node = CERT_LIST_HEAD(certList);
         !CERT_LIST_END(node, certList);
         ++i, node = CERT_LIST_NEXT(node)) {

      if (i == selectedIndex) {
        nsNSSCertificate *cert = nsNSSCertificate::Create(node->cert);
        if (!cert) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        nsIX509Cert *x509 = 0;
        nsresult rv = cert->QueryInterface(NS_GET_IID(nsIX509Cert), (void**)&x509);
        if (NS_FAILED(rv)) {
          break;
        }

        NS_ADDREF(x509);
        *_retval = x509;
        NS_RELEASE(cert);
        break;
      }
    }
  }

  return rv;
}
