




















































#include "seccomon.h"
#include "secitem.h"
#include "pkcs11.h"
#include "pkcs11i.h"
#include "softoken.h"
#include "lowkeyi.h"
#include "blapi.h"
#include "secder.h"
#include "secport.h"
#include "secrng.h"
#include "prtypes.h"
#include "nspr.h"
#include "softkver.h"
#include "secoid.h"
#include "sftkdb.h"
#include "sftkpars.h"
#include "ec.h"
#include "secasn1.h"

PRBool parentForkedAfterC_Initialize;

#ifndef NO_FORK_CHECK

PRBool sftkForkCheckDisabled;

#if defined(CHECK_FORK_PTHREAD) || defined(CHECK_FORK_MIXED)
PRBool forked = PR_FALSE;
#endif

#if defined(CHECK_FORK_GETPID) || defined(CHECK_FORK_MIXED)
#include <unistd.h>
pid_t myPid;
#endif

#ifdef CHECK_FORK_MIXED
#include <sys/systeminfo.h>
PRBool usePthread_atfork;
#endif

#endif






static char *manufacturerID      = "Mozilla Foundation              ";
static char manufacturerID_space[33];
static char *libraryDescription  = "NSS Internal Crypto Services    ";
static char libraryDescription_space[33];





static PRIntervalTime loginWaitTime;
static PRUint32	      minSessionObjectHandle = 1U;

#define __PASTE(x,y)    x##y




 
#undef CK_PKCS11_FUNCTION_INFO
#undef CK_NEED_ARG_LIST

#define CK_EXTERN extern
#define CK_PKCS11_FUNCTION_INFO(func) \
		CK_RV __PASTE(NS,func)
#define CK_NEED_ARG_LIST	1
 
#include "pkcs11f.h"
 
 
 

static const CK_FUNCTION_LIST sftk_funcList = {
    { 1, 10 },
 
#undef CK_PKCS11_FUNCTION_INFO
#undef CK_NEED_ARG_LIST
 
#define CK_PKCS11_FUNCTION_INFO(func) \
				__PASTE(NS,func),
#include "pkcs11f.h"
 
};
 
#undef CK_PKCS11_FUNCTION_INFO
#undef CK_NEED_ARG_LIST
 
 
#undef __PASTE

 
typedef unsigned char desKey[8];
static const desKey  sftk_desWeakTable[] = {
#ifdef noParity
    
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x1e, 0x1e, 0x1e, 0x1e, 0x0e, 0x0e, 0x0e, 0x0e },
    { 0xe0, 0xe0, 0xe0, 0xe0, 0xf0, 0xf0, 0xf0, 0xf0 },
    { 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe },
    
    { 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe, 0x00, 0xfe },
    { 0xfe, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00, 0xfe },

    { 0x1e, 0xe0, 0x1e, 0xe0, 0x0e, 0xf0, 0x0e, 0xf0 },
    { 0xe0, 0x1e, 0xe0, 0x1e, 0xf0, 0x0e, 0xf0, 0x0e },

    { 0x00, 0xe0, 0x00, 0xe0, 0x00, 0x0f, 0x00, 0x0f },
    { 0xe0, 0x00, 0xe0, 0x00, 0xf0, 0x00, 0xf0, 0x00 },

    { 0x1e, 0xfe, 0x1e, 0xfe, 0x0e, 0xfe, 0x0e, 0xfe },
    { 0xfe, 0x1e, 0xfe, 0x1e, 0xfe, 0x0e, 0xfe, 0x0e },

    { 0x00, 0x1e, 0x00, 0x1e, 0x00, 0x0e, 0x00, 0x0e },
    { 0x1e, 0x00, 0x1e, 0x00, 0x0e, 0x00, 0x0e, 0x00 },

    { 0xe0, 0xfe, 0xe0, 0xfe, 0xf0, 0xfe, 0xf0, 0xfe },
    { 0xfe, 0xe0, 0xfe, 0xe0, 0xfe, 0xf0, 0xfe, 0xf0 },
#else
    
    { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
    { 0x1f, 0x1f, 0x1f, 0x1f, 0x0e, 0x0e, 0x0e, 0x0e },
    { 0xe0, 0xe0, 0xe0, 0xe0, 0xf1, 0xf1, 0xf1, 0xf1 },
    { 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe },

    
    { 0x01, 0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01, 0xfe },
    { 0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01 },

    { 0x1f, 0xe0, 0x1f, 0xe0, 0x0e, 0xf1, 0x0e, 0xf1 },
    { 0xe0, 0x1f, 0xe0, 0x1f, 0xf1, 0x0e, 0xf1, 0x0e },

    { 0x01, 0xe0, 0x01, 0xe0, 0x01, 0xf1, 0x01, 0xf1 },
    { 0xe0, 0x01, 0xe0, 0x01, 0xf1, 0x01, 0xf1, 0x01 },

    { 0x1f, 0xfe, 0x1f, 0xfe, 0x0e, 0xfe, 0x0e, 0xfe },
    { 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x0e, 0xfe, 0x0e },

    { 0x01, 0x1f, 0x01, 0x1f, 0x01, 0x0e, 0x01, 0x0e },
    { 0x1f, 0x01, 0x1f, 0x01, 0x0e, 0x01, 0x0e, 0x01 },

    { 0xe0, 0xfe, 0xe0, 0xfe, 0xf1, 0xfe, 0xf1, 0xfe }, 
    { 0xfe, 0xe0, 0xfe, 0xe0, 0xfe, 0xf1, 0xfe, 0xf1 }
#endif
};

    
static const int sftk_desWeakTableSize = sizeof(sftk_desWeakTable)/
						sizeof(sftk_desWeakTable[0]);



static const unsigned char parityTable[256] = {

   0x01,0x02,0x04,0x07,0x08,0x0b,0x0d,0x0e,

   0x10,0x13,0x15,0x16,0x19,0x1a,0x1c,0x1f,

   0x20,0x23,0x25,0x26,0x29,0x2a,0x2c,0x2f,

   0x31,0x32,0x34,0x37,0x38,0x3b,0x3d,0x3e,

   0x40,0x43,0x45,0x46,0x49,0x4a,0x4c,0x4f,

   0x51,0x52,0x54,0x57,0x58,0x5b,0x5d,0x5e,

   0x61,0x62,0x64,0x67,0x68,0x6b,0x6d,0x6e,

   0x70,0x73,0x75,0x76,0x79,0x7a,0x7c,0x7f,

   0x80,0x83,0x85,0x86,0x89,0x8a,0x8c,0x8f,

   0x91,0x92,0x94,0x97,0x98,0x9b,0x9d,0x9e,

   0xa1,0xa2,0xa4,0xa7,0xa8,0xab,0xad,0xae,

   0xb0,0xb3,0xb5,0xb6,0xb9,0xba,0xbc,0xbf,

   0xc1,0xc2,0xc4,0xc7,0xc8,0xcb,0xcd,0xce,

   0xd0,0xd3,0xd5,0xd6,0xd9,0xda,0xdc,0xdf,

   0xe0,0xe3,0xe5,0xe6,0xe9,0xea,0xec,0xef,

   0xf1,0xf2,0xf4,0xf7,0xf8,0xfb,0xfd,0xfe,
};


struct mechanismList {
    CK_MECHANISM_TYPE	type;
    CK_MECHANISM_INFO	info;
    PRBool		privkey;
};






#define CKF_EN_DE		CKF_ENCRYPT      | CKF_DECRYPT
#define CKF_WR_UN		CKF_WRAP         | CKF_UNWRAP
#define CKF_SN_VR		CKF_SIGN         | CKF_VERIFY
#define CKF_SN_RE		CKF_SIGN_RECOVER | CKF_VERIFY_RECOVER

#define CKF_EN_DE_WR_UN 	CKF_EN_DE       | CKF_WR_UN
#define CKF_SN_VR_RE		CKF_SN_VR       | CKF_SN_RE
#define CKF_DUZ_IT_ALL		CKF_EN_DE_WR_UN | CKF_SN_VR_RE

#define CKF_EC_PNU		CKF_EC_FP | CKF_EC_NAMEDCURVE | CKF_EC_UNCOMPRESS

#define CKF_EC_BPNU		CKF_EC_F_2M | CKF_EC_PNU

#define CK_MAX 0xffffffff

static const struct mechanismList mechanisms[] = {

     



















     
     {CKM_RSA_PKCS_KEY_PAIR_GEN,{RSA_MIN_MODULUS_BITS,CK_MAX,
				 CKF_GENERATE_KEY_PAIR},PR_TRUE},
     {CKM_RSA_PKCS,             {RSA_MIN_MODULUS_BITS,CK_MAX,
				 CKF_DUZ_IT_ALL},       PR_TRUE},
#ifdef SFTK_RSA9796_SUPPORTED
     {CKM_RSA_9796,		{RSA_MIN_MODULUS_BITS,CK_MAX,
				 CKF_DUZ_IT_ALL},       PR_TRUE},
#endif
     {CKM_RSA_X_509,		{RSA_MIN_MODULUS_BITS,CK_MAX,
				 CKF_DUZ_IT_ALL},       PR_TRUE},
     
     {CKM_MD2_RSA_PKCS,		{RSA_MIN_MODULUS_BITS,CK_MAX,
				 CKF_SN_VR}, 	PR_TRUE},
     {CKM_MD5_RSA_PKCS,		{RSA_MIN_MODULUS_BITS,CK_MAX,
				 CKF_SN_VR}, 	PR_TRUE},
     {CKM_SHA1_RSA_PKCS,	{RSA_MIN_MODULUS_BITS,CK_MAX,
				 CKF_SN_VR}, 	PR_TRUE},
     {CKM_SHA256_RSA_PKCS,	{RSA_MIN_MODULUS_BITS,CK_MAX,
				 CKF_SN_VR}, 	PR_TRUE},
     {CKM_SHA384_RSA_PKCS,	{RSA_MIN_MODULUS_BITS,CK_MAX,
				 CKF_SN_VR}, 	PR_TRUE},
     {CKM_SHA512_RSA_PKCS,	{RSA_MIN_MODULUS_BITS,CK_MAX,
				 CKF_SN_VR}, 	PR_TRUE},
     
     {CKM_DSA_KEY_PAIR_GEN,	{DSA_MIN_P_BITS, DSA_MAX_P_BITS,
				 CKF_GENERATE_KEY_PAIR}, PR_TRUE},
     {CKM_DSA,			{DSA_MIN_P_BITS, DSA_MAX_P_BITS, 
				 CKF_SN_VR},              PR_TRUE},
     {CKM_DSA_SHA1,		{DSA_MIN_P_BITS, DSA_MAX_P_BITS,
				 CKF_SN_VR},              PR_TRUE},
     
     
     {CKM_DH_PKCS_KEY_PAIR_GEN,	{DH_MIN_P_BITS, DH_MAX_P_BITS, 
				 CKF_GENERATE_KEY_PAIR}, PR_TRUE}, 
     {CKM_DH_PKCS_DERIVE,	{DH_MIN_P_BITS, DH_MAX_P_BITS,
				 CKF_DERIVE}, 	PR_TRUE}, 
#ifdef NSS_ENABLE_ECC
     
     {CKM_EC_KEY_PAIR_GEN,      {112, 571, CKF_GENERATE_KEY_PAIR|CKF_EC_BPNU}, PR_TRUE}, 
     {CKM_ECDH1_DERIVE,         {112, 571, CKF_DERIVE|CKF_EC_BPNU}, PR_TRUE}, 
     {CKM_ECDSA,                {112, 571, CKF_SN_VR|CKF_EC_BPNU}, PR_TRUE}, 
     {CKM_ECDSA_SHA1,           {112, 571, CKF_SN_VR|CKF_EC_BPNU}, PR_TRUE}, 
#endif 
     
     {CKM_RC2_KEY_GEN,		{1, 128, CKF_GENERATE},		PR_TRUE},
     {CKM_RC2_ECB,		{1, 128, CKF_EN_DE_WR_UN},	PR_TRUE},
     {CKM_RC2_CBC,		{1, 128, CKF_EN_DE_WR_UN},	PR_TRUE},
     {CKM_RC2_MAC,		{1, 128, CKF_SN_VR},		PR_TRUE},
     {CKM_RC2_MAC_GENERAL,	{1, 128, CKF_SN_VR},		PR_TRUE},
     {CKM_RC2_CBC_PAD,		{1, 128, CKF_EN_DE_WR_UN},	PR_TRUE},
     
     {CKM_RC4_KEY_GEN,		{1, 256, CKF_GENERATE},		PR_FALSE},
     {CKM_RC4,			{1, 256, CKF_EN_DE_WR_UN},	PR_FALSE},
     
     {CKM_DES_KEY_GEN,		{ 8,  8, CKF_GENERATE},		PR_TRUE},
     {CKM_DES_ECB,		{ 8,  8, CKF_EN_DE_WR_UN},	PR_TRUE},
     {CKM_DES_CBC,		{ 8,  8, CKF_EN_DE_WR_UN},	PR_TRUE},
     {CKM_DES_MAC,		{ 8,  8, CKF_SN_VR},		PR_TRUE},
     {CKM_DES_MAC_GENERAL,	{ 8,  8, CKF_SN_VR},		PR_TRUE},
     {CKM_DES_CBC_PAD,		{ 8,  8, CKF_EN_DE_WR_UN},	PR_TRUE},
     {CKM_DES2_KEY_GEN,		{24, 24, CKF_GENERATE},		PR_TRUE},
     {CKM_DES3_KEY_GEN,		{24, 24, CKF_GENERATE},		PR_TRUE },
     {CKM_DES3_ECB,		{24, 24, CKF_EN_DE_WR_UN},	PR_TRUE },
     {CKM_DES3_CBC,		{24, 24, CKF_EN_DE_WR_UN},	PR_TRUE },
     {CKM_DES3_MAC,		{24, 24, CKF_SN_VR},		PR_TRUE },
     {CKM_DES3_MAC_GENERAL,	{24, 24, CKF_SN_VR},		PR_TRUE },
     {CKM_DES3_CBC_PAD,		{24, 24, CKF_EN_DE_WR_UN},	PR_TRUE },
     
     {CKM_CDMF_KEY_GEN,		{8,  8, CKF_GENERATE},		PR_TRUE},
     {CKM_CDMF_ECB,		{8,  8, CKF_EN_DE_WR_UN},	PR_TRUE},
     {CKM_CDMF_CBC,		{8,  8, CKF_EN_DE_WR_UN},	PR_TRUE},
     {CKM_CDMF_MAC,		{8,  8, CKF_SN_VR},		PR_TRUE},
     {CKM_CDMF_MAC_GENERAL,	{8,  8, CKF_SN_VR},		PR_TRUE},
     {CKM_CDMF_CBC_PAD,		{8,  8, CKF_EN_DE_WR_UN},	PR_TRUE},
     
     {CKM_AES_KEY_GEN,		{16, 32, CKF_GENERATE},		PR_TRUE},
     {CKM_AES_ECB,		{16, 32, CKF_EN_DE_WR_UN},	PR_TRUE},
     {CKM_AES_CBC,		{16, 32, CKF_EN_DE_WR_UN},	PR_TRUE},
     {CKM_AES_MAC,		{16, 32, CKF_SN_VR},		PR_TRUE},
     {CKM_AES_MAC_GENERAL,	{16, 32, CKF_SN_VR},		PR_TRUE},
     {CKM_AES_CBC_PAD,		{16, 32, CKF_EN_DE_WR_UN},	PR_TRUE},
     
     {CKM_CAMELLIA_KEY_GEN,	{16, 32, CKF_GENERATE},         PR_TRUE},
     {CKM_CAMELLIA_ECB,  	{16, 32, CKF_EN_DE_WR_UN},      PR_TRUE},
     {CKM_CAMELLIA_CBC, 	{16, 32, CKF_EN_DE_WR_UN},      PR_TRUE},
     {CKM_CAMELLIA_MAC, 	{16, 32, CKF_SN_VR},            PR_TRUE},
     {CKM_CAMELLIA_MAC_GENERAL,	{16, 32, CKF_SN_VR},            PR_TRUE},
     {CKM_CAMELLIA_CBC_PAD,	{16, 32, CKF_EN_DE_WR_UN},      PR_TRUE},
     
     {CKM_SEED_KEY_GEN,		{16, 16, CKF_GENERATE},		PR_TRUE},
     {CKM_SEED_ECB,		{16, 16, CKF_EN_DE_WR_UN},	PR_TRUE},
     {CKM_SEED_CBC,		{16, 16, CKF_EN_DE_WR_UN},	PR_TRUE},
     {CKM_SEED_MAC,		{16, 16, CKF_SN_VR},		PR_TRUE},
     {CKM_SEED_MAC_GENERAL,	{16, 16, CKF_SN_VR},		PR_TRUE},
     {CKM_SEED_CBC_PAD,		{16, 16, CKF_EN_DE_WR_UN},	PR_TRUE},
     
     {CKM_MD2,			{0,   0, CKF_DIGEST},		PR_FALSE},
     {CKM_MD2_HMAC,		{1, 128, CKF_SN_VR},		PR_TRUE},
     {CKM_MD2_HMAC_GENERAL,	{1, 128, CKF_SN_VR},		PR_TRUE},
     {CKM_MD5,			{0,   0, CKF_DIGEST},		PR_FALSE},
     {CKM_MD5_HMAC,		{1, 128, CKF_SN_VR},		PR_TRUE},
     {CKM_MD5_HMAC_GENERAL,	{1, 128, CKF_SN_VR},		PR_TRUE},
     {CKM_SHA_1,		{0,   0, CKF_DIGEST},		PR_FALSE},
     {CKM_SHA_1_HMAC,		{1, 128, CKF_SN_VR},		PR_TRUE},
     {CKM_SHA_1_HMAC_GENERAL,	{1, 128, CKF_SN_VR},		PR_TRUE},
     {CKM_SHA256,		{0,   0, CKF_DIGEST},		PR_FALSE},
     {CKM_SHA256_HMAC,		{1, 128, CKF_SN_VR},		PR_TRUE},
     {CKM_SHA256_HMAC_GENERAL,	{1, 128, CKF_SN_VR},		PR_TRUE},
     {CKM_SHA384,		{0,   0, CKF_DIGEST},		PR_FALSE},
     {CKM_SHA384_HMAC,		{1, 128, CKF_SN_VR},		PR_TRUE},
     {CKM_SHA384_HMAC_GENERAL,	{1, 128, CKF_SN_VR},		PR_TRUE},
     {CKM_SHA512,		{0,   0, CKF_DIGEST},		PR_FALSE},
     {CKM_SHA512_HMAC,		{1, 128, CKF_SN_VR},		PR_TRUE},
     {CKM_SHA512_HMAC_GENERAL,	{1, 128, CKF_SN_VR},		PR_TRUE},
     {CKM_TLS_PRF_GENERAL,	{0, 512, CKF_SN_VR},		PR_FALSE},
     
     {CKM_NSS_HKDF_SHA1,        {1, 128, CKF_DERIVE},           PR_TRUE},
     {CKM_NSS_HKDF_SHA256,      {1, 128, CKF_DERIVE},           PR_TRUE},
     {CKM_NSS_HKDF_SHA384,      {1, 128, CKF_DERIVE},           PR_TRUE},
     {CKM_NSS_HKDF_SHA512,      {1, 128, CKF_DERIVE},           PR_TRUE},
     
#ifdef NSS_SOFTOKEN_DOES_CAST
     
     {CKM_CAST_KEY_GEN,		{1,  8, CKF_GENERATE},		PR_TRUE}, 
     {CKM_CAST_ECB,		{1,  8, CKF_EN_DE_WR_UN},	PR_TRUE}, 
     {CKM_CAST_CBC,		{1,  8, CKF_EN_DE_WR_UN},	PR_TRUE}, 
     {CKM_CAST_MAC,		{1,  8, CKF_SN_VR},		PR_TRUE}, 
     {CKM_CAST_MAC_GENERAL,	{1,  8, CKF_SN_VR},		PR_TRUE}, 
     {CKM_CAST_CBC_PAD,		{1,  8, CKF_EN_DE_WR_UN},	PR_TRUE}, 
     {CKM_CAST3_KEY_GEN,	{1, 16, CKF_GENERATE},		PR_TRUE}, 
     {CKM_CAST3_ECB,		{1, 16, CKF_EN_DE_WR_UN},	PR_TRUE}, 
     {CKM_CAST3_CBC,		{1, 16, CKF_EN_DE_WR_UN},	PR_TRUE}, 
     {CKM_CAST3_MAC,		{1, 16, CKF_SN_VR},		PR_TRUE}, 
     {CKM_CAST3_MAC_GENERAL,	{1, 16, CKF_SN_VR},		PR_TRUE}, 
     {CKM_CAST3_CBC_PAD,	{1, 16, CKF_EN_DE_WR_UN},	PR_TRUE}, 
     {CKM_CAST5_KEY_GEN,	{1, 16, CKF_GENERATE},		PR_TRUE}, 
     {CKM_CAST5_ECB,		{1, 16, CKF_EN_DE_WR_UN},	PR_TRUE}, 
     {CKM_CAST5_CBC,		{1, 16, CKF_EN_DE_WR_UN},	PR_TRUE}, 
     {CKM_CAST5_MAC,		{1, 16, CKF_SN_VR},		PR_TRUE}, 
     {CKM_CAST5_MAC_GENERAL,	{1, 16, CKF_SN_VR},		PR_TRUE}, 
     {CKM_CAST5_CBC_PAD,	{1, 16, CKF_EN_DE_WR_UN},	PR_TRUE}, 
#endif
#if NSS_SOFTOKEN_DOES_RC5
     
     {CKM_RC5_KEY_GEN,		{1, 32, CKF_GENERATE},          PR_TRUE},
     {CKM_RC5_ECB,		{1, 32, CKF_EN_DE_WR_UN},	PR_TRUE},
     {CKM_RC5_CBC,		{1, 32, CKF_EN_DE_WR_UN},	PR_TRUE},
     {CKM_RC5_MAC,		{1, 32, CKF_SN_VR},  		PR_TRUE},
     {CKM_RC5_MAC_GENERAL,	{1, 32, CKF_SN_VR},  		PR_TRUE},
     {CKM_RC5_CBC_PAD,		{1, 32, CKF_EN_DE_WR_UN}, 	PR_TRUE},
#endif
#ifdef NSS_SOFTOKEN_DOES_IDEA
     
     {CKM_IDEA_KEY_GEN,		{16, 16, CKF_GENERATE}, 	PR_TRUE}, 
     {CKM_IDEA_ECB,		{16, 16, CKF_EN_DE_WR_UN},	PR_TRUE}, 
     {CKM_IDEA_CBC,		{16, 16, CKF_EN_DE_WR_UN},	PR_TRUE}, 
     {CKM_IDEA_MAC,		{16, 16, CKF_SN_VR},		PR_TRUE}, 
     {CKM_IDEA_MAC_GENERAL,	{16, 16, CKF_SN_VR},		PR_TRUE}, 
     {CKM_IDEA_CBC_PAD,		{16, 16, CKF_EN_DE_WR_UN}, 	PR_TRUE}, 
#endif
     
     {CKM_GENERIC_SECRET_KEY_GEN,	{1, 32, CKF_GENERATE}, PR_TRUE}, 
     {CKM_CONCATENATE_BASE_AND_KEY,	{1, 32, CKF_GENERATE}, PR_FALSE}, 
     {CKM_CONCATENATE_BASE_AND_DATA,	{1, 32, CKF_GENERATE}, PR_FALSE}, 
     {CKM_CONCATENATE_DATA_AND_BASE,	{1, 32, CKF_GENERATE}, PR_FALSE}, 
     {CKM_XOR_BASE_AND_DATA,		{1, 32, CKF_GENERATE}, PR_FALSE}, 
     {CKM_EXTRACT_KEY_FROM_KEY,		{1, 32, CKF_DERIVE},   PR_FALSE}, 
     
     {CKM_SSL3_PRE_MASTER_KEY_GEN,	{48, 48, CKF_GENERATE}, PR_FALSE}, 
     {CKM_SSL3_MASTER_KEY_DERIVE,	{48, 48, CKF_DERIVE},   PR_FALSE}, 
     {CKM_SSL3_MASTER_KEY_DERIVE_DH,	{8, 128, CKF_DERIVE},   PR_FALSE}, 
     {CKM_SSL3_KEY_AND_MAC_DERIVE,	{48, 48, CKF_DERIVE},   PR_FALSE}, 
     {CKM_SSL3_MD5_MAC,			{ 0, 16, CKF_DERIVE},   PR_FALSE}, 
     {CKM_SSL3_SHA1_MAC,		{ 0, 20, CKF_DERIVE},   PR_FALSE}, 
     {CKM_MD5_KEY_DERIVATION,		{ 0, 16, CKF_DERIVE},   PR_FALSE}, 
     {CKM_MD2_KEY_DERIVATION,		{ 0, 16, CKF_DERIVE},   PR_FALSE}, 
     {CKM_SHA1_KEY_DERIVATION,		{ 0, 20, CKF_DERIVE},   PR_FALSE}, 
     {CKM_TLS_MASTER_KEY_DERIVE,	{48, 48, CKF_DERIVE},   PR_FALSE}, 
     {CKM_TLS_MASTER_KEY_DERIVE_DH,	{8, 128, CKF_DERIVE},   PR_FALSE}, 
     {CKM_TLS_KEY_AND_MAC_DERIVE,	{48, 48, CKF_DERIVE},   PR_FALSE}, 
     
     {CKM_PBE_MD2_DES_CBC,		{8, 8, CKF_DERIVE},   PR_TRUE},
     {CKM_PBE_MD5_DES_CBC,		{8, 8, CKF_DERIVE},   PR_TRUE},
     
     {CKM_NETSCAPE_PBE_SHA1_DES_CBC,	     { 8, 8, CKF_GENERATE}, PR_TRUE},
     {CKM_NETSCAPE_PBE_SHA1_FAULTY_3DES_CBC, {24,24, CKF_GENERATE}, PR_TRUE},
     {CKM_PBE_SHA1_DES3_EDE_CBC,	     {24,24, CKF_GENERATE}, PR_TRUE},
     {CKM_PBE_SHA1_DES2_EDE_CBC,	     {24,24, CKF_GENERATE}, PR_TRUE},
     {CKM_PBE_SHA1_RC2_40_CBC,		     {40,40, CKF_GENERATE}, PR_TRUE},
     {CKM_PBE_SHA1_RC2_128_CBC,		     {128,128, CKF_GENERATE}, PR_TRUE},
     {CKM_PBE_SHA1_RC4_40,		     {40,40, CKF_GENERATE}, PR_TRUE},
     {CKM_PBE_SHA1_RC4_128,		     {128,128, CKF_GENERATE}, PR_TRUE},
     {CKM_PBA_SHA1_WITH_SHA1_HMAC,	     {20,20, CKF_GENERATE}, PR_TRUE},
     {CKM_PKCS5_PBKD2,   		     {1,256, CKF_GENERATE}, PR_TRUE},
     {CKM_NETSCAPE_PBE_SHA1_HMAC_KEY_GEN,    {20,20, CKF_GENERATE}, PR_TRUE},
     {CKM_NETSCAPE_PBE_MD5_HMAC_KEY_GEN,     {16,16, CKF_GENERATE}, PR_TRUE},
     {CKM_NETSCAPE_PBE_MD2_HMAC_KEY_GEN,     {16,16, CKF_GENERATE}, PR_TRUE},
     
     {CKM_NETSCAPE_AES_KEY_WRAP,	{16, 32, CKF_EN_DE_WR_UN},  PR_TRUE},
     {CKM_NETSCAPE_AES_KEY_WRAP_PAD,	{16, 32, CKF_EN_DE_WR_UN},  PR_TRUE},
     
     {CKM_NSS_JPAKE_ROUND1_SHA1,        {0, 0, CKF_GENERATE}, PR_TRUE},
     {CKM_NSS_JPAKE_ROUND1_SHA256,      {0, 0, CKF_GENERATE}, PR_TRUE},
     {CKM_NSS_JPAKE_ROUND1_SHA384,      {0, 0, CKF_GENERATE}, PR_TRUE},
     {CKM_NSS_JPAKE_ROUND1_SHA512,      {0, 0, CKF_GENERATE}, PR_TRUE},
     {CKM_NSS_JPAKE_ROUND2_SHA1,        {0, 0, CKF_DERIVE}, PR_TRUE},
     {CKM_NSS_JPAKE_ROUND2_SHA256,      {0, 0, CKF_DERIVE}, PR_TRUE},
     {CKM_NSS_JPAKE_ROUND2_SHA384,      {0, 0, CKF_DERIVE}, PR_TRUE},
     {CKM_NSS_JPAKE_ROUND2_SHA512,      {0, 0, CKF_DERIVE}, PR_TRUE},
     {CKM_NSS_JPAKE_FINAL_SHA1,         {0, 0, CKF_DERIVE}, PR_TRUE},
     {CKM_NSS_JPAKE_FINAL_SHA256,       {0, 0, CKF_DERIVE}, PR_TRUE},
     {CKM_NSS_JPAKE_FINAL_SHA384,       {0, 0, CKF_DERIVE}, PR_TRUE},
     {CKM_NSS_JPAKE_FINAL_SHA512,       {0, 0, CKF_DERIVE}, PR_TRUE}
};
static const CK_ULONG mechanismCount = sizeof(mechanisms)/sizeof(mechanisms[0]);


