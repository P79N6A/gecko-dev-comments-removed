






#define CERT_AddTempCertToPerm __CERT_AddTempCertToPerm

#include "nsNSSComponent.h"
#include "nsNSSCertificateDB.h"

#include "CertVerifier.h"
#include "nsNSSComponent.h"
#include "mozilla/Base64.h"
#include "nsCOMPtr.h"
#include "nsNSSCertificate.h"
#include "nsNSSHelper.h"
#include "nsNSSCertHelper.h"
#include "nsNSSCertCache.h"
#include "nsCRT.h"
#include "nsICertificateDialogs.h"
#include "nsNSSCertTrust.h"
#include "nsIFile.h"
#include "nsPKCS12Blob.h"
#include "nsPK11TokenDB.h"
#include "nsReadableUtils.h"
#include "nsIMutableArray.h"
#include "nsArrayUtils.h"
#include "nsNSSShutDown.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsComponentManagerUtils.h"
#include "nsIPrompt.h"
#include "nsThreadUtils.h"
#include "nsIObserverService.h"
#include "nsRecentBadCerts.h"
#include "SharedSSLState.h"

#include "nspr.h"
#include "certdb.h"
#include "secerr.h"
#include "nssb64.h"
#include "secasn1.h"
#include "secder.h"
#include "ssl.h"
#include "ocsp.h"
#include "plbase64.h"

using namespace mozilla;
using namespace mozilla::psm;
using mozilla::psm::SharedSSLState;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);


NS_IMPL_ISUPPORTS2(nsNSSCertificateDB, nsIX509CertDB, nsIX509CertDB2)

nsNSSCertificateDB::nsNSSCertificateDB()
: mBadCertsLock("nsNSSCertificateDB::mBadCertsLock")
{
  SharedSSLState::NoteCertDBServiceInstantiated();
}

nsNSSCertificateDB::~nsNSSCertificateDB()
{
}

NS_IMETHODIMP
nsNSSCertificateDB::FindCertByNickname(nsISupports *aToken,
                                      const nsAString &nickname,
                                      nsIX509Cert **_rvCert)
{
  NS_ENSURE_ARG_POINTER(_rvCert);
  *_rvCert = nullptr;

  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  ScopedCERTCertificate cert;
  char *asciiname = nullptr;
  NS_ConvertUTF16toUTF8 aUtf8Nickname(nickname);
  asciiname = const_cast<char*>(aUtf8Nickname.get());
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Getting \"%s\"\n", asciiname));
#if 0
  
  if (aToken) {
    cert = PK11_FindCertFromNickname(asciiname, nullptr);
  } else {
    cert = CERT_FindCertByNickname(CERT_GetDefaultCertDB(), asciiname);
  }
