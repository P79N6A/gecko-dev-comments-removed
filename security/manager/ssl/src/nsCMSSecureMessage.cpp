




































#include "nsMemory.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "nsISupports.h"
#include "nsIInterfaceRequestor.h"
#include "nsCRT.h"

#include "nsICMSSecureMessage.h"

#include "nsCMSSecureMessage.h"
#include "nsNSSCertificate.h"
#include "nsNSSHelper.h"
#include "nsNSSShutDown.h"

#include <string.h>
#include "plbase64.h"
#include "cert.h"
#include "cms.h"

#include "nsIServiceManager.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif









NS_IMPL_ISUPPORTS1(nsCMSSecureMessage, nsICMSSecureMessage)


nsCMSSecureMessage::nsCMSSecureMessage()
{
  
}


nsCMSSecureMessage::~nsCMSSecureMessage()
{
}


NS_IMETHODIMP nsCMSSecureMessage::
GetCertByPrefID(const char *certID, char **_retval)
{
  nsNSSShutDownPreventionLock locker;
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::GetCertByPrefID\n"));
  nsresult rv = NS_OK;
  CERTCertificate *cert = 0;
  nsXPIDLCString nickname;
  nsCOMPtr<nsIInterfaceRequestor> ctx = new PipUIContext();

  *_retval = 0;

  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    goto done;
  }

  rv = prefs->GetCharPref(certID,
                          getter_Copies(nickname));
  if (NS_FAILED(rv)) goto done;

  
  cert = CERT_FindUserCertByUsage(CERT_GetDefaultCertDB(), NS_CONST_CAST(char*, nickname.get()), 
           certUsageEmailRecipient, PR_TRUE, ctx);

  if (!cert) { 
    
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::GetCertByPrefID - can't find user cert\n"));
    goto done;
  } 

  
  encode(cert->derCert.data, cert->derCert.len, _retval);

done:
  if (cert) CERT_DestroyCertificate(cert);
  return rv;
}



nsresult nsCMSSecureMessage::
DecodeCert(const char *value, nsIX509Cert ** _retval)
{
  nsNSSShutDownPreventionLock locker;
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::DecodeCert\n"));
  nsresult rv = NS_OK;
  PRInt32 length;
  unsigned char *data = 0;

  *_retval = 0;

  if (!value) { return NS_ERROR_FAILURE; }

  rv = decode(value, &data, &length);
  if (NS_FAILED(rv)) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::DecodeCert - can't decode cert\n"));
    return rv;
  }

  nsCOMPtr<nsIX509Cert> cert =  nsNSSCertificate::ConstructFromDER((char *)data, length);

  if (cert) {
    *_retval = cert;
    NS_ADDREF(*_retval);
  }
  else {
    rv = NS_ERROR_FAILURE;
  }

  nsCRT::free((char*)data);
  return rv;
}


nsresult nsCMSSecureMessage::
SendMessage(const char *msg, const char *base64Cert, char ** _retval)
{
  nsNSSShutDownPreventionLock locker;
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::SendMessage\n"));
  nsresult rv = NS_OK;
  CERTCertificate *cert = 0;
  NSSCMSMessage *cmsMsg = 0;
  unsigned char *certDER = 0;
  PRInt32 derLen;
  NSSCMSEnvelopedData *env;
  NSSCMSContentInfo *cinfo;
  NSSCMSRecipientInfo *rcpt;
  SECItem item;
  SECItem output;
  PLArenaPool *arena = PORT_NewArena(1024);
  SECStatus s;
  nsCOMPtr<nsIInterfaceRequestor> ctx = new PipUIContext();

  
  cmsMsg = NSS_CMSMessage_Create(NULL);
  if (!cmsMsg) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::SendMessage - can't create NSSCMSMessage\n"));
    rv = NS_ERROR_FAILURE;
    goto done;
  }

  
  rv = decode(base64Cert, &certDER, &derLen);
  if (NS_FAILED(rv)) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::SendMessage - can't decode / import cert into NSS\n"));
    goto done;
  }

  cert = CERT_DecodeCertFromPackage((char *)certDER, derLen);
  if (!cert) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::SendMessage - can't decode cert from package\n"));
    rv = NS_ERROR_FAILURE;
    goto done;
  }

#if 0
  cert->dbhandle = CERT_GetDefaultCertDB();  
