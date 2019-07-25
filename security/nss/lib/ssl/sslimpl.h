










































#ifndef __sslimpl_h_
#define __sslimpl_h_

#ifdef DEBUG
#undef NDEBUG
#else
#undef NDEBUG
#define NDEBUG
#endif
#include "secport.h"
#include "secerr.h"
#include "sslerr.h"
#include "ssl3prot.h"
#include "hasht.h"
#include "nssilock.h"
#include "pkcs11t.h"
#if defined(XP_UNIX) || defined(XP_BEOS)
#include "unistd.h"
#endif
#include "nssrwlk.h"
#include "prthread.h"

#include "sslt.h" 





typedef SSLKEAType      SSL3KEAType;
typedef SSLMACAlgorithm SSL3MACAlgorithm;
typedef SSLSignType     SSL3SignType;

#define sign_null	ssl_sign_null
#define sign_rsa	ssl_sign_rsa
#define sign_dsa	ssl_sign_dsa
#define sign_ecdsa	ssl_sign_ecdsa

#define calg_null	ssl_calg_null
#define calg_rc4	ssl_calg_rc4
#define calg_rc2	ssl_calg_rc2
#define calg_des	ssl_calg_des
#define calg_3des	ssl_calg_3des
#define calg_idea	ssl_calg_idea
#define calg_fortezza	ssl_calg_fortezza /* deprecated, must preserve */
#define calg_aes	ssl_calg_aes
#define calg_camellia	ssl_calg_camellia
#define calg_seed	ssl_calg_seed

#define mac_null	ssl_mac_null
#define mac_md5 	ssl_mac_md5
#define mac_sha 	ssl_mac_sha
#define hmac_md5	ssl_hmac_md5
#define hmac_sha	ssl_hmac_sha

#define SET_ERROR_CODE
#define SEND_ALERT
#define TEST_FOR_FAILURE
#define DEAL_WITH_FAILURE

#if defined(DEBUG) || defined(TRACE)
#ifdef __cplusplus
#define Debug 1
#else
extern int Debug;
#endif
#else
#undef Debug
#endif

#if defined(DEBUG) && !defined(TRACE) && !defined(NISCC_TEST)
#define TRACE
#endif

#ifdef TRACE
#define SSL_TRC(a,b) if (ssl_trace >= (a)) ssl_Trace b
#define PRINT_BUF(a,b) if (ssl_trace >= (a)) ssl_PrintBuf b
#define DUMP_MSG(a,b) if (ssl_trace >= (a)) ssl_DumpMsg b
#else
#define SSL_TRC(a,b)
#define PRINT_BUF(a,b)
#define DUMP_MSG(a,b)
#endif

#ifdef DEBUG
#define SSL_DBG(b) if (ssl_debug) ssl_Trace b
#else
#define SSL_DBG(b)
#endif

#include "private/pprthred.h"	
#define ssl_InMonitor(m) PZ_InMonitor(m)

#define LSB(x) ((unsigned char) ((x) & 0xff))
#define MSB(x) ((unsigned char) (((unsigned)(x)) >> 8))



typedef enum { SSLAppOpRead = 0,
	       SSLAppOpWrite,
	       SSLAppOpRDWR,
	       SSLAppOpPost,
	       SSLAppOpHeader
} SSLAppOperation;

#define SSL_MIN_MASTER_KEY_BYTES	5
#define SSL_MAX_MASTER_KEY_BYTES	64

#define SSL2_SESSIONID_BYTES		16
#define SSL3_SESSIONID_BYTES		32

#define SSL_MIN_CHALLENGE_BYTES		16
#define SSL_MAX_CHALLENGE_BYTES		32
#define SSL_CHALLENGE_BYTES		16

#define SSL_CONNECTIONID_BYTES		16

#define SSL_MIN_CYPHER_ARG_BYTES	0
#define SSL_MAX_CYPHER_ARG_BYTES	32

#define SSL_MAX_MAC_BYTES		16

#define SSL3_RSA_PMS_LENGTH 48
#define SSL3_MASTER_SECRET_LENGTH 48


#define SSL_NUM_WRAP_MECHS              16


#define SSL_MAX_CACHED_CERT_LEN		4060

#define NUM_MIXERS                      9


#ifndef NSS_ECC_MORE_THAN_SUITE_B
#define SSL3_SUPPORTED_CURVES_MASK 0x3800000	/* only 3 curves, suite B*/
#else
#define SSL3_SUPPORTED_CURVES_MASK 0x3fffffe
#endif

#ifndef BPB
#define BPB 8 /* Bits Per Byte */
#endif

#define EXPORT_RSA_KEY_LENGTH 64	/* bytes */

typedef struct sslBufferStr             sslBuffer;
typedef struct sslConnectInfoStr        sslConnectInfo;
typedef struct sslGatherStr             sslGather;
typedef struct sslSecurityInfoStr       sslSecurityInfo;
typedef struct sslSessionIDStr          sslSessionID;
typedef struct sslSocketStr             sslSocket;
typedef struct sslSocketOpsStr          sslSocketOps;

typedef struct ssl3StateStr             ssl3State;
typedef struct ssl3CertNodeStr          ssl3CertNode;
typedef struct ssl3BulkCipherDefStr     ssl3BulkCipherDef;
typedef struct ssl3MACDefStr            ssl3MACDef;
typedef struct ssl3KeyPairStr		ssl3KeyPair;

struct ssl3CertNodeStr {
    struct ssl3CertNodeStr *next;
    CERTCertificate *       cert;
};

typedef SECStatus (*sslHandshakeFunc)(sslSocket *ss);






typedef PRInt32       (*sslSendFunc)(sslSocket *ss, const unsigned char *buf,
			             PRInt32 n, PRInt32 flags);

typedef void          (*sslSessionIDCacheFunc)  (sslSessionID *sid);
typedef void          (*sslSessionIDUncacheFunc)(sslSessionID *sid);
typedef sslSessionID *(*sslSessionIDLookupFunc)(const PRIPv6Addr    *addr,
						unsigned char* sid,
						unsigned int   sidLen,
                                                CERTCertDBHandle * dbHandle);




typedef PRInt32 (*ssl3HelloExtensionSenderFunc)(sslSocket *ss, PRBool append,
						PRUint32 maxBytes);




typedef SECStatus (* ssl3HelloExtensionHandlerFunc)(sslSocket *ss,
						    PRUint16   ex_type,
                                                    SECItem *  data);


typedef struct {
    PRInt32                      ex_type;
    ssl3HelloExtensionSenderFunc ex_sender;
} ssl3HelloExtensionSender;


