





































#include "prmem.h"
#include "prprf.h"

#include "nsISupportsArray.h"
#include "nsIFile.h"
#include "nsNetUtil.h"
#include "nsILocalFile.h"
#include "nsIDirectoryService.h"
#include "nsIWindowWatcher.h"
#include "nsIPrompt.h"
#include "nsProxiedService.h"
#include "nsThreadUtils.h"

#include "nsNSSComponent.h"
#include "nsNSSHelper.h"
#include "nsPKCS12Blob.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsXPIDLString.h"
#include "nsDirectoryServiceDefs.h"
#include "nsNSSHelper.h"
#include "nsNSSCertificate.h"
#include "nsKeygenHandler.h" 
#include "nsPK11TokenDB.h"
#include "nsICertificateDialogs.h"
#include "nsNSSShutDown.h"
#include "nsCRT.h"
#include "pk11func.h"
#include "secerr.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

#include "nsNSSCleaner.h"
NSSCleanupAutoPtrClass(CERTCertificate, CERT_DestroyCertificate)

static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);

#define PIP_PKCS12_TMPFILENAME   NS_LITERAL_CSTRING(".pip_p12tmp")
#define PIP_PKCS12_BUFFER_SIZE   2048
#define PIP_PKCS12_RESTORE_OK          1
#define PIP_PKCS12_BACKUP_OK           2
#define PIP_PKCS12_USER_CANCELED       3
#define PIP_PKCS12_NOSMARTCARD_EXPORT  4
#define PIP_PKCS12_RESTORE_FAILED      5
#define PIP_PKCS12_BACKUP_FAILED       6
#define PIP_PKCS12_NSS_ERROR           7


nsPKCS12Blob::nsPKCS12Blob():mCertArray(0),
                             mTmpFile(nsnull),
                             mTmpFilePath(nsnull),
                             mDigest(nsnull),
                             mDigestIterator(nsnull),
                             mTokenSet(PR_FALSE)
{
  mUIContext = new PipUIContext();
}


nsPKCS12Blob::~nsPKCS12Blob()
{
  delete mDigestIterator;
  delete mDigest;
}




nsresult
nsPKCS12Blob::SetToken(nsIPK11Token *token)
{
 nsNSSShutDownPreventionLock locker;
 nsresult rv = NS_OK;
 if (token) {
   mToken = token;
 } else {
   PK11SlotInfo *slot;
   rv = GetSlotWithMechanism(CKM_RSA_PKCS, mUIContext,&slot);
   if (NS_FAILED(rv)) {
      mToken = 0;  
   } else {
     mToken = new nsPK11Token(slot);
     PK11_FreeSlot(slot);
   }
 }
 mTokenSet = PR_TRUE;
 return rv;
}