#endif
  cert = PK11_FindCertFromNickname(asciiname, nullptr);
  if (!cert) {
    cert = CERT_FindCertByNickname(CERT_GetDefaultCertDB(), asciiname);
  }
  if (cert) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("got it\n"));
    nsCOMPtr<nsIX509Cert> pCert = nsNSSCertificate::Create(cert);
    if (pCert) {
      pCert.forget(_rvCert);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsNSSCertificateDB::FindCertByDBKey(const char *aDBkey, nsISupports *aToken,
                                   nsIX509Cert **_cert)
{
  NS_ENSURE_ARG_POINTER(aDBkey);
  NS_ENSURE_ARG(aDBkey[0]);
  NS_ENSURE_ARG_POINTER(_cert);
  *_cert = nullptr;

  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  SECItem keyItem = {siBuffer, nullptr, 0};
  SECItem *dummy;
  CERTIssuerAndSN issuerSN;
  

  dummy = NSSBase64_DecodeBuffer(nullptr, &keyItem, aDBkey,
                                 (uint32_t)strlen(aDBkey)); 
  if (!dummy || keyItem.len < NS_NSS_LONG*4) {
    PR_FREEIF(keyItem.data);
    return NS_ERROR_INVALID_ARG;
  }

  ScopedCERTCertificate cert;
  
  
  

  
  issuerSN.serialNumber.len = NS_NSS_GET_LONG(&keyItem.data[NS_NSS_LONG*2]);
  issuerSN.derIssuer.len = NS_NSS_GET_LONG(&keyItem.data[NS_NSS_LONG*3]);
  if (issuerSN.serialNumber.len == 0 || issuerSN.derIssuer.len == 0
      || issuerSN.serialNumber.len + issuerSN.derIssuer.len
         != keyItem.len - NS_NSS_LONG*4) {
    PR_FREEIF(keyItem.data);
    return NS_ERROR_INVALID_ARG;
  }
  issuerSN.serialNumber.data= &keyItem.data[NS_NSS_LONG*4];
  issuerSN.derIssuer.data= &keyItem.data[NS_NSS_LONG*4+
                                              issuerSN.serialNumber.len];

  cert = CERT_FindCertByIssuerAndSN(CERT_GetDefaultCertDB(), &issuerSN);
  PR_FREEIF(keyItem.data);
  if (cert) {
    nsCOMPtr<nsIX509Cert> nssCert = nsNSSCertificate::Create(cert);
    if (!nssCert)
      return NS_ERROR_OUT_OF_MEMORY;
    nssCert.forget(_cert);
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsNSSCertificateDB::FindCertNicknames(nsISupports *aToken, 
                                     uint32_t      aType,
                                     uint32_t     *_count,
                                     PRUnichar  ***_certNames)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult rv = NS_ERROR_FAILURE;
  


  ScopedCERTCertList certList;
  PK11CertListType pk11type;
#if 0
  
  
  if (aType == nsIX509Cert::USER_CERT)
    pk11type = PK11CertListUser;
  else 
#endif
    pk11type = PK11CertListUnique;
  certList = PK11_ListCerts(pk11type, nullptr);
  if (!certList)
    goto cleanup;
  



  getCertNames(certList, aType, _count, _certNames, locker);
  rv = NS_OK;
  


cleanup:
  return rv;
}

SECStatus
collect_certs(void *arg, SECItem **certs, int numcerts)
{
  CERTDERCerts *collectArgs;
  SECItem *cert;
  SECStatus rv;

  collectArgs = (CERTDERCerts *)arg;

  collectArgs->numcerts = numcerts;
  collectArgs->rawCerts = (SECItem *) PORT_ArenaZAlloc(collectArgs->arena,
                                           sizeof(SECItem) * numcerts);
  if (!collectArgs->rawCerts)
    return(SECFailure);

  cert = collectArgs->rawCerts;

  while ( numcerts-- ) {
    rv = SECITEM_CopyItem(collectArgs->arena, cert, *certs);
    if ( rv == SECFailure )
      return(SECFailure);
    cert++;
    certs++;
  }

  return (SECSuccess);
}

CERTDERCerts*
nsNSSCertificateDB::getCertsFromPackage(PLArenaPool *arena, uint8_t *data, 
                                        uint32_t length,
                                        const nsNSSShutDownPreventionLock &)
{
  CERTDERCerts *collectArgs = 
               (CERTDERCerts *)PORT_ArenaZAlloc(arena, sizeof(CERTDERCerts));
  if (!collectArgs)
    return nullptr;

  collectArgs->arena = arena;
  SECStatus sec_rv = CERT_DecodeCertPackage(reinterpret_cast<char *>(data), 
                                            length, collect_certs, 
                                            (void *)collectArgs);
  if (sec_rv != SECSuccess)
    return nullptr;

  return collectArgs;
}

nsresult
nsNSSCertificateDB::handleCACertDownload(nsIArray *x509Certs,
                                         nsIInterfaceRequestor *ctx,
                                         const nsNSSShutDownPreventionLock &proofOfLock)
{
  
  
  
  
  
  
  
  
  
  
  
  

  uint32_t numCerts;

  x509Certs->GetLength(&numCerts);
  NS_ASSERTION(numCerts > 0, "Didn't get any certs to import.");
  if (numCerts == 0)
    return NS_OK; 

  nsCOMPtr<nsIX509Cert> certToShow;
  nsCOMPtr<nsISupports> isupports;
  uint32_t selCertIndex;
  if (numCerts == 1) {
    
    selCertIndex = 0;
    certToShow = do_QueryElementAt(x509Certs, selCertIndex);
  } else {
    nsCOMPtr<nsIX509Cert> cert0;    
    nsCOMPtr<nsIX509Cert> cert1;    
    nsCOMPtr<nsIX509Cert> certn_2;  
    nsCOMPtr<nsIX509Cert> certn_1;  

    cert0 = do_QueryElementAt(x509Certs, 0);
    cert1 = do_QueryElementAt(x509Certs, 1);
    certn_2 = do_QueryElementAt(x509Certs, numCerts-2);
    certn_1 = do_QueryElementAt(x509Certs, numCerts-1);

    nsXPIDLString cert0SubjectName;
    nsXPIDLString cert1IssuerName;
    nsXPIDLString certn_2IssuerName;
    nsXPIDLString certn_1SubjectName;

    cert0->GetSubjectName(cert0SubjectName);
    cert1->GetIssuerName(cert1IssuerName);
    certn_2->GetIssuerName(certn_2IssuerName);
    certn_1->GetSubjectName(certn_1SubjectName);

    if (cert1IssuerName.Equals(cert0SubjectName)) {
      
      
      selCertIndex = 0;
      certToShow = cert0;
    } else 
    if (certn_2IssuerName.Equals(certn_1SubjectName)) { 
      
      
      selCertIndex = numCerts-1;
      certToShow = certn_1;
    } else {
      
      
      selCertIndex = 0;
      certToShow = cert0;
    }
  }

  if (!certToShow)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsICertificateDialogs> dialogs;
  nsresult rv = ::getNSSDialogs(getter_AddRefs(dialogs), 
                                NS_GET_IID(nsICertificateDialogs),
                                NS_CERTIFICATEDIALOGS_CONTRACTID);
                       
  if (NS_FAILED(rv))
    return rv;
 
  SECItem der;
  rv=certToShow->GetRawDER(&der.len, (uint8_t **)&der.data);

  if (NS_FAILED(rv))
    return rv;

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Creating temp cert\n"));
  ScopedCERTCertificate tmpCert;
  CERTCertDBHandle *certdb = CERT_GetDefaultCertDB();
  tmpCert = CERT_FindCertByDERCert(certdb, &der);
  if (!tmpCert) {
    tmpCert = CERT_NewTempCertificate(certdb, &der,
                                      nullptr, false, true);
  }
  nsMemory::Free(der.data);
  der.data = nullptr;
  der.len = 0;
  
  if (!tmpCert) {
    NS_ERROR("Couldn't create cert from DER blob");
    return NS_ERROR_FAILURE;
  }

  if (!CERT_IsCACert(tmpCert, nullptr)) {
    DisplayCertificateAlert(ctx, "NotACACert", certToShow, proofOfLock);
    return NS_ERROR_FAILURE;
  }

  if (tmpCert->isperm) {
    DisplayCertificateAlert(ctx, "CaCertExists", certToShow, proofOfLock);
    return NS_ERROR_FAILURE;
  }

  uint32_t trustBits;
  bool allows;
  rv = dialogs->ConfirmDownloadCACert(ctx, certToShow, &trustBits, &allows);
  if (NS_FAILED(rv))
    return rv;

  if (!allows)
    return NS_ERROR_NOT_AVAILABLE;

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("trust is %d\n", trustBits));
  nsXPIDLCString nickname;
  nickname.Adopt(CERT_MakeCANickname(tmpCert));

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Created nick \"%s\"\n", nickname.get()));

  nsNSSCertTrust trust;
  trust.SetValidCA();
  trust.AddCATrust(!!(trustBits & nsIX509CertDB::TRUSTED_SSL),
                   !!(trustBits & nsIX509CertDB::TRUSTED_EMAIL),
                   !!(trustBits & nsIX509CertDB::TRUSTED_OBJSIGN));

  SECStatus srv = __CERT_AddTempCertToPerm(tmpCert,
                                           const_cast<char*>(nickname.get()),
                                           trust.GetTrust());

  if (srv != SECSuccess)
    return NS_ERROR_FAILURE;

  

  
  ScopedCERTCertList certList(CERT_NewCertList());
  if (!certList) {
    return NS_ERROR_FAILURE;
  }

  

  for (uint32_t i=0; i<numCerts; i++) {
    if (i == selCertIndex) {
      
      continue;
    }

    certToShow = do_QueryElementAt(x509Certs, i);
    certToShow->GetRawDER(&der.len, (uint8_t **)&der.data);

    CERTCertificate *tmpCert2 = 
      CERT_NewTempCertificate(certdb, &der, nullptr, false, true);

    nsMemory::Free(der.data);
    der.data = nullptr;
    der.len = 0;

    if (!tmpCert2) {
      NS_ERROR("Couldn't create temp cert from DER blob");
      continue;  
    }
    
    CERT_AddCertToListTail(certList, tmpCert2);
  }

  return ImportValidCACertsInList(certList, ctx, proofOfLock);
}






