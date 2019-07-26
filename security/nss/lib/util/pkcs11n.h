



#ifndef _PKCS11N_H_
#define _PKCS11N_H_

#ifdef DEBUG
static const char CKT_CVS_ID[] = "@(#) $RCSfile: pkcs11n.h,v $ $Revision: 1.32 $ $Date: 2013/02/12 22:33:53 $";
#endif 




















#define NSSCK_VENDOR_NSS 0x4E534350 /* NSCP */





#define CKO_NSS (CKO_VENDOR_DEFINED|NSSCK_VENDOR_NSS)

#define CKO_NSS_CRL                (CKO_NSS + 1)
#define CKO_NSS_SMIME              (CKO_NSS + 2)
#define CKO_NSS_TRUST              (CKO_NSS + 3)
#define CKO_NSS_BUILTIN_ROOT_LIST  (CKO_NSS + 4)
#define CKO_NSS_NEWSLOT            (CKO_NSS + 5)
#define CKO_NSS_DELSLOT            (CKO_NSS + 6)






#define CKK_NSS (CKK_VENDOR_DEFINED|NSSCK_VENDOR_NSS)

#define CKK_NSS_PKCS8              (CKK_NSS + 1)

#define CKK_NSS_JPAKE_ROUND1       (CKK_NSS + 2)
#define CKK_NSS_JPAKE_ROUND2       (CKK_NSS + 3)





#define CKC_NSS (CKC_VENDOR_DEFINED|NSSCK_VENDOR_NSS)


#define CKA_DIGEST            0x81000000L
#define CKA_FLAGS_ONLY        0 /* CKA_CLASS */





#define CKA_NSS (CKA_VENDOR_DEFINED|NSSCK_VENDOR_NSS)

#define CKA_NSS_URL                (CKA_NSS +  1)
#define CKA_NSS_EMAIL              (CKA_NSS +  2)
#define CKA_NSS_SMIME_INFO         (CKA_NSS +  3)
#define CKA_NSS_SMIME_TIMESTAMP    (CKA_NSS +  4)
#define CKA_NSS_PKCS8_SALT         (CKA_NSS +  5)
#define CKA_NSS_PASSWORD_CHECK     (CKA_NSS +  6)
#define CKA_NSS_EXPIRES            (CKA_NSS +  7)
#define CKA_NSS_KRL                (CKA_NSS +  8)

#define CKA_NSS_PQG_COUNTER        (CKA_NSS +  20)
#define CKA_NSS_PQG_SEED           (CKA_NSS +  21)
#define CKA_NSS_PQG_H              (CKA_NSS +  22)
#define CKA_NSS_PQG_SEED_BITS      (CKA_NSS +  23)
#define CKA_NSS_MODULE_SPEC        (CKA_NSS +  24)
#define CKA_NSS_OVERRIDE_EXTENSIONS (CKA_NSS +  25)

#define CKA_NSS_JPAKE_SIGNERID     (CKA_NSS +  26)
#define CKA_NSS_JPAKE_PEERID       (CKA_NSS +  27)
#define CKA_NSS_JPAKE_GX1          (CKA_NSS +  28)
#define CKA_NSS_JPAKE_GX2          (CKA_NSS +  29)
#define CKA_NSS_JPAKE_GX3          (CKA_NSS +  30)
#define CKA_NSS_JPAKE_GX4          (CKA_NSS +  31)
#define CKA_NSS_JPAKE_X2           (CKA_NSS +  32)
#define CKA_NSS_JPAKE_X2S          (CKA_NSS +  33)








#define CKA_TRUST (CKA_NSS + 0x2000)


#define CKA_TRUST_DIGITAL_SIGNATURE     (CKA_TRUST +  1)
#define CKA_TRUST_NON_REPUDIATION       (CKA_TRUST +  2)
#define CKA_TRUST_KEY_ENCIPHERMENT      (CKA_TRUST +  3)
#define CKA_TRUST_DATA_ENCIPHERMENT     (CKA_TRUST +  4)
#define CKA_TRUST_KEY_AGREEMENT         (CKA_TRUST +  5)
#define CKA_TRUST_KEY_CERT_SIGN         (CKA_TRUST +  6)
#define CKA_TRUST_CRL_SIGN              (CKA_TRUST +  7)