nsresult
nsPKCS12Blob::ImportFromFile(nsILocalFile *file)
{
  nsNSSShutDownPreventionLock locker;
  nsresult rv = NS_OK;

  if (!mToken) {
    if (!mTokenSet) {
      rv = SetToken(NULL); 
      if (NS_FAILED(rv)) {
        handleError(PIP_PKCS12_USER_CANCELED);
        return rv;
      }
    }
  }

  if (!mToken) {
    handleError(PIP_PKCS12_RESTORE_FAILED);
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  rv = mToken->Login(PR_TRUE);
  if (NS_FAILED(rv)) return rv;
  
  RetryReason wantRetry;
  
  do {
    rv = ImportFromFileHelper(file, im_standard_prompt, wantRetry);
    
    if (NS_SUCCEEDED(rv) && wantRetry == rr_auto_retry_empty_password_flavors)
    {
      rv = ImportFromFileHelper(file, im_try_zero_length_secitem, wantRetry);
    }
  }
  while (NS_SUCCEEDED(rv) && (wantRetry != rr_do_not_retry));
  
  return rv;
}

nsresult
nsPKCS12Blob::ImportFromFileHelper(nsILocalFile *file, 
                                   nsPKCS12Blob::ImportMode aImportMode,
                                   nsPKCS12Blob::RetryReason &aWantRetry)
{
  nsNSSShutDownPreventionLock locker;
  nsresult rv;
  SECStatus srv = SECSuccess;
  SEC_PKCS12DecoderContext *dcx = NULL;
  SECItem unicodePw;

  PK11SlotInfo *slot=nsnull;
  nsXPIDLString tokenName;
  unicodePw.data = NULL;
  
  aWantRetry = rr_do_not_retry;

  if (aImportMode == im_try_zero_length_secitem)
  {
    unicodePw.len = 0;
  }
  else
  {
    
    rv = getPKCS12FilePassword(&unicodePw);
    if (NS_FAILED(rv)) goto finish;
    if (unicodePw.data == NULL) {
      handleError(PIP_PKCS12_USER_CANCELED);
      return NS_OK;
    }
  }
  
  mToken->GetTokenName(getter_Copies(tokenName));
  {
    NS_ConvertUTF16toUTF8 tokenNameCString(tokenName);
    slot = PK11_FindSlotByName(tokenNameCString.get());
  }
  if (!slot) {
    srv = SECFailure;
    goto finish;
  }

  
  dcx = SEC_PKCS12DecoderStart(&unicodePw, slot, NULL,
                               digest_open, digest_close,
                               digest_read, digest_write,
                               this);
  if (!dcx) {
    srv = SECFailure;
    goto finish;
  }
  
  rv = inputToDecoder(dcx, file);
  if (NS_FAILED(rv)) {
    if (NS_ERROR_ABORT == rv) {
      
      srv = SECFailure;
    }
    goto finish;
  }
  
  srv = SEC_PKCS12DecoderVerify(dcx);
  if (srv) goto finish;
  
  srv = SEC_PKCS12DecoderValidateBags(dcx, nickname_collision);
  if (srv) goto finish;
  
  srv = SEC_PKCS12DecoderImportBags(dcx);
  if (srv) goto finish;
  
  handleError(PIP_PKCS12_RESTORE_OK);
finish:
  
  
  
  if (srv != SECSuccess) {
    if (SEC_ERROR_BAD_PASSWORD == PORT_GetError()) {
      if (unicodePw.len == sizeof(PRUnichar))
      {
        
        
        aWantRetry = rr_auto_retry_empty_password_flavors;
      }
      else
      {
        aWantRetry = rr_bad_password;
        handleError(PIP_PKCS12_NSS_ERROR);
      }
    }
    else
    {
      handleError(PIP_PKCS12_NSS_ERROR);
    }
  } else if (NS_FAILED(rv)) { 
    handleError(PIP_PKCS12_RESTORE_FAILED);
  }
  if (slot)
    PK11_FreeSlot(slot);
  
  if (dcx)
    SEC_PKCS12DecoderFinish(dcx);
  return NS_OK;
}

#if 0




nsresult
nsPKCS12Blob::LoadCerts(const PRUnichar **certNames, int numCerts)
{
  nsresult rv;
  char namecpy[256];
  
  if (!mCertArray) {
    rv = NS_NewISupportsArray(getter_AddRefs(mCertArray));
    if (NS_FAILED(rv)) {
      if (!handleError())
        return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  
  for (int i=0; i<numCerts; i++) {
    strcpy(namecpy, NS_ConvertUTF16toUTF8(certNames[i]));
    CERTCertificate *nssCert = PK11_FindCertFromNickname(namecpy, NULL);
    if (!nssCert) {
      if (!handleError())
        return NS_ERROR_FAILURE;
      else continue; 
    }
    nsCOMPtr<nsIX509Cert> cert = new nsNSSCertificate(nssCert);
    CERT_DestroyCertificate(nssCert);
    if (!cert) {
      if (!handleError())
        return NS_ERROR_OUT_OF_MEMORY;
    } else {
      mCertArray->AppendElement(cert);
    }
  }
  return NS_OK;
}
#endif

static PRBool
isExtractable(SECKEYPrivateKey *privKey)
{
  SECItem value;
  PRBool  isExtractable = PR_FALSE;
  SECStatus rv;

  rv=PK11_ReadRawAttribute(PK11_TypePrivKey, privKey, CKA_EXTRACTABLE, &value);
  if (rv != SECSuccess) {
    return PR_FALSE;
  }
  if ((value.len == 1) && (value.data != NULL)) {
    isExtractable = *(CK_BBOOL*)value.data;
  }
  SECITEM_FreeItem(&value, PR_FALSE);
  return isExtractable;
}
  










nsresult
nsPKCS12Blob::ExportToFile(nsILocalFile *file, 
                           nsIX509Cert **certs, int numCerts)
{
  nsNSSShutDownPreventionLock locker;
  nsresult rv;
  SECStatus srv = SECSuccess;
  SEC_PKCS12ExportContext *ecx = NULL;
  SEC_PKCS12SafeInfo *certSafe = NULL, *keySafe = NULL;
  SECItem unicodePw;
  nsAutoString filePath;
  int i;
  nsCOMPtr<nsILocalFile> localFileRef;
  NS_ASSERTION(mToken, "Need to set the token before exporting");
  

  PRBool InformedUserNoSmartcardBackup = PR_FALSE;
  int numCertsExported = 0;

  rv = mToken->Login(PR_TRUE);
  if (NS_FAILED(rv)) goto finish;
  
  unicodePw.data = NULL;
  rv = newPKCS12FilePassword(&unicodePw);
  if (NS_FAILED(rv)) goto finish;
  if (unicodePw.data == NULL) {
    handleError(PIP_PKCS12_USER_CANCELED);
    return NS_OK;
  }
  
  
  ecx = SEC_PKCS12CreateExportContext(NULL, NULL, NULL , NULL);
  if (!ecx) {
    srv = SECFailure;
    goto finish;
  }
  
  srv = SEC_PKCS12AddPasswordIntegrity(ecx, &unicodePw, SEC_OID_SHA1);
  if (srv) goto finish;
#if 0
  
  nrv = mCertArray->Count(&numCerts);
  if (NS_FAILED(nrv)) goto finish;
  
  for (i=0; i<numCerts; i++) {
    nsCOMPtr<nsIX509Cert> cert;
    nrv = mCertArray->GetElementAt(i, getter_AddRefs(cert));
    if (NS_FAILED(nrv)) goto finish;
#endif
  for (i=0; i<numCerts; i++) {


    nsNSSCertificate *cert = (nsNSSCertificate *)certs[i];
    
    CERTCertificate *nssCert = NULL;
    CERTCertificateCleaner nssCertCleaner(nssCert);
    nssCert = cert->GetCert();
    if (!nssCert) {
      rv = NS_ERROR_FAILURE;
      goto finish;
    }
    
    
    
    
    
    if (nssCert->slot && !PK11_IsInternal(nssCert->slot)) {
      
      SECKEYPrivateKey *privKey=PK11_FindKeyByDERCert(nssCert->slot,
                                                      nssCert, this);

      if (privKey) {
        PRBool privKeyIsExtractable = isExtractable(privKey);

        SECKEY_DestroyPrivateKey(privKey);

        if (!privKeyIsExtractable) {
          if (!InformedUserNoSmartcardBackup) {
            InformedUserNoSmartcardBackup = PR_TRUE;
            handleError(PIP_PKCS12_NOSMARTCARD_EXPORT);
          }
          continue;
        }
      }
    }

    
    
    
    keySafe = SEC_PKCS12CreateUnencryptedSafe(ecx);
    if (!SEC_PKCS12IsEncryptionAllowed() || PK11_IsFIPS()) {
      certSafe = keySafe;
    } else {
      certSafe = SEC_PKCS12CreatePasswordPrivSafe(ecx, &unicodePw,
                           SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC);
    }
    if (!certSafe || !keySafe) {
      rv = NS_ERROR_FAILURE;
      goto finish;
    }
    
    srv = SEC_PKCS12AddCertAndKey(ecx, certSafe, NULL, nssCert,
                                  CERT_GetDefaultCertDB(), 
                                  keySafe, NULL, PR_TRUE, &unicodePw,
                      SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_3KEY_TRIPLE_DES_CBC);
    if (srv) goto finish;
    
    ++numCertsExported;
  }
  
  if (!numCertsExported) goto finish;
  
  
  this->mTmpFile = NULL;
  file->GetPath(filePath);
  
  
  
  if (filePath.RFind(".p12", PR_TRUE, -1, 4) < 0) {
    
    
    
    filePath.AppendLiteral(".p12");
    localFileRef = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) goto finish;
    localFileRef->InitWithPath(filePath);
    file = localFileRef;
  }
  rv = file->OpenNSPRFileDesc(PR_RDWR|PR_CREATE_FILE|PR_TRUNCATE, 0664, 
                              &mTmpFile);
  if (NS_FAILED(rv) || !this->mTmpFile) goto finish;
  
  srv = SEC_PKCS12Encode(ecx, write_export_file, this);
  if (srv) goto finish;
  handleError(PIP_PKCS12_BACKUP_OK);
finish:
  if (NS_FAILED(rv) || srv != SECSuccess) {
    handleError(PIP_PKCS12_BACKUP_FAILED);
  }
  if (ecx)
    SEC_PKCS12DestroyExportContext(ecx);
  if (this->mTmpFile) {
    PR_Close(this->mTmpFile);
    this->mTmpFile = NULL;
  }
  return rv;
}












void
nsPKCS12Blob::unicodeToItem(const PRUnichar *uni, SECItem *item)
{
  int len = 0;
  while (uni[len++] != 0);
  SECITEM_AllocItem(NULL, item, sizeof(PRUnichar) * len);
#ifdef IS_LITTLE_ENDIAN
  int i = 0;
  for (i=0; i<len; i++) {
    item->data[2*i  ] = (unsigned char )(uni[i] << 8);
    item->data[2*i+1] = (unsigned char )(uni[i]);
  }
#else
  memcpy(item->data, uni, item->len);
#endif
}





nsresult
nsPKCS12Blob::newPKCS12FilePassword(SECItem *unicodePw)
{
  nsresult rv = NS_OK;
  nsAutoString password;
  nsCOMPtr<nsICertificateDialogs> certDialogs;
  rv = ::getNSSDialogs(getter_AddRefs(certDialogs), 
                       NS_GET_IID(nsICertificateDialogs),
                       NS_CERTIFICATEDIALOGS_CONTRACTID);
  if (NS_FAILED(rv)) return rv;
  PRBool pressedOK;
  {
    nsPSMUITracker tracker;
    if (tracker.isUIForbidden()) {
      rv = NS_ERROR_NOT_AVAILABLE;
    }
    else {
      rv = certDialogs->SetPKCS12FilePassword(mUIContext, password, &pressedOK);
    }
  }
  if (NS_FAILED(rv) || !pressedOK) return rv;
  unicodeToItem(password.get(), unicodePw);
  return NS_OK;
}





nsresult
nsPKCS12Blob::getPKCS12FilePassword(SECItem *unicodePw)
{
  nsresult rv = NS_OK;
  nsAutoString password;
  nsCOMPtr<nsICertificateDialogs> certDialogs;
  rv = ::getNSSDialogs(getter_AddRefs(certDialogs), 
                       NS_GET_IID(nsICertificateDialogs),
                       NS_CERTIFICATEDIALOGS_CONTRACTID);
  if (NS_FAILED(rv)) return rv;
  PRBool pressedOK;
  {
    nsPSMUITracker tracker;
    if (tracker.isUIForbidden()) {
      rv = NS_ERROR_NOT_AVAILABLE;
    }
    else {
      rv = certDialogs->GetPKCS12FilePassword(mUIContext, password, &pressedOK);
    }
  }
  if (NS_FAILED(rv) || !pressedOK) return rv;
  unicodeToItem(password.get(), unicodePw);
  return NS_OK;
}




nsresult
nsPKCS12Blob::inputToDecoder(SEC_PKCS12DecoderContext *dcx, nsILocalFile *file)
{
  nsNSSShutDownPreventionLock locker;
  nsresult rv;
  SECStatus srv;
  PRUint32 amount;
  char buf[PIP_PKCS12_BUFFER_SIZE];

  nsCOMPtr<nsIInputStream> fileStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(fileStream), file);
  
  if (NS_FAILED(rv)) {
    return rv;
  }

  while (PR_TRUE) {
    rv = fileStream->Read(buf, PIP_PKCS12_BUFFER_SIZE, &amount);
    if (NS_FAILED(rv)) {
      return rv;
    }
    
    srv = SEC_PKCS12DecoderUpdate(dcx, 
				  (unsigned char*) buf, 
				  amount);
    if (srv) {
      
      int pr_err = PORT_GetError();
      PORT_SetError(pr_err);
      return NS_ERROR_ABORT;
    }
    if (amount < PIP_PKCS12_BUFFER_SIZE)
      break;
  }
  return NS_OK;
}

#ifdef XP_MAC

OSErr ConvertMacPathToUnixPath(const char *macPath, char **unixPath)
{
  PRIntn len;
  char *cursor;
  
  len = PL_strlen(macPath);
  cursor = (char*)PR_Malloc(len+2);
  if (!cursor)
    return memFullErr;
    
  memcpy(cursor+1, macPath, len+1);
  *unixPath = cursor;
  *cursor = '/';
  while ((cursor = PL_strchr(cursor, ':')) != NULL) {
    *cursor = '/';
    cursor++;
  }
  return noErr;
}
#endif







SECStatus PR_CALLBACK
nsPKCS12Blob::digest_open(void *arg, PRBool reading)
{
  nsPKCS12Blob *cx = NS_REINTERPRET_POINTER_CAST(nsPKCS12Blob *, arg);
  NS_ENSURE_TRUE(cx, SECFailure);
  
  if (reading) {
    NS_ENSURE_TRUE(cx->mDigest, SECFailure);

    delete cx->mDigestIterator;
    cx->mDigestIterator = new nsCString::const_iterator;

    if (!cx->mDigestIterator) {
      PORT_SetError(SEC_ERROR_NO_MEMORY);
      return SECFailure;
    }

    cx->mDigest->BeginReading(*cx->mDigestIterator);
  }
  else {
    delete cx->mDigest;
    cx->mDigest = new nsCString;

    if (!cx->mDigest) {
      PORT_SetError(SEC_ERROR_NO_MEMORY);
      return SECFailure;
    }
  }

  return SECSuccess;
}




SECStatus PR_CALLBACK
nsPKCS12Blob::digest_close(void *arg, PRBool remove_it)
{
  nsPKCS12Blob *cx = NS_REINTERPRET_POINTER_CAST(nsPKCS12Blob *, arg);
  NS_ENSURE_TRUE(cx, SECFailure);

  delete cx->mDigestIterator;
  cx->mDigestIterator = nsnull;

  if (remove_it) {  
    delete cx->mDigest;
    cx->mDigest = nsnull;
  }
  
  return SECSuccess;
}



int PR_CALLBACK
nsPKCS12Blob::digest_read(void *arg, unsigned char *buf, unsigned long len)
{
  nsPKCS12Blob *cx = NS_REINTERPRET_POINTER_CAST(nsPKCS12Blob *, arg);
  NS_ENSURE_TRUE(cx, SECFailure);
  NS_ENSURE_TRUE(cx->mDigest, SECFailure);

  
  NS_ENSURE_TRUE(cx->mDigestIterator, SECFailure);

  unsigned long available = cx->mDigestIterator->size_forward();
  
  if (len > available)
    len = available;

  memcpy(buf, cx->mDigestIterator->get(), len);
  cx->mDigestIterator->advance(len);
  
  return len;
}



int PR_CALLBACK
nsPKCS12Blob::digest_write(void *arg, unsigned char *buf, unsigned long len)
{
  nsPKCS12Blob *cx = NS_REINTERPRET_POINTER_CAST(nsPKCS12Blob *, arg);
  NS_ENSURE_TRUE(cx, SECFailure);
  NS_ENSURE_TRUE(cx->mDigest, SECFailure);

  
  NS_ENSURE_FALSE(cx->mDigestIterator, SECFailure);
  
  cx->mDigest->Append(NS_REINTERPRET_CAST(char *, buf),
                     NS_STATIC_CAST(PRUint32, len));
  
  return len;
}




SECItem * PR_CALLBACK
nsPKCS12Blob::nickname_collision(SECItem *oldNick, PRBool *cancel, void *wincx)
{
  nsNSSShutDownPreventionLock locker;
  *cancel = PR_FALSE;
  nsresult rv;
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv)) return nsnull;
  int count = 1;
  nsCString nickname;
  nsAutoString nickFromProp;
  nssComponent->GetPIPNSSBundleString("P12DefaultNickname", nickFromProp);
  NS_ConvertUTF16toUTF8 nickFromPropC(nickFromProp);
  
  
  
  
  
  
  
  
  
  
  
  while (1) {
    
    
    
    
    
    
    
    
    
    
    
    
    if (count > 1) {
      nickname.Adopt(PR_smprintf("%s #%d", nickFromPropC.get(), count));
    } else {
      nickname = nickFromPropC;
    }
    CERTCertificate *cert = CERT_FindCertByNickname(CERT_GetDefaultCertDB(),
                                           NS_CONST_CAST(char*,nickname.get()));
    if (!cert) {
      break;
    }
    CERT_DestroyCertificate(cert);
    count++;
  }
  SECItem *newNick = new SECItem;
  if (!newNick)
    return nsnull;

  newNick->type = siAsciiString;
  newNick->data = (unsigned char*) nsCRT::strdup(nickname.get());
  newNick->len  = strlen((char*)newNick->data);
  return newNick;
}



