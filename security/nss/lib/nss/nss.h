







































#ifndef __nss_h_
#define __nss_h_


#ifdef NSS_ENABLE_ECC
#ifdef NSS_ECC_MORE_THAN_SUITE_B
#define _NSS_ECC_STRING " Extended ECC"
#else
#define _NSS_ECC_STRING " Basic ECC"
#endif
#else
#define _NSS_ECC_STRING ""
#endif


#if defined(NSS_ALLOW_UNSUPPORTED_CRITICAL)
#define _NSS_CUSTOMIZED " (Customized build)"
#else
#define _NSS_CUSTOMIZED 
#endif








#define NSS_VERSION  "3.12.4.5" _NSS_ECC_STRING _NSS_CUSTOMIZED
#define NSS_VMAJOR   3
#define NSS_VMINOR   12
#define NSS_VPATCH   4
#define NSS_BETA     PR_FALSE

#ifndef RC_INVOKED

#include "seccomon.h"

SEC_BEGIN_PROTOS











extern PRBool NSS_VersionCheck(const char *importedVersion);







extern SECStatus NSS_Init(const char *configdir);




extern PRBool NSS_IsInitialized(void);







extern SECStatus NSS_InitReadWrite(const char *configdir);






























































#define NSS_INIT_READONLY	0x1
#define NSS_INIT_NOCERTDB	0x2
#define NSS_INIT_NOMODDB	0x4
#define NSS_INIT_FORCEOPEN	0x8
#define NSS_INIT_NOROOTINIT     0x10
#define NSS_INIT_OPTIMIZESPACE  0x20
#define NSS_INIT_PK11THREADSAFE   0x40
#define NSS_INIT_PK11RELOAD       0x80
#define NSS_INIT_NOPK11FINALIZE   0x100
#define NSS_INIT_RESERVED         0x200

#define NSS_INIT_COOPERATE NSS_INIT_PK11THREADSAFE | \
        NSS_INIT_PK11RELOAD | \
        NSS_INIT_NOPK11FINALIZE | \
        NSS_INIT_RESERVED

#ifdef macintosh
#define SECMOD_DB "Security Modules"
#else
#define SECMOD_DB "secmod.db"
#endif

extern SECStatus NSS_Initialize(const char *configdir, 
	const char *certPrefix, const char *keyPrefix, 
	const char *secmodName, PRUint32 flags);











extern SECStatus NSS_InitWithMerge(const char *configdir, 
	const char *certPrefix, const char *keyPrefix, const char *secmodName,
	const char *updatedir,  const char *updCertPrefix, 
	const char *updKeyPrefix, const char *updateID, 
	const char *updateName, PRUint32 flags);



SECStatus NSS_NoDB_Init(const char *configdir);













typedef SECStatus (*NSS_ShutdownFunc)(void *appData, void *nssData);




SECStatus NSS_RegisterShutdown(NSS_ShutdownFunc sFunc, void *appData);





SECStatus NSS_UnregisterShutdown(NSS_ShutdownFunc sFunc, void *appData);




extern SECStatus NSS_Shutdown(void);




void PK11_ConfigurePKCS11(const char *man, const char *libdes, 
	const char *tokdes, const char *ptokdes, const char *slotdes, 
	const char *pslotdes, const char *fslotdes, const char *fpslotdes,
        int minPwd, int pwRequired);





void nss_DumpCertificateCacheInfo(void);

SEC_END_PROTOS

#endif 
#endif 