#define CKA_TRUST_SERVER_AUTH           (CKA_TRUST +  8)
#define CKA_TRUST_CLIENT_AUTH           (CKA_TRUST +  9)
#define CKA_TRUST_CODE_SIGNING          (CKA_TRUST + 10)
#define CKA_TRUST_EMAIL_PROTECTION      (CKA_TRUST + 11)
#define CKA_TRUST_IPSEC_END_SYSTEM      (CKA_TRUST + 12)
#define CKA_TRUST_IPSEC_TUNNEL          (CKA_TRUST + 13)
#define CKA_TRUST_IPSEC_USER            (CKA_TRUST + 14)
#define CKA_TRUST_TIME_STAMPING         (CKA_TRUST + 15)
#define CKA_TRUST_STEP_UP_APPROVED      (CKA_TRUST + 16)

#define CKA_CERT_SHA1_HASH	        (CKA_TRUST + 100)
#define CKA_CERT_MD5_HASH		(CKA_TRUST + 101)




#define CKA_NETSCAPE_DB                 0xD5A0DB00L
#define CKA_NETSCAPE_TRUST              0x80000001L


#define CKM_FAKE_RANDOM       0x80000efeUL
#define CKM_INVALID_MECHANISM 0xffffffffUL





#define CKM_NSS (CKM_VENDOR_DEFINED|NSSCK_VENDOR_NSS)

#define CKM_NSS_AES_KEY_WRAP      (CKM_NSS + 1)
#define CKM_NSS_AES_KEY_WRAP_PAD  (CKM_NSS + 2)


#define CKM_NSS_HKDF_SHA1         (CKM_NSS + 3)
#define CKM_NSS_HKDF_SHA256       (CKM_NSS + 4)
#define CKM_NSS_HKDF_SHA384       (CKM_NSS + 5)
#define CKM_NSS_HKDF_SHA512       (CKM_NSS + 6)










#define CKM_NSS_JPAKE_ROUND1_SHA1   (CKM_NSS + 7)
#define CKM_NSS_JPAKE_ROUND1_SHA256 (CKM_NSS + 8)
#define CKM_NSS_JPAKE_ROUND1_SHA384 (CKM_NSS + 9)
#define CKM_NSS_JPAKE_ROUND1_SHA512 (CKM_NSS + 10)









#define CKM_NSS_JPAKE_ROUND2_SHA1   (CKM_NSS + 11)
#define CKM_NSS_JPAKE_ROUND2_SHA256 (CKM_NSS + 12)
#define CKM_NSS_JPAKE_ROUND2_SHA384 (CKM_NSS + 13)
#define CKM_NSS_JPAKE_ROUND2_SHA512 (CKM_NSS + 14)











#define CKM_NSS_JPAKE_FINAL_SHA1    (CKM_NSS + 15)
#define CKM_NSS_JPAKE_FINAL_SHA256  (CKM_NSS + 16)
#define CKM_NSS_JPAKE_FINAL_SHA384  (CKM_NSS + 17)
#define CKM_NSS_JPAKE_FINAL_SHA512  (CKM_NSS + 18)















#define CKM_NSS_HMAC_CONSTANT_TIME      (CKM_NSS + 19)
#define CKM_NSS_SSL3_MAC_CONSTANT_TIME  (CKM_NSS + 20)







#define CKM_NETSCAPE_PBE_SHA1_DES_CBC           0x80000002UL
#define CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC    0x80000003UL
#define CKM_NETSCAPE_PBE_SHA1_40_BIT_RC2_CBC    0x80000004UL
#define CKM_NETSCAPE_PBE_SHA1_128_BIT_RC2_CBC   0x80000005UL
#define CKM_NETSCAPE_PBE_SHA1_40_BIT_RC4        0x80000006UL
#define CKM_NETSCAPE_PBE_SHA1_128_BIT_RC4       0x80000007UL
#define CKM_NETSCAPE_PBE_SHA1_FAULTY_3DES_CBC   0x80000008UL
#define CKM_NETSCAPE_PBE_SHA1_HMAC_KEY_GEN      0x80000009UL
#define CKM_NETSCAPE_PBE_MD5_HMAC_KEY_GEN       0x8000000aUL
#define CKM_NETSCAPE_PBE_MD2_HMAC_KEY_GEN       0x8000000bUL

#define CKM_TLS_PRF_GENERAL                     0x80000373UL

