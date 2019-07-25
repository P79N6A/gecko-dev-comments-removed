









































#include "nsNSSComponent.h"
#include "nsNSSCertificate.h"
#include "nsNSSCleaner.h"
#include "nsNSSIOLayer.h"

#include "ssl.h"
#include "cert.h"
#include "secerr.h"
#include "sslerr.h"

using namespace mozilla;

static NS_DEFINE_CID(kNSSComponentCID, NS_NSSCOMPONENT_CID);
NSSCleanupAutoPtrClass(CERTCertificate, CERT_DestroyCertificate)

#ifdef PR_LOGGING
extern PRLogModuleInfo* gPIPNSSLog;
#endif

SECStatus
PSM_SSL_PKIX_AuthCertificate(PRFileDesc *fd, CERTCertificate *peerCert, bool checksig, bool isServer)
{
    SECStatus          rv;
    SECCertUsage       certUsage;
    SECCertificateUsage certificateusage;
    void *             pinarg;
    char *             hostname;
    
    pinarg = SSL_RevealPinArg(fd);
    hostname = SSL_RevealURL(fd);
    
    
    certUsage = isServer ? certUsageSSLClient : certUsageSSLServer;
    certificateusage = isServer ? certificateUsageSSLClient : certificateUsageSSLServer;

    if (!nsNSSComponent::globalConstFlagUsePKIXVerification) {
        rv = CERT_VerifyCertNow(CERT_GetDefaultCertDB(), peerCert, checksig, certUsage,
                                pinarg);
    }
    else {
        nsresult nsrv;
        nsCOMPtr<nsINSSComponent> inss = do_GetService(kNSSComponentCID, &nsrv);
        if (!inss)
          return SECFailure;
        nsRefPtr<nsCERTValInParamWrapper> survivingParams;
        if (NS_FAILED(inss->GetDefaultCERTValInParam(survivingParams)))
          return SECFailure;

        CERTValOutParam cvout[1];
        cvout[0].type = cert_po_end;

        rv = CERT_PKIXVerifyCert(peerCert, certificateusage,
                                survivingParams->GetRawPointerForNSS(),
                                cvout, pinarg);
    }

    if ( rv == SECSuccess && !isServer ) {
        



        if (hostname && hostname[0])
            rv = CERT_VerifyCertName(peerCert, hostname);
        else
            rv = SECFailure;
        if (rv != SECSuccess)
            PORT_SetError(SSL_ERROR_BAD_CERT_DOMAIN);
    }
        
    PORT_Free(hostname);
    return rv;
}

struct nsSerialBinaryBlacklistEntry
{
  unsigned int len;
  const char *binary_serial;
};


static struct nsSerialBinaryBlacklistEntry myUTNBlacklistEntries[] = {
  { 17, "\x00\x92\x39\xd5\x34\x8f\x40\xd1\x69\x5a\x74\x54\x70\xe1\xf2\x3f\x43" },
  { 17, "\x00\xd8\xf3\x5f\x4e\xb7\x87\x2b\x2d\xab\x06\x92\xe3\x15\x38\x2f\xb0" },
  { 16, "\x72\x03\x21\x05\xc5\x0c\x08\x57\x3d\x8e\xa5\x30\x4e\xfe\xe8\xb0" },
  { 17, "\x00\xb0\xb7\x13\x3e\xd0\x96\xf9\xb5\x6f\xae\x91\xc8\x74\xbd\x3a\xc0" },
  { 16, "\x39\x2a\x43\x4f\x0e\x07\xdf\x1f\x8a\xa3\x05\xde\x34\xe0\xc2\x29" },
  { 16, "\x3e\x75\xce\xd4\x6b\x69\x30\x21\x21\x88\x30\xae\x86\xa8\x2a\x71" },
  { 17, "\x00\xe9\x02\x8b\x95\x78\xe4\x15\xdc\x1a\x71\x0a\x2b\x88\x15\x44\x47" },
  { 17, "\x00\xd7\x55\x8f\xda\xf5\xf1\x10\x5b\xb2\x13\x28\x2b\x70\x77\x29\xa3" },
  { 16, "\x04\x7e\xcb\xe9\xfc\xa5\x5f\x7b\xd0\x9e\xae\x36\xe1\x0c\xae\x1e" },
  { 17, "\x00\xf5\xc8\x6a\xf3\x61\x62\xf1\x3a\x64\xf5\x4f\x6d\xc9\x58\x7c\x06" },
  { 0, 0 } 
};