NS_IMETHODIMP 
nsNSSCertificateDB::ImportCertificates(uint8_t * data, uint32_t length, 
                                       uint32_t type, 
                                       nsIInterfaceRequestor *ctx)

{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult nsrv;

  PLArenaPool *arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  if (!arena)
    return NS_ERROR_OUT_OF_MEMORY;

  CERTDERCerts *certCollection = getCertsFromPackage(arena, data, length, locker);
  if (!certCollection) {
    PORT_FreeArena(arena, false);
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsIMutableArray> array =
    do_CreateInstance(NS_ARRAY_CONTRACTID, &nsrv);
  if (NS_FAILED(nsrv)) {
    PORT_FreeArena(arena, false);
    return nsrv;
  }

  
  nsCOMPtr<nsIX509Cert> x509Cert;
  nsNSSCertificate *nssCert;
  SECItem *currItem;
  for (int i=0; i<certCollection->numcerts; i++) {
     currItem = &certCollection->rawCerts[i];
     nssCert = nsNSSCertificate::ConstructFromDER((char*)currItem->data, currItem->len);
     if (!nssCert)
       return NS_ERROR_FAILURE;
     x509Cert = do_QueryInterface((nsIX509Cert*)nssCert);
     array->AppendElement(x509Cert, false);
  }
  switch (type) {
  case nsIX509Cert::CA_CERT:
    nsrv = handleCACertDownload(array, ctx, locker);
    break;
  default:
    
     nsrv = NS_ERROR_FAILURE;
     break;
  }  
  PORT_FreeArena(arena, false);
  return nsrv;
}

static 
SECStatus 
ImportCertsIntoPermanentStorage(const ScopedCERTCertList &certChain, const SECCertUsage usage,
                               const PRBool caOnly)
{
  CERTCertDBHandle *certdb = CERT_GetDefaultCertDB();

  int chainLen = 0;
  for (CERTCertListNode *chainNode = CERT_LIST_HEAD(certChain);
       !CERT_LIST_END(chainNode, certChain);
       chainNode = CERT_LIST_NEXT(chainNode)) {
    chainLen++;
  }

  SECItem **rawArray;
  rawArray = (SECItem **) PORT_Alloc(chainLen * sizeof(SECItem *));
  if (!rawArray) {
    return SECFailure;
  }

  int i = 0;
  for (CERTCertListNode *chainNode = CERT_LIST_HEAD(certChain);
       !CERT_LIST_END(chainNode, certChain);
       chainNode = CERT_LIST_NEXT(chainNode), i++) {
    rawArray[i] = &chainNode->cert->derCert;
  }
  SECStatus srv = CERT_ImportCerts(certdb, usage, chainLen, rawArray,
                                   nullptr, true, caOnly, nullptr);

  PORT_Free(rawArray);
  return srv;
} 






NS_IMETHODIMP
nsNSSCertificateDB::ImportEmailCertificate(uint8_t * data, uint32_t length, 
                                       nsIInterfaceRequestor *ctx)

{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  SECStatus srv = SECFailure;
  nsresult nsrv = NS_OK;
  CERTCertDBHandle *certdb;
  CERTCertificate **certArray = nullptr;
  ScopedCERTCertList certList;
  CERTCertListNode *node;
  SECItem **rawArray;
  int numcerts;
  int i;

  PLArenaPool *arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  if (!arena)
    return NS_ERROR_OUT_OF_MEMORY;

  CERTDERCerts *certCollection = getCertsFromPackage(arena, data, length, locker);
  if (!certCollection) {
    PORT_FreeArena(arena, false);
    return NS_ERROR_FAILURE;
  }

  RefPtr<CertVerifier> certVerifier(GetDefaultCertVerifier());
  NS_ENSURE_TRUE(certVerifier, NS_ERROR_UNEXPECTED);

  certdb = CERT_GetDefaultCertDB();
  const PRTime now = PR_Now();

  numcerts = certCollection->numcerts;

  rawArray = (SECItem **) PORT_Alloc(sizeof(SECItem *) * numcerts);
  if ( !rawArray ) {
    nsrv = NS_ERROR_FAILURE;
    goto loser;
  }

  for (i=0; i < numcerts; i++) {
    rawArray[i] = &certCollection->rawCerts[i];
  }

  srv = CERT_ImportCerts(certdb, certUsageEmailRecipient, numcerts, rawArray,
                         &certArray, false, false, nullptr);

  PORT_Free(rawArray);
  rawArray = nullptr;

  if (srv != SECSuccess) {
    nsrv = NS_ERROR_FAILURE;
    goto loser;
  }

  
  certList = CERT_NewCertList();
  if (!certList) {
    nsrv = NS_ERROR_FAILURE;
    goto loser;
  }
  for (i=0; i < numcerts; i++) {
    CERTCertificate *cert = certArray[i];
    if (cert)
      cert = CERT_DupCertificate(cert);
    if (cert)
      CERT_AddCertToListTail(certList, cert);
  }

  



  for (node = CERT_LIST_HEAD(certList);
       !CERT_LIST_END(node,certList);
       node = CERT_LIST_NEXT(node)) {

    if (!node->cert) {
      continue;
    }

    CERTCertList *verifyCertChain = nullptr;

    SECStatus rv = certVerifier->VerifyCert(node->cert,
                                            certificateUsageEmailRecipient,
                                            now, ctx, 0, &verifyCertChain);

    ScopedCERTCertList certChain(verifyCertChain);

    if (rv != SECSuccess) {
      nsCOMPtr<nsIX509Cert> certToShow = nsNSSCertificate::Create(node->cert);
      DisplayCertificateAlert(ctx, "NotImportingUnverifiedCert", certToShow, locker);
      continue;
    }
    rv = ImportCertsIntoPermanentStorage(certChain, certUsageEmailRecipient, false);
    if (rv != SECSuccess) {
      goto loser;
    } 
    CERT_SaveSMimeProfile(node->cert, nullptr, nullptr);

  }

loser:
  if (certArray) {
    CERT_DestroyCertArray(certArray, numcerts);
  }
  if (arena) 
    PORT_FreeArena(arena, true);
  return nsrv;
}

NS_IMETHODIMP
nsNSSCertificateDB::ImportServerCertificate(uint8_t * data, uint32_t length, 
                                            nsIInterfaceRequestor *ctx)

{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  SECStatus srv = SECFailure;
  nsresult nsrv = NS_OK;
  ScopedCERTCertificate cert;
  SECItem **rawCerts = nullptr;
  int numcerts;
  int i;
  nsNSSCertTrust trust;
  char *serverNickname = nullptr;
 
  PLArenaPool *arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  if (!arena)
    return NS_ERROR_OUT_OF_MEMORY;

  CERTDERCerts *certCollection = getCertsFromPackage(arena, data, length, locker);
  if (!certCollection) {
    PORT_FreeArena(arena, false);
    return NS_ERROR_FAILURE;
  }
  cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(), certCollection->rawCerts,
                                 nullptr, false, true);
  if (!cert) {
    nsrv = NS_ERROR_FAILURE;
    goto loser;
  }
  numcerts = certCollection->numcerts;
  rawCerts = (SECItem **) PORT_Alloc(sizeof(SECItem *) * numcerts);
  if ( !rawCerts ) {
    nsrv = NS_ERROR_FAILURE;
    goto loser;
  }

  for ( i = 0; i < numcerts; i++ ) {
    rawCerts[i] = &certCollection->rawCerts[i];
  }

  serverNickname = nsNSSCertificate::defaultServerNickname(cert);
  srv = CERT_ImportCerts(CERT_GetDefaultCertDB(), certUsageSSLServer,
             numcerts, rawCerts, nullptr, true, false,
             serverNickname);
  PR_FREEIF(serverNickname);
  if ( srv != SECSuccess ) {
    nsrv = NS_ERROR_FAILURE;
    goto loser;
  }

  trust.SetValidServerPeer();
  srv = CERT_ChangeCertTrust(CERT_GetDefaultCertDB(), cert, trust.GetTrust());
  if ( srv != SECSuccess ) {
    nsrv = NS_ERROR_FAILURE;
    goto loser;
  }
loser:
  PORT_Free(rawCerts);
  if (arena) 
    PORT_FreeArena(arena, true);
  return nsrv;
}