typedef struct {
    PRInt32                       ex_type;
    ssl3HelloExtensionHandlerFunc ex_handler;
} ssl3HelloExtensionHandler;

extern SECStatus 
ssl3_RegisterServerHelloExtensionSender(sslSocket *ss, PRUint16 ex_type,
				        ssl3HelloExtensionSenderFunc cb);

extern PRInt32
ssl3_CallHelloExtensionSenders(sslSocket *ss, PRBool append, PRUint32 maxBytes,
                               const ssl3HelloExtensionSender *sender);


struct sslSocketOpsStr {
    int         (*connect) (sslSocket *, const PRNetAddr *);
    PRFileDesc *(*accept)  (sslSocket *, PRNetAddr *);
    int         (*bind)    (sslSocket *, const PRNetAddr *);
    int         (*listen)  (sslSocket *, int);
    int         (*shutdown)(sslSocket *, int);
    int         (*close)   (sslSocket *);

    int         (*recv)    (sslSocket *, unsigned char *, int, int);

    
    int         (*send)    (sslSocket *, const unsigned char *, int, int);
    int         (*read)    (sslSocket *, unsigned char *, int);
    int         (*write)   (sslSocket *, const unsigned char *, int);

    int         (*getpeername)(sslSocket *, PRNetAddr *);
    int         (*getsockname)(sslSocket *, PRNetAddr *);
};


#define ssl_SEND_FLAG_FORCE_INTO_BUFFER	0x40000000
#define ssl_SEND_FLAG_NO_BUFFER		0x20000000
#define ssl_SEND_FLAG_MASK		0x7f000000




struct sslBufferStr {
    unsigned char *	buf;
    unsigned int 	len;
    unsigned int 	space;
};




typedef struct {
#if !defined(_WIN32)
    unsigned int    cipher_suite : 16;
    unsigned int    policy       :  8;
    unsigned int    enabled      :  1;
    unsigned int    isPresent    :  1;
#else
    ssl3CipherSuite cipher_suite;
    PRUint8         policy;
    unsigned char   enabled   : 1;
    unsigned char   isPresent : 1;
#endif
} ssl3CipherSuiteCfg;

#ifdef NSS_ENABLE_ECC
#define ssl_V3_SUITES_IMPLEMENTED 50
#else
#define ssl_V3_SUITES_IMPLEMENTED 30
#endif 

typedef struct sslOptionsStr {
    unsigned int useSecurity		: 1;  
    unsigned int useSocks		: 1;  
    unsigned int requestCertificate	: 1;  
    unsigned int requireCertificate	: 2;  
    unsigned int handshakeAsClient	: 1;  
    unsigned int handshakeAsServer	: 1;  
    unsigned int enableSSL2		: 1;  
    unsigned int enableSSL3		: 1;  
    unsigned int enableTLS		: 1;  
    unsigned int noCache		: 1;  
    unsigned int fdx			: 1;  
    unsigned int v2CompatibleHello	: 1;  
    unsigned int detectRollBack  	: 1;  
    unsigned int noStepDown             : 1;  
    unsigned int bypassPKCS11           : 1;  
    unsigned int noLocks                : 1;  
    unsigned int enableSessionTickets   : 1;  
    unsigned int enableDeflate          : 1;  
    unsigned int enableRenegotiation    : 2;  
    unsigned int requireSafeNegotiation : 1;  
    unsigned int enableFalseStart       : 1;  
} sslOptions;

typedef enum { sslHandshakingUndetermined = 0,
	       sslHandshakingAsClient,
	       sslHandshakingAsServer 
} sslHandshakingType;

typedef struct sslServerCertsStr {
    
    CERTCertificate *     serverCert;
    CERTCertificateList * serverCertChain;
    ssl3KeyPair *         serverKeyPair;
    unsigned int          serverKeyBits;
} sslServerCerts;

#define SERVERKEY serverKeyPair->privKey

#define SSL_LOCK_RANK_SPEC 	255
#define SSL_LOCK_RANK_GLOBAL 	NSS_RWLOCK_RANK_NONE







#define ssl_SHUTDOWN_NONE	0	/* NOT shutdown at all */
#define ssl_SHUTDOWN_RCV	1	/* PR_SHUTDOWN_RCV  +1 */
#define ssl_SHUTDOWN_SEND	2	/* PR_SHUTDOWN_SEND +1 */
#define ssl_SHUTDOWN_BOTH	3	/* PR_SHUTDOWN_BOTH +1 */






struct sslGatherStr {
    int           state;	     

    




    sslBuffer     buf;					

    



    unsigned int  offset;                                       

    
    unsigned int  remainder;                                    

    
    unsigned int  count;					

    


    unsigned int  recordLen;					

    
    


    unsigned int  recordPadding;				

    
    unsigned int  recordOffset;					

    int           encrypted;    

    







    unsigned int  readOffset;  



    
    unsigned int  writeOffset; 

    
    sslBuffer     inbuf;				

    





    unsigned char hdr[5];					
};


#define GS_INIT		0
#define GS_HEADER	1
#define GS_MAC		2
#define GS_DATA		3
#define GS_PAD		4

typedef SECStatus (*SSLCipher)(void *               context, 
                               unsigned char *      out,
			       int *                outlen, 
			       int                  maxout, 
			       const unsigned char *in,
			       int                  inlen);
typedef SECStatus (*SSLCompressor)(void *               context,
                                   unsigned char *      out,
                                   int *                outlen,
                                   int                  maxout,
                                   const unsigned char *in,
                                   int                  inlen);
typedef SECStatus (*SSLDestroy)(void *context, PRBool freeit);








typedef enum {
    cipher_null,
    cipher_rc4, 
    cipher_rc4_40,
    cipher_rc4_56,
    cipher_rc2, 
    cipher_rc2_40,
    cipher_des, 
    cipher_3des, 
    cipher_des40,
    cipher_idea, 
    cipher_aes_128,
    cipher_aes_256,
    cipher_camellia_128,
    cipher_camellia_256,
    cipher_seed,
    cipher_missing              
    
} SSL3BulkCipher;

typedef enum { type_stream, type_block } CipherType;

#define MAX_IV_LENGTH 64




typedef struct {
    PRUint32         high;
    PRUint32         low;
} SSL3SequenceNumber;

#define MAX_MAC_CONTEXT_BYTES 400
#define MAX_MAC_CONTEXT_LLONGS (MAX_MAC_CONTEXT_BYTES / 8)