typedef struct CK_NSS_JPAKEPublicValue {
    CK_BYTE * pGX;
    CK_ULONG ulGXLen;
    CK_BYTE * pGV;
    CK_ULONG ulGVLen;
    CK_BYTE * pR;
    CK_ULONG ulRLen;
} CK_NSS_JPAKEPublicValue;

typedef struct CK_NSS_JPAKERound1Params {
    CK_NSS_JPAKEPublicValue gx1; 
    CK_NSS_JPAKEPublicValue gx2; 
} CK_NSS_JPAKERound1Params;

typedef struct CK_NSS_JPAKERound2Params {
    CK_BYTE * pSharedKey;        
    CK_ULONG ulSharedKeyLen;     
    CK_NSS_JPAKEPublicValue gx3; 
    CK_NSS_JPAKEPublicValue gx4; 
    CK_NSS_JPAKEPublicValue A;   
} CK_NSS_JPAKERound2Params;

typedef struct CK_NSS_JPAKEFinalParams {
    CK_NSS_JPAKEPublicValue B; 
} CK_NSS_JPAKEFinalParams;
















typedef struct CK_NSS_MAC_CONSTANT_TIME_PARAMS {
    CK_MECHANISM_TYPE macAlg;   
    CK_ULONG ulBodyTotalLen;    
    CK_BYTE * pHeader;          
    CK_ULONG ulHeaderLen;       
} CK_NSS_MAC_CONSTANT_TIME_PARAMS;





#define CKR_NSS (CKM_VENDOR_DEFINED|NSSCK_VENDOR_NSS)

#define CKR_NSS_CERTDB_FAILED      (CKR_NSS + 1)
#define CKR_NSS_KEYDB_FAILED       (CKR_NSS + 2)



















typedef struct CK_NSS_HKDFParams {
    CK_BBOOL bExtract;
    CK_BYTE_PTR pSalt;
    CK_ULONG ulSaltLen;
    CK_BBOOL bExpand;
    CK_BYTE_PTR pInfo;
    CK_ULONG ulInfoLen;
} CK_NSS_HKDFParams;











typedef CK_ULONG          CK_TRUST;


#define CKT_VENDOR_DEFINED     0x80000000

#define CKT_NSS (CKT_VENDOR_DEFINED|NSSCK_VENDOR_NSS)


#define CKT_NSS_TRUSTED            (CKT_NSS + 1)
#define CKT_NSS_TRUSTED_DELEGATOR  (CKT_NSS + 2)
#define CKT_NSS_MUST_VERIFY_TRUST  (CKT_NSS + 3)
#define CKT_NSS_NOT_TRUSTED        (CKT_NSS + 10)
#define CKT_NSS_TRUST_UNKNOWN      (CKT_NSS + 5) /* default */





#define CKT_NSS_VALID_DELEGATOR    (CKT_NSS + 11)







#if defined(__GNUC__) && (__GNUC__ > 3)















#if (__GNUC__  == 4) && (__GNUC_MINOR__ < 5)


typedef CK_TRUST __CKT_NSS_UNTRUSTED __attribute__((deprecated));
typedef CK_TRUST __CKT_NSS_VALID __attribute__ ((deprecated));
typedef CK_TRUST __CKT_NSS_MUST_VERIFY __attribute__((deprecated));
#else


typedef CK_TRUST __CKT_NSS_UNTRUSTED __attribute__((deprecated
    ("CKT_NSS_UNTRUSTED really means CKT_NSS_MUST_VERIFY_TRUST")));
typedef CK_TRUST __CKT_NSS_VALID __attribute__ ((deprecated
    ("CKT_NSS_VALID really means CKT_NSS_NOT_TRUSTED")));
typedef CK_TRUST __CKT_NSS_MUST_VERIFY __attribute__((deprecated
    ("CKT_NSS_MUST_VERIFY really functions as CKT_NSS_TRUST_UNKNOWN")));
#endif
#define CKT_NSS_UNTRUSTED ((__CKT_NSS_UNTRUSTED)CKT_NSS_MUST_VERIFY_TRUST)
#define CKT_NSS_VALID     ((__CKT_NSS_VALID) CKT_NSS_NOT_TRUSTED)

#define CKT_NSS_MUST_VERIFY ((__CKT_NSS_MUST_VERIFY)(CKT_NSS +4))
#else
#ifdef _WIN32