nsresult
nsNSSCertificateDB::ImportValidCACerts(int numCACerts, SECItem *CACerts, nsIInterfaceRequestor *ctx,  const nsNSSShutDownPreventionLock &proofOfLock)
{
  ScopedCERTCertList certList;
  SECItem **rawArray;

  
  certList = CERT_NewCertList();
  if (!certList) {
    return NS_ERROR_FAILURE;
  }

  
  SECStatus srv = SECFailure;
  CERTCertificate **certArray = nullptr;

  rawArray = (SECItem **) PORT_Alloc(sizeof(SECItem *) * numCACerts);
  if ( !rawArray ) {
    return NS_ERROR_FAILURE;
  }

  for (int i=0; i < numCACerts; i++) {
    rawArray[i] = &CACerts[i];
  }

  srv = CERT_ImportCerts(CERT_GetDefaultCertDB(), certUsageAnyCA, numCACerts, rawArray, 
                         &certArray, false, true, nullptr);

  PORT_Free(rawArray);
  rawArray = nullptr;

  if (srv != SECSuccess) {
    return NS_ERROR_FAILURE;
  }

  for (int i2=0; i2 < numCACerts; i2++) {
    CERTCertificate *cacert = certArray[i2];
    if (cacert)
      cacert = CERT_DupCertificate(cacert);
    if (cacert)
      CERT_AddCertToListTail(certList, cacert);
  }

  CERT_DestroyCertArray(certArray, numCACerts);

  return ImportValidCACertsInList(certList, ctx, proofOfLock);
}

nsresult
nsNSSCertificateDB::ImportValidCACertsInList(CERTCertList *certList, nsIInterfaceRequestor *ctx,
                                             const nsNSSShutDownPreventionLock &proofOfLock)
{
  RefPtr<CertVerifier> certVerifier(GetDefaultCertVerifier());
  if (!certVerifier)
    return NS_ERROR_UNEXPECTED;

  
  SECStatus srv = CERT_FilterCertListByUsage(certList, certUsageAnyCA, true);
  if (srv != SECSuccess) {
    return NS_ERROR_FAILURE;
  }

  


  CERTCertListNode *node;

  for (node = CERT_LIST_HEAD(certList);
       !CERT_LIST_END(node,certList);
       node = CERT_LIST_NEXT(node)) {

    
    CERTCertList *verifyCertChain = nullptr;

    SECStatus rv = certVerifier->VerifyCert(node->cert, certificateUsageVerifyCA,
                                            PR_Now(), ctx, 0, &verifyCertChain);

    ScopedCERTCertList certChain(verifyCertChain);

    if (rv != SECSuccess) {
      nsCOMPtr<nsIX509Cert> certToShow = nsNSSCertificate::Create(node->cert);
      DisplayCertificateAlert(ctx, "NotImportingUnverifiedCert", certToShow, proofOfLock);
      continue;
    }

    rv = ImportCertsIntoPermanentStorage(certChain, certUsageAnyCA, true);
    if (rv != SECSuccess) {
      return NS_ERROR_FAILURE;
    }
  }
  
  return NS_OK;
}