#define MAX_CIPHER_CONTEXT_BYTES 2080
#define MAX_CIPHER_CONTEXT_LLONGS (MAX_CIPHER_CONTEXT_BYTES / 8)

typedef struct {
    SSL3Opaque        client_write_iv         [24];
    SSL3Opaque        server_write_iv         [24];
    SSL3Opaque        wrapped_master_secret   [48];
    PRUint16          wrapped_master_secret_len;
    PRUint8           msIsWrapped;
    PRUint8           resumable;
} ssl3SidKeys; 

typedef struct {
    PK11SymKey  *write_key;
    PK11SymKey  *write_mac_key;
    PK11Context *write_mac_context;
    SECItem     write_key_item;
    SECItem     write_iv_item;
    SECItem     write_mac_key_item;
    SSL3Opaque  write_iv[MAX_IV_LENGTH];
    PRUint64    cipher_context[MAX_CIPHER_CONTEXT_LLONGS];
} ssl3KeyMaterial;






typedef struct {
    const ssl3BulkCipherDef *cipher_def;
    const ssl3MACDef * mac_def;
    SSLCompressionMethod compression_method;
    int                mac_size;
    SSLCipher          encode;
    SSLCipher          decode;
    SSLDestroy         destroy;
    void *             encodeContext;
    void *             decodeContext;
    SSLCompressor      compressor;    
    SSLCompressor      decompressor;  
                                       
    SSLDestroy         destroyCompressContext;
    void *             compressContext;
    SSLDestroy         destroyDecompressContext;
    void *             decompressContext;
    PRBool             bypassCiphers;	
    PK11SymKey *       master_secret;
    SSL3SequenceNumber write_seq_num;
    SSL3SequenceNumber read_seq_num;
    SSL3ProtocolVersion version;
    ssl3KeyMaterial    client;
    ssl3KeyMaterial    server;
    SECItem            msItem;
    unsigned char      key_block[NUM_MIXERS * MD5_LENGTH];
    unsigned char      raw_master_secret[56];
    SECItem            srvVirtName;    


} ssl3CipherSpec;

typedef enum {	never_cached, 
		in_client_cache, 
		in_server_cache, 
		invalid_cache		
} Cached;

struct sslSessionIDStr {
    sslSessionID *        next;   

    CERTCertificate *     peerCert;
    const char *          peerID;     
    const char *          urlSvrName; 
    CERTCertificate *     localCert;

    PRIPv6Addr            addr;
    PRUint16              port;

    SSL3ProtocolVersion   version;

    PRUint32              creationTime;		
    PRUint32              lastAccessTime;	
    PRUint32              expirationTime;	
    Cached                cached;
    int                   references;

    SSLSignType           authAlgorithm;
    PRUint32              authKeyBits;
    SSLKEAType            keaType;
    PRUint32              keaKeyBits;

    union {
	struct {
	    
	    unsigned char         sessionID[SSL2_SESSIONID_BYTES];

	    
	    SECItem               masterKey;        
	    int                   cipherType;
	    SECItem               cipherArg;
	    int                   keyBits;
	    int                   secretKeyBits;
	} ssl2;
	struct {
	    
	    uint8                 sessionIDLength;
	    SSL3Opaque            sessionID[SSL3_SESSIONID_BYTES];

	    ssl3CipherSuite       cipherSuite;
	    SSLCompressionMethod  compression;
	    int                   policy;
	    ssl3SidKeys           keys;
	    CK_MECHANISM_TYPE     masterWrapMech;
				  
            SSL3KEAType           exchKeyType;
				  

#ifdef NSS_ENABLE_ECC
	    PRUint32              negotiatedECCurves;
#endif 

	    


 	    PK11SymKey *      clientWriteKey;
	    PK11SymKey *      serverWriteKey;

	    


	    SECMODModuleID    masterModuleID;
				    
	    CK_SLOT_ID        masterSlotID;
	    PRUint16	      masterWrapIndex;
				
	    PRUint16          masterWrapSeries;
	                        



	    


	    SECMODModuleID    clAuthModuleID;
	    CK_SLOT_ID        clAuthSlotID;
	    PRUint16          clAuthSeries;

            char              masterValid;
	    char              clAuthValid;

	    


	    NewSessionTicket  sessionTicket;
            SECItem           srvName;
	} ssl3;
    } u;
};


typedef struct ssl3CipherSuiteDefStr {
    ssl3CipherSuite          cipher_suite;
    SSL3BulkCipher           bulk_cipher_alg;
    SSL3MACAlgorithm         mac_alg;
    SSL3KeyExchangeAlgorithm key_exchange_alg;
} ssl3CipherSuiteDef;




typedef struct {
    SSL3KeyExchangeAlgorithm kea;
    SSL3KEAType              exchKeyType;
    SSL3SignType             signKeyType;
    PRBool                   is_limited;
    int                      key_size_limit;
    PRBool                   tls_keygen;
} ssl3KEADef;

typedef enum { kg_null, kg_strong, kg_export } SSL3KeyGenMode;




struct ssl3BulkCipherDefStr {
    SSL3BulkCipher  cipher;
    SSLCipherAlgorithm calg;
    int             key_size;
    int             secret_key_size;
    CipherType      type;
    int             iv_size;
    int             block_size;
    SSL3KeyGenMode  keygen_mode;
};




struct ssl3MACDefStr {
    SSL3MACAlgorithm mac;
    CK_MECHANISM_TYPE mmech;
    int              pad_size;
    int              mac_size;
};

typedef enum {
    wait_client_hello, 
    wait_client_cert, 
    wait_client_key,
    wait_cert_verify, 
    wait_change_cipher, 
    wait_finished,
    wait_server_hello, 
    wait_server_cert, 
    wait_server_key,
    wait_cert_request, 
    wait_hello_done,
    wait_new_session_ticket,
    idle_handshake
} SSL3WaitState;




typedef struct TLSExtensionDataStr       TLSExtensionData;
typedef struct SessionTicketDataStr      SessionTicketData;

struct TLSExtensionDataStr {
    
    ssl3HelloExtensionSender serverSenders[SSL_MAX_EXTENSIONS];
    
    PRUint16 numAdvertised;
    PRUint16 numNegotiated;
    PRUint16 advertised[SSL_MAX_EXTENSIONS];
    PRUint16 negotiated[SSL_MAX_EXTENSIONS];

    
    PRBool ticketTimestampVerified;
    PRBool emptySessionTicket;

    



    SECItem *sniNameArr;
    PRUint32 sniNameArrSize;
};





