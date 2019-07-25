







































#ifndef __ssl_h_
#define __ssl_h_

#include "prtypes.h"
#include "prerror.h"
#include "prio.h"
#include "seccomon.h"
#include "cert.h"
#include "keyt.h"

#include "sslt.h"  

#if defined(_WIN32) && !defined(IN_LIBSSL) && !defined(NSS_USE_STATIC_LIBS)
#define SSL_IMPORT extern __declspec(dllimport)
#else
#define SSL_IMPORT extern
#endif

SEC_BEGIN_PROTOS


SSL_IMPORT const PRUint16 SSL_ImplementedCiphers[];


SSL_IMPORT const PRUint16 *SSL_GetImplementedCiphers(void);


SSL_IMPORT const PRUint16 SSL_NumImplementedCiphers;


SSL_IMPORT PRUint16 SSL_GetNumImplementedCiphers(void);


#define SSL_IS_SSL2_CIPHER(which) (((which) & 0xfff0) == 0xff00)





SSL_IMPORT PRFileDesc *SSL_ImportFD(PRFileDesc *model, PRFileDesc *fd);















#define SSL_SECURITY			1 /* (on by default) */
#define SSL_SOCKS			2 /* (off by default) */
#define SSL_REQUEST_CERTIFICATE		3 /* (off by default) */
#define SSL_HANDSHAKE_AS_CLIENT		5 /* force accept to hs as client */
                               		  
#define SSL_HANDSHAKE_AS_SERVER		6 /* force connect to hs as server */
                               		  
#define SSL_ENABLE_SSL2			7 /* enable ssl v2 (on by default) */
#define SSL_ENABLE_SSL3		        8 /* enable ssl v3 (on by default) */
#define SSL_NO_CACHE		        9 /* don't use the session cache */
                    		          
#define SSL_REQUIRE_CERTIFICATE        10 /* (SSL_REQUIRE_FIRST_HANDSHAKE */
                                          
#define SSL_ENABLE_FDX                 11 /* permit simultaneous read/write */
                                          
#define SSL_V2_COMPATIBLE_HELLO        12 /* send v3 client hello in v2 fmt */
                                          
#define SSL_ENABLE_TLS		       13 /* enable TLS (on by default) */
#define SSL_ROLLBACK_DETECTION         14 /* for compatibility, default: on */
#define SSL_NO_STEP_DOWN               15 /* Disable export cipher suites   */
                                          
					  
					  
#define SSL_BYPASS_PKCS11              16 /* use PKCS#11 for pub key only   */
#define SSL_NO_LOCKS                   17 /* Don't use locks for protection */
#define SSL_ENABLE_SESSION_TICKETS     18 /* Enable TLS SessionTicket       */
                                          
#define SSL_ENABLE_DEFLATE             19 /* Enable TLS compression with    */
                                          
#define SSL_ENABLE_RENEGOTIATION       20 /* Values below (default: never)  */
#define SSL_REQUIRE_SAFE_NEGOTIATION   21 /* Peer must send Signaling       */
					  
                                          
					  
                                          
#define SSL_ENABLE_FALSE_START         22 /* Enable SSL false start (off by */
                                          
                                          









#ifdef SSL_DEPRECATED_FUNCTION 

SSL_IMPORT SECStatus SSL_Enable(PRFileDesc *fd, int option, PRBool on);
SSL_IMPORT SECStatus SSL_EnableDefault(int option, PRBool on);
#endif


SSL_IMPORT SECStatus SSL_OptionSet(PRFileDesc *fd, PRInt32 option, PRBool on);
SSL_IMPORT SECStatus SSL_OptionGet(PRFileDesc *fd, PRInt32 option, PRBool *on);
SSL_IMPORT SECStatus SSL_OptionSetDefault(PRInt32 option, PRBool on);
SSL_IMPORT SECStatus SSL_OptionGetDefault(PRInt32 option, PRBool *on);
SSL_IMPORT SECStatus SSL_CertDBHandleSet(PRFileDesc *fd, CERTCertDBHandle *dbHandle);








#ifdef SSL_DEPRECATED_FUNCTION 

SSL_IMPORT SECStatus SSL_EnableCipher(long which, PRBool enabled);
SSL_IMPORT SECStatus SSL_SetPolicy(long which, int policy);
#endif


SSL_IMPORT SECStatus SSL_CipherPrefSet(PRFileDesc *fd, PRInt32 cipher, PRBool enabled);
SSL_IMPORT SECStatus SSL_CipherPrefGet(PRFileDesc *fd, PRInt32 cipher, PRBool *enabled);
SSL_IMPORT SECStatus SSL_CipherPrefSetDefault(PRInt32 cipher, PRBool enabled);
SSL_IMPORT SECStatus SSL_CipherPrefGetDefault(PRInt32 cipher, PRBool *enabled);
SSL_IMPORT SECStatus SSL_CipherPolicySet(PRInt32 cipher, PRInt32 policy);
SSL_IMPORT SECStatus SSL_CipherPolicyGet(PRInt32 cipher, PRInt32 *policy);