void nsNSSCertificateDB::DisplayCertificateAlert(nsIInterfaceRequestor *ctx, 
                                                 const char *stringID, 
                                                 nsIX509Cert *certToShow,
                                                 const nsNSSShutDownPreventionLock &)
{
  if (!NS_IsMainThread()) {
    NS_ERROR("nsNSSCertificateDB::DisplayCertificateAlert called off the main thread");
    return;
  }

  nsPSMUITracker tracker;
  if (!tracker.isUIForbidden()) {

    nsCOMPtr<nsIInterfaceRequestor> my_ctx = ctx;
    if (!my_ctx)
      my_ctx = new PipUIContext();

    
    

    nsresult rv;
    nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
    if (NS_SUCCEEDED(rv)) {
      nsAutoString tmpMessage;
      nssComponent->GetPIPNSSBundleString(stringID, tmpMessage);

      nsCOMPtr<nsIPrompt> prompt (do_GetInterface(my_ctx));
      if (!prompt)
        return;
    
      prompt->Alert(nullptr, tmpMessage.get());
    }
  }
}


NS_IMETHODIMP 
nsNSSCertificateDB::ImportUserCertificate(uint8_t *data, uint32_t length, nsIInterfaceRequestor *ctx)
{
  if (!NS_IsMainThread()) {
    NS_ERROR("nsNSSCertificateDB::ImportUserCertificate called off the main thread");
    return NS_ERROR_NOT_SAME_THREAD;
  }
  
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  ScopedPK11SlotInfo slot;
  nsAutoCString nickname;
  nsresult rv = NS_ERROR_FAILURE;
  int numCACerts;
  SECItem *CACerts;
  CERTDERCerts * collectArgs;
  PLArenaPool *arena;
  ScopedCERTCertificate cert;

  arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
  if (!arena) {
    goto loser;
  }

  collectArgs = getCertsFromPackage(arena, data, length, locker);
  if (!collectArgs) {
    goto loser;
  }

  cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(), collectArgs->rawCerts,
                                 nullptr, false, true);
  if (!cert) {
    goto loser;
  }

  slot = PK11_KeyForCertExists(cert, nullptr, ctx);
  if (!slot) {
    nsCOMPtr<nsIX509Cert> certToShow = nsNSSCertificate::Create(cert);
    DisplayCertificateAlert(ctx, "UserCertIgnoredNoPrivateKey", certToShow, locker);
    goto loser;
  }
  slot = nullptr;

  
  if (cert->nickname) {
	


  	nickname = cert->nickname;
  }
  else {
    get_default_nickname(cert, ctx, nickname, locker);
  }

  
  {
    char *cast_const_away = const_cast<char*>(nickname.get());
    slot = PK11_ImportCertForKey(cert, cast_const_away, ctx);
  }
  if (!slot) {
    goto loser;
  }
  slot = nullptr;

  {
    nsCOMPtr<nsIX509Cert> certToShow = nsNSSCertificate::Create(cert);
    DisplayCertificateAlert(ctx, "UserCertImported", certToShow, locker);
  }
  rv = NS_OK;

  numCACerts = collectArgs->numcerts - 1;
  if (numCACerts) {
    CACerts = collectArgs->rawCerts+1;
    rv = ImportValidCACerts(numCACerts, CACerts, ctx, locker);
  }
  
loser:
  if (arena) {
    PORT_FreeArena(arena, false);
  }
  return rv;
}




NS_IMETHODIMP 
nsNSSCertificateDB::DeleteCertificate(nsIX509Cert *aCert)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  nsCOMPtr<nsIX509Cert2> nssCert = do_QueryInterface(aCert);
  ScopedCERTCertificate cert(nssCert->GetCert());
  if (!cert) return NS_ERROR_FAILURE;
  SECStatus srv = SECSuccess;

  uint32_t certType;
  nssCert->GetCertType(&certType);
  if (NS_FAILED(nssCert->MarkForPermDeletion()))
  {
    return NS_ERROR_FAILURE;
  }

  if (cert->slot && certType != nsIX509Cert::USER_CERT) {
    
    
    
    
    
    
    
    nsNSSCertTrust trust(0, 0, 0);
    srv = CERT_ChangeCertTrust(CERT_GetDefaultCertDB(), 
                               cert, trust.GetTrust());
  }
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("cert deleted: %d", srv));
  return (srv) ? NS_ERROR_FAILURE : NS_OK;
}






NS_IMETHODIMP 
nsNSSCertificateDB::SetCertTrust(nsIX509Cert *cert, 
                                 uint32_t type,
                                 uint32_t trusted)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  SECStatus srv;
  nsNSSCertTrust trust;
  nsCOMPtr<nsIX509Cert2> pipCert = do_QueryInterface(cert);
  if (!pipCert)
    return NS_ERROR_FAILURE;
  ScopedCERTCertificate nsscert(pipCert->GetCert());

  if (type == nsIX509Cert::CA_CERT) {
    
    trust.SetValidCA();
    trust.AddCATrust(!!(trusted & nsIX509CertDB::TRUSTED_SSL),
                     !!(trusted & nsIX509CertDB::TRUSTED_EMAIL),
                     !!(trusted & nsIX509CertDB::TRUSTED_OBJSIGN));
    srv = CERT_ChangeCertTrust(CERT_GetDefaultCertDB(), 
                               nsscert,
                               trust.GetTrust());
  } else if (type == nsIX509Cert::SERVER_CERT) {
    
    trust.SetValidPeer();
    trust.AddPeerTrust(trusted & nsIX509CertDB::TRUSTED_SSL, 0, 0);
    srv = CERT_ChangeCertTrust(CERT_GetDefaultCertDB(), 
                               nsscert,
                               trust.GetTrust());
  } else if (type == nsIX509Cert::EMAIL_CERT) {
    
    trust.SetValidPeer();
    trust.AddPeerTrust(0, !!(trusted & nsIX509CertDB::TRUSTED_EMAIL), 0);
    srv = CERT_ChangeCertTrust(CERT_GetDefaultCertDB(), 
                               nsscert,
                               trust.GetTrust());
  } else {
    
    return NS_OK;
  }
  return (srv) ? NS_ERROR_FAILURE : NS_OK;
}