typedef struct SSL3HandshakeStateStr {
    SSL3Random            server_random;
    SSL3Random            client_random;
    SSL3WaitState         ws;
    PRUint64              md5_cx[MAX_MAC_CONTEXT_LLONGS];
    PRUint64              sha_cx[MAX_MAC_CONTEXT_LLONGS];
    PK11Context *         md5;            
    PK11Context *         sha;
const ssl3KEADef *        kea_def;
    ssl3CipherSuite       cipher_suite;
const ssl3CipherSuiteDef *suite_def;
    SSLCompressionMethod  compression;
    sslBuffer             msg_body;    
                               
    unsigned int          header_bytes; 
                               
                               
    SSL3HandshakeType     msg_type;
    unsigned long         msg_len;
    SECItem               ca_list;     
    PRBool                isResuming;  
    PRBool                rehandshake; 

    PRBool                usedStepDownKey;  
    PRBool                sendingSCSV; 
    sslBuffer             msgState;    
                                       
    sslBuffer             messages;    
    PRUint16              finishedBytes; 
    union {
	TLSFinished       tFinished[2]; 
	SSL3Hashes        sFinished[2];
	SSL3Opaque        data[72];
    }                     finishedMsgs;
#ifdef NSS_ENABLE_ECC
    PRUint32              negotiatedECCurves; 
#endif 
} SSL3HandshakeState;











struct ssl3StateStr {

    



    ssl3CipherSpec *     crSpec; 	
    ssl3CipherSpec *     prSpec; 	
    ssl3CipherSpec *     cwSpec; 	
    ssl3CipherSpec *     pwSpec; 	

    CERTCertificate *    clientCertificate;  
    SECKEYPrivateKey *   clientPrivateKey;   
    CERTCertificateList *clientCertChain;    
    PRBool               sendEmptyCert;      

    int                  policy;
			


    PRArenaPool *        peerCertArena;  
			    
    void *               peerCertChain;     
			    
    CERTDistNames *      ca_list; 
			    
    PRBool               initialized;
    SSL3HandshakeState   hs;
    ssl3CipherSpec       specs[2];	
};

typedef struct {
    SSL3ContentType      type;
    SSL3ProtocolVersion  version;
    sslBuffer *          buf;
} SSL3Ciphertext;

struct ssl3KeyPairStr {
    SECKEYPrivateKey *    privKey;
    SECKEYPublicKey *     pubKey;
    PRInt32               refCount;	
};

typedef struct SSLWrappedSymWrappingKeyStr {
    SSL3Opaque        wrappedSymmetricWrappingkey[512];
    SSL3Opaque        wrapIV[24];
    CK_MECHANISM_TYPE symWrapMechanism;  
		    
    CK_MECHANISM_TYPE asymWrapMechanism; 
		    

    SSL3KEAType       exchKeyType;   
    PRInt32           symWrapMechIndex;
    PRUint16          wrappedSymKeyLen;
    PRUint16          wrapIVLen;
} SSLWrappedSymWrappingKey;

typedef struct SessionTicketStr {
    uint16                ticket_version;
    SSL3ProtocolVersion   ssl_version;
    ssl3CipherSuite       cipher_suite;
    SSLCompressionMethod  compression_method;
    SSLSignType           authAlgorithm;
    uint32                authKeyBits;
    SSLKEAType            keaType;
    uint32                keaKeyBits;
    



    uint8                 ms_is_wrapped;
    SSLKEAType            exchKeyType; 
    CK_MECHANISM_TYPE     msWrapMech;
    uint16                ms_length;
    SSL3Opaque            master_secret[48];
    ClientIdentity        client_identity;
    SECItem               peer_cert;
    uint32                timestamp;
    SECItem               srvName; 
}  SessionTicket;















struct sslConnectInfoStr {
    
    sslBuffer       sendBuf;	                 

    PRIPv6Addr      peer;                                       
    unsigned short  port;                                       

    sslSessionID   *sid;                                        

    
    char            elements;					
    char            requiredElements;				
    char            sentElements;                               

    char            sentFinished;                               

    
    int             serverChallengeLen;                         
    
    unsigned char   authType;                                   

    
    
    unsigned char   clientChallenge[SSL_MAX_CHALLENGE_BYTES];   

    
    unsigned char   connectionID[SSL_CONNECTIONID_BYTES];	

    
    unsigned char   serverChallenge[SSL_MAX_CHALLENGE_BYTES];	

    
    unsigned char   readKey[SSL_MAX_MASTER_KEY_BYTES];		
    unsigned char   writeKey[SSL_MAX_MASTER_KEY_BYTES];		
    unsigned        keySize;					
};


#define CIS_HAVE_MASTER_KEY		0x01
#define CIS_HAVE_CERTIFICATE		0x02
#define CIS_HAVE_FINISHED		0x04
#define CIS_HAVE_VERIFY			0x08







struct sslSecurityInfoStr {
    sslSendFunc      send;				
    int              isServer;				
    sslBuffer        writeBuf;				

    int              cipherType;				
    int              keyBits;					
    int              secretKeyBits;				
    CERTCertificate *localCert;					
    CERTCertificate *peerCert;					
    SECKEYPublicKey *peerKey;					

    SSLSignType      authAlgorithm;
    PRUint32         authKeyBits;
    SSLKEAType       keaType;
    PRUint32         keaKeyBits;

    




    sslSessionIDCacheFunc     cache;				
    sslSessionIDUncacheFunc   uncache;				

    




    PRUint32           sendSequence;			
    PRUint32           rcvSequence;			

    
    const SECHashObject   *hash;		 
    void            *hashcx;				

    SECItem          sendSecret;			
    SECItem          rcvSecret;				

    
    void            *readcx;				
    void            *writecx;				
    SSLCipher        enc;				
    SSLCipher        dec;				
    void           (*destroy)(void *, PRBool);		

    
    int              blockShift;			
    int              blockSize;				

    
    sslConnectInfo   ci;					

};







struct sslSocketStr {
    PRFileDesc *	fd;

    
    const sslSocketOps * ops;

    
    sslOptions       opt;

    
    unsigned long    clientAuthRequested;
    unsigned long    delayDisabled;       
    unsigned long    firstHsDone;         
    unsigned long    handshakeBegun;     
    unsigned long    lastWriteBlocked;   
    unsigned long    recvdCloseNotify;    
    unsigned long    TCPconnected;       
    unsigned long    appDataBuffered;
    unsigned long    peerRequestedProtection; 

    
    SSL3ProtocolVersion version;
    SSL3ProtocolVersion clientHelloVersion; 

    sslSecurityInfo  sec;		

    
    const char      *url;				