void PR_CALLBACK
nsPKCS12Blob::write_export_file(void *arg, const char *buf, unsigned long len)
{
  nsPKCS12Blob *cx = (nsPKCS12Blob *)arg;
  PR_Write(cx->mTmpFile, buf, len);
}




PRBool
pip_ucs2_ascii_conversion_fn(PRBool toUnicode,
                             unsigned char *inBuf,
                             unsigned int inBufLen,
                             unsigned char *outBuf,
                             unsigned int maxOutBufLen,
                             unsigned int *outBufLen,
                             PRBool swapBytes)
{
  
  *outBufLen = inBufLen;
  memcpy(outBuf, inBuf, inBufLen);
  return PR_TRUE;
}

PRBool
nsPKCS12Blob::handleError(int myerr)
{
  nsPSMUITracker tracker;
  if (tracker.isUIForbidden()) {
    return PR_FALSE;
  }

  nsresult rv;
  PRBool keepGoing = PR_FALSE;
  int prerr = PORT_GetError();
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("PKCS12: NSS/NSPR error(%d)", prerr));
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("PKCS12: I called(%d)", myerr));
  nsCOMPtr<nsINSSComponent> nssComponent(do_GetService(kNSSComponentCID, &rv));
  if (NS_FAILED(rv)) return PR_FALSE;
  nsCOMPtr<nsIPrompt> errPrompt;
  nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
  if (wwatch) {
    wwatch->GetNewPrompter(0, getter_AddRefs(errPrompt));
    if (errPrompt) {
      nsCOMPtr<nsIPrompt> proxyPrompt;
      NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                           NS_GET_IID(nsIPrompt), errPrompt,
                           NS_PROXY_SYNC, getter_AddRefs(proxyPrompt));
      if (!proxyPrompt) return PR_FALSE;
    } else {
      return PR_FALSE;
    }
  } else {
    return PR_FALSE;
  }
  nsAutoString errorMsg;
  switch (myerr) {
  case PIP_PKCS12_RESTORE_OK:
    rv = nssComponent->GetPIPNSSBundleString("SuccessfulP12Restore", errorMsg);
    if (NS_FAILED(rv)) return rv;
    errPrompt->Alert(nsnull, errorMsg.get());
    return PR_TRUE;
  case PIP_PKCS12_BACKUP_OK:
    rv = nssComponent->GetPIPNSSBundleString("SuccessfulP12Backup", errorMsg);
    if (NS_FAILED(rv)) return rv;
    errPrompt->Alert(nsnull, errorMsg.get());
    return PR_TRUE;
  case PIP_PKCS12_USER_CANCELED:
    return PR_TRUE;  
  case PIP_PKCS12_NOSMARTCARD_EXPORT:
    rv = nssComponent->GetPIPNSSBundleString("PKCS12InfoNoSmartcardBackup", errorMsg);
    if (NS_FAILED(rv)) return rv;
    errPrompt->Alert(nsnull, errorMsg.get());
    return PR_TRUE;
  case PIP_PKCS12_RESTORE_FAILED:
    rv = nssComponent->GetPIPNSSBundleString("PKCS12UnknownErrRestore", errorMsg);
    if (NS_FAILED(rv)) return rv;
    errPrompt->Alert(nsnull, errorMsg.get());
    return PR_TRUE;
  case PIP_PKCS12_BACKUP_FAILED:
    rv = nssComponent->GetPIPNSSBundleString("PKCS12UnknownErrBackup", errorMsg);
    if (NS_FAILED(rv)) return rv;
    errPrompt->Alert(nsnull, errorMsg.get());
    return PR_TRUE;
  case PIP_PKCS12_NSS_ERROR:
    switch (prerr) {
    
    
    case 0: break;
    case SEC_ERROR_PKCS12_CERT_COLLISION:
      
      
      
      
#if 0
      
      
      
    case SEC_ERROR_PKCS12_PRIVACY_PASSWORD_INCORRECT:
      rv = nssComponent->GetPIPNSSBundleString("PKCS12PasswordInvalid", errorMsg);
      if (NS_FAILED(rv)) return rv;
      errPrompt->Alert(nsnull, errorMsg.get());
    break;
#endif
    case SEC_ERROR_BAD_PASSWORD:
      rv = nssComponent->GetPIPNSSBundleString("PK11BadPassword", errorMsg);
      if (NS_FAILED(rv)) return rv;
      errPrompt->Alert(nsnull, errorMsg.get());
      break;
    case SEC_ERROR_BAD_DER:
    case SEC_ERROR_PKCS12_CORRUPT_PFX_STRUCTURE:
    case SEC_ERROR_PKCS12_INVALID_MAC:
      rv = nssComponent->GetPIPNSSBundleString("PKCS12DecodeErr", errorMsg);
      if (NS_FAILED(rv)) return rv;
      errPrompt->Alert(nsnull, errorMsg.get());
      break;
    case SEC_ERROR_PKCS12_DUPLICATE_DATA:
      rv = nssComponent->GetPIPNSSBundleString("PKCS12DupData", errorMsg);
      if (NS_FAILED(rv)) return rv;
      errPrompt->Alert(nsnull, errorMsg.get());
      break;
    default:
      rv = nssComponent->GetPIPNSSBundleString("PKCS12UnknownErr", errorMsg);
      if (NS_FAILED(rv)) return rv;
      errPrompt->Alert(nsnull, errorMsg.get());
    }
    break;
  case 0: 
  default:
    rv = nssComponent->GetPIPNSSBundleString("PKCS12UnknownErr", errorMsg);
    if (NS_FAILED(rv)) return rv;
    errPrompt->Alert(nsnull, errorMsg.get());
    break;
  }
  if (NS_FAILED(rv)) return rv;
  return keepGoing;
}