NS_IMETHODIMP 
nsNSSCertificateDB::IsCertTrusted(nsIX509Cert *cert, 
                                  uint32_t certType,
                                  uint32_t trustType,
                                  bool *_isTrusted)
{
  NS_ENSURE_ARG_POINTER(_isTrusted);
  *_isTrusted = false;

  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  SECStatus srv;
  nsCOMPtr<nsIX509Cert2> pipCert = do_QueryInterface(cert);
  ScopedCERTCertificate nsscert(pipCert->GetCert());
  CERTCertTrust nsstrust;
  srv = CERT_GetCertTrust(nsscert, &nsstrust);
  if (srv != SECSuccess)
    return NS_ERROR_FAILURE;

  nsNSSCertTrust trust(&nsstrust);
  if (certType == nsIX509Cert::CA_CERT) {
    if (trustType & nsIX509CertDB::TRUSTED_SSL) {
      *_isTrusted = trust.HasTrustedCA(true, false, false);
    } else if (trustType & nsIX509CertDB::TRUSTED_EMAIL) {
      *_isTrusted = trust.HasTrustedCA(false, true, false);
    } else if (trustType & nsIX509CertDB::TRUSTED_OBJSIGN) {
      *_isTrusted = trust.HasTrustedCA(false, false, true);
    } else {
      return NS_ERROR_FAILURE;
    }
  } else if (certType == nsIX509Cert::SERVER_CERT) {
    if (trustType & nsIX509CertDB::TRUSTED_SSL) {
      *_isTrusted = trust.HasTrustedPeer(true, false, false);
    } else if (trustType & nsIX509CertDB::TRUSTED_EMAIL) {
      *_isTrusted = trust.HasTrustedPeer(false, true, false);
    } else if (trustType & nsIX509CertDB::TRUSTED_OBJSIGN) {
      *_isTrusted = trust.HasTrustedPeer(false, false, true);
    } else {
      return NS_ERROR_FAILURE;
    }
  } else if (certType == nsIX509Cert::EMAIL_CERT) {
    if (trustType & nsIX509CertDB::TRUSTED_SSL) {
      *_isTrusted = trust.HasTrustedPeer(true, false, false);
    } else if (trustType & nsIX509CertDB::TRUSTED_EMAIL) {
      *_isTrusted = trust.HasTrustedPeer(false, true, false);
    } else if (trustType & nsIX509CertDB::TRUSTED_OBJSIGN) {
      *_isTrusted = trust.HasTrustedPeer(false, false, true);
    } else {
      return NS_ERROR_FAILURE;
    }
  } 
  return NS_OK;
}


NS_IMETHODIMP 
nsNSSCertificateDB::ImportCertsFromFile(nsISupports *aToken, 
                                        nsIFile *aFile,
                                        uint32_t aType)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  NS_ENSURE_ARG(aFile);
  switch (aType) {
    case nsIX509Cert::CA_CERT:
    case nsIX509Cert::EMAIL_CERT:
    case nsIX509Cert::SERVER_CERT:
      
      break;
    
    default:
      
      return NS_ERROR_FAILURE;
  }

  nsresult rv;
  PRFileDesc *fd = nullptr;

  rv = aFile->OpenNSPRFileDesc(PR_RDONLY, 0, &fd);

  if (NS_FAILED(rv))
    return rv;

  if (!fd)
    return NS_ERROR_FAILURE;

  PRFileInfo file_info;
  if (PR_SUCCESS != PR_GetOpenFileInfo(fd, &file_info))
    return NS_ERROR_FAILURE;
  
  unsigned char *buf = new unsigned char[file_info.size];
  
  int32_t bytes_obtained = PR_Read(fd, buf, file_info.size);
  PR_Close(fd);
  
  if (bytes_obtained != file_info.size)
    rv = NS_ERROR_FAILURE;
  else {
	  nsCOMPtr<nsIInterfaceRequestor> cxt = new PipUIContext();

    switch (aType) {
      case nsIX509Cert::CA_CERT:
        rv = ImportCertificates(buf, bytes_obtained, aType, cxt);
        break;
        
      case nsIX509Cert::SERVER_CERT:
        rv = ImportServerCertificate(buf, bytes_obtained, cxt);
        break;

      case nsIX509Cert::EMAIL_CERT:
        rv = ImportEmailCertificate(buf, bytes_obtained, cxt);
        break;
      
      default:
        break;
    }
  }

  delete [] buf;
  return rv;  
}

NS_IMETHODIMP 
nsNSSCertificateDB::ImportPKCS12File(nsISupports *aToken, 
                                     nsIFile *aFile)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  NS_ENSURE_ARG(aFile);
  nsPKCS12Blob blob;
  nsCOMPtr<nsIPK11Token> token = do_QueryInterface(aToken);
  if (token) {
    blob.SetToken(token);
  }
  return blob.ImportFromFile(aFile);
}

NS_IMETHODIMP 
nsNSSCertificateDB::ExportPKCS12File(nsISupports     *aToken, 
                                     nsIFile          *aFile,
                                     uint32_t          count,
                                     nsIX509Cert     **certs)
                                     
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  NS_ENSURE_ARG(aFile);
  nsPKCS12Blob blob;
  if (count == 0) return NS_OK;
  nsCOMPtr<nsIPK11Token> localRef;
  if (!aToken) {
    ScopedPK11SlotInfo keySlot(PK11_GetInternalKeySlot());
    NS_ASSERTION(keySlot,"Failed to get the internal key slot");
    localRef = new nsPK11Token(keySlot);
  }
  else {
    localRef = do_QueryInterface(aToken);
  }
  blob.SetToken(localRef);
  
  
  return blob.ExportToFile(aFile, certs, count);
}





#define DELIM '\001'