    sslHandshakeFunc handshake;				
    sslHandshakeFunc nextHandshake;			
    sslHandshakeFunc securityHandshake;			

    
    char *           peerID;	

    unsigned char *  cipherSpecs;
    unsigned int     sizeCipherSpecs;
const unsigned char *  preferredCipher;

    ssl3KeyPair *         stepDownKeyPair;	

    
    SSLAuthCertificate        authCertificate;
    void                     *authCertificateArg;
    SSLGetClientAuthData      getClientAuthData;
    void                     *getClientAuthDataArg;
    SSLSNISocketConfig        sniSocketConfig;
    void                     *sniSocketConfigArg;
    SSLBadCertHandler         handleBadCert;
    void                     *badCertArg;
    SSLHandshakeCallback      handshakeCallback;
    void                     *handshakeCallbackData;
    void                     *pkcs11PinArg;

    PRIntervalTime            rTimeout; 
    PRIntervalTime            wTimeout; 
    PRIntervalTime            cTimeout; 

    PZLock *      recvLock;	
    PZLock *      sendLock;	

    PZMonitor *   recvBufLock;	
    PZMonitor *   xmitBufLock;	

    



    PZMonitor *   firstHandshakeLock; 

    


    PZMonitor *   ssl3HandshakeLock;

    


    NSSRWLock *   specLock;

    


    CERTCertDBHandle * dbHandle;

    PRThread *  writerThread;   

    PRUint16	shutdownHow; 	

    PRUint16	allowedByPolicy;          
    PRUint16	maybeAllowedByPolicy;     
    PRUint16	chosenPreference;         

    sslHandshakingType handshaking;

    
    sslGather        gs;				

    sslBuffer        saveBuf;				
    sslBuffer        pendingBuf;			

    
    
    sslServerCerts        serverCerts[kt_kea_size];

    ssl3CipherSuiteCfg cipherSuites[ssl_V3_SUITES_IMPLEMENTED];
    ssl3KeyPair *         ephemeralECDHKeyPair; 

    
    ssl3State        ssl3;

    


    
    PRBool               statelessResume;
    TLSExtensionData     xtnData;
};






extern NSSRWLock *             ssl_global_data_lock;
extern char                    ssl_debug;
extern char                    ssl_trace;
extern FILE *                  ssl_trace_iob;
extern FILE *                  ssl_keylog_iob;
extern CERTDistNames *         ssl3_server_ca_list;
extern PRUint32                ssl_sid_timeout;
extern PRUint32                ssl3_sid_timeout;
extern PRBool                  ssl3_global_policy_some_restricted;

extern const char * const      ssl_cipherName[];
extern const char * const      ssl3_cipherName[];

extern sslSessionIDLookupFunc  ssl_sid_lookup;
extern sslSessionIDCacheFunc   ssl_sid_cache;
extern sslSessionIDUncacheFunc ssl_sid_uncache;



SEC_BEGIN_PROTOS


extern int ssl_DefConnect(sslSocket *ss, const PRNetAddr *addr);
extern PRFileDesc *ssl_DefAccept(sslSocket *ss, PRNetAddr *addr);
extern int ssl_DefBind(sslSocket *ss, const PRNetAddr *addr);
extern int ssl_DefListen(sslSocket *ss, int backlog);
extern int ssl_DefShutdown(sslSocket *ss, int how);
extern int ssl_DefClose(sslSocket *ss);
extern int ssl_DefRecv(sslSocket *ss, unsigned char *buf, int len, int flags);
extern int ssl_DefSend(sslSocket *ss, const unsigned char *buf,
		       int len, int flags);
extern int ssl_DefRead(sslSocket *ss, unsigned char *buf, int len);
extern int ssl_DefWrite(sslSocket *ss, const unsigned char *buf, int len);
extern int ssl_DefGetpeername(sslSocket *ss, PRNetAddr *name);
extern int ssl_DefGetsockname(sslSocket *ss, PRNetAddr *name);
extern int ssl_DefGetsockopt(sslSocket *ss, PRSockOption optname,
			     void *optval, PRInt32 *optlen);
extern int ssl_DefSetsockopt(sslSocket *ss, PRSockOption optname,
			     const void *optval, PRInt32 optlen);


extern int ssl_SocksConnect(sslSocket *ss, const PRNetAddr *addr);
extern PRFileDesc *ssl_SocksAccept(sslSocket *ss, PRNetAddr *addr);
extern int ssl_SocksBind(sslSocket *ss, const PRNetAddr *addr);
extern int ssl_SocksListen(sslSocket *ss, int backlog);
extern int ssl_SocksGetsockname(sslSocket *ss, PRNetAddr *name);
extern int ssl_SocksRecv(sslSocket *ss, unsigned char *buf, int len, int flags);
extern int ssl_SocksSend(sslSocket *ss, const unsigned char *buf,
			 int len, int flags);
extern int ssl_SocksRead(sslSocket *ss, unsigned char *buf, int len);
extern int ssl_SocksWrite(sslSocket *ss, const unsigned char *buf, int len);


extern int ssl_SecureConnect(sslSocket *ss, const PRNetAddr *addr);
extern PRFileDesc *ssl_SecureAccept(sslSocket *ss, PRNetAddr *addr);
extern int ssl_SecureRecv(sslSocket *ss, unsigned char *buf,
			  int len, int flags);
extern int ssl_SecureSend(sslSocket *ss, const unsigned char *buf,
			  int len, int flags);
extern int ssl_SecureRead(sslSocket *ss, unsigned char *buf, int len);
extern int ssl_SecureWrite(sslSocket *ss, const unsigned char *buf, int len);
extern int ssl_SecureShutdown(sslSocket *ss, int how);
extern int ssl_SecureClose(sslSocket *ss);


extern int ssl_SecureSocksConnect(sslSocket *ss, const PRNetAddr *addr);
extern PRFileDesc *ssl_SecureSocksAccept(sslSocket *ss, PRNetAddr *addr);
extern PRFileDesc *ssl_FindTop(sslSocket *ss);


extern sslGather * ssl_NewGather(void);
extern SECStatus   ssl_InitGather(sslGather *gs);
extern void        ssl_DestroyGather(sslGather *gs);
extern int         ssl2_GatherData(sslSocket *ss, sslGather *gs, int flags);
extern int         ssl2_GatherRecord(sslSocket *ss, int flags);
extern SECStatus   ssl_GatherRecord1stHandshake(sslSocket *ss);

extern SECStatus   ssl2_HandleClientHelloMessage(sslSocket *ss);
extern SECStatus   ssl2_HandleServerHelloMessage(sslSocket *ss);
extern int         ssl2_StartGatherBytes(sslSocket *ss, sslGather *gs, 
                                         unsigned int count);