#define SSL_NOT_ALLOWED		 0	      /* or invalid or unimplemented */
#define SSL_ALLOWED		 1
#define SSL_RESTRICTED		 2	      /* only with "Step-Up" certs. */


#define SSL_REQUIRE_NEVER           ((PRBool)0)
#define SSL_REQUIRE_ALWAYS          ((PRBool)1)
#define SSL_REQUIRE_FIRST_HANDSHAKE ((PRBool)2)
#define SSL_REQUIRE_NO_ERROR        ((PRBool)3)



#define SSL_RENEGOTIATE_NEVER        ((PRBool)0)


#define SSL_RENEGOTIATE_UNRESTRICTED ((PRBool)1)


#define SSL_RENEGOTIATE_REQUIRES_XTN ((PRBool)2) 




#define SSL_RENEGOTIATE_TRANSITIONAL ((PRBool)3)






SSL_IMPORT SECStatus SSL_ResetHandshake(PRFileDesc *fd, PRBool asServer);





SSL_IMPORT SECStatus SSL_ForceHandshake(PRFileDesc *fd);




SSL_IMPORT SECStatus SSL_ForceHandshakeWithTimeout(PRFileDesc *fd,
                                                   PRIntervalTime timeout);












SSL_IMPORT SECStatus SSL_SecurityStatus(PRFileDesc *fd, int *on, char **cipher,
			                int *keySize, int *secretKeySize,
			                char **issuer, char **subject);


#define SSL_SECURITY_STATUS_NOOPT	-1
#define SSL_SECURITY_STATUS_OFF		0
#define SSL_SECURITY_STATUS_ON_HIGH	1
#define SSL_SECURITY_STATUS_ON_LOW	2
#define SSL_SECURITY_STATUS_FORTEZZA	3 /* NO LONGER SUPPORTED */








SSL_IMPORT CERTCertificate *SSL_PeerCertificate(PRFileDesc *fd);






typedef SECStatus (PR_CALLBACK *SSLAuthCertificate)(void *arg, PRFileDesc *fd, 
                                                    PRBool checkSig,
                                                    PRBool isServer);

SSL_IMPORT SECStatus SSL_AuthCertificateHook(PRFileDesc *fd, 
					     SSLAuthCertificate f,
				             void *arg);


SSL_IMPORT SECStatus SSL_AuthCertificate(void *arg, PRFileDesc *fd, 
					 PRBool checkSig, PRBool isServer);








typedef SECStatus (PR_CALLBACK *SSLGetClientAuthData)(void *arg,
                                PRFileDesc *fd,
                                CERTDistNames *caNames,
                                CERTCertificate **pRetCert,
                                SECKEYPrivateKey **pRetKey);








SSL_IMPORT SECStatus SSL_GetClientAuthDataHook(PRFileDesc *fd, 
			                       SSLGetClientAuthData f, void *a);




























typedef PRInt32 (PR_CALLBACK *SSLSNISocketConfig)(PRFileDesc *fd,
                                            const SECItem *srvNameArr,
                                                  PRUint32 srvNameArrSize,
                                                  void *arg);








#define SSL_SNI_CURRENT_CONFIG_IS_USED           -1
#define SSL_SNI_SEND_ALERT                       -2




SSL_IMPORT SECStatus SSL_SNISocketConfigHook(PRFileDesc *fd, 
                                             SSLSNISocketConfig f,
                                             void *arg);






SSL_IMPORT PRFileDesc *SSL_ReconfigFD(PRFileDesc *model, PRFileDesc *fd);






SSL_IMPORT SECStatus SSL_SetPKCS11PinArg(PRFileDesc *fd, void *a);






typedef SECStatus (PR_CALLBACK *SSLBadCertHandler)(void *arg, PRFileDesc *fd);
SSL_IMPORT SECStatus SSL_BadCertHook(PRFileDesc *fd, SSLBadCertHandler f, 
				     void *arg);






SSL_IMPORT SECStatus SSL_ConfigSecureServer(
				PRFileDesc *fd, CERTCertificate *cert,
				SECKEYPrivateKey *key, SSLKEAType kea);





SSL_IMPORT SECStatus
SSL_ConfigSecureServerWithCertChain(PRFileDesc *fd, CERTCertificate *cert,
                                    const CERTCertificateList *certChainOpt,
                                    SECKEYPrivateKey *key, SSLKEAType kea);









SSL_IMPORT SECStatus SSL_ConfigServerSessionIDCache(int      maxCacheEntries,
					            PRUint32 timeout,
					            PRUint32 ssl3_timeout,
				              const char *   directory);