void
nsNSSCertificateDB::getCertNames(CERTCertList *certList,
                                 uint32_t      type, 
                                 uint32_t     *_count,
                                 PRUnichar  ***_certNames,
                                 const nsNSSShutDownPreventionLock &)
{
  CERTCertListNode *node;
  uint32_t numcerts = 0, i=0;
  PRUnichar **tmpArray = nullptr;

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("List of certs %d:\n", type));
  for (node = CERT_LIST_HEAD(certList);
       !CERT_LIST_END(node, certList);
       node = CERT_LIST_NEXT(node)) {
    if (getCertType(node->cert) == type) {
      numcerts++;
    }
  }
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("num certs: %d\n", numcerts));
  int nc = (numcerts == 0) ? 1 : numcerts;
  tmpArray = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *) * nc);
  if (numcerts == 0) goto finish;
  for (node = CERT_LIST_HEAD(certList);
       !CERT_LIST_END(node, certList);
       node = CERT_LIST_NEXT(node)) {
    if (getCertType(node->cert) == type) {
      nsNSSCertificate pipCert(node->cert);
      char *dbkey = nullptr;
      char *namestr = nullptr;
      nsAutoString certstr;
      pipCert.GetDbKey(&dbkey);
      nsAutoString keystr = NS_ConvertASCIItoUTF16(dbkey);
      PR_FREEIF(dbkey);
      if (type == nsIX509Cert::EMAIL_CERT) {
        namestr = node->cert->emailAddr;
      } else {
        namestr = node->cert->nickname;
        if (namestr) {
          char *sc = strchr(namestr, ':');
          if (sc) *sc = DELIM;
        }
      }
      nsAutoString certname = NS_ConvertASCIItoUTF16(namestr ? namestr : "");
      certstr.Append(PRUnichar(DELIM));
      certstr += certname;
      certstr.Append(PRUnichar(DELIM));
      certstr += keystr;
      tmpArray[i++] = ToNewUnicode(certstr);
    }
  }
finish:
  *_count = numcerts;
  *_certNames = tmpArray;
}


NS_IMETHODIMP
nsNSSCertificateDB::FindEmailEncryptionCert(const nsAString &aNickname, nsIX509Cert **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nullptr;

  if (aNickname.IsEmpty())
    return NS_OK;

  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIInterfaceRequestor> ctx = new PipUIContext();
  char *asciiname = nullptr;
  NS_ConvertUTF16toUTF8 aUtf8Nickname(aNickname);
  asciiname = const_cast<char*>(aUtf8Nickname.get());

  
  ScopedCERTCertificate cert;
  cert = CERT_FindUserCertByUsage(CERT_GetDefaultCertDB(), asciiname, 
           certUsageEmailRecipient, true, ctx);
  if (!cert) {
    return NS_OK;
  }

  nsCOMPtr<nsIX509Cert> nssCert = nsNSSCertificate::Create(cert);
  if (!nssCert) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nssCert.forget(_retval);
  return NS_OK;
}


NS_IMETHODIMP
nsNSSCertificateDB::FindEmailSigningCert(const nsAString &aNickname, nsIX509Cert **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nullptr;

  if (aNickname.IsEmpty())
    return NS_OK;

  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  ScopedCERTCertificate cert;
  nsCOMPtr<nsIInterfaceRequestor> ctx = new PipUIContext();
  char *asciiname = nullptr;
  NS_ConvertUTF16toUTF8 aUtf8Nickname(aNickname);
  asciiname = const_cast<char*>(aUtf8Nickname.get());

  
  cert = CERT_FindUserCertByUsage(CERT_GetDefaultCertDB(), asciiname, 
           certUsageEmailSigner, true, ctx);
  if (!cert) {
    return NS_OK;
  }

  nsCOMPtr<nsIX509Cert> nssCert = nsNSSCertificate::Create(cert);
  if (!nssCert) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nssCert.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificateDB::FindCertByEmailAddress(nsISupports *aToken, const char *aEmailAddress, nsIX509Cert **_retval)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  
  RefPtr<CertVerifier> certVerifier(GetDefaultCertVerifier());
  NS_ENSURE_TRUE(certVerifier, NS_ERROR_UNEXPECTED);

  ScopedCERTCertList certlist(
      PK11_FindCertsFromEmailAddress(aEmailAddress, nullptr));
  if (!certlist)
    return NS_ERROR_FAILURE;  

  
  

  if (CERT_LIST_END(CERT_LIST_HEAD(certlist), certlist))
    return NS_ERROR_FAILURE; 

  CERTCertListNode *node;
  
  for (node = CERT_LIST_HEAD(certlist);
       !CERT_LIST_END(node, certlist);
       node = CERT_LIST_NEXT(node)) {

    SECStatus srv = certVerifier->VerifyCert(node->cert,
                                             certificateUsageEmailRecipient,
                                             PR_Now(), nullptr );
    if (srv == SECSuccess) {
      break;
    }
  }

  if (CERT_LIST_END(node, certlist)) {
    
    return NS_ERROR_FAILURE;
  }

  
  nsNSSCertificate *nssCert = nsNSSCertificate::Create(node->cert);
  if (!nssCert)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(nssCert);
  *_retval = static_cast<nsIX509Cert*>(nssCert);
  return NS_OK;
}


NS_IMETHODIMP
nsNSSCertificateDB::ConstructX509FromBase64(const char *base64,
                                            nsIX509Cert **_retval)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  NS_ENSURE_ARG_POINTER(_retval);

  
  
  uint32_t len = base64 ? strlen(base64) : 0;
  char *certDER = PL_Base64Decode(base64, len, nullptr);
  if (!certDER)
    return NS_ERROR_ILLEGAL_VALUE;
  if (!*certDER) {
    PL_strfree(certDER);
    return NS_ERROR_ILLEGAL_VALUE;
  }

  
  
  
  uint32_t lengthDER = (len * 3) / 4;
  if (base64[len-1] == '=') {
    lengthDER--;
    if (base64[len-2] == '=')
      lengthDER--;
  }


  SECItem secitem_cert;
  secitem_cert.type = siDERCertBuffer;
  secitem_cert.data = (unsigned char*)certDER;
  secitem_cert.len = lengthDER;

  ScopedCERTCertificate cert;
  cert =
    CERT_NewTempCertificate(CERT_GetDefaultCertDB(), &secitem_cert,
                            nullptr, false, true);
  PL_strfree(certDER);

  if (!cert)
    return (PORT_GetError() == SEC_ERROR_NO_MEMORY)
      ? NS_ERROR_OUT_OF_MEMORY : NS_ERROR_FAILURE;

  nsCOMPtr<nsIX509Cert> nssCert = nsNSSCertificate::Create(cert);
  if (!nssCert) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nssCert.forget(_retval);
  return NS_OK;
}