extern SECStatus   ssl_CreateSecurityInfo(sslSocket *ss);
extern SECStatus   ssl_CopySecurityInfo(sslSocket *ss, sslSocket *os);
extern void        ssl_ResetSecurityInfo(sslSecurityInfo *sec, PRBool doMemset);
extern void        ssl_DestroySecurityInfo(sslSecurityInfo *sec);

extern sslSocket * ssl_DupSocket(sslSocket *old);

extern void        ssl_PrintBuf(sslSocket *ss, const char *msg, const void *cp, int len);
extern void        ssl_DumpMsg(sslSocket *ss, unsigned char *bp, unsigned len);

extern int         ssl_SendSavedWriteData(sslSocket *ss);
extern SECStatus ssl_SaveWriteData(sslSocket *ss, 
                                   const void* p, unsigned int l);
extern SECStatus ssl2_BeginClientHandshake(sslSocket *ss);
extern SECStatus ssl2_BeginServerHandshake(sslSocket *ss);
extern int       ssl_Do1stHandshake(sslSocket *ss);

extern SECStatus sslBuffer_Grow(sslBuffer *b, unsigned int newLen);
extern SECStatus sslBuffer_Append(sslBuffer *b, const void * data, 
		                  unsigned int len);

extern void      ssl2_UseClearSendFunc(sslSocket *ss);
extern void      ssl_ChooseSessionIDProcs(sslSecurityInfo *sec);

extern sslSessionID *ssl3_NewSessionID(sslSocket *ss, PRBool is_server);
extern sslSessionID *ssl_LookupSID(const PRIPv6Addr *addr, PRUint16 port, 
                                   const char *peerID, const char *urlSvrName);
extern void      ssl_FreeSID(sslSessionID *sid);

extern int       ssl3_SendApplicationData(sslSocket *ss, const PRUint8 *in,
				          int len, int flags);

extern PRBool    ssl_FdIsBlocking(PRFileDesc *fd);

extern PRBool    ssl_SocketIsBlocking(sslSocket *ss);

extern void      ssl_SetAlwaysBlock(sslSocket *ss);

extern SECStatus ssl_EnableNagleDelay(sslSocket *ss, PRBool enabled);

extern PRBool    ssl3_CanFalseStart(sslSocket *ss);

#define SSL_LOCK_READER(ss)		if (ss->recvLock) PZ_Lock(ss->recvLock)
#define SSL_UNLOCK_READER(ss)		if (ss->recvLock) PZ_Unlock(ss->recvLock)
#define SSL_LOCK_WRITER(ss)		if (ss->sendLock) PZ_Lock(ss->sendLock)
#define SSL_UNLOCK_WRITER(ss)		if (ss->sendLock) PZ_Unlock(ss->sendLock)

#define ssl_Get1stHandshakeLock(ss)     \
    { if (!ss->opt.noLocks) PZ_EnterMonitor((ss)->firstHandshakeLock); }
#define ssl_Release1stHandshakeLock(ss) \
    { if (!ss->opt.noLocks) PZ_ExitMonitor((ss)->firstHandshakeLock); }
#define ssl_Have1stHandshakeLock(ss)    \
    (PZ_InMonitor((ss)->firstHandshakeLock))

#define ssl_GetSSL3HandshakeLock(ss)	\
    { if (!ss->opt.noLocks) PZ_EnterMonitor((ss)->ssl3HandshakeLock); }
#define ssl_ReleaseSSL3HandshakeLock(ss) \
    { if (!ss->opt.noLocks) PZ_ExitMonitor((ss)->ssl3HandshakeLock); }
#define ssl_HaveSSL3HandshakeLock(ss)	\
    (PZ_InMonitor((ss)->ssl3HandshakeLock))

#define ssl_GetSpecReadLock(ss)		\
    { if (!ss->opt.noLocks) NSSRWLock_LockRead((ss)->specLock); }
#define ssl_ReleaseSpecReadLock(ss)	\
    { if (!ss->opt.noLocks) NSSRWLock_UnlockRead((ss)->specLock); }

#define ssl_GetSpecWriteLock(ss)	\
    { if (!ss->opt.noLocks) NSSRWLock_LockWrite((ss)->specLock); }
#define ssl_ReleaseSpecWriteLock(ss)	\
    { if (!ss->opt.noLocks) NSSRWLock_UnlockWrite((ss)->specLock); }
#define ssl_HaveSpecWriteLock(ss)	\
    (NSSRWLock_HaveWriteLock((ss)->specLock))

#define ssl_GetRecvBufLock(ss)		\
    { if (!ss->opt.noLocks) PZ_EnterMonitor((ss)->recvBufLock); }
#define ssl_ReleaseRecvBufLock(ss)	\
    { if (!ss->opt.noLocks) PZ_ExitMonitor( (ss)->recvBufLock); }
#define ssl_HaveRecvBufLock(ss)		\
    (PZ_InMonitor((ss)->recvBufLock))

#define ssl_GetXmitBufLock(ss)		\
    { if (!ss->opt.noLocks) PZ_EnterMonitor((ss)->xmitBufLock); }
#define ssl_ReleaseXmitBufLock(ss)	\
    { if (!ss->opt.noLocks) PZ_ExitMonitor( (ss)->xmitBufLock); }
#define ssl_HaveXmitBufLock(ss)		\
    (PZ_InMonitor((ss)->xmitBufLock))


extern SECStatus ssl3_KeyAndMacDeriveBypass(ssl3CipherSpec * pwSpec,
		    const unsigned char * cr, const unsigned char * sr,
		    PRBool isTLS, PRBool isExport);
extern  SECStatus ssl3_MasterKeyDeriveBypass( ssl3CipherSpec * pwSpec,
		    const unsigned char * cr, const unsigned char * sr,
		    const SECItem * pms, PRBool isTLS, PRBool isRSA);



extern int ssl2_SendErrorMessage(struct sslSocketStr *ss, int error);
extern int SSL_RestartHandshakeAfterServerCert(struct sslSocketStr *ss);
extern int SSL_RestartHandshakeAfterCertReq(struct sslSocketStr *ss,
					    CERTCertificate *cert,
					    SECKEYPrivateKey *key,
					    CERTCertificateList *certChain);
extern sslSocket *ssl_FindSocket(PRFileDesc *fd);
extern void ssl_FreeSocket(struct sslSocketStr *ssl);
extern SECStatus SSL3_SendAlert(sslSocket *ss, SSL3AlertLevel level,
				SSL3AlertDescription desc);