PRBool nsc_init = PR_FALSE;

#if defined(CHECK_FORK_PTHREAD) || defined(CHECK_FORK_MIXED)

#include <pthread.h>

static void ForkedChild(void)
{
    if (nsc_init || nsf_init) {
        forked = PR_TRUE;
    }
}

#endif

static char *
sftk_setStringName(const char *inString, char *buffer, int buffer_length, 		PRBool nullTerminate)
{
    int full_length, string_length;

    full_length = nullTerminate ? buffer_length -1 : buffer_length;
    string_length = PORT_Strlen(inString);
    



















    while ( string_length > full_length ) {
	
	while ( string_length > 0 && 
	      ((inString[string_length-1]&(char)0xc0) == (char)0x80)) {
	    
	    string_length--;
	}
	




	if ( string_length ) {
	    
	    string_length--;
	}
    }
    PORT_Memset(buffer,' ',full_length);
    if (nullTerminate) {
	buffer[full_length] = 0;
    }
    PORT_Memcpy(buffer,inString,string_length);
    return buffer;
}



static CK_RV
sftk_configure(const char *man, const char *libdes)
{

    
    if (man) {
	manufacturerID = sftk_setStringName(man,manufacturerID_space,
					sizeof(manufacturerID_space), PR_TRUE);
    }
    if (libdes) {
	libraryDescription = sftk_setStringName(libdes,
		libraryDescription_space, sizeof(libraryDescription_space), 
		PR_TRUE);
    }

    return CKR_OK;
}








static PRBool
sftk_hasNullPassword(SFTKSlot *slot, SFTKDBHandle *keydb)
{
    PRBool pwenabled;
   
    pwenabled = PR_FALSE;
    if (sftkdb_HasPasswordSet(keydb) == SECSuccess) {
	PRBool tokenRemoved = PR_FALSE;
    	SECStatus rv = sftkdb_CheckPassword(keydb, "", &tokenRemoved);
	if (tokenRemoved) {
	    sftk_CloseAllSessions(slot, PR_FALSE);
	}
	return (rv  == SECSuccess);
    }

    return pwenabled;
}









CK_RV
sftk_defaultAttribute(SFTKObject *object,CK_ATTRIBUTE_TYPE type,void *value,
							unsigned int len)
{
    if ( !sftk_hasAttribute(object, type)) {
	return sftk_AddAttributeType(object,type,value,len);
    }
    return CKR_OK;
}




static CK_RV
sftk_handleDataObject(SFTKSession *session,SFTKObject *object)
{
    CK_RV crv;

    
    if (sftk_isTrue(object,CKA_PRIVATE) || sftk_isTrue(object,CKA_TOKEN)) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }

    
    crv = sftk_defaultAttribute(object,CKA_APPLICATION,NULL,0);
    if (crv != CKR_OK) return crv;
    crv = sftk_defaultAttribute(object,CKA_VALUE,NULL,0);
    if (crv != CKR_OK) return crv;

    return CKR_OK;
}




static CK_RV
sftk_handleCertObject(SFTKSession *session,SFTKObject *object)
{
    CK_CERTIFICATE_TYPE type;
    SFTKAttribute *attribute;
    CK_RV crv;

    
    if ( !sftk_hasAttribute(object,CKA_CERTIFICATE_TYPE) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }

    
    if (sftk_isTrue(object,CKA_PRIVATE)) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }
	
    
    attribute = sftk_FindAttribute(object,CKA_CERTIFICATE_TYPE);
    if (attribute == NULL) return CKR_TEMPLATE_INCOMPLETE;
    type = *(CK_CERTIFICATE_TYPE *)attribute->attrib.pValue;
    sftk_FreeAttribute(attribute);

    if (type != CKC_X_509) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }

    

    
    if ( !sftk_hasAttribute(object,CKA_VALUE) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }

    
    if ( !sftk_hasAttribute(object,CKA_SUBJECT) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }

    
    if ( !sftk_hasAttribute(object,CKA_ISSUER) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }

    
    if ( !sftk_hasAttribute(object,CKA_SERIAL_NUMBER) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }

    
    object->objectInfo = NULL;
    object->infoFree = (SFTKFree) NULL;
    
    
    crv = sftk_defaultAttribute(object, CKA_ID, NULL, 0);
    if (crv != CKR_OK) { return crv; }

    if (sftk_isTrue(object,CKA_TOKEN)) {
	SFTKSlot *slot = session->slot;
	SFTKDBHandle *certHandle = sftk_getCertDB(slot);

	if (certHandle == NULL) {
	    return CKR_TOKEN_WRITE_PROTECTED;
	}

	crv = sftkdb_write(certHandle, object, &object->handle);
	sftk_freeDB(certHandle);
	return crv;
    }

    return CKR_OK;
}
	



static CK_RV
sftk_handleTrustObject(SFTKSession *session,SFTKObject *object)
{
    
    if (sftk_isTrue(object,CKA_PRIVATE)) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }

    
    if ( !sftk_hasAttribute(object,CKA_ISSUER) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }
    if ( !sftk_hasAttribute(object,CKA_SERIAL_NUMBER) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }
    if ( !sftk_hasAttribute(object,CKA_CERT_SHA1_HASH) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }
    if ( !sftk_hasAttribute(object,CKA_CERT_MD5_HASH) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }

    if (sftk_isTrue(object,CKA_TOKEN)) {
	SFTKSlot *slot = session->slot;
	SFTKDBHandle *certHandle = sftk_getCertDB(slot);
	CK_RV crv;

	if (certHandle == NULL) {
	    return CKR_TOKEN_WRITE_PROTECTED;
	}

	crv = sftkdb_write(certHandle, object, &object->handle);
	sftk_freeDB(certHandle);
	return crv;
    }

    return CKR_OK;
}




static CK_RV
sftk_handleSMimeObject(SFTKSession *session,SFTKObject *object)
{

    
    if (sftk_isTrue(object,CKA_PRIVATE)) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }

    
    if ( !sftk_hasAttribute(object,CKA_SUBJECT) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }
    if ( !sftk_hasAttribute(object,CKA_NETSCAPE_EMAIL) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }

    if (sftk_isTrue(object,CKA_TOKEN)) {
	SFTKSlot *slot = session->slot;
	SFTKDBHandle *certHandle;
	CK_RV crv;

	PORT_Assert(slot);
	if (slot == NULL) {
	    return CKR_SESSION_HANDLE_INVALID;
	}

	certHandle = sftk_getCertDB(slot);
	if (certHandle == NULL) {
	    return CKR_TOKEN_WRITE_PROTECTED;
	}

	crv = sftkdb_write(certHandle, object, &object->handle);
	sftk_freeDB(certHandle);
	return crv;
    }

    return CKR_OK;
}




static CK_RV
sftk_handleCrlObject(SFTKSession *session,SFTKObject *object)
{

    
    if (sftk_isTrue(object,CKA_PRIVATE)) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }

    
    if ( !sftk_hasAttribute(object,CKA_SUBJECT) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }
    if ( !sftk_hasAttribute(object,CKA_VALUE) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }

    if (sftk_isTrue(object,CKA_TOKEN)) {
	SFTKSlot *slot = session->slot;
	SFTKDBHandle *certHandle = sftk_getCertDB(slot);
	CK_RV crv;

	if (certHandle == NULL) {
	    return CKR_TOKEN_WRITE_PROTECTED;
	}

	crv = sftkdb_write(certHandle, object, &object->handle);
	sftk_freeDB(certHandle);
	return crv;
    }

    return CKR_OK;
}