#pragma deprecated(CKT_NSS_UNTRUSTED, CKT_NSS_MUST_VERIFY, CKT_NSS_VALID)
#endif

#define CKT_NSS_UNTRUSTED          CKT_NSS_MUST_VERIFY_TRUST

#define CKT_NSS_VALID              CKT_NSS_NOT_TRUSTED

#define CKT_NSS_MUST_VERIFY        (CKT_NSS + 4)  /*really means trust unknown*/
#endif



#define CKO_NETSCAPE_CRL                CKO_NSS_CRL
#define CKO_NETSCAPE_SMIME              CKO_NSS_SMIME
#define CKO_NETSCAPE_TRUST              CKO_NSS_TRUST
#define CKO_NETSCAPE_BUILTIN_ROOT_LIST  CKO_NSS_BUILTIN_ROOT_LIST
#define CKO_NETSCAPE_NEWSLOT            CKO_NSS_NEWSLOT
#define CKO_NETSCAPE_DELSLOT            CKO_NSS_DELSLOT
#define CKK_NETSCAPE_PKCS8              CKK_NSS_PKCS8
#define CKA_NETSCAPE_URL                CKA_NSS_URL
#define CKA_NETSCAPE_EMAIL              CKA_NSS_EMAIL
#define CKA_NETSCAPE_SMIME_INFO         CKA_NSS_SMIME_INFO
#define CKA_NETSCAPE_SMIME_TIMESTAMP    CKA_NSS_SMIME_TIMESTAMP
#define CKA_NETSCAPE_PKCS8_SALT         CKA_NSS_PKCS8_SALT
#define CKA_NETSCAPE_PASSWORD_CHECK     CKA_NSS_PASSWORD_CHECK
#define CKA_NETSCAPE_EXPIRES            CKA_NSS_EXPIRES
#define CKA_NETSCAPE_KRL                CKA_NSS_KRL
#define CKA_NETSCAPE_PQG_COUNTER        CKA_NSS_PQG_COUNTER
#define CKA_NETSCAPE_PQG_SEED           CKA_NSS_PQG_SEED
#define CKA_NETSCAPE_PQG_H              CKA_NSS_PQG_H
#define CKA_NETSCAPE_PQG_SEED_BITS      CKA_NSS_PQG_SEED_BITS
#define CKA_NETSCAPE_MODULE_SPEC        CKA_NSS_MODULE_SPEC
#define CKM_NETSCAPE_AES_KEY_WRAP	CKM_NSS_AES_KEY_WRAP
#define CKM_NETSCAPE_AES_KEY_WRAP_PAD	CKM_NSS_AES_KEY_WRAP_PAD
#define CKR_NETSCAPE_CERTDB_FAILED      CKR_NSS_CERTDB_FAILED
#define CKR_NETSCAPE_KEYDB_FAILED       CKR_NSS_KEYDB_FAILED

#define CKT_NETSCAPE_TRUSTED            CKT_NSS_TRUSTED
#define CKT_NETSCAPE_TRUSTED_DELEGATOR  CKT_NSS_TRUSTED_DELEGATOR
#define CKT_NETSCAPE_UNTRUSTED          CKT_NSS_UNTRUSTED
#define CKT_NETSCAPE_MUST_VERIFY        CKT_NSS_MUST_VERIFY
#define CKT_NETSCAPE_TRUST_UNKNOWN      CKT_NSS_TRUST_UNKNOWN
#define CKT_NETSCAPE_VALID              CKT_NSS_VALID
#define CKT_NETSCAPE_VALID_DELEGATOR    CKT_NSS_VALID_DELEGATOR












#define SECMOD_MODULE_DB_FUNCTION_FIND  0
#define SECMOD_MODULE_DB_FUNCTION_ADD   1
#define SECMOD_MODULE_DB_FUNCTION_DEL   2
#define SECMOD_MODULE_DB_FUNCTION_RELEASE 3 
typedef char ** (PR_CALLBACK *SECMODModuleDBFunc)(unsigned long function,
                                        char *parameters, void *moduleSpec);


#define SFTK_MIN_USER_SLOT_ID 4
#define SFTK_MAX_USER_SLOT_ID 100
#define SFTK_MIN_FIPS_USER_SLOT_ID 101
#define SFTK_MAX_FIPS_USER_SLOT_ID 127


#endif 