extern int ssl2_RestartHandshakeAfterCertReq(sslSocket *          ss,
					     CERTCertificate *    cert, 
					     SECKEYPrivateKey *   key);

extern SECStatus ssl3_RestartHandshakeAfterCertReq(sslSocket *    ss,
					     CERTCertificate *    cert, 
					     SECKEYPrivateKey *   key,
					     CERTCertificateList *certChain);

extern int ssl2_RestartHandshakeAfterServerCert(sslSocket *ss);
extern int ssl3_RestartHandshakeAfterServerCert(sslSocket *ss);




extern SECStatus ssl3_HandleV2ClientHello(
    sslSocket *ss, unsigned char *buffer, int length);
extern SECStatus ssl3_StartHandshakeHash(
    sslSocket *ss, unsigned char *buf, int length);




SECStatus ssl3_SendClientHello(sslSocket *ss);




SECStatus ssl3_HandleRecord(
    sslSocket *ss, SSL3Ciphertext *cipher, sslBuffer *out);

int ssl3_GatherAppDataRecord(sslSocket *ss, int flags);
int ssl3_GatherCompleteHandshake(sslSocket *ss, int flags);






extern SECStatus ssl3_CreateRSAStepDownKeys(sslSocket *ss);

#ifdef NSS_ENABLE_ECC
extern void      ssl3_FilterECCipherSuitesByServerCerts(sslSocket *ss);
extern PRBool    ssl3_IsECCEnabled(sslSocket *ss);
extern SECStatus ssl3_DisableECCSuites(sslSocket * ss, 
                                       const ssl3CipherSuite * suite);


#define SSL_RSASTRENGTH_TO_ECSTRENGTH(s) \
        ((s <= 1024) ? 160 \
	  : ((s <= 2048) ? 224 \
	    : ((s <= 3072) ? 256 \
	      : ((s <= 7168) ? 384 : 521 ) ) ) )


typedef enum { ec_type_explicitPrime      = 1,
	       ec_type_explicitChar2Curve = 2,
	       ec_type_named
} ECType;

typedef enum { ec_noName     = 0,
	       ec_sect163k1  = 1, 
	       ec_sect163r1  = 2, 
	       ec_sect163r2  = 3,
	       ec_sect193r1  = 4, 
	       ec_sect193r2  = 5, 
	       ec_sect233k1  = 6,
	       ec_sect233r1  = 7, 
	       ec_sect239k1  = 8, 
	       ec_sect283k1  = 9,
	       ec_sect283r1  = 10, 
	       ec_sect409k1  = 11, 
	       ec_sect409r1  = 12,
	       ec_sect571k1  = 13, 
	       ec_sect571r1  = 14, 
	       ec_secp160k1  = 15,
	       ec_secp160r1  = 16, 
	       ec_secp160r2  = 17, 
	       ec_secp192k1  = 18,
	       ec_secp192r1  = 19, 
	       ec_secp224k1  = 20, 
	       ec_secp224r1  = 21,
	       ec_secp256k1  = 22, 
	       ec_secp256r1  = 23, 
	       ec_secp384r1  = 24,
	       ec_secp521r1  = 25,
	       ec_pastLastName
} ECName;

extern SECStatus ssl3_ECName2Params(PRArenaPool *arena, ECName curve,
				   SECKEYECParams *params);
ECName	ssl3_GetCurveWithECKeyStrength(PRUint32 curvemsk, int requiredECCbits);


#endif 

extern SECStatus ssl3_CipherPrefSetDefault(ssl3CipherSuite which, PRBool on);
extern SECStatus ssl3_CipherPrefGetDefault(ssl3CipherSuite which, PRBool *on);
extern SECStatus ssl2_CipherPrefSetDefault(PRInt32 which, PRBool enabled);
extern SECStatus ssl2_CipherPrefGetDefault(PRInt32 which, PRBool *enabled);

extern SECStatus ssl3_CipherPrefSet(sslSocket *ss, ssl3CipherSuite which, PRBool on);
extern SECStatus ssl3_CipherPrefGet(sslSocket *ss, ssl3CipherSuite which, PRBool *on);
extern SECStatus ssl2_CipherPrefSet(sslSocket *ss, PRInt32 which, PRBool enabled);
extern SECStatus ssl2_CipherPrefGet(sslSocket *ss, PRInt32 which, PRBool *enabled);

extern SECStatus ssl3_SetPolicy(ssl3CipherSuite which, PRInt32 policy);
extern SECStatus ssl3_GetPolicy(ssl3CipherSuite which, PRInt32 *policy);
extern SECStatus ssl2_SetPolicy(PRInt32 which, PRInt32 policy);
extern SECStatus ssl2_GetPolicy(PRInt32 which, PRInt32 *policy);

extern void      ssl2_InitSocketPolicy(sslSocket *ss);
extern void      ssl3_InitSocketPolicy(sslSocket *ss);

extern SECStatus ssl3_ConstructV2CipherSpecsHack(sslSocket *ss,
						 unsigned char *cs, int *size);

extern SECStatus ssl3_RedoHandshake(sslSocket *ss, PRBool flushCache);

extern void ssl3_DestroySSL3Info(sslSocket *ss);

extern SECStatus ssl3_NegotiateVersion(sslSocket *ss, 
                                       SSL3ProtocolVersion peerVersion);

extern SECStatus ssl_GetPeerInfo(sslSocket *ss);

#ifdef NSS_ENABLE_ECC

extern SECStatus ssl3_SendECDHClientKeyExchange(sslSocket * ss, 
			     SECKEYPublicKey * svrPubKey);
extern SECStatus ssl3_HandleECDHServerKeyExchange(sslSocket *ss, 
					SSL3Opaque *b, PRUint32 length);
extern SECStatus ssl3_HandleECDHClientKeyExchange(sslSocket *ss, 
				     SSL3Opaque *b, PRUint32 length,
                                     SECKEYPublicKey *srvrPubKey,
                                     SECKEYPrivateKey *srvrPrivKey);
extern SECStatus ssl3_SendECDHServerKeyExchange(sslSocket *ss);
#endif

extern SECStatus ssl3_ComputeCommonKeyHash(PRUint8 * hashBuf, 
				unsigned int bufLen, SSL3Hashes *hashes, 
				PRBool bypassPKCS11);
extern SECStatus ssl3_InitPendingCipherSpec(sslSocket *ss, PK11SymKey *pms);
extern SECStatus ssl3_AppendHandshake(sslSocket *ss, const void *void_src, 
			PRInt32 bytes);