static CK_RV
sftk_handlePublicKeyObject(SFTKSession *session, SFTKObject *object,
							 CK_KEY_TYPE key_type)
{
    CK_BBOOL encrypt = CK_TRUE;
    CK_BBOOL recover = CK_TRUE;
    CK_BBOOL wrap = CK_TRUE;
    CK_BBOOL derive = CK_FALSE;
    CK_BBOOL verify = CK_TRUE;
    CK_RV crv;

    switch (key_type) {
    case CKK_RSA:
	crv = sftk_ConstrainAttribute(object, CKA_MODULUS,
						 RSA_MIN_MODULUS_BITS, 0, 0);
	if (crv != CKR_OK) {
	    return crv;
	}
	crv = sftk_ConstrainAttribute(object, CKA_PUBLIC_EXPONENT, 2, 0, 0);
	if (crv != CKR_OK) {
	    return crv;
	}
	break;
    case CKK_DSA:
	crv = sftk_ConstrainAttribute(object, CKA_SUBPRIME, 
						DSA_Q_BITS, DSA_Q_BITS, 0);
	if (crv != CKR_OK) {
	    return crv;
	}
	crv = sftk_ConstrainAttribute(object, CKA_PRIME, 
					DSA_MIN_P_BITS, DSA_MAX_P_BITS, 64);
	if (crv != CKR_OK) {
	    return crv;
	}
	crv = sftk_ConstrainAttribute(object, CKA_BASE, 1, DSA_MAX_P_BITS, 0);
	if (crv != CKR_OK) {
	    return crv;
	}
	crv = sftk_ConstrainAttribute(object, CKA_VALUE, 1, DSA_MAX_P_BITS, 0);
	if (crv != CKR_OK) {
	    return crv;
	}
	encrypt = CK_FALSE;
	recover = CK_FALSE;
	wrap = CK_FALSE;
	break;
    case CKK_DH:
	crv = sftk_ConstrainAttribute(object, CKA_PRIME, 
					DH_MIN_P_BITS, DH_MAX_P_BITS, 0);
	if (crv != CKR_OK) {
	    return crv;
	}
	crv = sftk_ConstrainAttribute(object, CKA_BASE, 1, DH_MAX_P_BITS, 0);
	if (crv != CKR_OK) {
	    return crv;
	}
	crv = sftk_ConstrainAttribute(object, CKA_VALUE, 1, DH_MAX_P_BITS, 0);
	if (crv != CKR_OK) {
	    return crv;
	}
	verify = CK_FALSE;
	derive = CK_TRUE;
	encrypt = CK_FALSE;
	recover = CK_FALSE;
	wrap = CK_FALSE;
	break;
#ifdef NSS_ENABLE_ECC
    case CKK_EC:
	if ( !sftk_hasAttribute(object, CKA_EC_PARAMS)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !sftk_hasAttribute(object, CKA_EC_POINT)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	derive = CK_TRUE;    
	verify = CK_TRUE;    
	encrypt = CK_FALSE;
	recover = CK_FALSE;
	wrap = CK_FALSE;
	break;
#endif 
    default:
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }

    
    crv = sftk_defaultAttribute(object,CKA_SUBJECT,NULL,0);
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_ENCRYPT,&encrypt,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_VERIFY,&verify,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_VERIFY_RECOVER,
						&recover,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_WRAP,&wrap,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_DERIVE,&derive,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 

    object->objectInfo = sftk_GetPubKey(object,key_type, &crv);
    if (object->objectInfo == NULL) {
	return crv;
    }
    object->infoFree = (SFTKFree) nsslowkey_DestroyPublicKey;

    if (sftk_isTrue(object,CKA_TOKEN)) {
	SFTKSlot *slot = session->slot;
	SFTKDBHandle *certHandle = sftk_getCertDB(slot);

	if (certHandle == NULL) {
	    return CKR_TOKEN_WRITE_PROTECTED;
	}

	crv = sftkdb_write(certHandle, object, &object->handle);
	sftk_freeDB(certHandle);
	return crv;
    }

    return CKR_OK;
}

static NSSLOWKEYPrivateKey * 
sftk_mkPrivKey(SFTKObject *object,CK_KEY_TYPE key, CK_RV *rvp);

static SECStatus
sftk_fillRSAPrivateKey(SFTKObject *object);




static CK_RV
sftk_handlePrivateKeyObject(SFTKSession *session,SFTKObject *object,CK_KEY_TYPE key_type)
{
    CK_BBOOL cktrue = CK_TRUE;
    CK_BBOOL encrypt = CK_TRUE;
    CK_BBOOL sign = CK_FALSE;
    CK_BBOOL recover = CK_TRUE;
    CK_BBOOL wrap = CK_TRUE;
    CK_BBOOL derive = CK_TRUE;
    CK_BBOOL ckfalse = CK_FALSE;
    PRBool createObjectInfo = PR_TRUE;
    int missing_rsa_mod_component = 0;
    int missing_rsa_exp_component = 0;
    int missing_rsa_crt_component = 0;
    
    SECItem mod;
    CK_RV crv;

    switch (key_type) {
    case CKK_RSA:
	if ( !sftk_hasAttribute(object, CKA_MODULUS)) {
	    missing_rsa_mod_component++;
	}
	if ( !sftk_hasAttribute(object, CKA_PUBLIC_EXPONENT)) {
	    missing_rsa_exp_component++;
	}
	if ( !sftk_hasAttribute(object, CKA_PRIVATE_EXPONENT)) {
	    missing_rsa_exp_component++;
	}
	if ( !sftk_hasAttribute(object, CKA_PRIME_1)) {
	    missing_rsa_mod_component++;
	}
	if ( !sftk_hasAttribute(object, CKA_PRIME_2)) {
	    missing_rsa_mod_component++;
	}
	if ( !sftk_hasAttribute(object, CKA_EXPONENT_1)) {
	    missing_rsa_crt_component++;
	}
	if ( !sftk_hasAttribute(object, CKA_EXPONENT_2)) {
	    missing_rsa_crt_component++;
	}
	if ( !sftk_hasAttribute(object, CKA_COEFFICIENT)) {
	    missing_rsa_crt_component++;
	}
	if (missing_rsa_mod_component || missing_rsa_exp_component || 
					 missing_rsa_crt_component) {
	    

	    int have_exp = 2- missing_rsa_exp_component;
	    int have_component = 5- 
		(missing_rsa_exp_component+missing_rsa_mod_component);
	    SECStatus rv;

	    if ((have_exp == 0) || (have_component < 3)) {
		
		return CKR_TEMPLATE_INCOMPLETE;
	    }
	    
	    rv = sftk_fillRSAPrivateKey(object);
	    if (rv != SECSuccess) {
		return CKR_TEMPLATE_INCOMPLETE;
	    }
	}
		
	
	crv = sftk_Attribute2SSecItem(NULL, &mod, object, CKA_MODULUS);
	if (crv != CKR_OK) return crv;
	crv = sftk_forceAttribute(object, CKA_NETSCAPE_DB, 
						sftk_item_expand(&mod));
	if (mod.data) PORT_Free(mod.data);
	if (crv != CKR_OK) return crv;

	sign = CK_TRUE;
	derive = CK_FALSE;
	break;
    case CKK_DSA:
	if ( !sftk_hasAttribute(object, CKA_SUBPRIME)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	sign = CK_TRUE;
	derive = CK_FALSE;
	
    case CKK_DH:
	if ( !sftk_hasAttribute(object, CKA_PRIME)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !sftk_hasAttribute(object, CKA_BASE)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !sftk_hasAttribute(object, CKA_VALUE)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	encrypt = CK_FALSE;
	recover = CK_FALSE;
	wrap = CK_FALSE;
	break;
#ifdef NSS_ENABLE_ECC
    case CKK_EC:
	if ( !sftk_hasAttribute(object, CKA_EC_PARAMS)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	if ( !sftk_hasAttribute(object, CKA_VALUE)) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	encrypt = CK_FALSE;
	sign = CK_TRUE;
	recover = CK_FALSE;
	wrap = CK_FALSE;
	break;
#endif 
    case CKK_NSS_JPAKE_ROUND1:
        if (!sftk_hasAttribute(object, CKA_PRIME ||
            !sftk_hasAttribute(object, CKA_SUBPRIME) ||
            !sftk_hasAttribute(object, CKA_BASE))) {
            return CKR_TEMPLATE_INCOMPLETE;
        }
        
    case CKK_NSS_JPAKE_ROUND2:
        

        encrypt = sign = recover = wrap = CK_FALSE;
        derive = CK_TRUE;
        createObjectInfo = PR_FALSE;
        break;
    default:
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }
    crv = sftk_defaultAttribute(object,CKA_SUBJECT,NULL,0);
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_SENSITIVE,&cktrue,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_EXTRACTABLE,&cktrue,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_DECRYPT,&encrypt,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_SIGN,&sign,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_SIGN_RECOVER,&recover,
							     sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_UNWRAP,&wrap,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_DERIVE,&derive,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    
    crv = sftk_forceAttribute(object,CKA_ALWAYS_SENSITIVE,
						&ckfalse,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_forceAttribute(object,CKA_NEVER_EXTRACTABLE,
						&ckfalse,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 

    

    if (sftk_isTrue(object,CKA_TOKEN)) {
	SFTKSlot *slot = session->slot;
	SFTKDBHandle *keyHandle = sftk_getKeyDB(slot);
	CK_RV crv;

	if (keyHandle == NULL) {
	    return CKR_TOKEN_WRITE_PROTECTED;
	}

	crv = sftkdb_write(keyHandle, object, &object->handle);
	sftk_freeDB(keyHandle);
	return crv;
    } else if (createObjectInfo) {
	object->objectInfo = sftk_mkPrivKey(object,key_type,&crv);
	if (object->objectInfo == NULL) return crv;
	object->infoFree = (SFTKFree) nsslowkey_DestroyPrivateKey;
    }
    return CKR_OK;
}


void sftk_FormatDESKey(unsigned char *key, int length);


static CK_RV
validateSecretKey(SFTKSession *session, SFTKObject *object, 
					CK_KEY_TYPE key_type, PRBool isFIPS)
{
    CK_RV crv;
    CK_BBOOL cktrue = CK_TRUE;
    CK_BBOOL ckfalse = CK_FALSE;
    SFTKAttribute *attribute = NULL;
    unsigned long requiredLen;

    crv = sftk_defaultAttribute(object,CKA_SENSITIVE,
				isFIPS?&cktrue:&ckfalse,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_EXTRACTABLE,
						&cktrue,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_ENCRYPT,&cktrue,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_DECRYPT,&cktrue,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_SIGN,&ckfalse,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_VERIFY,&ckfalse,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_WRAP,&cktrue,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_UNWRAP,&cktrue,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 

    if ( !sftk_hasAttribute(object, CKA_VALUE)) {
	return CKR_TEMPLATE_INCOMPLETE;
    }
    
    crv = sftk_forceAttribute(object,CKA_ALWAYS_SENSITIVE,
						&ckfalse,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 
    crv = sftk_forceAttribute(object,CKA_NEVER_EXTRACTABLE,
						&ckfalse,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 

    
    crv = CKR_OK;
    switch (key_type) {
    
    case CKK_GENERIC_SECRET:
    case CKK_RC2:
    case CKK_RC4:
#if NSS_SOFTOKEN_DOES_RC5
    case CKK_RC5:
#endif
#ifdef NSS_SOFTOKEN_DOES_CAST
    case CKK_CAST:
    case CKK_CAST3:
    case CKK_CAST5:
#endif
#if NSS_SOFTOKEN_DOES_IDEA
    case CKK_IDEA:
#endif
	attribute = sftk_FindAttribute(object,CKA_VALUE);
	
	if (attribute == NULL) return CKR_TEMPLATE_INCOMPLETE;
	crv = sftk_forceAttribute(object, CKA_VALUE_LEN, 
			&attribute->attrib.ulValueLen, sizeof(CK_ULONG));
	sftk_FreeAttribute(attribute);
	break;
    
    case CKK_DES:
    case CKK_DES2:
    case CKK_DES3:
    case CKK_CDMF:
	attribute = sftk_FindAttribute(object,CKA_VALUE);
	
	if (attribute == NULL) 
	    return CKR_TEMPLATE_INCOMPLETE;
	requiredLen = sftk_MapKeySize(key_type);
	if (attribute->attrib.ulValueLen != requiredLen) {
	    sftk_FreeAttribute(attribute);
	    return CKR_KEY_SIZE_RANGE;
	}
	sftk_FormatDESKey((unsigned char*)attribute->attrib.pValue,
						 attribute->attrib.ulValueLen);
	sftk_FreeAttribute(attribute);
	break;
    case CKK_AES:
	attribute = sftk_FindAttribute(object,CKA_VALUE);
	
	if (attribute == NULL) 
	    return CKR_TEMPLATE_INCOMPLETE;
	if (attribute->attrib.ulValueLen != 16 &&
	    attribute->attrib.ulValueLen != 24 &&
	    attribute->attrib.ulValueLen != 32) {
	    sftk_FreeAttribute(attribute);
	    return CKR_KEY_SIZE_RANGE;
	}
	crv = sftk_forceAttribute(object, CKA_VALUE_LEN, 
			&attribute->attrib.ulValueLen, sizeof(CK_ULONG));
	sftk_FreeAttribute(attribute);
	break;
    default:
	break;
    }

    return crv;
}




static CK_RV
sftk_handleSecretKeyObject(SFTKSession *session,SFTKObject *object,
					CK_KEY_TYPE key_type, PRBool isFIPS)
{
    CK_RV crv;

    
    crv = validateSecretKey(session, object, key_type, isFIPS);
    if (crv != CKR_OK) goto loser;

    
    if (sftk_isTrue(object,CKA_TOKEN)) {
	SFTKSlot *slot = session->slot;
	SFTKDBHandle *keyHandle = sftk_getKeyDB(slot);
	CK_RV crv;

	if (keyHandle == NULL) {
	    return CKR_TOKEN_WRITE_PROTECTED;
	}

	crv = sftkdb_write(keyHandle, object, &object->handle);
	sftk_freeDB(keyHandle);
	return crv;
    }

loser:

    return crv;
}




static CK_RV
sftk_handleKeyObject(SFTKSession *session, SFTKObject *object)
{
    SFTKAttribute *attribute;
    CK_KEY_TYPE key_type;
    CK_BBOOL ckfalse = CK_FALSE;
    CK_RV crv;

    
    if ( !sftk_hasAttribute(object,CKA_KEY_TYPE) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }

    
    crv = sftk_defaultAttribute(object,CKA_ID,NULL,0);
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_START_DATE,NULL,0);
    if (crv != CKR_OK)  return crv; 
    crv = sftk_defaultAttribute(object,CKA_END_DATE,NULL,0);
    if (crv != CKR_OK)  return crv; 
    

    crv = sftk_defaultAttribute(object,CKA_LOCAL,&ckfalse,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 

    
    attribute = sftk_FindAttribute(object,CKA_KEY_TYPE);
    if (!attribute) {
        return CKR_ATTRIBUTE_VALUE_INVALID;
    }
    key_type = *(CK_KEY_TYPE *)attribute->attrib.pValue;
    sftk_FreeAttribute(attribute);

    switch (object->objclass) {
    case CKO_PUBLIC_KEY:
	return sftk_handlePublicKeyObject(session,object,key_type);
    case CKO_PRIVATE_KEY:
	return sftk_handlePrivateKeyObject(session,object,key_type);
    case CKO_SECRET_KEY:
	
	return sftk_handleSecretKeyObject(session,object,key_type,
			     (PRBool)(session->slot->slotID == FIPS_SLOT_ID));
    default:
	break;
    }
    return CKR_ATTRIBUTE_VALUE_INVALID;
}




static CK_RV
sftk_handleDSAParameterObject(SFTKSession *session, SFTKObject *object)
{
    SFTKAttribute *primeAttr = NULL;
    SFTKAttribute *subPrimeAttr = NULL;
    SFTKAttribute *baseAttr = NULL;
    SFTKAttribute *seedAttr = NULL;
    SFTKAttribute *hAttr = NULL;
    SFTKAttribute *attribute;
    CK_RV crv = CKR_TEMPLATE_INCOMPLETE;
    PQGParams params;
    PQGVerify vfy, *verify = NULL;
    SECStatus result,rv;

    primeAttr = sftk_FindAttribute(object,CKA_PRIME);
    if (primeAttr == NULL) goto loser;
    params.prime.data = primeAttr->attrib.pValue;
    params.prime.len = primeAttr->attrib.ulValueLen;

    subPrimeAttr = sftk_FindAttribute(object,CKA_SUBPRIME);
    if (subPrimeAttr == NULL) goto loser;
    params.subPrime.data = subPrimeAttr->attrib.pValue;
    params.subPrime.len = subPrimeAttr->attrib.ulValueLen;

    baseAttr = sftk_FindAttribute(object,CKA_BASE);
    if (baseAttr == NULL) goto loser;
    params.base.data = baseAttr->attrib.pValue;
    params.base.len = baseAttr->attrib.ulValueLen;

    attribute = sftk_FindAttribute(object, CKA_NETSCAPE_PQG_COUNTER);
    if (attribute != NULL) {
	vfy.counter = *(CK_ULONG *) attribute->attrib.pValue;
	sftk_FreeAttribute(attribute);

	seedAttr = sftk_FindAttribute(object, CKA_NETSCAPE_PQG_SEED);
	if (seedAttr == NULL) goto loser;
	vfy.seed.data = seedAttr->attrib.pValue;
	vfy.seed.len = seedAttr->attrib.ulValueLen;

	hAttr = sftk_FindAttribute(object, CKA_NETSCAPE_PQG_H);
	if (hAttr == NULL) goto loser;
	vfy.h.data = hAttr->attrib.pValue;
	vfy.h.len = hAttr->attrib.ulValueLen;

	verify = &vfy;
    }

    crv = CKR_FUNCTION_FAILED;
    rv = PQG_VerifyParams(&params,verify,&result);
    if (rv == SECSuccess) {
	crv = (result== SECSuccess) ? CKR_OK : CKR_ATTRIBUTE_VALUE_INVALID;
    }

loser:
    if (hAttr) sftk_FreeAttribute(hAttr);
    if (seedAttr) sftk_FreeAttribute(seedAttr);
    if (baseAttr) sftk_FreeAttribute(baseAttr);
    if (subPrimeAttr) sftk_FreeAttribute(subPrimeAttr);
    if (primeAttr) sftk_FreeAttribute(primeAttr);

    return crv;
}




static CK_RV
sftk_handleKeyParameterObject(SFTKSession *session, SFTKObject *object)
{
    SFTKAttribute *attribute;
    CK_KEY_TYPE key_type;
    CK_BBOOL ckfalse = CK_FALSE;
    CK_RV crv;

    
    if ( !sftk_hasAttribute(object,CKA_KEY_TYPE) ) {
	return CKR_TEMPLATE_INCOMPLETE;
    }

    
    crv = sftk_defaultAttribute(object,CKA_LOCAL,&ckfalse,sizeof(CK_BBOOL));
    if (crv != CKR_OK)  return crv; 

    
    attribute = sftk_FindAttribute(object,CKA_KEY_TYPE);
    if (!attribute) {
        return CKR_ATTRIBUTE_VALUE_INVALID;
    }
    key_type = *(CK_KEY_TYPE *)attribute->attrib.pValue;
    sftk_FreeAttribute(attribute);

    switch (key_type) {
    case CKK_DSA:
	return sftk_handleDSAParameterObject(session,object);
	
    default:
	break;
    }
    return CKR_KEY_TYPE_INCONSISTENT;
}







CK_RV
sftk_handleObject(SFTKObject *object, SFTKSession *session)
{
    SFTKSlot *slot = session->slot;
    SFTKAttribute *attribute;
    SFTKObject *duplicateObject = NULL;
    CK_OBJECT_HANDLE handle;
    CK_BBOOL ckfalse = CK_FALSE;
    CK_BBOOL cktrue = CK_TRUE;
    CK_RV crv;

    

    crv = sftk_defaultAttribute(object,CKA_TOKEN,&ckfalse,sizeof(CK_BBOOL));
    if (crv != CKR_OK) return crv;
    crv = sftk_defaultAttribute(object,CKA_PRIVATE,&ckfalse,sizeof(CK_BBOOL));
    if (crv != CKR_OK) return crv;
    crv = sftk_defaultAttribute(object,CKA_LABEL,NULL,0);
    if (crv != CKR_OK) return crv;
    crv = sftk_defaultAttribute(object,CKA_MODIFIABLE,&cktrue,sizeof(CK_BBOOL));
    if (crv != CKR_OK) return crv;

    
    if ((!slot->isLoggedIn) && (slot->needLogin) &&
				(sftk_isTrue(object,CKA_PRIVATE))) {
	return CKR_USER_NOT_LOGGED_IN;
    }


    if (((session->info.flags & CKF_RW_SESSION) == 0) &&
				(sftk_isTrue(object,CKA_TOKEN))) {
	return CKR_SESSION_READ_ONLY;
    }
	
    












    do {
	PRUint32 wrappedAround;

	duplicateObject = NULL;
	PZ_Lock(slot->objectLock);
	wrappedAround = slot->sessionObjectHandleCount &  SFTK_TOKEN_MASK;
	handle        = slot->sessionObjectHandleCount & ~SFTK_TOKEN_MASK;
	if (!handle) 
	    handle = minSessionObjectHandle;  
	slot->sessionObjectHandleCount = (handle + 1U) | wrappedAround;
	
	if (wrappedAround) {
	    sftkqueue_find(duplicateObject, handle, slot->sessObjHashTable, \
	                   slot->sessObjHashSize);
	}
	PZ_Unlock(slot->objectLock);
    } while (duplicateObject != NULL);
    object->handle = handle;

    
    attribute = sftk_FindAttribute(object,CKA_CLASS);
    if (attribute == NULL) {
	return CKR_TEMPLATE_INCOMPLETE;
    }
    object->objclass = *(CK_OBJECT_CLASS *)attribute->attrib.pValue;
    sftk_FreeAttribute(attribute);

    



    switch (object->objclass) {
    case CKO_DATA:
	crv = sftk_handleDataObject(session,object);
	break;
    case CKO_CERTIFICATE:
	crv = sftk_handleCertObject(session,object);
	break;
    case CKO_NETSCAPE_TRUST:
	crv = sftk_handleTrustObject(session,object);
	break;
    case CKO_NETSCAPE_CRL:
	crv = sftk_handleCrlObject(session,object);
	break;
    case CKO_NETSCAPE_SMIME:
	crv = sftk_handleSMimeObject(session,object);
	break;
    case CKO_PRIVATE_KEY:
    case CKO_PUBLIC_KEY:
    case CKO_SECRET_KEY:
	crv = sftk_handleKeyObject(session,object);
	break;
    case CKO_KG_PARAMETERS:
	crv = sftk_handleKeyParameterObject(session,object);
	break;
    default:
	crv = CKR_ATTRIBUTE_VALUE_INVALID;
	break;
    }

    

    if (crv != CKR_OK) {
	return crv;
    }

    




    if (sftk_isToken(object->handle)) {
	sftk_convertSessionToToken(object);
    } else {
	object->slot = slot;
	sftk_AddObject(session,object);
    }

    return CKR_OK;
}





NSSLOWKEYPublicKey *sftk_GetPubKey(SFTKObject *object,CK_KEY_TYPE key_type, 
								CK_RV *crvp)
{
    NSSLOWKEYPublicKey *pubKey;
    PLArenaPool *arena;
    CK_RV crv;

    if (object->objclass != CKO_PUBLIC_KEY) {
	*crvp = CKR_KEY_TYPE_INCONSISTENT;
	return NULL;
    }

    if (sftk_isToken(object->handle)) {

    }

    
    if (object->objectInfo) {
	*crvp = CKR_OK;
	return (NSSLOWKEYPublicKey *)object->objectInfo;
    }

    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	*crvp = CKR_HOST_MEMORY;
	return NULL;
    }

    pubKey = (NSSLOWKEYPublicKey *)
			PORT_ArenaAlloc(arena,sizeof(NSSLOWKEYPublicKey));
    if (pubKey == NULL) {
    	PORT_FreeArena(arena,PR_FALSE);
	*crvp = CKR_HOST_MEMORY;
	return NULL;
    }

    
    pubKey->arena = arena;
    switch (key_type) {
    case CKK_RSA:
	pubKey->keyType = NSSLOWKEYRSAKey;
	crv = sftk_Attribute2SSecItem(arena,&pubKey->u.rsa.modulus,
							object,CKA_MODULUS);
    	if (crv != CKR_OK) break;
    	crv = sftk_Attribute2SSecItem(arena,&pubKey->u.rsa.publicExponent,
						object,CKA_PUBLIC_EXPONENT);
	break;
    case CKK_DSA:
	pubKey->keyType = NSSLOWKEYDSAKey;
	crv = sftk_Attribute2SSecItem(arena,&pubKey->u.dsa.params.prime,
							object,CKA_PRIME);
    	if (crv != CKR_OK) break;
	crv = sftk_Attribute2SSecItem(arena,&pubKey->u.dsa.params.subPrime,
							object,CKA_SUBPRIME);
    	if (crv != CKR_OK) break;
	crv = sftk_Attribute2SSecItem(arena,&pubKey->u.dsa.params.base,
							object,CKA_BASE);
    	if (crv != CKR_OK) break;
    	crv = sftk_Attribute2SSecItem(arena,&pubKey->u.dsa.publicValue,
							object,CKA_VALUE);
	break;
    case CKK_DH:
	pubKey->keyType = NSSLOWKEYDHKey;
	crv = sftk_Attribute2SSecItem(arena,&pubKey->u.dh.prime,
							object,CKA_PRIME);
    	if (crv != CKR_OK) break;
	crv = sftk_Attribute2SSecItem(arena,&pubKey->u.dh.base,
							object,CKA_BASE);
    	if (crv != CKR_OK) break;
    	crv = sftk_Attribute2SSecItem(arena,&pubKey->u.dh.publicValue,
							object,CKA_VALUE);
	break;
#ifdef NSS_ENABLE_ECC
    case CKK_EC:
	pubKey->keyType = NSSLOWKEYECKey;
	crv = sftk_Attribute2SSecItem(arena,
	                              &pubKey->u.ec.ecParams.DEREncoding,
	                              object,CKA_EC_PARAMS);
	if (crv != CKR_OK) break;

	


	if (EC_FillParams(arena, &pubKey->u.ec.ecParams.DEREncoding,
		    &pubKey->u.ec.ecParams) != SECSuccess) {
	    crv = CKR_DOMAIN_PARAMS_INVALID;
	    break;
	}
	    
	crv = sftk_Attribute2SSecItem(arena,&pubKey->u.ec.publicValue,
	                              object,CKA_EC_POINT);
	if (crv == CKR_OK) {
	    int keyLen,curveLen;

	    curveLen = (pubKey->u.ec.ecParams.fieldID.size +7)/8;
	    keyLen = (2*curveLen)+1;

	    



	    	
	    if (pubKey->u.ec.publicValue.data[0] == EC_POINT_FORM_UNCOMPRESSED
		&& pubKey->u.ec.publicValue.len == keyLen) {
		break; 
	    }

	    

	    
	    if ((pubKey->u.ec.publicValue.data[0] == SEC_ASN1_OCTET_STRING) 
		&& pubKey->u.ec.publicValue.len > keyLen) {
		SECItem publicValue;
		SECStatus rv;

		rv = SEC_QuickDERDecodeItem(arena, &publicValue, 
					 SEC_ASN1_GET(SEC_OctetStringTemplate), 
					 &pubKey->u.ec.publicValue);
		
		if ((rv != SECSuccess)
		    || (publicValue.data[0] != EC_POINT_FORM_UNCOMPRESSED)
		    || (publicValue.len != keyLen)) {
	   	    crv = CKR_ATTRIBUTE_VALUE_INVALID;
		    break;
		}
		
		pubKey->u.ec.publicValue = publicValue;
		break;
	    }
	   crv = CKR_ATTRIBUTE_VALUE_INVALID;
	}
	break;
#endif 
    default:
	crv = CKR_KEY_TYPE_INCONSISTENT;
	break;
    }
    *crvp = crv;
    if (crv != CKR_OK) {
    	PORT_FreeArena(arena,PR_FALSE);
	return NULL;
    }

    object->objectInfo = pubKey;
    object->infoFree = (SFTKFree) nsslowkey_DestroyPublicKey;
    return pubKey;
}


static NSSLOWKEYPrivateKey *
sftk_mkPrivKey(SFTKObject *object, CK_KEY_TYPE key_type, CK_RV *crvp)
{
    NSSLOWKEYPrivateKey *privKey;
    SFTKItemTemplate itemTemplate[SFTK_MAX_ITEM_TEMPLATE];
    int itemTemplateCount = 0;
    PLArenaPool *arena;
    CK_RV crv = CKR_OK;
    SECStatus rv;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	*crvp = CKR_HOST_MEMORY;
	return NULL;
    }

    privKey = (NSSLOWKEYPrivateKey *)
			PORT_ArenaZAlloc(arena,sizeof(NSSLOWKEYPrivateKey));
    if (privKey == NULL)  {
	PORT_FreeArena(arena,PR_FALSE);
	*crvp = CKR_HOST_MEMORY;
	return NULL;
    }

    
    privKey->arena = arena;
    switch (key_type) {
    case CKK_RSA:
	privKey->keyType = NSSLOWKEYRSAKey;

	SFTK_SET_ITEM_TEMPLATE(itemTemplate, itemTemplateCount,
		&privKey->u.rsa.modulus,CKA_MODULUS);
	itemTemplateCount++;
	SFTK_SET_ITEM_TEMPLATE(itemTemplate, itemTemplateCount,
		&privKey->u.rsa.publicExponent, CKA_PUBLIC_EXPONENT);
	itemTemplateCount++;
	SFTK_SET_ITEM_TEMPLATE(itemTemplate, itemTemplateCount,
		&privKey->u.rsa.privateExponent, CKA_PRIVATE_EXPONENT);
	itemTemplateCount++;
	SFTK_SET_ITEM_TEMPLATE(itemTemplate, itemTemplateCount,
		&privKey->u.rsa.prime1, CKA_PRIME_1);
	itemTemplateCount++;
	SFTK_SET_ITEM_TEMPLATE(itemTemplate, itemTemplateCount,
		&privKey->u.rsa.prime2, CKA_PRIME_2);
	itemTemplateCount++;
	SFTK_SET_ITEM_TEMPLATE(itemTemplate, itemTemplateCount,
		&privKey->u.rsa.exponent1, CKA_EXPONENT_1);
	itemTemplateCount++;
	SFTK_SET_ITEM_TEMPLATE(itemTemplate, itemTemplateCount,
		&privKey->u.rsa.exponent2, CKA_EXPONENT_2);
	itemTemplateCount++;
	SFTK_SET_ITEM_TEMPLATE(itemTemplate, itemTemplateCount,
		&privKey->u.rsa.coefficient, CKA_COEFFICIENT);
	itemTemplateCount++;
        rv = DER_SetUInteger(privKey->arena, &privKey->u.rsa.version,
                          NSSLOWKEY_PRIVATE_KEY_INFO_VERSION);
	if (rv != SECSuccess) crv = CKR_HOST_MEMORY;
	break;

    case CKK_DSA:
	privKey->keyType = NSSLOWKEYDSAKey;
	SFTK_SET_ITEM_TEMPLATE(itemTemplate, itemTemplateCount,
		&privKey->u.dsa.params.prime, CKA_PRIME);
	itemTemplateCount++;
	SFTK_SET_ITEM_TEMPLATE(itemTemplate, itemTemplateCount,
		&privKey->u.dsa.params.subPrime, CKA_SUBPRIME);
	itemTemplateCount++;
	SFTK_SET_ITEM_TEMPLATE(itemTemplate, itemTemplateCount,
		&privKey->u.dsa.params.base, CKA_BASE);
	itemTemplateCount++;
	SFTK_SET_ITEM_TEMPLATE(itemTemplate, itemTemplateCount,
		&privKey->u.dsa.privateValue, CKA_VALUE);
	itemTemplateCount++;
	

	break;

    case CKK_DH:
	privKey->keyType = NSSLOWKEYDHKey;
	SFTK_SET_ITEM_TEMPLATE(itemTemplate, itemTemplateCount,
		&privKey->u.dh.prime, CKA_PRIME);
	itemTemplateCount++;
	SFTK_SET_ITEM_TEMPLATE(itemTemplate, itemTemplateCount,
		&privKey->u.dh.base, CKA_BASE);
	itemTemplateCount++;
	SFTK_SET_ITEM_TEMPLATE(itemTemplate, itemTemplateCount,
		&privKey->u.dh.privateValue, CKA_VALUE);
	itemTemplateCount++;
	

	break;

#ifdef NSS_ENABLE_ECC
    case CKK_EC:
	privKey->keyType = NSSLOWKEYECKey;
	crv = sftk_Attribute2SSecItem(arena, 
				      &privKey->u.ec.ecParams.DEREncoding,
				      object,CKA_EC_PARAMS);
    	if (crv != CKR_OK) break;

	


	if (EC_FillParams(arena, &privKey->u.ec.ecParams.DEREncoding,
		    &privKey->u.ec.ecParams) != SECSuccess) {
	    crv = CKR_DOMAIN_PARAMS_INVALID;
	    break;
	}
	crv = sftk_Attribute2SSecItem(arena,&privKey->u.ec.privateValue,
							object,CKA_VALUE);
	if (crv != CKR_OK) break;

	if (sftk_hasAttribute(object, CKA_NETSCAPE_DB)) {
	    crv = sftk_Attribute2SSecItem(arena, &privKey->u.ec.publicValue,
 				object, CKA_NETSCAPE_DB);
	    if (crv != CKR_OK) break;
	    

	}
        rv = DER_SetUInteger(privKey->arena, &privKey->u.ec.version,
                          NSSLOWKEY_EC_PRIVATE_KEY_VERSION);
	if (rv != SECSuccess) crv = CKR_HOST_MEMORY;
	break;
#endif 

    default:
	crv = CKR_KEY_TYPE_INCONSISTENT;
	break;
    }
    if (crv == CKR_OK && itemTemplateCount != 0) {
	PORT_Assert(itemTemplateCount > 0);
	PORT_Assert(itemTemplateCount <= SFTK_MAX_ITEM_TEMPLATE);
	crv = sftk_MultipleAttribute2SecItem(arena, object, itemTemplate, 
						itemTemplateCount);
    }
    *crvp = crv;
    if (crv != CKR_OK) {
	PORT_FreeArena(arena,PR_FALSE);
	return NULL;
    }
    return privKey;
}




static SECStatus
sftk_fillRSAPrivateKey(SFTKObject *object)
{
    RSAPrivateKey tmpKey = { 0 };
    SFTKAttribute *modulus = NULL;
    SFTKAttribute *prime1 = NULL;
    SFTKAttribute *prime2 = NULL;
    SFTKAttribute *privateExponent = NULL;
    SFTKAttribute *publicExponent = NULL;
    SECStatus rv;
    CK_RV crv;

    

    tmpKey.arena = NULL;
    modulus = sftk_FindAttribute(object, CKA_MODULUS);
    if (modulus) {
	tmpKey.modulus.data = modulus->attrib.pValue;
	tmpKey.modulus.len  = modulus->attrib.ulValueLen;
    } 
    prime1 = sftk_FindAttribute(object, CKA_PRIME_1);
    if (prime1) {
	tmpKey.prime1.data = prime1->attrib.pValue;
	tmpKey.prime1.len  = prime1->attrib.ulValueLen;
    } 
    prime2 = sftk_FindAttribute(object, CKA_PRIME_2);
    if (prime2) {
	tmpKey.prime2.data = prime2->attrib.pValue;
	tmpKey.prime2.len  = prime2->attrib.ulValueLen;
    } 
    privateExponent = sftk_FindAttribute(object, CKA_PRIVATE_EXPONENT);
    if (privateExponent) {
	tmpKey.privateExponent.data = privateExponent->attrib.pValue;
	tmpKey.privateExponent.len  = privateExponent->attrib.ulValueLen;
    } 
    publicExponent = sftk_FindAttribute(object, CKA_PUBLIC_EXPONENT);
    if (publicExponent) {
	tmpKey.publicExponent.data = publicExponent->attrib.pValue;
	tmpKey.publicExponent.len  = publicExponent->attrib.ulValueLen;
    } 

    




    rv = RSA_PopulatePrivateKey(&tmpKey);
    if (rv != SECSuccess) {
	goto loser;
    }

    
    rv = SECFailure;
    crv = sftk_forceAttribute(object,CKA_MODULUS,
                       sftk_item_expand(&tmpKey.modulus));
    if (crv != CKR_OK) goto loser;
    crv = sftk_forceAttribute(object,CKA_PUBLIC_EXPONENT,
                       sftk_item_expand(&tmpKey.publicExponent));
    if (crv != CKR_OK) goto loser;
    crv = sftk_forceAttribute(object,CKA_PRIVATE_EXPONENT,
                       sftk_item_expand(&tmpKey.privateExponent));
    if (crv != CKR_OK) goto loser;
    crv = sftk_forceAttribute(object,CKA_PRIME_1,
                       sftk_item_expand(&tmpKey.prime1));
    if (crv != CKR_OK) goto loser;
    crv = sftk_forceAttribute(object,CKA_PRIME_2,
                       sftk_item_expand(&tmpKey.prime2));
    if (crv != CKR_OK) goto loser;
    crv = sftk_forceAttribute(object,CKA_EXPONENT_1,
                       sftk_item_expand(&tmpKey.exponent1));
    if (crv != CKR_OK) goto loser;
    crv = sftk_forceAttribute(object,CKA_EXPONENT_2,
                       sftk_item_expand(&tmpKey.exponent2));
    if (crv != CKR_OK) goto loser;
    crv = sftk_forceAttribute(object,CKA_COEFFICIENT,
                       sftk_item_expand(&tmpKey.coefficient));
    if (crv != CKR_OK) goto loser;
    rv = SECSuccess;

    
loser:
    if (tmpKey.arena) {
	PORT_FreeArena(tmpKey.arena,PR_TRUE);
    }
    if (modulus) {
	sftk_FreeAttribute(modulus);
    }
    if (prime1) {
	sftk_FreeAttribute(prime1);
    }
    if (prime2) {
	sftk_FreeAttribute(prime2);
    }
    if (privateExponent) {
	sftk_FreeAttribute(privateExponent);
    }
    if (publicExponent) {
	sftk_FreeAttribute(publicExponent);
    }
    return rv;
}








NSSLOWKEYPrivateKey *
sftk_GetPrivKey(SFTKObject *object,CK_KEY_TYPE key_type, CK_RV *crvp)
{
    NSSLOWKEYPrivateKey *priv = NULL;

    if (object->objclass != CKO_PRIVATE_KEY) {
	*crvp = CKR_KEY_TYPE_INCONSISTENT;
	return NULL;
    }
    if (object->objectInfo) {
	*crvp = CKR_OK;
	return (NSSLOWKEYPrivateKey *)object->objectInfo;
    }

    priv = sftk_mkPrivKey(object, key_type, crvp);
    object->objectInfo = priv;
    object->infoFree = (SFTKFree) nsslowkey_DestroyPrivateKey;
    return priv;
}







void
sftk_FormatDESKey(unsigned char *key, int length)
{
    int i;

    
    for (i=0; i < length; i++) {
	key[i] = parityTable[key[i]>>1];
    }
}




PRBool
sftk_CheckDESKey(unsigned char *key)
{
    int i;

    
    sftk_FormatDESKey(key, 8);

    for (i=0; i < sftk_desWeakTableSize; i++) {
	if (PORT_Memcmp(key,sftk_desWeakTable[i],8) == 0) {
	    return PR_TRUE;
	}
    }
    return PR_FALSE;
}




PRBool
sftk_IsWeakKey(unsigned char *key,CK_KEY_TYPE key_type)
{

    switch(key_type) {
    case CKK_DES:
	return sftk_CheckDESKey(key);
    case CKM_DES2_KEY_GEN:
	if (sftk_CheckDESKey(key)) return PR_TRUE;
	return sftk_CheckDESKey(&key[8]);
    case CKM_DES3_KEY_GEN:
	if (sftk_CheckDESKey(key)) return PR_TRUE;
	if (sftk_CheckDESKey(&key[8])) return PR_TRUE;
	return sftk_CheckDESKey(&key[16]);
    default:
	break;
    }
    return PR_FALSE;
}










CK_RV NSC_GetFunctionList(CK_FUNCTION_LIST_PTR *pFunctionList)
{
    CHECK_FORK();

    *pFunctionList = (CK_FUNCTION_LIST_PTR) &sftk_funcList;
    return CKR_OK;
}


CK_RV C_GetFunctionList(CK_FUNCTION_LIST_PTR *pFunctionList)
{
    CHECK_FORK();

    return NSC_GetFunctionList(pFunctionList);
}

static PLHashNumber
sftk_HashNumber(const void *key)
{
    return (PLHashNumber) key;
}






const char *
sftk_getDefTokName(CK_SLOT_ID slotID)
{
    static char buf[33];

    switch (slotID) {
    case NETSCAPE_SLOT_ID:
	return "NSS Generic Crypto Services     ";
    case PRIVATE_KEY_SLOT_ID:
	return "NSS Certificate DB              ";
    case FIPS_SLOT_ID:
        return "NSS FIPS 140-2 Certificate DB   ";
    default:
	break;
    }
    sprintf(buf,"NSS Application Token %08x  ",(unsigned int) slotID);
    return buf;
}

const char *
sftk_getDefSlotName(CK_SLOT_ID slotID)
{
    static char buf[65];

    switch (slotID) {
    case NETSCAPE_SLOT_ID:
	return 
	 "NSS Internal Cryptographic Services                             ";
    case PRIVATE_KEY_SLOT_ID:
	return 
	 "NSS User Private Key and Certificate Services                   ";
    case FIPS_SLOT_ID:
        return 
         "NSS FIPS 140-2 User Private Key Services                        ";
    default:
	break;
    }
    sprintf(buf,
     "NSS Application Slot %08x                                   ",
							(unsigned int) slotID);
    return buf;
}

static CK_ULONG nscSlotCount[2] = {0 , 0};
static CK_SLOT_ID_PTR nscSlotList[2] = {NULL, NULL};
static CK_ULONG nscSlotListSize[2] = {0, 0};
static PLHashTable *nscSlotHashTable[2] = {NULL, NULL};

static int
sftk_GetModuleIndex(CK_SLOT_ID slotID)
{
    if ((slotID == FIPS_SLOT_ID) || (slotID >= SFTK_MIN_FIPS_USER_SLOT_ID)) {
	return NSC_FIPS_MODULE;
    }
    return NSC_NON_FIPS_MODULE;
}





SFTKSlot *
sftk_SlotFromID(CK_SLOT_ID slotID, PRBool all)
{
    SFTKSlot *slot;
    int index = sftk_GetModuleIndex(slotID);
    
    if (nscSlotHashTable[index] == NULL) return NULL;
    slot = (SFTKSlot *)PL_HashTableLookupConst(nscSlotHashTable[index], 
							(void *)slotID);
    
    if (slot && !all && !slot->present) slot = NULL;
    return slot;
}

SFTKSlot *
sftk_SlotFromSessionHandle(CK_SESSION_HANDLE handle)
{
    CK_ULONG slotIDIndex = (handle >> 24) & 0x7f;
    CK_ULONG moduleIndex = (handle >> 31) & 1;

    if (slotIDIndex >= nscSlotCount[moduleIndex]) {
	return NULL;
    }

    return sftk_SlotFromID(nscSlotList[moduleIndex][slotIDIndex], PR_FALSE);
}
 
static CK_RV
sftk_RegisterSlot(SFTKSlot *slot, int moduleIndex)
{
    PLHashEntry *entry;
    int index;

    index = sftk_GetModuleIndex(slot->slotID);

    
    if (moduleIndex != index) {
	return CKR_SLOT_ID_INVALID;
    }

    if (nscSlotList[index] == NULL) {
	nscSlotListSize[index] = NSC_SLOT_LIST_BLOCK_SIZE;
	nscSlotList[index] = (CK_SLOT_ID *)
		PORT_ZAlloc(nscSlotListSize[index]*sizeof(CK_SLOT_ID));
	if (nscSlotList[index] == NULL) {
	    return CKR_HOST_MEMORY;
	}
    }
    if (nscSlotCount[index] >= nscSlotListSize[index]) {
	CK_SLOT_ID* oldNscSlotList = nscSlotList[index];
	CK_ULONG oldNscSlotListSize = nscSlotListSize[index];
	nscSlotListSize[index] += NSC_SLOT_LIST_BLOCK_SIZE;
	nscSlotList[index] = (CK_SLOT_ID *) PORT_Realloc(oldNscSlotList,
				nscSlotListSize[index]*sizeof(CK_SLOT_ID));
	if (nscSlotList[index] == NULL) {
            nscSlotList[index] = oldNscSlotList;
            nscSlotListSize[index] = oldNscSlotListSize;
            return CKR_HOST_MEMORY;
	}
    }

    if (nscSlotHashTable[index] == NULL) {
	nscSlotHashTable[index] = PL_NewHashTable(64,sftk_HashNumber,
				PL_CompareValues, PL_CompareValues, NULL, 0);
	if (nscSlotHashTable[index] == NULL) {
	    return CKR_HOST_MEMORY;
	}
    }

    entry = PL_HashTableAdd(nscSlotHashTable[index],(void *)slot->slotID,slot);
    if (entry == NULL) {
	return CKR_HOST_MEMORY;
    }
    slot->index = (nscSlotCount[index] & 0x7f) | ((index << 7) & 0x80);
    nscSlotList[index][nscSlotCount[index]++] = slot->slotID;

    return CKR_OK;
}

























CK_RV
SFTK_SlotReInit(SFTKSlot *slot, char *configdir, char *updatedir, 
	char *updateID, sftk_token_parameters *params, int moduleIndex)
{
    PRBool needLogin = !params->noKeyDB;
    CK_RV crv;

    slot->hasTokens = PR_FALSE;
    slot->sessionIDConflict = 0;
    slot->sessionCount = 0;
    slot->rwSessionCount = 0;
    slot->needLogin = PR_FALSE;
    slot->isLoggedIn = PR_FALSE;
    slot->ssoLoggedIn = PR_FALSE;
    slot->DB_loaded = PR_FALSE;
    slot->certDB = NULL;
    slot->keyDB = NULL;
    slot->minimumPinLen = 0;
    slot->readOnly = params->readOnly;
    sftk_setStringName(params->tokdes ? params->tokdes : 
	sftk_getDefTokName(slot->slotID), slot->tokDescription, 
					sizeof(slot->tokDescription),PR_TRUE);
    sftk_setStringName(params->updtokdes ? params->updtokdes : " ", 
				slot->updateTokDescription, 
				sizeof(slot->updateTokDescription),PR_TRUE);

    if ((!params->noCertDB) || (!params->noKeyDB)) {
	SFTKDBHandle * certHandle = NULL;
	SFTKDBHandle *keyHandle = NULL;
	crv = sftk_DBInit(params->configdir ? params->configdir : configdir,
		params->certPrefix, params->keyPrefix, 
		params->updatedir ? params->updatedir : updatedir,
		params->updCertPrefix, params->updKeyPrefix,
		params->updateID  ? params->updateID : updateID, 
		params->readOnly, params->noCertDB, params->noKeyDB,
		params->forceOpen, 
		moduleIndex == NSC_FIPS_MODULE,
		&certHandle, &keyHandle);
	if (crv != CKR_OK) {
	    goto loser;
	}

	slot->certDB = certHandle;
	slot->keyDB = keyHandle;
    }
    if (needLogin) {
	
	slot->needLogin = 
		(PRBool)!sftk_hasNullPassword(slot, slot->keyDB);
	if ((params->minPW >= 0) && (params->minPW <= SFTK_MAX_PIN)) {
	    slot->minimumPinLen = params->minPW;
	}
	if ((slot->minimumPinLen == 0) && (params->pwRequired)) {
	    slot->minimumPinLen = 1;
	}
	if ((moduleIndex == NSC_FIPS_MODULE) &&
		(slot->minimumPinLen < FIPS_MIN_PIN)) {
	    slot->minimumPinLen = FIPS_MIN_PIN;
	}
    }

    slot->present = PR_TRUE;
    return CKR_OK;

loser:
    SFTK_ShutdownSlot(slot);
    return crv;
}




CK_RV
SFTK_SlotInit(char *configdir, char *updatedir, char *updateID,
		sftk_token_parameters *params, int moduleIndex)
{
    unsigned int i;
    CK_SLOT_ID slotID = params->slotID;
    SFTKSlot *slot;
    CK_RV crv = CKR_HOST_MEMORY;

    




    slot = PORT_ZNew(SFTKSlot);

    if (slot == NULL) {
	return CKR_HOST_MEMORY;
    }

    slot->optimizeSpace = params->optimizeSpace;
    if (slot->optimizeSpace) {
	slot->sessObjHashSize = SPACE_SESSION_OBJECT_HASH_SIZE;
	slot->sessHashSize = SPACE_SESSION_HASH_SIZE;
	slot->numSessionLocks = 1;
    } else {
	slot->sessObjHashSize = TIME_SESSION_OBJECT_HASH_SIZE;
	slot->sessHashSize = TIME_SESSION_HASH_SIZE;
	slot->numSessionLocks = slot->sessHashSize/BUCKETS_PER_SESSION_LOCK;
    }
    slot->sessionLockMask = slot->numSessionLocks-1;

    slot->slotLock = PZ_NewLock(nssILockSession);
    if (slot->slotLock == NULL)
	goto mem_loser;
    slot->sessionLock = PORT_ZNewArray(PZLock *, slot->numSessionLocks);
    if (slot->sessionLock == NULL)
	goto mem_loser;
    for (i=0; i < slot->numSessionLocks; i++) {
        slot->sessionLock[i] = PZ_NewLock(nssILockSession);
        if (slot->sessionLock[i] == NULL) 
	    goto mem_loser;
    }
    slot->objectLock = PZ_NewLock(nssILockObject);
    if (slot->objectLock == NULL) 
    	goto mem_loser;
    slot->pwCheckLock = PR_NewLock();
    if (slot->pwCheckLock == NULL) 
    	goto mem_loser;
    slot->head = PORT_ZNewArray(SFTKSession *, slot->sessHashSize);
    if (slot->head == NULL) 
	goto mem_loser;
    slot->sessObjHashTable = PORT_ZNewArray(SFTKObject *, slot->sessObjHashSize);
    if (slot->sessObjHashTable == NULL) 
	goto mem_loser;
    slot->tokObjHashTable = PL_NewHashTable(64,sftk_HashNumber,PL_CompareValues,
					SECITEM_HashCompare, NULL, 0);
    if (slot->tokObjHashTable == NULL) 
	goto mem_loser;

    slot->sessionIDCount = 0;
    slot->sessionObjectHandleCount = minSessionObjectHandle;
    slot->slotID = slotID;
    sftk_setStringName(params->slotdes ? params->slotdes : 
	      sftk_getDefSlotName(slotID), slot->slotDescription, 
					sizeof(slot->slotDescription), PR_TRUE);

    

    crv = SFTK_SlotReInit(slot, configdir, updatedir, updateID,
			   params, moduleIndex);
    if (crv != CKR_OK) {
	goto loser;
    }
    crv = sftk_RegisterSlot(slot, moduleIndex);
    if (crv != CKR_OK) {
	goto loser;
    }
    return CKR_OK;

mem_loser:
    crv = CKR_HOST_MEMORY;
loser:
   SFTK_DestroySlotData(slot);
    return crv;
}


CK_RV sftk_CloseAllSessions(SFTKSlot *slot, PRBool logout)
{
    SFTKSession *session;
    unsigned int i;
    SFTKDBHandle *handle;

    
    





    if (logout) {
	handle = sftk_getKeyDB(slot);
	SKIP_AFTER_FORK(PZ_Lock(slot->slotLock));
	slot->isLoggedIn = PR_FALSE;
	if (slot->needLogin && handle) {
	    sftkdb_ClearPassword(handle);
	}
	SKIP_AFTER_FORK(PZ_Unlock(slot->slotLock));
	if (handle) {
            sftk_freeDB(handle);
	}
    }

    
    



    for (i=0; i < slot->sessHashSize; i++) {
	PZLock *lock = SFTK_SESSION_LOCK(slot,i);
	do {
	    SKIP_AFTER_FORK(PZ_Lock(lock));
	    session = slot->head[i];
	    
	    


	    if (session) {
		slot->head[i] = session->next;
		if (session->next) session->next->prev = NULL;
		session->next = session->prev = NULL;
		SKIP_AFTER_FORK(PZ_Unlock(lock));
		SKIP_AFTER_FORK(PZ_Lock(slot->slotLock));
		--slot->sessionCount;
		SKIP_AFTER_FORK(PZ_Unlock(slot->slotLock));
		if (session->info.flags & CKF_RW_SESSION) {
		    PR_AtomicDecrement(&slot->rwSessionCount);
		}
	    } else {
		SKIP_AFTER_FORK(PZ_Unlock(lock));
	    }
	    if (session) sftk_FreeSession(session);
	} while (session != NULL);
    }
    return CKR_OK;
}











static void
sftk_DBShutdown(SFTKSlot *slot)
{
    SFTKDBHandle *certHandle;
    SFTKDBHandle      *keyHandle;
    SKIP_AFTER_FORK(PZ_Lock(slot->slotLock));
    certHandle = slot->certDB;
    slot->certDB = NULL;
    keyHandle = slot->keyDB;
    slot->keyDB = NULL;
    SKIP_AFTER_FORK(PZ_Unlock(slot->slotLock));
    if (certHandle) {
	sftk_freeDB(certHandle);
    }
    if (keyHandle) {
	sftk_freeDB(keyHandle);
    }
}

CK_RV
SFTK_ShutdownSlot(SFTKSlot *slot)
{
    
    slot->present = PR_FALSE;

    


    if (slot->head) {
	sftk_CloseAllSessions(slot, PR_TRUE);
     }

    



    if (slot->tokObjHashTable) {
	SFTK_ClearTokenKeyHashTable(slot);
    }

    
    PORT_Memset(slot->tokDescription, 0, sizeof(slot->tokDescription));

    
    sftk_DBShutdown(slot);
    return CKR_OK;
}




CK_RV
SFTK_DestroySlotData(SFTKSlot *slot)
{
    unsigned int i;

    SFTK_ShutdownSlot(slot);

    if (slot->tokObjHashTable) {
	PL_HashTableDestroy(slot->tokObjHashTable);
	slot->tokObjHashTable = NULL;
    }

    if (slot->sessObjHashTable) {
	PORT_Free(slot->sessObjHashTable);
	slot->sessObjHashTable = NULL;
    }
    slot->sessObjHashSize = 0;

    if (slot->head) {
	PORT_Free(slot->head);
	slot->head = NULL;
    }
    slot->sessHashSize = 0;

    

    SKIP_AFTER_FORK(PZ_DestroyLock(slot->slotLock));
    slot->slotLock = NULL;
    if (slot->sessionLock) {
	for (i=0; i < slot->numSessionLocks; i++) {
	    if (slot->sessionLock[i]) {
		SKIP_AFTER_FORK(PZ_DestroyLock(slot->sessionLock[i]));
		slot->sessionLock[i] = NULL;
	    }
	}
	PORT_Free(slot->sessionLock);
	slot->sessionLock = NULL;
    }
    if (slot->objectLock) {
	SKIP_AFTER_FORK(PZ_DestroyLock(slot->objectLock));
	slot->objectLock = NULL;
    }
    if (slot->pwCheckLock) {
	SKIP_AFTER_FORK(PR_DestroyLock(slot->pwCheckLock));
	slot->pwCheckLock = NULL;
    }
    PORT_Free(slot);
    return CKR_OK;
}

#ifndef NO_FORK_CHECK

static CK_RV ForkCheck(void)
{
    CHECK_FORK();
    return CKR_OK;
}

#endif




char **
NSC_ModuleDBFunc(unsigned long function,char *parameters, void *args)
{
    char *secmod = NULL;
    char *appName = NULL;
    char *filename = NULL;
#ifdef NSS_DISABLE_DBM
    SDBType dbType = SDB_SQL;
#else
    SDBType dbType = SDB_LEGACY;
#endif
    PRBool rw;
    static char *success="Success";
    char **rvstr = NULL;

#ifndef NO_FORK_CHECK
    if (CKR_OK != ForkCheck()) return NULL;
#endif

    secmod = sftk_getSecmodName(parameters, &dbType, &appName,&filename, &rw);

    switch (function) {
    case SECMOD_MODULE_DB_FUNCTION_FIND:
	rvstr = sftkdb_ReadSecmodDB(dbType,appName,filename,secmod,(char *)parameters,rw);
	break;
    case SECMOD_MODULE_DB_FUNCTION_ADD:
	rvstr = (sftkdb_AddSecmodDB(dbType,appName,filename,secmod,(char *)args,rw) 
				== SECSuccess) ? &success: NULL;
	break;
    case SECMOD_MODULE_DB_FUNCTION_DEL:
	rvstr = (sftkdb_DeleteSecmodDB(dbType,appName,filename,secmod,(char *)args,rw)
				 == SECSuccess) ? &success: NULL;
	break;
    case SECMOD_MODULE_DB_FUNCTION_RELEASE:
	rvstr = (sftkdb_ReleaseSecmodDBData(dbType, appName,filename,secmod,
			(char **)args,rw) == SECSuccess) ? &success: NULL;
	break;
    }
    if (secmod) PR_smprintf_free(secmod);
    if (appName) PORT_Free(appName);
    if (filename) PORT_Free(filename);
    return rvstr;
}

static void nscFreeAllSlots(int moduleIndex)
{
    
    SFTKSlot *slot = NULL;
    CK_SLOT_ID slotID;
    int i;

    if (nscSlotList[moduleIndex]) {
	CK_ULONG tmpSlotCount = nscSlotCount[moduleIndex];
	CK_SLOT_ID_PTR tmpSlotList = nscSlotList[moduleIndex];
	PLHashTable *tmpSlotHashTable = nscSlotHashTable[moduleIndex];

	
	for (i=0; i < (int) tmpSlotCount; i++) {
	    slotID = tmpSlotList[i];
	    (void) NSC_CloseAllSessions(slotID);
	}

	
	nscSlotList[moduleIndex] = NULL;
	nscSlotCount[moduleIndex] = 0;
	nscSlotHashTable[moduleIndex] = NULL;
	nscSlotListSize[moduleIndex] = 0;

	for (i=0; i < (int) tmpSlotCount; i++) {
	    slotID = tmpSlotList[i];
	    slot = (SFTKSlot *)
			PL_HashTableLookup(tmpSlotHashTable, (void *)slotID);
	    PORT_Assert(slot);
	    if (!slot) continue;
	    SFTK_DestroySlotData(slot);
	    PL_HashTableRemove(tmpSlotHashTable, (void *)slotID);
	}
	PORT_Free(tmpSlotList);
	PL_HashTableDestroy(tmpSlotHashTable);
    }
}

static void
sftk_closePeer(PRBool isFIPS)
{
    CK_SLOT_ID slotID = isFIPS ? PRIVATE_KEY_SLOT_ID: FIPS_SLOT_ID;
    SFTKSlot *slot;
    int moduleIndex = isFIPS? NSC_NON_FIPS_MODULE : NSC_FIPS_MODULE;
    PLHashTable *tmpSlotHashTable = nscSlotHashTable[moduleIndex];

    slot = (SFTKSlot *) PL_HashTableLookup(tmpSlotHashTable, (void *)slotID);
    if (slot == NULL) {
	return;
    }
    sftk_DBShutdown(slot);
    return;
}


CK_RV nsc_CommonInitialize(CK_VOID_PTR pReserved, PRBool isFIPS)
{
    CK_RV crv = CKR_OK;
    SECStatus rv;
    CK_C_INITIALIZE_ARGS *init_args = (CK_C_INITIALIZE_ARGS *) pReserved;
    int i;
    int moduleIndex = isFIPS? NSC_FIPS_MODULE : NSC_NON_FIPS_MODULE;

    if (isFIPS) {
	loginWaitTime = PR_SecondsToInterval(1);
    }

    ENABLE_FORK_CHECK();

    rv = SECOID_Init();
    if (rv != SECSuccess) {
	crv = CKR_DEVICE_ERROR;
	return crv;
    }

    rv = RNG_RNGInit();         
    if (rv != SECSuccess) {
	crv = CKR_DEVICE_ERROR;
	return crv;
    }
    rv = BL_Init();             
    if (rv != SECSuccess) {
	crv = CKR_DEVICE_ERROR;
	return crv;
    }

    





   
    if (init_args && (!(init_args->flags & CKF_OS_LOCKING_OK))) {
        if (init_args->CreateMutex && init_args->DestroyMutex &&
            init_args->LockMutex && init_args->UnlockMutex) {
            


            crv = CKR_CANT_LOCK;
            return crv;
        }
        if (init_args->CreateMutex || init_args->DestroyMutex ||
            init_args->LockMutex || init_args->UnlockMutex) {
            


            crv = CKR_ARGUMENTS_BAD;
            return crv;
        }
    }
    crv = CKR_ARGUMENTS_BAD;
    if ((init_args && init_args->LibraryParameters)) {
	sftk_parameters paramStrings;
       
	crv = sftk_parseParameters
		((char *)init_args->LibraryParameters, &paramStrings, isFIPS);
	if (crv != CKR_OK) {
	    return crv;
	}
	crv = sftk_configure(paramStrings.man, paramStrings.libdes);
        if (crv != CKR_OK) {
	    goto loser;
	}

	

	if ((isFIPS && nsc_init) || (!isFIPS && nsf_init)) {
	    sftk_closePeer(isFIPS);
	    if (sftk_audit_enabled) {
		if (isFIPS && nsc_init) {
		    sftk_LogAuditMessage(NSS_AUDIT_INFO, NSS_AUDIT_FIPS_STATE, 
				"enabled FIPS mode");
		} else {
		    sftk_LogAuditMessage(NSS_AUDIT_INFO, NSS_AUDIT_FIPS_STATE, 
				"disabled FIPS mode");
		}
	    }
	}

	for (i=0; i < paramStrings.token_count; i++) {
	    crv = SFTK_SlotInit(paramStrings.configdir, 
			paramStrings.updatedir, paramStrings.updateID,
			&paramStrings.tokens[i], moduleIndex);
	    if (crv != CKR_OK) {
                nscFreeAllSlots(moduleIndex);
                break;
            }
	}
loser:
	sftk_freeParams(&paramStrings);
    }
    if (CKR_OK == crv) {
        sftk_InitFreeLists();
    }

#ifndef NO_FORK_CHECK
    if (CKR_OK == crv) {
#if defined(CHECK_FORK_MIXED)
        



        char buf[200];
        int major = 0, minor = 0;

        long rv = sysinfo(SI_RELEASE, buf, sizeof(buf));
        if (rv > 0 && rv < sizeof(buf)) {
            if (2 == sscanf(buf, "%d.%d", &major, &minor)) {
                
                if (major >5 || (5 == major && minor >= 10)) {
                    
                    usePthread_atfork = PR_TRUE;
                }
            }
        }
        if (usePthread_atfork) {
            pthread_atfork(NULL, NULL, ForkedChild);
        } else {
            myPid = getpid();
        }

#elif defined(CHECK_FORK_PTHREAD)
        pthread_atfork(NULL, NULL, ForkedChild);
#elif defined(CHECK_FORK_GETPID)
        myPid = getpid();
#else
#error Incorrect fork check method.
#endif
    }
#endif
    return crv;
}

CK_RV NSC_Initialize(CK_VOID_PTR pReserved)
{
    CK_RV crv;
    
    sftk_ForkReset(pReserved, &crv);

    if (nsc_init) {
	return CKR_CRYPTOKI_ALREADY_INITIALIZED;
    }
    crv = nsc_CommonInitialize(pReserved,PR_FALSE);
    nsc_init = (PRBool) (crv == CKR_OK);
    return crv;
}




CK_RV nsc_CommonFinalize (CK_VOID_PTR pReserved, PRBool isFIPS)
{
    
    BL_SetForkState(parentForkedAfterC_Initialize);
    UTIL_SetForkState(parentForkedAfterC_Initialize);

    nscFreeAllSlots(isFIPS ? NSC_FIPS_MODULE : NSC_NON_FIPS_MODULE);

    
    if (isFIPS && nsc_init) {
	return CKR_OK;
    }
    if (!isFIPS && nsf_init) {
	return CKR_OK;
    }

    sftk_CleanupFreeLists();
    sftkdb_Shutdown();

    
    RNG_RNGShutdown();

    
    BL_Cleanup();
    
    

    BL_SetForkState(PR_FALSE);
    
    

    BL_Unload();

    
    SECOID_Shutdown();

    
    UTIL_SetForkState(PR_FALSE);

    nsc_init = PR_FALSE;

#ifdef CHECK_FORK_MIXED
    if (!usePthread_atfork) {
        myPid = 0; 

    } else {
        forked = PR_FALSE; 
    }
#elif defined(CHECK_FORK_GETPID)
    myPid = 0; 
#elif defined (CHECK_FORK_PTHREAD)
    forked = PR_FALSE; 
#endif
    return CKR_OK;
}



PRBool sftk_ForkReset(CK_VOID_PTR pReserved, CK_RV* crv)
{
#ifndef NO_FORK_CHECK
    if (PARENT_FORKED()) {
        parentForkedAfterC_Initialize = PR_TRUE;
        if (nsc_init) {
            
            *crv = nsc_CommonFinalize(pReserved, PR_FALSE);
            PORT_Assert(CKR_OK == *crv);
            nsc_init = (PRBool) !(*crv == CKR_OK);
        }
        if (nsf_init) {
            
            *crv = nsc_CommonFinalize(pReserved, PR_TRUE);
            PORT_Assert(CKR_OK == *crv);
            nsf_init = (PRBool) !(*crv == CKR_OK);
        }
        parentForkedAfterC_Initialize = PR_FALSE;
        return PR_TRUE;
    }
#endif
    return PR_FALSE;
}



CK_RV NSC_Finalize (CK_VOID_PTR pReserved)
{
    CK_RV crv;

    
    if (sftk_ForkReset(pReserved, &crv)) {
        return crv;
    }

    if (!nsc_init) {
        return CKR_OK;
    }

    crv = nsc_CommonFinalize (pReserved, PR_FALSE);

    nsc_init = (PRBool) !(crv == CKR_OK);

    return crv;
}

extern const char __nss_softokn_rcsid[];
extern const char __nss_softokn_sccsid[];


CK_RV  NSC_GetInfo(CK_INFO_PTR pInfo)
{
    volatile char c; 

    CHECK_FORK();
    
    c = __nss_softokn_rcsid[0] + __nss_softokn_sccsid[0]; 
    pInfo->cryptokiVersion.major = 2;
    pInfo->cryptokiVersion.minor = 20;
    PORT_Memcpy(pInfo->manufacturerID,manufacturerID,32);
    pInfo->libraryVersion.major = SOFTOKEN_VMAJOR;
    pInfo->libraryVersion.minor = SOFTOKEN_VMINOR;
    PORT_Memcpy(pInfo->libraryDescription,libraryDescription,32);
    pInfo->flags = 0;
    return CKR_OK;
}



CK_RV nsc_CommonGetSlotList(CK_BBOOL tokenPresent, 
	CK_SLOT_ID_PTR	pSlotList, CK_ULONG_PTR pulCount, int moduleIndex)
{
    *pulCount = nscSlotCount[moduleIndex];
    if (pSlotList != NULL) {
	PORT_Memcpy(pSlotList,nscSlotList[moduleIndex],
				nscSlotCount[moduleIndex]*sizeof(CK_SLOT_ID));
    }
    return CKR_OK;
}


CK_RV NSC_GetSlotList(CK_BBOOL tokenPresent,
	 		CK_SLOT_ID_PTR	pSlotList, CK_ULONG_PTR pulCount)
{
    CHECK_FORK();
    return nsc_CommonGetSlotList(tokenPresent, pSlotList, pulCount, 
							NSC_NON_FIPS_MODULE);
}
	

CK_RV NSC_GetSlotInfo(CK_SLOT_ID slotID, CK_SLOT_INFO_PTR pInfo)
{
    SFTKSlot *slot = sftk_SlotFromID(slotID, PR_TRUE);

    CHECK_FORK();

    if (slot == NULL) return CKR_SLOT_ID_INVALID;

    pInfo->firmwareVersion.major = 0;
    pInfo->firmwareVersion.minor = 0;

    PORT_Memcpy(pInfo->manufacturerID,manufacturerID,
		sizeof(pInfo->manufacturerID));
    PORT_Memcpy(pInfo->slotDescription,slot->slotDescription,
		sizeof(pInfo->slotDescription));
    pInfo->flags = (slot->present) ? CKF_TOKEN_PRESENT : 0;

    
    if (slotID >= SFTK_MIN_USER_SLOT_ID) {
	pInfo->flags |= CKF_REMOVABLE_DEVICE;
    } else {
	


	SFTKDBHandle *handle = sftk_getKeyDB(slot);
	if (handle) { 
	    if (sftkdb_InUpdateMerge(handle)) {
		pInfo->flags |= CKF_REMOVABLE_DEVICE;
	    }
            sftk_freeDB(handle);
	}
    }

    
    
    pInfo->hardwareVersion.major = SOFTOKEN_VMAJOR;
    pInfo->hardwareVersion.minor = SOFTOKEN_VMINOR;
    return CKR_OK;
}





static PRBool
sftk_checkNeedLogin(SFTKSlot *slot, SFTKDBHandle *keyHandle)
{
    if (sftkdb_PWCached(keyHandle) == SECSuccess) {
	return slot->needLogin;
    }
    slot->needLogin = (PRBool)!sftk_hasNullPassword(slot, keyHandle);
    return (slot->needLogin);
}

static PRBool
sftk_isBlank(const char *s, int len)
{
    int i;
    for (i=0; i < len; i++) {
	if (s[i] != ' ') {
	    return PR_FALSE;
	}
    }
    return PR_TRUE;
}



CK_RV NSC_GetTokenInfo(CK_SLOT_ID slotID,CK_TOKEN_INFO_PTR pInfo)
{
    SFTKSlot *slot;
    SFTKDBHandle *handle;

    CHECK_FORK();
    
    if (!nsc_init && !nsf_init) return CKR_CRYPTOKI_NOT_INITIALIZED;
    slot = sftk_SlotFromID(slotID, PR_FALSE);
    if (slot == NULL) return CKR_SLOT_ID_INVALID;

    PORT_Memcpy(pInfo->manufacturerID,manufacturerID,32);
    PORT_Memcpy(pInfo->model,"NSS 3           ",16);
    PORT_Memcpy(pInfo->serialNumber,"0000000000000000",16);
    PORT_Memcpy(pInfo->utcTime,"0000000000000000",16);
    pInfo->ulMaxSessionCount = 0; 
    pInfo->ulSessionCount = slot->sessionCount;
    pInfo->ulMaxRwSessionCount = 0; 
    pInfo->ulRwSessionCount = slot->rwSessionCount;
    pInfo->firmwareVersion.major = 0;
    pInfo->firmwareVersion.minor = 0;
    PORT_Memcpy(pInfo->label,slot->tokDescription,sizeof(pInfo->label));
    handle = sftk_getKeyDB(slot);
    pInfo->flags = CKF_RNG | CKF_DUAL_CRYPTO_OPERATIONS;
    if (handle == NULL) {
	pInfo->flags |= CKF_WRITE_PROTECTED;
	pInfo->ulMaxPinLen = 0;
	pInfo->ulMinPinLen = 0;
	pInfo->ulTotalPublicMemory = 0;
	pInfo->ulFreePublicMemory = 0;
	pInfo->ulTotalPrivateMemory = 0;
	pInfo->ulFreePrivateMemory = 0;
	pInfo->hardwareVersion.major = 4;
	pInfo->hardwareVersion.minor = 0;
    } else {
	








	if (sftkdb_HasPasswordSet(handle) == SECFailure) {
	    pInfo->flags |= CKF_LOGIN_REQUIRED;
	} else if (!sftk_checkNeedLogin(slot,handle)) {
	    pInfo->flags |= CKF_USER_PIN_INITIALIZED;
	} else {
	    pInfo->flags |= CKF_LOGIN_REQUIRED | CKF_USER_PIN_INITIALIZED;
	




	if (sftkdb_NeedUpdateDBPassword(handle)) {
	    

	    if (!sftk_isBlank(slot->updateTokDescription,
						sizeof(pInfo->label))) {
		PORT_Memcpy(pInfo->label,slot->updateTokDescription,
				sizeof(pInfo->label));
	    } else {
		
		const char *updateID = sftkdb_GetUpdateID(handle);
		if (updateID) {
		    sftk_setStringName(updateID, (char *)pInfo->label,
				 sizeof(pInfo->label), PR_FALSE);
		}
	    }
	}
	}
	pInfo->ulMaxPinLen = SFTK_MAX_PIN;
	pInfo->ulMinPinLen = (CK_ULONG)slot->minimumPinLen;
	pInfo->ulTotalPublicMemory = 1;
	pInfo->ulFreePublicMemory = 1;
	pInfo->ulTotalPrivateMemory = 1;
	pInfo->ulFreePrivateMemory = 1;
#ifdef SHDB_FIXME
	pInfo->hardwareVersion.major = CERT_DB_FILE_VERSION;
	pInfo->hardwareVersion.minor = handle->version;
#else
	pInfo->hardwareVersion.major = 0;
	pInfo->hardwareVersion.minor = 0;
#endif
        sftk_freeDB(handle);
    }
    







    if (!(pInfo->flags & CKF_LOGIN_REQUIRED) ||
	(pInfo->flags & CKF_USER_PIN_INITIALIZED)) {
	pInfo->flags |= CKF_TOKEN_INITIALIZED;
    }
    return CKR_OK;
}



CK_RV NSC_GetMechanismList(CK_SLOT_ID slotID,
	CK_MECHANISM_TYPE_PTR pMechanismList, CK_ULONG_PTR pulCount)
{
    CK_ULONG i;

    CHECK_FORK();

    switch (slotID) {
    
    case NETSCAPE_SLOT_ID:
	*pulCount = mechanismCount;
	if (pMechanismList != NULL) {
	    for (i=0; i < mechanismCount; i++) {
		pMechanismList[i] = mechanisms[i].type;
	    }
	}
	break;
     default:
	*pulCount = 0;
	for (i=0; i < mechanismCount; i++) {
	    if (mechanisms[i].privkey) {
		(*pulCount)++;
		if (pMechanismList != NULL) {
		    *pMechanismList++ = mechanisms[i].type;
		}
	    }
	}
	break;
    }
    return CKR_OK;
}




CK_RV NSC_GetMechanismInfo(CK_SLOT_ID slotID, CK_MECHANISM_TYPE type,
    					CK_MECHANISM_INFO_PTR pInfo)
{
    PRBool isPrivateKey;
    CK_ULONG i;

    CHECK_FORK();
    
    switch (slotID) {
    case NETSCAPE_SLOT_ID:
	isPrivateKey = PR_FALSE;
	break;
    default:
	isPrivateKey = PR_TRUE;
	break;
    }
    for (i=0; i < mechanismCount; i++) {
        if (type == mechanisms[i].type) {
	    if (isPrivateKey && !mechanisms[i].privkey) {
    		return CKR_MECHANISM_INVALID;
	    }
	    PORT_Memcpy(pInfo,&mechanisms[i].info, sizeof(CK_MECHANISM_INFO));
	    return CKR_OK;
	}
    }
    return CKR_MECHANISM_INVALID;
}

CK_RV sftk_MechAllowsOperation(CK_MECHANISM_TYPE type, CK_ATTRIBUTE_TYPE op)
{
    CK_ULONG i;
    CK_FLAGS flags;

    switch (op) {
    case CKA_ENCRYPT:         flags = CKF_ENCRYPT;         break;
    case CKA_DECRYPT:         flags = CKF_DECRYPT;         break;
    case CKA_WRAP:            flags = CKF_WRAP;            break;
    case CKA_UNWRAP:          flags = CKF_UNWRAP;          break;
    case CKA_SIGN:            flags = CKF_SIGN;            break;
    case CKA_SIGN_RECOVER:    flags = CKF_SIGN_RECOVER;    break;
    case CKA_VERIFY:          flags = CKF_VERIFY;          break;
    case CKA_VERIFY_RECOVER:  flags = CKF_VERIFY_RECOVER;  break;
    case CKA_DERIVE:          flags = CKF_DERIVE;          break;
    default:
    	return CKR_ARGUMENTS_BAD;
    }
    for (i=0; i < mechanismCount; i++) {
        if (type == mechanisms[i].type) {
	    return (flags & mechanisms[i].info.flags) ? CKR_OK 
	                                              : CKR_MECHANISM_INVALID;
	}
    }
    return CKR_MECHANISM_INVALID;
}


CK_RV NSC_InitToken(CK_SLOT_ID slotID,CK_CHAR_PTR pPin,
 				CK_ULONG ulPinLen,CK_CHAR_PTR pLabel) {
    SFTKSlot *slot = sftk_SlotFromID(slotID, PR_FALSE);
    SFTKDBHandle *handle;
    SFTKDBHandle *certHandle;
    SECStatus rv;
    unsigned int i;
    SFTKObject *object;

    CHECK_FORK();

    if (slot == NULL) return CKR_SLOT_ID_INVALID;

    


    if (slotID == NETSCAPE_SLOT_ID) {
	return CKR_TOKEN_WRITE_PROTECTED;
    }

    

    PZ_Lock(slot->objectLock);
    for (i=0; i < slot->sessObjHashSize; i++) {
	do {
	    object = slot->sessObjHashTable[i];
	    
	    


	    if (object) {
		slot->sessObjHashTable[i] = object->next;

		if (object->next) object->next->prev = NULL;
		object->next = object->prev = NULL;
	    }
	    if (object) sftk_FreeObject(object);
	} while (object != NULL);
    }
    slot->DB_loaded = PR_FALSE;
    PZ_Unlock(slot->objectLock);

    
    handle = sftk_getKeyDB(slot);
    if (handle == NULL) {
	return CKR_TOKEN_WRITE_PROTECTED;
    }

    rv = sftkdb_ResetKeyDB(handle);
    sftk_freeDB(handle);
    if (rv != SECSuccess) {
	return CKR_DEVICE_ERROR;
    }

    
    certHandle = sftk_getCertDB(slot);
    if (certHandle == NULL) return CKR_OK;

    sftk_freeDB(certHandle);

    return CKR_OK; 
}



CK_RV NSC_InitPIN(CK_SESSION_HANDLE hSession,
    					CK_CHAR_PTR pPin, CK_ULONG ulPinLen)
{
    SFTKSession *sp = NULL;
    SFTKSlot *slot;
    SFTKDBHandle *handle = NULL;
    char newPinStr[SFTK_MAX_PIN+1];
    SECStatus rv;
    CK_RV crv = CKR_SESSION_HANDLE_INVALID;
    PRBool tokenRemoved = PR_FALSE;

    CHECK_FORK();
    
    sp = sftk_SessionFromHandle(hSession);
    if (sp == NULL) {
	goto loser;
    }

    slot = sftk_SlotFromSession(sp);
    if (slot == NULL) {
	goto loser;
    }

    handle = sftk_getKeyDB(slot);
    if (handle == NULL) {
	crv = CKR_PIN_LEN_RANGE;
	goto loser;
    }


    if (sp->info.state != CKS_RW_SO_FUNCTIONS) {
	crv = CKR_USER_NOT_LOGGED_IN;
	goto loser;
    }

    sftk_FreeSession(sp);
    sp = NULL;

    
    if (ulPinLen > SFTK_MAX_PIN) {
	crv = CKR_PIN_LEN_RANGE;
	goto loser;
    }
    if (ulPinLen < (CK_ULONG)slot->minimumPinLen) {
	crv = CKR_PIN_LEN_RANGE;
	goto loser;
    }

    if (sftkdb_HasPasswordSet(handle) != SECFailure) {
	crv = CKR_DEVICE_ERROR;
	goto loser;
    }

    
    PORT_Memcpy(newPinStr, pPin, ulPinLen);
    newPinStr[ulPinLen] = 0; 

    

    
    rv = sftkdb_ChangePassword(handle, NULL, newPinStr, &tokenRemoved);
    if (tokenRemoved) {
	sftk_CloseAllSessions(slot, PR_FALSE);
    }
    sftk_freeDB(handle);
    handle = NULL;

    
    if (rv == SECSuccess) {
	if (ulPinLen == 0) slot->needLogin = PR_FALSE;
	return CKR_OK;
    }
    crv = CKR_PIN_INCORRECT;

loser:
    if (sp) {
	sftk_FreeSession(sp);
    }
    if (handle) {
	sftk_freeDB(handle);
    }
    return crv;
}




CK_RV NSC_SetPIN(CK_SESSION_HANDLE hSession, CK_CHAR_PTR pOldPin,
    CK_ULONG ulOldLen, CK_CHAR_PTR pNewPin, CK_ULONG ulNewLen)
{
    SFTKSession *sp = NULL;
    SFTKSlot *slot;
    SFTKDBHandle *handle = NULL;
    char newPinStr[SFTK_MAX_PIN+1],oldPinStr[SFTK_MAX_PIN+1];
    SECStatus rv;
    CK_RV crv = CKR_SESSION_HANDLE_INVALID;
    PRBool tokenRemoved = PR_FALSE;

    CHECK_FORK();
    
    sp = sftk_SessionFromHandle(hSession);
    if (sp == NULL) {
	goto loser;
    }

    slot = sftk_SlotFromSession(sp);
    if (!slot) {
	goto loser;
    }

    handle = sftk_getKeyDB(slot);
    if (handle == NULL) {
	sftk_FreeSession(sp);
	return CKR_PIN_LEN_RANGE; 
    }

    if (slot->needLogin && sp->info.state != CKS_RW_USER_FUNCTIONS) {
	crv = CKR_USER_NOT_LOGGED_IN;
	goto loser;
    }

    sftk_FreeSession(sp);
    sp = NULL;

    
    if ((ulNewLen > SFTK_MAX_PIN) || (ulOldLen > SFTK_MAX_PIN)) {
	crv = CKR_PIN_LEN_RANGE;
	goto loser;
    }
    if (ulNewLen < (CK_ULONG)slot->minimumPinLen) {
	crv = CKR_PIN_LEN_RANGE;
	goto loser;
    }


    
    PORT_Memcpy(newPinStr,pNewPin,ulNewLen);
    newPinStr[ulNewLen] = 0; 
    PORT_Memcpy(oldPinStr,pOldPin,ulOldLen);
    oldPinStr[ulOldLen] = 0; 

    
    PR_Lock(slot->pwCheckLock);
    rv = sftkdb_ChangePassword(handle, oldPinStr, newPinStr, &tokenRemoved);
    if (tokenRemoved) {
	sftk_CloseAllSessions(slot, PR_FALSE);
    }
    if ((rv != SECSuccess) && (slot->slotID == FIPS_SLOT_ID)) {
	PR_Sleep(loginWaitTime);
    }
    PR_Unlock(slot->pwCheckLock);

    
    if (rv == SECSuccess) {
	slot->needLogin = (PRBool)(ulNewLen != 0);
        
        if (ulNewLen == 0) {
            PRBool tokenRemoved = PR_FALSE;
            PZ_Lock(slot->slotLock);
            slot->isLoggedIn = PR_FALSE;
            slot->ssoLoggedIn = PR_FALSE;
            PZ_Unlock(slot->slotLock);

            rv = sftkdb_CheckPassword(handle, "", &tokenRemoved);
            if (tokenRemoved) {
                sftk_CloseAllSessions(slot, PR_FALSE);
            }
        }
        sftk_update_all_states(slot);
        sftk_freeDB(handle);
	return CKR_OK;
    }
    crv = CKR_PIN_INCORRECT;
loser:
    if (sp) {
	sftk_FreeSession(sp);
    }
    if (handle) {
	sftk_freeDB(handle);
    }
    return crv;
}


CK_RV NSC_OpenSession(CK_SLOT_ID slotID, CK_FLAGS flags,
   CK_VOID_PTR pApplication,CK_NOTIFY Notify,CK_SESSION_HANDLE_PTR phSession)
{
    SFTKSlot *slot;
    CK_SESSION_HANDLE sessionID;
    SFTKSession *session;
    SFTKSession *sameID;

    CHECK_FORK();
    
    slot = sftk_SlotFromID(slotID, PR_FALSE);
    if (slot == NULL) return CKR_SLOT_ID_INVALID;

    
    session = sftk_NewSession(slotID, Notify, pApplication,
						 flags | CKF_SERIAL_SESSION);
    if (session == NULL) return CKR_HOST_MEMORY;

    if (slot->readOnly && (flags & CKF_RW_SESSION)) {
	
	session->info.flags &= ~CKF_RW_SESSION;
    }
    PZ_Lock(slot->slotLock);
    ++slot->sessionCount;
    PZ_Unlock(slot->slotLock);
    if (session->info.flags & CKF_RW_SESSION) {
	PR_AtomicIncrement(&slot->rwSessionCount);
    }

    do {
        PZLock *lock;
        do {
            sessionID = (PR_AtomicIncrement(&slot->sessionIDCount) & 0xffffff)
                        | (slot->index << 24);
        } while (sessionID == CK_INVALID_HANDLE);
        lock = SFTK_SESSION_LOCK(slot,sessionID);
        PZ_Lock(lock);
        sftkqueue_find(sameID, sessionID, slot->head, slot->sessHashSize);
        if (sameID == NULL) {
            session->handle = sessionID;
            sftk_update_state(slot, session);
            sftkqueue_add(session, sessionID, slot->head,slot->sessHashSize);
        } else {
            slot->sessionIDConflict++;  
        }
        PZ_Unlock(lock);
    } while (sameID != NULL);

    *phSession = sessionID;
    return CKR_OK;
}



CK_RV NSC_CloseSession(CK_SESSION_HANDLE hSession)
{
    SFTKSlot *slot;
    SFTKSession *session;
    PRBool sessionFound;
    PZLock *lock;

    CHECK_FORK();

    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    slot = sftk_SlotFromSession(session);
    sessionFound = PR_FALSE;

    
    lock = SFTK_SESSION_LOCK(slot,hSession);
    PZ_Lock(lock);
    if (sftkqueue_is_queued(session,hSession,slot->head,slot->sessHashSize)) {
	sessionFound = PR_TRUE;
	sftkqueue_delete(session,hSession,slot->head,slot->sessHashSize);
	session->refCount--; 
	PORT_Assert(session->refCount > 0);
    }
    PZ_Unlock(lock);

    if (sessionFound) {
	SFTKDBHandle *handle;
	handle = sftk_getKeyDB(slot);
	PZ_Lock(slot->slotLock);
	if (--slot->sessionCount == 0) {
	    slot->isLoggedIn = PR_FALSE;
	    if (slot->needLogin && handle) {
		sftkdb_ClearPassword(handle);
	    }
	}
	PZ_Unlock(slot->slotLock);
	if (handle) {
	    sftk_freeDB(handle);
	}
	if (session->info.flags & CKF_RW_SESSION) {
	    PR_AtomicDecrement(&slot->rwSessionCount);
	}
    }

    sftk_FreeSession(session);
    return CKR_OK;
}



CK_RV NSC_CloseAllSessions (CK_SLOT_ID slotID)
{
    SFTKSlot *slot;

#ifndef NO_CHECK_FORK
    
    if (!parentForkedAfterC_Initialize) {
        CHECK_FORK();
    }
#endif

    slot = sftk_SlotFromID(slotID, PR_FALSE);
    if (slot == NULL) return CKR_SLOT_ID_INVALID;

    return sftk_CloseAllSessions(slot, PR_TRUE);
}




CK_RV NSC_GetSessionInfo(CK_SESSION_HANDLE hSession,
    						CK_SESSION_INFO_PTR pInfo)
{
    SFTKSession *session;

    CHECK_FORK();

    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;

    PORT_Memcpy(pInfo,&session->info,sizeof(CK_SESSION_INFO));
    sftk_FreeSession(session);
    return CKR_OK;
}


CK_RV NSC_Login(CK_SESSION_HANDLE hSession, CK_USER_TYPE userType,
				    CK_CHAR_PTR pPin, CK_ULONG ulPinLen)
{
    SFTKSlot *slot;
    SFTKSession *session;
    SFTKDBHandle *handle;
    CK_FLAGS sessionFlags;
    SECStatus rv;
    CK_RV crv;
    char pinStr[SFTK_MAX_PIN+1];
    PRBool tokenRemoved = PR_FALSE;

    CHECK_FORK();

    
    slot = sftk_SlotFromSessionHandle(hSession);
    if (slot == NULL) {
	return CKR_SESSION_HANDLE_INVALID;
    }

    
    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) {
	return CKR_SESSION_HANDLE_INVALID;
    }
    sessionFlags = session->info.flags;
    sftk_FreeSession(session);
    session = NULL;

    
    if (slot->slotID == NETSCAPE_SLOT_ID) {
	 return CKR_USER_TYPE_INVALID;
    }

    if (slot->isLoggedIn) return CKR_USER_ALREADY_LOGGED_IN;
    if (!slot->needLogin) {
        return ulPinLen ? CKR_PIN_INCORRECT : CKR_OK;
    }
    slot->ssoLoggedIn = PR_FALSE;

    if (ulPinLen > SFTK_MAX_PIN) return CKR_PIN_LEN_RANGE;

    
    PORT_Memcpy(pinStr,pPin,ulPinLen);
    pinStr[ulPinLen] = 0; 

    handle = sftk_getKeyDB(slot);
    if (handle == NULL) {
	 return CKR_USER_TYPE_INVALID;
    }

    




    rv = sftkdb_HasPasswordSet(handle);
    if (rv == SECFailure) {
	

	if (((userType == CKU_SO) && (sessionFlags & CKF_RW_SESSION))
	    
					|| (slot->slotID == FIPS_SLOT_ID)) {
	    
	    if (ulPinLen == 0) {
		sftkdb_ClearPassword(handle);
    		PZ_Lock(slot->slotLock);
		slot->isLoggedIn = PR_TRUE;
		slot->ssoLoggedIn = (PRBool)(userType == CKU_SO);
		PZ_Unlock(slot->slotLock);
		sftk_update_all_states(slot);
		crv = CKR_OK;
		goto done;
	    }
	    crv = CKR_PIN_INCORRECT;
	    goto done;
	} 
	crv = CKR_USER_TYPE_INVALID;
	goto done;
    } 

    
    if (userType != CKU_USER) { 
	crv = CKR_USER_TYPE_INVALID; 
	goto done;
    }


    
    PR_Lock(slot->pwCheckLock);
    rv = sftkdb_CheckPassword(handle,pinStr, &tokenRemoved);
    if (tokenRemoved) {
	sftk_CloseAllSessions(slot, PR_FALSE);
    }
    if ((rv != SECSuccess) && (slot->slotID == FIPS_SLOT_ID)) {
	PR_Sleep(loginWaitTime);
    }
    PR_Unlock(slot->pwCheckLock);
    if (rv == SECSuccess) {
	PZ_Lock(slot->slotLock);
	

	slot->isLoggedIn = sftkdb_PWCached(handle) == SECSuccess ?
		PR_TRUE : PR_FALSE;
	PZ_Unlock(slot->slotLock);

 	sftk_freeDB(handle);
	handle = NULL;

	
	sftk_update_all_states(slot);
	return CKR_OK;
    }

    crv = CKR_PIN_INCORRECT;
done:
    if (handle) {
	sftk_freeDB(handle);
    }
    return crv;
}


CK_RV NSC_Logout(CK_SESSION_HANDLE hSession)
{
    SFTKSlot *slot = sftk_SlotFromSessionHandle(hSession);
    SFTKSession *session;
    SFTKDBHandle *handle;

    CHECK_FORK();

    if (slot == NULL) {
	return CKR_SESSION_HANDLE_INVALID;
    }
    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    sftk_FreeSession(session);
    session = NULL;

    if (!slot->isLoggedIn) return CKR_USER_NOT_LOGGED_IN;

    handle = sftk_getKeyDB(slot);
    PZ_Lock(slot->slotLock);
    slot->isLoggedIn = PR_FALSE;
    slot->ssoLoggedIn = PR_FALSE;
    if (slot->needLogin && handle) {
	sftkdb_ClearPassword(handle);
    }
    PZ_Unlock(slot->slotLock);
    if (handle) {
	sftk_freeDB(handle);
    }

    sftk_update_all_states(slot);
    return CKR_OK;
}








static CK_RV sftk_CreateNewSlot(SFTKSlot *slot, CK_OBJECT_CLASS class,
                                SFTKObject *object)
{
    CK_SLOT_ID idMin, idMax;
    PRBool isFIPS = PR_FALSE;
    unsigned long moduleIndex;
    SFTKAttribute *attribute;
    sftk_parameters paramStrings;
    char *paramString;
    CK_SLOT_ID slotID = 0;
    SFTKSlot *newSlot = NULL;
    CK_RV crv = CKR_OK;

    
    if (slot->slotID == NETSCAPE_SLOT_ID) {
	idMin = SFTK_MIN_USER_SLOT_ID;
	idMax = SFTK_MAX_USER_SLOT_ID;
	moduleIndex = NSC_NON_FIPS_MODULE;
	isFIPS = PR_FALSE;
    } else if (slot->slotID == FIPS_SLOT_ID) {
	idMin = SFTK_MIN_FIPS_USER_SLOT_ID;
	idMax = SFTK_MAX_FIPS_USER_SLOT_ID;
	moduleIndex = NSC_FIPS_MODULE;
	isFIPS = PR_TRUE;
    } else {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }
    attribute = sftk_FindAttribute(object,CKA_NETSCAPE_MODULE_SPEC);
    if (attribute == NULL) {
	return CKR_TEMPLATE_INCOMPLETE;
    }
    paramString = (char *)attribute->attrib.pValue;
    crv = sftk_parseParameters(paramString, &paramStrings, isFIPS);
    if (crv != CKR_OK) {
	goto loser;
    }

    
    if (paramStrings.token_count != 1) {
	crv = CKR_ATTRIBUTE_VALUE_INVALID;
	goto loser;
    }

    slotID = paramStrings.tokens[0].slotID;

    
    if ((slotID < idMin) || (slotID > idMax)) {
	crv = CKR_ATTRIBUTE_VALUE_INVALID;
	goto loser;
    }

    
    newSlot = sftk_SlotFromID(slotID, PR_TRUE);
    if (newSlot && newSlot->present) {
	crv = SFTK_ShutdownSlot(newSlot);
	if (crv != CKR_OK) {
	    goto loser;
	}
    }

    
    if (class == CKO_NETSCAPE_DELSLOT) {
	

	crv = newSlot ? CKR_OK : CKR_SLOT_ID_INVALID;
	goto loser; 
    }

    if (newSlot) {
	crv = SFTK_SlotReInit(newSlot, paramStrings.configdir, 
			paramStrings.updatedir, paramStrings.updateID,
			&paramStrings.tokens[0], moduleIndex);
    } else {
	crv = SFTK_SlotInit(paramStrings.configdir, 
			paramStrings.updatedir, paramStrings.updateID,
			&paramStrings.tokens[0], moduleIndex);
    }
    if (crv != CKR_OK) {
	goto loser;
    }
loser:
    sftk_freeParams(&paramStrings);
    sftk_FreeAttribute(attribute);

    return crv;
}



CK_RV NSC_CreateObject(CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount, 
					CK_OBJECT_HANDLE_PTR phObject)
{
    SFTKSlot *slot = sftk_SlotFromSessionHandle(hSession);
    SFTKSession *session;
    SFTKObject *object;
    

    CK_OBJECT_CLASS class = CKO_VENDOR_DEFINED;
    CK_RV crv;
    int i;

    CHECK_FORK();

    *phObject = CK_INVALID_HANDLE;

    if (slot == NULL) {
	return CKR_SESSION_HANDLE_INVALID;
    }
    


    object = sftk_NewObject(slot); 
    if (object == NULL) {
	return CKR_HOST_MEMORY;
    }

    


    for (i=0; i < (int) ulCount; i++) {
	crv = sftk_AddAttributeType(object,sftk_attr_expand(&pTemplate[i]));
	if (crv != CKR_OK) {
	    sftk_FreeObject(object);
	    return crv;
	}
	if ((pTemplate[i].type == CKA_CLASS) && pTemplate[i].pValue) {
	    class = *(CK_OBJECT_CLASS *)pTemplate[i].pValue;
	}
    }

    
    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) {
	sftk_FreeObject(object);
        return CKR_SESSION_HANDLE_INVALID;
    }

    


    if ((class == CKO_NETSCAPE_NEWSLOT)  || (class == CKO_NETSCAPE_DELSLOT)) {
	crv = sftk_CreateNewSlot(slot, class, object);
	goto done;
    } 

    


    crv = sftk_handleObject(object,session);
    *phObject = object->handle;
done:
    sftk_FreeSession(session);
    sftk_FreeObject(object);

    return crv;
}




CK_RV NSC_CopyObject(CK_SESSION_HANDLE hSession,
       CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount,
					CK_OBJECT_HANDLE_PTR phNewObject) 
{
    SFTKObject *destObject,*srcObject;
    SFTKSession *session;
    CK_RV crv = CKR_OK;
    SFTKSlot *slot = sftk_SlotFromSessionHandle(hSession);
    int i;

    CHECK_FORK();

    if (slot == NULL) {
	return CKR_SESSION_HANDLE_INVALID;
    }
    
    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) {
        return CKR_SESSION_HANDLE_INVALID;
    }
    srcObject = sftk_ObjectFromHandle(hObject,session);
    if (srcObject == NULL) {
	sftk_FreeSession(session);
	return CKR_OBJECT_HANDLE_INVALID;
    }
    


    destObject = sftk_NewObject(slot); 
    if (destObject == NULL) {
	sftk_FreeSession(session);
        sftk_FreeObject(srcObject);
	return CKR_HOST_MEMORY;
    }

    


    for (i=0; i < (int) ulCount; i++) {
	if (sftk_modifyType(pTemplate[i].type,srcObject->objclass) == SFTK_NEVER) {
	    crv = CKR_ATTRIBUTE_READ_ONLY;
	    break;
	}
	crv = sftk_AddAttributeType(destObject,sftk_attr_expand(&pTemplate[i]));
	if (crv != CKR_OK) { break; }
    }
    if (crv != CKR_OK) {
	sftk_FreeSession(session);
        sftk_FreeObject(srcObject);
	sftk_FreeObject(destObject);
	return crv;
    }

    
    if (sftk_hasAttribute(destObject,CKA_SENSITIVE)) {
	if (!sftk_isTrue(destObject,CKA_SENSITIVE)) {
	    sftk_FreeSession(session);
            sftk_FreeObject(srcObject);
	    sftk_FreeObject(destObject);
	    return CKR_ATTRIBUTE_READ_ONLY;
	}
    }

    


    
    

    crv = sftk_CopyObject(destObject,srcObject);

    destObject->objclass = srcObject->objclass;
    sftk_FreeObject(srcObject);
    if (crv != CKR_OK) {
	sftk_FreeObject(destObject);
	sftk_FreeSession(session);
        return crv;
    }

    crv = sftk_handleObject(destObject,session);
    *phNewObject = destObject->handle;
    sftk_FreeSession(session);
    sftk_FreeObject(destObject);
    
    return crv;
}



CK_RV NSC_GetObjectSize(CK_SESSION_HANDLE hSession,
    			CK_OBJECT_HANDLE hObject, CK_ULONG_PTR pulSize)
{
    CHECK_FORK();

    *pulSize = 0;
    return CKR_OK;
}



CK_RV NSC_GetAttributeValue(CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE hObject,CK_ATTRIBUTE_PTR pTemplate,CK_ULONG ulCount)
{
    SFTKSlot *slot = sftk_SlotFromSessionHandle(hSession);
    SFTKSession *session;
    SFTKObject *object;
    SFTKAttribute *attribute;
    PRBool sensitive;
    CK_RV crv;
    int i;

    CHECK_FORK();

    if (slot == NULL) {
	return CKR_SESSION_HANDLE_INVALID;
    }
    


    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) {
        return CKR_SESSION_HANDLE_INVALID;
    }

    
    if (sftk_isToken(hObject)) {
	SFTKSlot *slot = sftk_SlotFromSession(session);
	SFTKDBHandle *dbHandle = sftk_getDBForTokenObject(slot, hObject);
	SFTKDBHandle *keydb = NULL;

	if (dbHandle == NULL) {
	    sftk_FreeSession(session);
	    return CKR_OBJECT_HANDLE_INVALID;
	}

	crv = sftkdb_GetAttributeValue(dbHandle, hObject, pTemplate, ulCount);

	
	keydb = sftk_getKeyDB(slot);
	if (dbHandle == keydb) {
	    for (i=0; i < (int) ulCount; i++) {
		if (sftk_isSensitive(pTemplate[i].type,CKO_PRIVATE_KEY)) {
		    crv = CKR_ATTRIBUTE_SENSITIVE;
		    if (pTemplate[i].pValue && (pTemplate[i].ulValueLen!= -1)){
			PORT_Memset(pTemplate[i].pValue, 0, 
					pTemplate[i].ulValueLen);
		    }
		    pTemplate[i].ulValueLen = -1;
		}
	    }
	}

	sftk_FreeSession(session);
	sftk_freeDB(dbHandle);
	if (keydb) {
	    sftk_freeDB(keydb);
	}
	return crv;
    }

    
    object = sftk_ObjectFromHandle(hObject,session);
    sftk_FreeSession(session);
    if (object == NULL) {
	return CKR_OBJECT_HANDLE_INVALID;
    }

    
    if ((!slot->isLoggedIn) && (slot->needLogin) &&
				(sftk_isTrue(object,CKA_PRIVATE))) {
	sftk_FreeObject(object);
	return CKR_USER_NOT_LOGGED_IN;
    }

    crv = CKR_OK;
    sensitive = sftk_isTrue(object,CKA_SENSITIVE);
    for (i=0; i < (int) ulCount; i++) {
	
	if (sensitive && sftk_isSensitive(pTemplate[i].type,object->objclass)) {
	    crv = CKR_ATTRIBUTE_SENSITIVE;
	    pTemplate[i].ulValueLen = -1;
	    continue;
	}
	attribute = sftk_FindAttribute(object,pTemplate[i].type);
	if (attribute == NULL) {
	    crv = CKR_ATTRIBUTE_TYPE_INVALID;
	    pTemplate[i].ulValueLen = -1;
	    continue;
	}
	if (pTemplate[i].pValue != NULL) {
	    PORT_Memcpy(pTemplate[i].pValue,attribute->attrib.pValue,
						attribute->attrib.ulValueLen);
	}
	pTemplate[i].ulValueLen = attribute->attrib.ulValueLen;
	sftk_FreeAttribute(attribute);
    }

    sftk_FreeObject(object);
    return crv;
}


CK_RV NSC_SetAttributeValue (CK_SESSION_HANDLE hSession,
 CK_OBJECT_HANDLE hObject,CK_ATTRIBUTE_PTR pTemplate,CK_ULONG ulCount)
{
    SFTKSlot *slot = sftk_SlotFromSessionHandle(hSession);
    SFTKSession *session;
    SFTKAttribute *attribute;
    SFTKObject *object;
    PRBool isToken;
    CK_RV crv = CKR_OK;
    CK_BBOOL legal;
    int i;

    CHECK_FORK();

    if (slot == NULL) {
	return CKR_SESSION_HANDLE_INVALID;
    }
    


    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) {
        return CKR_SESSION_HANDLE_INVALID;
    }

    object = sftk_ObjectFromHandle(hObject,session);
    if (object == NULL) {
        sftk_FreeSession(session);
	return CKR_OBJECT_HANDLE_INVALID;
    }

    
    if ((!slot->isLoggedIn) && (slot->needLogin) &&
				(sftk_isTrue(object,CKA_PRIVATE))) {
	sftk_FreeSession(session);
	sftk_FreeObject(object);
	return CKR_USER_NOT_LOGGED_IN;
    }

    
    isToken = sftk_isTrue(object,CKA_TOKEN);
    if (((session->info.flags & CKF_RW_SESSION) == 0) && isToken) {
	sftk_FreeSession(session);
	sftk_FreeObject(object);
	return CKR_SESSION_READ_ONLY;
    }
    sftk_FreeSession(session);

    
    if (!sftk_isTrue(object,CKA_MODIFIABLE)) {
	sftk_FreeObject(object);
	return CKR_ATTRIBUTE_READ_ONLY;
    }

    for (i=0; i < (int) ulCount; i++) {
	
	switch (sftk_modifyType(pTemplate[i].type,object->objclass)) {
	case SFTK_NEVER:
	case SFTK_ONCOPY:
        default:
	    crv = CKR_ATTRIBUTE_READ_ONLY;
	    break;

        case SFTK_SENSITIVE:
	    legal = (pTemplate[i].type == CKA_EXTRACTABLE) ? CK_FALSE : CK_TRUE;
	    if ((*(CK_BBOOL *)pTemplate[i].pValue) != legal) {
	        crv = CKR_ATTRIBUTE_READ_ONLY;
	    }
	    break;
        case SFTK_ALWAYS:
	    break;
	}
	if (crv != CKR_OK) break;

	
	attribute = sftk_FindAttribute(object,pTemplate[i].type);
	if (attribute == NULL) {
	    crv =CKR_ATTRIBUTE_TYPE_INVALID;
	    break;
	}
    	sftk_FreeAttribute(attribute);
	crv = sftk_forceAttribute(object,sftk_attr_expand(&pTemplate[i]));
	if (crv != CKR_OK) break;

    }

    sftk_FreeObject(object);
    return crv;
}

static CK_RV
sftk_expandSearchList(SFTKSearchResults *search, int count)
{
    search->array_size += count;
    search->handles = (CK_OBJECT_HANDLE *)PORT_Realloc(search->handles,
			sizeof(CK_OBJECT_HANDLE)*search->array_size);
    return search->handles ? CKR_OK : CKR_HOST_MEMORY;
}



static CK_RV
sftk_searchDatabase(SFTKDBHandle *handle, SFTKSearchResults *search,
                        const CK_ATTRIBUTE *pTemplate, CK_ULONG ulCount)
{
    CK_RV crv;
    int objectListSize = search->array_size-search->size;
    CK_OBJECT_HANDLE *array = &search->handles[search->size];
    SDBFind *find;
    CK_ULONG count;

    crv = sftkdb_FindObjectsInit(handle, pTemplate, ulCount, &find);
    if (crv != CKR_OK)
	return crv;
    do {
	crv = sftkdb_FindObjects(handle, find, array, objectListSize, &count);
	if ((crv != CKR_OK) || (count == 0))
	    break;
	search->size += count;
	objectListSize -= count;
	if (objectListSize > 0)
	    break;
	crv = sftk_expandSearchList(search,NSC_SEARCH_BLOCK_SIZE);
	objectListSize = NSC_SEARCH_BLOCK_SIZE;
	array = &search->handles[search->size];
    } while (crv == CKR_OK);
    sftkdb_FindObjectsFinal(handle, find);

    return crv;
}





CK_RV
sftk_emailhack(SFTKSlot *slot, SFTKDBHandle *handle, 
    SFTKSearchResults *search, CK_ATTRIBUTE *pTemplate, CK_ULONG ulCount)
{
    PRBool isCert = PR_FALSE;
    int emailIndex = -1;
    int i;
    SFTKSearchResults smime_search;
    CK_ATTRIBUTE smime_template[2];
    CK_OBJECT_CLASS smime_class = CKO_NETSCAPE_SMIME;
    SFTKAttribute *attribute = NULL;
    SFTKObject *object = NULL;
    CK_RV crv = CKR_OK;


    smime_search.handles = NULL; 


    
    for (i=0; i < ulCount; i++) {
	if (pTemplate[i].type == CKA_CLASS) {
	   if ((pTemplate[i].ulValueLen != sizeof(CK_OBJECT_CLASS) ||
	       (*(CK_OBJECT_CLASS *)pTemplate[i].pValue) != CKO_CERTIFICATE)) {
		
		break;
	   }
	   isCert = PR_TRUE;
	} else if (pTemplate[i].type == CKA_NETSCAPE_EMAIL) {
	   emailIndex = i;
	 
	}
	if (isCert && (emailIndex != -1)) break;
    }

    if (!isCert || (emailIndex == -1)) {
	return CKR_OK;
    }

    
    smime_template[0].type = CKA_CLASS;
    smime_template[0].pValue = &smime_class;
    smime_template[0].ulValueLen = sizeof(smime_class);
    smime_template[1] = pTemplate[emailIndex];

    smime_search.handles = (CK_OBJECT_HANDLE *)
		PORT_Alloc(sizeof(CK_OBJECT_HANDLE) * NSC_SEARCH_BLOCK_SIZE);
    if (smime_search.handles == NULL) {
	crv = CKR_HOST_MEMORY;
	goto loser;
    }
    smime_search.index = 0;
    smime_search.size = 0;
    smime_search.array_size = NSC_SEARCH_BLOCK_SIZE;
	
    crv = sftk_searchDatabase(handle, &smime_search, smime_template, 2);
    if (crv != CKR_OK || smime_search.size == 0) {
	goto loser;
    }

    
    object = sftk_NewTokenObject(slot, NULL, smime_search.handles[0]);
    if (object == NULL) {
	crv = CKR_HOST_MEMORY; 
	goto loser;
    }
    attribute = sftk_FindAttribute(object,CKA_SUBJECT);
    if (attribute == NULL) {
	crv = CKR_ATTRIBUTE_TYPE_INVALID;
	goto loser;
    }

    
    pTemplate[emailIndex] = attribute->attrib;
    
    crv = sftk_searchDatabase(handle, search, pTemplate, ulCount);
    pTemplate[emailIndex] = smime_template[1]; 

loser:
    if (attribute) {
	sftk_FreeAttribute(attribute);
    }
    if (object) {
	sftk_FreeObject(object);
    }
    if (smime_search.handles) {
	PORT_Free(smime_search.handles);
    }

    return crv;
}
	
static void
sftk_pruneSearch(CK_ATTRIBUTE *pTemplate, CK_ULONG ulCount,
			PRBool *searchCertDB, PRBool *searchKeyDB) {
    CK_ULONG i;

    *searchCertDB = PR_TRUE;
    *searchKeyDB = PR_TRUE;
    for (i = 0; i < ulCount; i++) {
	if (pTemplate[i].type == CKA_CLASS && pTemplate[i].pValue != NULL) {
	    CK_OBJECT_CLASS class = *((CK_OBJECT_CLASS*)pTemplate[i].pValue);
	    if (class == CKO_PRIVATE_KEY || class == CKO_SECRET_KEY) {
		*searchCertDB = PR_FALSE;
	    } else {
		*searchKeyDB = PR_FALSE;
	    }
	    break;
	}
    }
}

static CK_RV
sftk_searchTokenList(SFTKSlot *slot, SFTKSearchResults *search,
                        CK_ATTRIBUTE *pTemplate, CK_ULONG ulCount,
                        PRBool *tokenOnly, PRBool isLoggedIn)
{
    CK_RV crv = CKR_OK;
    CK_RV crv2;
    PRBool searchCertDB;
    PRBool searchKeyDB;
    
    sftk_pruneSearch(pTemplate, ulCount, &searchCertDB, &searchKeyDB);

    if (searchCertDB) {
	SFTKDBHandle *certHandle = sftk_getCertDB(slot);
	crv = sftk_searchDatabase(certHandle, search, pTemplate, ulCount);
	crv2 = sftk_emailhack(slot, certHandle, search, pTemplate, ulCount);
	if (crv == CKR_OK) crv = crv2;
	sftk_freeDB(certHandle);
    }

    if (crv == CKR_OK && isLoggedIn && searchKeyDB) {
	SFTKDBHandle *keyHandle = sftk_getKeyDB(slot);
    	crv = sftk_searchDatabase(keyHandle, search, pTemplate, ulCount);
    	sftk_freeDB(keyHandle);
    }
    return crv;
}



CK_RV NSC_FindObjectsInit(CK_SESSION_HANDLE hSession,
    			CK_ATTRIBUTE_PTR pTemplate,CK_ULONG ulCount)
{
    SFTKSearchResults *search = NULL, *freeSearch = NULL;
    SFTKSession *session = NULL;
    SFTKSlot *slot = sftk_SlotFromSessionHandle(hSession);
    PRBool tokenOnly = PR_FALSE;
    CK_RV crv = CKR_OK;
    PRBool isLoggedIn;

    CHECK_FORK();
    
    if (slot == NULL) {
	return CKR_SESSION_HANDLE_INVALID;
    }
    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) {
	crv = CKR_SESSION_HANDLE_INVALID;
	goto loser;
    }
   
    search = (SFTKSearchResults *)PORT_Alloc(sizeof(SFTKSearchResults));
    if (search == NULL) {
	crv = CKR_HOST_MEMORY;
	goto loser;
    }
    search->handles = (CK_OBJECT_HANDLE *)
		PORT_Alloc(sizeof(CK_OBJECT_HANDLE) * NSC_SEARCH_BLOCK_SIZE);
    if (search->handles == NULL) {
	crv = CKR_HOST_MEMORY;
	goto loser;
    }
    search->index = 0;
    search->size = 0;
    search->array_size = NSC_SEARCH_BLOCK_SIZE;
    isLoggedIn = (PRBool)((!slot->needLogin) || slot->isLoggedIn);

    crv = sftk_searchTokenList(slot, search, pTemplate, ulCount, &tokenOnly,
								isLoggedIn);
    if (crv != CKR_OK) {
	goto loser;
    }
    
    
    if (!tokenOnly) {
	crv = sftk_searchObjectList(search, slot->sessObjHashTable, 
				slot->sessObjHashSize, slot->objectLock, 
					pTemplate, ulCount, isLoggedIn);
    }
    if (crv != CKR_OK) {
	goto loser;
    }

    if ((freeSearch = session->search) != NULL) {
	session->search = NULL;
	sftk_FreeSearch(freeSearch);
    }
    session->search = search;
    sftk_FreeSession(session);
    return CKR_OK;

loser:
    if (search) {
	sftk_FreeSearch(search);
    }
    if (session) {
	sftk_FreeSession(session);
    }
    return crv;
}




CK_RV NSC_FindObjects(CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE_PTR phObject,CK_ULONG ulMaxObjectCount,
    					CK_ULONG_PTR pulObjectCount)
{
    SFTKSession *session;
    SFTKSearchResults *search;
    int	transfer;
    int left;

    CHECK_FORK();

    *pulObjectCount = 0;
    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    if (session->search == NULL) {
	sftk_FreeSession(session);
	return CKR_OK;
    }
    search = session->search;
    left = session->search->size - session->search->index;
    transfer = ((int)ulMaxObjectCount > left) ? left : ulMaxObjectCount;
    if (transfer > 0) {
	PORT_Memcpy(phObject,&search->handles[search->index],
                                        transfer*sizeof(CK_OBJECT_HANDLE));
    } else {
       *phObject = CK_INVALID_HANDLE;
    }

    search->index += transfer;
    if (search->index == search->size) {
	session->search = NULL;
	sftk_FreeSearch(search);
    }
    *pulObjectCount = transfer;
    sftk_FreeSession(session);
    return CKR_OK;
}


CK_RV NSC_FindObjectsFinal(CK_SESSION_HANDLE hSession)
{
    SFTKSession *session;
    SFTKSearchResults *search;

    CHECK_FORK();

    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    search = session->search;
    session->search = NULL;
    sftk_FreeSession(session);
    if (search != NULL) {
	sftk_FreeSearch(search);
    }
    return CKR_OK;
}



CK_RV NSC_WaitForSlotEvent(CK_FLAGS flags, CK_SLOT_ID_PTR pSlot,
							 CK_VOID_PTR pReserved)
{
    CHECK_FORK();

    return CKR_FUNCTION_NOT_SUPPORTED;
}

