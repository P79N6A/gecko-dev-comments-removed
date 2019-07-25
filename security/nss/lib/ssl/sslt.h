








































#ifndef __sslt_h_
#define __sslt_h_

#include "prtypes.h"

typedef struct SSL3StatisticsStr {
    
    long sch_sid_cache_hits;
    long sch_sid_cache_misses;
    long sch_sid_cache_not_ok;

    
    long hsh_sid_cache_hits;
    long hsh_sid_cache_misses;
    long hsh_sid_cache_not_ok;

    
    long hch_sid_cache_hits;
    long hch_sid_cache_misses;
    long hch_sid_cache_not_ok;

    
    long sch_sid_stateless_resumes;
    long hsh_sid_stateless_resumes;
    long hch_sid_stateless_resumes;
    long hch_sid_ticket_parse_failures;
} SSL3Statistics;


typedef enum {
    ssl_kea_null     = 0,
    ssl_kea_rsa      = 1,
    ssl_kea_dh       = 2,
    ssl_kea_fortezza = 3,       
    ssl_kea_ecdh     = 4,
    ssl_kea_size		
} SSLKEAType;






#define kt_null   	ssl_kea_null
#define kt_rsa   	ssl_kea_rsa
#define kt_dh   	ssl_kea_dh
#define kt_fortezza	ssl_kea_fortezza       /* deprecated, now unused */
#define kt_ecdh   	ssl_kea_ecdh
#define kt_kea_size	ssl_kea_size

typedef enum {
    ssl_sign_null   = 0, 
    ssl_sign_rsa    = 1,
    ssl_sign_dsa    = 2,
    ssl_sign_ecdsa  = 3
} SSLSignType;

typedef enum {
    ssl_auth_null   = 0, 
    ssl_auth_rsa    = 1,
    ssl_auth_dsa    = 2,
    ssl_auth_kea    = 3,
    ssl_auth_ecdsa  = 4
} SSLAuthType;

typedef enum {
    ssl_calg_null     = 0,
    ssl_calg_rc4      = 1,
    ssl_calg_rc2      = 2,
    ssl_calg_des      = 3,
    ssl_calg_3des     = 4,
    ssl_calg_idea     = 5,
    ssl_calg_fortezza = 6,      
    ssl_calg_aes      = 7,      
    ssl_calg_camellia = 8,
    ssl_calg_seed     = 9
} SSLCipherAlgorithm;

typedef enum { 
    ssl_mac_null      = 0, 
    ssl_mac_md5       = 1, 
    ssl_mac_sha       = 2, 
    ssl_hmac_md5      = 3, 	
    ssl_hmac_sha      = 4 	
} SSLMACAlgorithm;

typedef enum {
    ssl_compression_null = 0,
    ssl_compression_deflate = 1  
} SSLCompressionMethod;

typedef struct SSLChannelInfoStr {
    PRUint32             length;
    PRUint16             protocolVersion;
    PRUint16             cipherSuite;

    
    PRUint32             authKeyBits;

    
    PRUint32             keaKeyBits;

    
    PRUint32             creationTime;		
    PRUint32             lastAccessTime;	
    PRUint32             expirationTime;	
    PRUint32             sessionIDLength;	
    PRUint8              sessionID    [32];

    

    
    const char *         compressionMethodName;
    SSLCompressionMethod compressionMethod;
} SSLChannelInfo;

typedef struct SSLCipherSuiteInfoStr {
    PRUint16             length;
    PRUint16             cipherSuite;

    
    const char *         cipherSuiteName;

    
    const char *         authAlgorithmName;
    SSLAuthType          authAlgorithm;

    
    const char *         keaTypeName;
    SSLKEAType           keaType;

    
    const char *         symCipherName;
    SSLCipherAlgorithm   symCipher;
    PRUint16             symKeyBits;
    PRUint16             symKeySpace;
    PRUint16             effectiveKeyBits;

    
    const char *         macAlgorithmName;
    SSLMACAlgorithm      macAlgorithm;
    PRUint16             macBits;

    PRUintn              isFIPS       : 1;
    PRUintn              isExportable : 1;
    PRUintn              nonStandard  : 1;
    PRUintn              reservedBits :29;

} SSLCipherSuiteInfo;

typedef enum {
    SSL_sni_host_name                    = 0,
    SSL_sni_type_total
} SSLSniNameType;



typedef enum {
    ssl_server_name_xtn              = 0,
#ifdef NSS_ENABLE_ECC
    ssl_elliptic_curves_xtn          = 10,
    ssl_ec_point_formats_xtn         = 11,
#endif
    ssl_session_ticket_xtn           = 35,
    ssl_next_proto_neg_xtn           = 13172,
    ssl_renegotiation_info_xtn       = 0xff01	
} SSLExtensionType;

#define SSL_MAX_EXTENSIONS             6

#endif 