#endif

  

  

  
  env = NSS_CMSEnvelopedData_Create(cmsMsg, SEC_OID_DES_EDE3_CBC, 0);
  if (!env) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::SendMessage - can't create envelope data\n"));
    rv = NS_ERROR_FAILURE;
    goto done;
  }

  cinfo = NSS_CMSEnvelopedData_GetContentInfo(env);
  item.data = (unsigned char *)msg;
  item.len = strlen(msg);  
  s = NSS_CMSContentInfo_SetContent_Data(cmsMsg, cinfo, 0, PR_FALSE);
  if (s != SECSuccess) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::SendMessage - can't set content data\n"));
    rv = NS_ERROR_FAILURE;
    goto done;
  }

  rcpt = NSS_CMSRecipientInfo_Create(cmsMsg, cert);
  if (!rcpt) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::SendMessage - can't create recipient info\n"));
    rv = NS_ERROR_FAILURE;
    goto done;
  }

  s = NSS_CMSEnvelopedData_AddRecipient(env, rcpt);
  if (s != SECSuccess) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::SendMessage - can't add recipient\n"));
    rv = NS_ERROR_FAILURE;
    goto done;
  }

  
  cinfo = NSS_CMSMessage_GetContentInfo(cmsMsg);
  s = NSS_CMSContentInfo_SetContent_EnvelopedData(cmsMsg, cinfo, env);
  if (s != SECSuccess) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::SendMessage - can't set content enveloped data\n"));
    rv = NS_ERROR_FAILURE;
    goto done;
  }
  
  
  NSSCMSEncoderContext *ecx;

  output.data = 0; output.len = 0;
  ecx = NSS_CMSEncoder_Start(cmsMsg, 0, 0, &output, arena,
            0, ctx, 0, 0, 0, 0);
  if (!ecx) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::SendMessage - can't start cms encoder\n"));
    rv = NS_ERROR_FAILURE;
    goto done;
  }

  s = NSS_CMSEncoder_Update(ecx, msg, strlen(msg));
  if (s != SECSuccess) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::SendMessage - can't update encoder\n"));
    rv = NS_ERROR_FAILURE;
    goto done;
  }

  s = NSS_CMSEncoder_Finish(ecx);
  if (s != SECSuccess) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::SendMessage - can't finish encoder\n"));
    rv = NS_ERROR_FAILURE;
    goto done;
  }

  
  rv = encode(output.data, output.len, _retval);

done:
  if (certDER) nsCRT::free((char *)certDER);
  if (cert) CERT_DestroyCertificate(cert);
  if (cmsMsg) NSS_CMSMessage_Destroy(cmsMsg);
  if (arena) PORT_FreeArena(arena, PR_FALSE);  

  return rv;
}




nsresult nsCMSSecureMessage::
ReceiveMessage(const char *msg, char **_retval)
{
  nsNSSShutDownPreventionLock locker;
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::ReceiveMessage\n"));
  nsresult rv = NS_OK;
  NSSCMSDecoderContext *dcx;
  unsigned char *der = 0;
  PRInt32 derLen;
  NSSCMSMessage *cmsMsg = 0;
  SECItem *content;
  nsCOMPtr<nsIInterfaceRequestor> ctx = new PipUIContext();

  
  rv = decode(msg, &der, &derLen);
  if (NS_FAILED(rv)) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::ReceiveMessage - can't base64 decode\n"));
    goto done;
  }

  dcx = NSS_CMSDecoder_Start(0, 0, 0,  0, ctx,  0, 0);
  if (!dcx) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::ReceiveMessage - can't start decoder\n"));
    rv = NS_ERROR_FAILURE;
    goto done;
  }

  (void)NSS_CMSDecoder_Update(dcx, (char *)der, derLen);
  cmsMsg = NSS_CMSDecoder_Finish(dcx);
  if (!cmsMsg) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::ReceiveMessage - can't finish decoder\n"));
    rv = NS_ERROR_FAILURE;
    
    goto done;
  }

  content = NSS_CMSMessage_GetContent(cmsMsg);
  if (!content) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::ReceiveMessage - can't get content\n"));
    rv = NS_ERROR_FAILURE;
    goto done;
  }

  
  *_retval = (char*)malloc(content->len+1);
  memcpy(*_retval, content->data, content->len);
  (*_retval)[content->len] = 0;

done:
  if (der) free(der);
  if (cmsMsg) NSS_CMSMessage_Destroy(cmsMsg);

  return rv;
}

nsresult nsCMSSecureMessage::
encode(const unsigned char *data, PRInt32 dataLen, char **_retval)
{
  nsresult rv = NS_OK;

  *_retval = PL_Base64Encode((const char *)data, dataLen, NULL);
  if (!*_retval) { rv = NS_ERROR_OUT_OF_MEMORY; goto loser; }

loser:
  return rv;
}

nsresult nsCMSSecureMessage::
decode(const char *data, unsigned char **result, PRInt32 * _retval)
{
  PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::decode\n"));
  nsresult rv = NS_OK;
  PRUint32 len = PL_strlen(data);
  int adjust = 0;

  
  if (data[len-1] == '=') {
    adjust++;
    if (data[len-2] == '=') adjust++;
  }

  *result = (unsigned char *)PL_Base64Decode(data, len, NULL);
  if (!*result) {
    PR_LOG(gPIPNSSLog, PR_LOG_DEBUG, ("nsCMSSecureMessage::decode - error decoding base64\n"));
    rv = NS_ERROR_ILLEGAL_VALUE;
    goto loser;
  }

  *_retval = (len*3)/4 - adjust;

loser:
  return rv;
}