PRErrorCode
PSM_SSL_DigiNotarTreatAsRevoked(CERTCertificate * serverCert,
                                CERTCertList * serverCertChain)
{
  
  
  
  
  PRTime cutoff = 0;
  PRStatus status = PR_ParseTimeString("01-JUL-2011 00:00", true, &cutoff);
  if (status != PR_SUCCESS) {
    NS_ASSERTION(status == PR_SUCCESS, "PR_ParseTimeString failed");
    
  } else {
    PRTime notBefore = 0, notAfter = 0;
    if (CERT_GetCertTimes(serverCert, &notBefore, &notAfter) == SECSuccess &&
           notBefore < cutoff) {
      
      return 0;
    }
  }
  
  for (CERTCertListNode *node = CERT_LIST_HEAD(serverCertChain);
       !CERT_LIST_END(node, serverCertChain);
       node = CERT_LIST_NEXT(node)) {
    if (node->cert->issuerName &&
        strstr(node->cert->issuerName, "CN=DigiNotar")) {
      return SEC_ERROR_REVOKED_CERTIFICATE;
    }
  }
  
  return 0;
}


PRErrorCode
PSM_SSL_BlacklistDigiNotar(CERTCertificate * serverCert,
                           CERTCertList * serverCertChain)
{
  bool isDigiNotarIssuedCert = false;

  for (CERTCertListNode *node = CERT_LIST_HEAD(serverCertChain);
       !CERT_LIST_END(node, serverCertChain);
       node = CERT_LIST_NEXT(node)) {
    if (!node->cert->issuerName)
      continue;

    if (strstr(node->cert->issuerName, "CN=DigiNotar")) {
      isDigiNotarIssuedCert = true;
    }
  }

  if (isDigiNotarIssuedCert) {
    
    PRErrorCode revoked_code = PSM_SSL_DigiNotarTreatAsRevoked(serverCert, serverCertChain);
    return (revoked_code != 0) ? revoked_code : SEC_ERROR_UNTRUSTED_ISSUER;
  }

  return 0;
}