void
nsNSSCertificateDB::get_default_nickname(CERTCertificate *cert, 
                                         nsIInterfaceRequestor* ctx,
                                         nsCString &nickname,
                                         const nsNSSShutDownPreventionLock &)
{
  nickname.Truncate();

  nsresult rv;
  CK_OBJECT_HANDLE keyHandle;

  CERTCertDBHandle *defaultcertdb = CERT_GetDefaultCertDB();
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv))
    return;

  nsAutoCString username;
  char *temp_un = CERT_GetCommonName(&cert->subject);
  if (temp_un) {
    username = temp_un;
    PORT_Free(temp_un);
    temp_un = nullptr;
  }

  nsAutoCString caname;
  char *temp_ca = CERT_GetOrgName(&cert->issuer);
  if (temp_ca) {
    caname = temp_ca;
    PORT_Free(temp_ca);
    temp_ca = nullptr;
  }

  nsAutoString tmpNickFmt;
  nssComponent->GetPIPNSSBundleString("nick_template", tmpNickFmt);
  NS_ConvertUTF16toUTF8 nickFmt(tmpNickFmt);

  nsAutoCString baseName;
  char *temp_nn = PR_smprintf(nickFmt.get(), username.get(), caname.get());
  if (!temp_nn) {
    return;
  } else {
    baseName = temp_nn;
    PR_smprintf_free(temp_nn);
    temp_nn = nullptr;
  }

  nickname = baseName;

  




  ScopedPK11SlotInfo slot(PK11_KeyForCertExists(cert, &keyHandle, ctx));
  if (!slot)
    return;

  if (!PK11_IsInternal(slot)) {
    char *tmp = PR_smprintf("%s:%s", PK11_GetTokenName(slot), baseName.get());
    if (!tmp) {
      nickname.Truncate();
      return;
    }
    baseName = tmp;
    PR_smprintf_free(tmp);

    nickname = baseName;
  }

  int count = 1;
  while (true) {
    if ( count > 1 ) {
      char *tmp = PR_smprintf("%s #%d", baseName.get(), count);
      if (!tmp) {
        nickname.Truncate();
        return;
      }
      nickname = tmp;
      PR_smprintf_free(tmp);
    }

    ScopedCERTCertificate dummycert;

    if (PK11_IsInternal(slot)) {
      
      dummycert = CERT_FindCertByNickname(defaultcertdb, nickname.get());

    } else {
      



      dummycert = PK11_FindCertFromNickname(nickname.get(), ctx);
      if (dummycert) {
	

 
	if (CERT_CompareName(&cert->subject, &dummycert->subject) == SECEqual)
	{
	  




	  dummycert = nullptr;
	}
      }
    }
    if (!dummycert) 
      break;
    
    count++;
  }
}

NS_IMETHODIMP nsNSSCertificateDB::AddCertFromBase64(const char *aBase64, const char *aTrust, const char *aName)
{
  NS_ENSURE_ARG_POINTER(aBase64);
  nsCOMPtr <nsIX509Cert> newCert;

  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsNSSCertTrust trust;

  
  SECStatus stat = CERT_DecodeTrustString(trust.GetTrust(),
    (char *) aTrust);
  NS_ENSURE_STATE(stat == SECSuccess); 


  nsresult rv = ConstructX509FromBase64(aBase64, getter_AddRefs(newCert));
  NS_ENSURE_SUCCESS(rv, rv);

  SECItem der;
  rv = newCert->GetRawDER(&der.len, (uint8_t **)&der.data);
  NS_ENSURE_SUCCESS(rv, rv);

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Creating temp cert\n"));
  CERTCertDBHandle *certdb = CERT_GetDefaultCertDB();
  ScopedCERTCertificate tmpCert(CERT_FindCertByDERCert(certdb, &der));
  if (!tmpCert) 
    tmpCert = CERT_NewTempCertificate(certdb, &der,
                                      nullptr, false, true);
  nsMemory::Free(der.data);
  der.data = nullptr;
  der.len = 0;

  if (!tmpCert) {
    NS_ERROR("Couldn't create cert from DER blob");
    return NS_ERROR_FAILURE;
  }

  if (tmpCert->isperm) {
    return NS_OK;
  }

  nsXPIDLCString nickname;
  nickname.Adopt(CERT_MakeCANickname(tmpCert));

  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("Created nick \"%s\"\n", nickname.get()));

  SECStatus srv = __CERT_AddTempCertToPerm(tmpCert,
                                           const_cast<char*>(nickname.get()),
                                           trust.GetTrust());


  return (srv == SECSuccess) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsNSSCertificateDB::AddCert(const nsACString & aCertDER, const char *aTrust,
                            const char *aName)
{
  nsCString base64;
  nsresult rv = Base64Encode(aCertDER, base64);
  NS_ENSURE_SUCCESS(rv, rv);
  return AddCertFromBase64(base64.get(), aTrust, aName);
}

NS_IMETHODIMP 
nsNSSCertificateDB::GetCerts(nsIX509CertList **_retval)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  CERTCertList *certList;

  nsCOMPtr<nsIInterfaceRequestor> ctx = new PipUIContext();
  nsCOMPtr<nsIX509CertList> nssCertList;
  certList = PK11_ListCerts(PK11CertListUnique, ctx);

  
  
  nssCertList = new nsNSSCertList(certList, true);

  *_retval = nssCertList;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP
nsNSSCertificateDB::GetRecentBadCerts(bool isPrivate, nsIRecentBadCerts** result)
{
  nsNSSShutDownPreventionLock locker;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  MutexAutoLock lock(mBadCertsLock);
  if (isPrivate) {
    if (!mPrivateRecentBadCerts) {
      mPrivateRecentBadCerts = new nsRecentBadCerts;
    }
    NS_ADDREF(*result = mPrivateRecentBadCerts);
  } else {
    if (!mPublicRecentBadCerts) {
      mPublicRecentBadCerts = new nsRecentBadCerts;
    }
    NS_ADDREF(*result = mPublicRecentBadCerts);
  }
  return NS_OK;
}