extern SECStatus ssl3_AppendHandshakeHeader(sslSocket *ss, 
			SSL3HandshakeType t, PRUint32 length);
extern SECStatus ssl3_AppendHandshakeNumber(sslSocket *ss, PRInt32 num, 
			PRInt32 lenSize);
extern SECStatus ssl3_AppendHandshakeVariable( sslSocket *ss, 
			const SSL3Opaque *src, PRInt32 bytes, PRInt32 lenSize);
extern SECStatus ssl3_ConsumeHandshake(sslSocket *ss, void *v, PRInt32 bytes, 
			SSL3Opaque **b, PRUint32 *length);
extern PRInt32   ssl3_ConsumeHandshakeNumber(sslSocket *ss, PRInt32 bytes, 
			SSL3Opaque **b, PRUint32 *length);
extern SECStatus ssl3_ConsumeHandshakeVariable(sslSocket *ss, SECItem *i, 
			PRInt32 bytes, SSL3Opaque **b, PRUint32 *length);
extern SECStatus ssl3_SignHashes(SSL3Hashes *hash, SECKEYPrivateKey *key, 
			SECItem *buf, PRBool isTLS);
extern SECStatus ssl3_VerifySignedHashes(SSL3Hashes *hash, 
			CERTCertificate *cert, SECItem *buf, PRBool isTLS, 
			void *pwArg);
extern SECStatus ssl3_CacheWrappedMasterSecret(sslSocket *ss,
			sslSessionID *sid, ssl3CipherSpec *spec,
			SSL3KEAType effectiveExchKeyType);


extern SECStatus ssl3_HandleServerNameXtn(sslSocket * ss,
			PRUint16 ex_type, SECItem *data);
extern SECStatus ssl3_HandleSupportedCurvesXtn(sslSocket * ss,
			PRUint16 ex_type, SECItem *data);
extern SECStatus ssl3_HandleSupportedPointFormatsXtn(sslSocket * ss,
			PRUint16 ex_type, SECItem *data);
extern SECStatus ssl3_ClientHandleSessionTicketXtn(sslSocket *ss,
			PRUint16 ex_type, SECItem *data);
extern SECStatus ssl3_ServerHandleSessionTicketXtn(sslSocket *ss,
			PRUint16 ex_type, SECItem *data);





extern PRInt32 ssl3_SendSessionTicketXtn(sslSocket *ss, PRBool append,
			PRUint32 maxBytes);




extern PRInt32 ssl3_SendServerNameXtn(sslSocket *ss, PRBool append,
                     PRUint32 maxBytes);





extern SECStatus ssl_ConfigSecureServer(sslSocket *ss, CERTCertificate *cert,
                                        CERTCertificateList *certChain,
                                        ssl3KeyPair *keyPair, SSLKEAType kea);

extern SSLKEAType ssl_FindCertKEAType(CERTCertificate * cert);

#ifdef NSS_ENABLE_ECC
extern PRInt32 ssl3_SendSupportedCurvesXtn(sslSocket *ss,
			PRBool append, PRUint32 maxBytes);
extern PRInt32 ssl3_SendSupportedPointFormatsXtn(sslSocket *ss,
			PRBool append, PRUint32 maxBytes);
#endif


extern SECStatus ssl3_HandleHelloExtensions(sslSocket *ss, 
			SSL3Opaque **b, PRUint32 *length);


extern PRBool ssl3_ExtensionNegotiated(sslSocket *ss, PRUint16 ex_type);
extern SECStatus ssl3_SetSIDSessionTicket(sslSessionID *sid,
			NewSessionTicket *session_ticket);
extern SECStatus ssl3_SendNewSessionTicket(sslSocket *ss);
extern PRBool ssl_GetSessionTicketKeys(unsigned char *keyName,
			unsigned char *encKey, unsigned char *macKey);
extern PRBool ssl_GetSessionTicketKeysPKCS11(SECKEYPrivateKey *svrPrivKey,
			SECKEYPublicKey *svrPubKey, void *pwArg,
			unsigned char *keyName, PK11SymKey **aesKey,
			PK11SymKey **macKey);


#define TLS_EX_SESS_TICKET_LIFETIME_HINT    (2 * 24 * 60 * 60) /* 2 days */
#define TLS_EX_SESS_TICKET_VERSION          (0x0100)


extern PRFileDesc *ssl_NewPRSocket(sslSocket *ss, PRFileDesc *fd);
extern void ssl_FreePRSocket(PRFileDesc *fd);



extern int ssl3_config_match_init(sslSocket *);



extern ssl3KeyPair * ssl3_NewKeyPair( SECKEYPrivateKey * privKey, 
                                      SECKEYPublicKey * pubKey);


extern ssl3KeyPair * ssl3_GetKeyPairRef(ssl3KeyPair * keyPair);


extern void ssl3_FreeKeyPair(ssl3KeyPair * keyPair);


extern PRBool
ssl_GetWrappingKey( PRInt32                   symWrapMechIndex,
                    SSL3KEAType               exchKeyType, 
		    SSLWrappedSymWrappingKey *wswk);










extern PRBool
ssl_SetWrappingKey(SSLWrappedSymWrappingKey *wswk);


extern SECStatus SSL3_ShutdownServerCache(void);

extern SECStatus ssl_InitSymWrapKeysLock(void);

extern SECStatus ssl_FreeSymWrapKeysLock(void);

extern SECStatus ssl_InitSessionCacheLocks(PRBool lazyInit);

extern SECStatus ssl_FreeSessionCacheLocks(void);




extern int ssl_MapLowLevelError(int hiLevelError);

extern PRUint32 ssl_Time(void);

extern void SSL_AtomicIncrementLong(long * x);

SECStatus SSL_DisableDefaultExportCipherSuites(void);
SECStatus SSL_DisableExportCipherSuites(PRFileDesc * fd);
PRBool    SSL_IsExportCipherSuite(PRUint16 cipherSuite);


#ifdef TRACE
#define SSL_TRACE(msg) ssl_Trace msg
#else
#define SSL_TRACE(msg)
#endif

void ssl_Trace(const char *format, ...);

SEC_END_PROTOS

#if defined(XP_UNIX) || defined(XP_OS2) || defined(XP_BEOS)
#define SSL_GETPID getpid
#elif defined(_WIN32_WCE)
#define SSL_GETPID GetCurrentProcessId
#elif defined(WIN32)
extern int __cdecl _getpid(void);
#define SSL_GETPID _getpid
#else
#define SSL_GETPID() 0
#endif

#endif 