SECStatus PR_CALLBACK AuthCertificateCallback(void* client_data, PRFileDesc* fd,
                                              PRBool checksig, PRBool isServer) {
  nsNSSShutDownPreventionLock locker;

  CERTCertificate *serverCert = SSL_PeerCertificate(fd);
  CERTCertificateCleaner serverCertCleaner(serverCert);

  if (serverCert && 
      serverCert->serialNumber.data &&
      serverCert->issuerName &&
      !strcmp(serverCert->issuerName, 
        "CN=UTN-USERFirst-Hardware,OU=http://www.usertrust.com,O=The USERTRUST Network,L=Salt Lake City,ST=UT,C=US")) {

    unsigned char *server_cert_comparison_start = (unsigned char*)serverCert->serialNumber.data;
    unsigned int server_cert_comparison_len = serverCert->serialNumber.len;

    while (server_cert_comparison_len) {
      if (*server_cert_comparison_start != 0)
        break;

      ++server_cert_comparison_start;
      --server_cert_comparison_len;
    }

    nsSerialBinaryBlacklistEntry *walk = myUTNBlacklistEntries;
    for ( ; walk && walk->len; ++walk) {

      unsigned char *locked_cert_comparison_start = (unsigned char*)walk->binary_serial;
      unsigned int locked_cert_comparison_len = walk->len;
      
      while (locked_cert_comparison_len) {
        if (*locked_cert_comparison_start != 0)
          break;
        
        ++locked_cert_comparison_start;
        --locked_cert_comparison_len;
      }

      if (server_cert_comparison_len == locked_cert_comparison_len &&
          !memcmp(server_cert_comparison_start, locked_cert_comparison_start, locked_cert_comparison_len)) {
        PR_SetError(SEC_ERROR_REVOKED_CERTIFICATE, 0);
        return SECFailure;
      }
    }
  }

  SECStatus rv = PSM_SSL_PKIX_AuthCertificate(fd, serverCert, checksig, isServer);

  
  
  

  if (serverCert) {
    nsNSSSocketInfo* infoObject = (nsNSSSocketInfo*) fd->higher->secret;
    nsRefPtr<nsSSLStatus> status = infoObject->SSLStatus();
    nsRefPtr<nsNSSCertificate> nsc;

    if (!status || !status->mServerCert) {
      nsc = nsNSSCertificate::Create(serverCert);
    }

    CERTCertList *certList = nsnull;
    certList = CERT_GetCertChainFromCert(serverCert, PR_Now(), certUsageSSLCA);
    if (!certList) {
      rv = SECFailure;
    } else {
      PRErrorCode blacklistErrorCode;
      if (rv == SECSuccess) { 
        blacklistErrorCode = PSM_SSL_BlacklistDigiNotar(serverCert, certList);
      } else { 
        PRErrorCode savedErrorCode = PORT_GetError();
        
        blacklistErrorCode = PSM_SSL_DigiNotarTreatAsRevoked(serverCert, certList);
        if (blacklistErrorCode == 0) {
          
          PORT_SetError(savedErrorCode);
        }
      }
      
      if (blacklistErrorCode != 0) {
        infoObject->SetCertIssuerBlacklisted();
        PORT_SetError(blacklistErrorCode);
        rv = SECFailure;
      }
    }

    if (rv == SECSuccess) {
      if (nsc) {
        bool dummyIsEV;
        nsc->GetIsExtendedValidation(&dummyIsEV); 
      }
    
      nsCOMPtr<nsINSSComponent> nssComponent;
      
      for (CERTCertListNode *node = CERT_LIST_HEAD(certList);
           !CERT_LIST_END(node, certList);
           node = CERT_LIST_NEXT(node)) {

        if (node->cert->slot) {
          
          continue;
        }

        if (node->cert->isperm) {
          
          continue;
        }
        
        if (node->cert == serverCert) {
          
          
          continue;
        }

        
        char* nickname = nsNSSCertificate::defaultServerNickname(node->cert);
        if (nickname && *nickname) {
          PK11SlotInfo *slot = PK11_GetInternalKeySlot();
          if (slot) {
            PK11_ImportCert(slot, node->cert, CK_INVALID_HANDLE, 
                            nickname, false);
            PK11_FreeSlot(slot);
          }
        }
        PR_FREEIF(nickname);
      }

    }

    if (certList) {
      CERT_DestroyCertList(certList);
    }

    
    
    
    if (!status) {
      status = new nsSSLStatus();
      infoObject->SetSSLStatus(status);
    }

    if (rv == SECSuccess) {
      
      
      nsSSLIOLayerHelpers::mHostsWithCertErrors->RememberCertHasError(
        infoObject, nsnull, rv);
    }
    else {
      
      nsSSLIOLayerHelpers::mHostsWithCertErrors->LookupCertErrorBits(
        infoObject, status);
    }

    if (status && !status->mServerCert) {
      status->mServerCert = nsc;
      PR_LOG(gPIPNSSLog, PR_LOG_DEBUG,
             ("AuthCertificateCallback setting NEW cert %p\n", status->mServerCert.get()));
    }
  }

  return rv;
}