SSL_IMPORT SECStatus SSL_ConfigServerSessionIDCacheWithOpt(
                                                           PRUint32 timeout,
                                                       PRUint32 ssl3_timeout,
                                                     const char *   directory,
                                                          int maxCacheEntries,
                                                      int maxCertCacheEntries,
                                                    int maxSrvNameCacheEntries,
                                                           PRBool enableMPCache);










SSL_IMPORT SECStatus SSL_ConfigMPServerSIDCache(int      maxCacheEntries, 
				                PRUint32 timeout,
			       	                PRUint32 ssl3_timeout, 
		                          const char *   directory);









SSL_IMPORT PRUint32  SSL_GetMaxServerCacheLocks(void);
SSL_IMPORT SECStatus SSL_SetMaxServerCacheLocks(PRUint32 maxLocks);




#define SSL_ENV_VAR_NAME            "SSL_INHERITANCE"






SSL_IMPORT SECStatus SSL_InheritMPServerSIDCache(const char * envString);





typedef void (PR_CALLBACK *SSLHandshakeCallback)(PRFileDesc *fd,
                                                 void *client_data);
SSL_IMPORT SECStatus SSL_HandshakeCallback(PRFileDesc *fd, 
			          SSLHandshakeCallback cb, void *client_data);









SSL_IMPORT SECStatus SSL_ReHandshake(PRFileDesc *fd, PRBool flushCache);




SSL_IMPORT SECStatus SSL_ReHandshakeWithTimeout(PRFileDesc *fd,
                                                PRBool flushCache,
                                                PRIntervalTime timeout);


#ifdef SSL_DEPRECATED_FUNCTION 






SSL_IMPORT SECStatus SSL_RedoHandshake(PRFileDesc *fd);
#endif




SSL_IMPORT SECStatus SSL_SetURL(PRFileDesc *fd, const char *url);





SSL_IMPORT SECStatus SSL_SetTrustAnchors(PRFileDesc *fd, CERTCertList *list);





SSL_IMPORT int SSL_DataPending(PRFileDesc *fd);




SSL_IMPORT SECStatus SSL_InvalidateSession(PRFileDesc *fd);




SSL_IMPORT SECItem *SSL_GetSessionID(PRFileDesc *fd);




SSL_IMPORT void SSL_ClearSessionCache(void);




SSL_IMPORT SECStatus SSL_ShutdownServerSessionIDCache(void);





SSL_IMPORT SECStatus SSL_SetSockPeerID(PRFileDesc *fd, const char *peerID);




SSL_IMPORT CERTCertificate * SSL_RevealCert(PRFileDesc * socket);
SSL_IMPORT void * SSL_RevealPinArg(PRFileDesc * socket);
SSL_IMPORT char * SSL_RevealURL(PRFileDesc * socket);










SSL_IMPORT SECStatus
NSS_GetClientAuthData(void *                       arg,
                      PRFileDesc *                 socket,
                      struct CERTDistNamesStr *    caNames,
                      struct CERTCertificateStr ** pRetCert,
                      struct SECKEYPrivateKeyStr **pRetKey);







SSL_IMPORT SECStatus NSS_CmpCertChainWCANames(CERTCertificate *cert, 
                                          CERTDistNames *caNames);




SSL_IMPORT SSLKEAType NSS_FindCertKEAType(CERTCertificate * cert);




SSL_IMPORT SECStatus NSS_SetDomesticPolicy(void);









SSL_IMPORT SECStatus NSS_SetExportPolicy(void);






SSL_IMPORT SECStatus NSS_SetFrancePolicy(void);

SSL_IMPORT SSL3Statistics * SSL_GetStatistics(void);




SSL_IMPORT SECStatus SSL_GetChannelInfo(PRFileDesc *fd, SSLChannelInfo *info,
                                        PRUintn len);
SSL_IMPORT SECStatus SSL_GetCipherSuiteInfo(PRUint16 cipherSuite, 
                                        SSLCipherSuiteInfo *info, PRUintn len);


SSL_IMPORT SECItem *SSL_GetNegotiatedHostInfo(PRFileDesc *fd);





SSL_IMPORT CERTCertificate * SSL_LocalCertificate(PRFileDesc *fd);























#define SSL_CBP_SSL3	0x0001	        /* test SSL v3 mechanisms */
#define SSL_CBP_TLS1_0	0x0002		/* test TLS v1.0 mechanisms */

SSL_IMPORT SECStatus SSL_CanBypass(CERTCertificate *cert,
                                   SECKEYPrivateKey *privKey,
				   PRUint32 protocolmask,
				   PRUint16 *ciphers, int nciphers,
                                   PRBool *pcanbypass, void *pwArg);





SSL_IMPORT SECStatus SSL_HandshakeNegotiatedExtension(PRFileDesc * socket,
                                                      SSLExtensionType extId,
                                                      PRBool *yes);

SEC_END_PROTOS

#endif 
