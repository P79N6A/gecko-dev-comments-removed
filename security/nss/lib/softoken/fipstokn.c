


















































#include "seccomon.h"
#include "softoken.h"
#include "lowkeyi.h"
#include "pkcs11.h"
#include "pkcs11i.h"
#include "prenv.h"
#include "prprf.h"

#include <ctype.h>

#ifdef XP_UNIX
#define NSS_AUDIT_WITH_SYSLOG 1
#include <syslog.h>
#include <unistd.h>
#endif

#ifdef SOLARIS
#include <bsm/libbsm.h>
#define AUE_FIPS_AUDIT 34444
#endif

#ifdef LINUX
#include <pthread.h>
#include <dlfcn.h>
#define LIBAUDIT_NAME "libaudit.so.0"
#ifndef AUDIT_CRYPTO_TEST_USER
#define AUDIT_CRYPTO_TEST_USER          2400 /* Crypto test results */
#define AUDIT_CRYPTO_PARAM_CHANGE_USER  2401 /* Crypto attribute change */
#define AUDIT_CRYPTO_LOGIN              2402 /* Logged in as crypto officer */
#define AUDIT_CRYPTO_LOGOUT             2403 /* Logged out from crypto */
#define AUDIT_CRYPTO_KEY_USER           2404 /* Create,delete,negotiate */
#define AUDIT_CRYPTO_FAILURE_USER       2405 /* Fail decrypt,encrypt,randomize */
#endif
static void *libaudit_handle;
static int (*audit_open_func)(void);
static void (*audit_close_func)(int fd);
static int (*audit_log_user_message_func)(int audit_fd, int type,
    const char *message, const char *hostname, const char *addr,
    const char *tty, int result);
static int (*audit_send_user_message_func)(int fd, int type,
    const char *message);

static pthread_once_t libaudit_once_control = PTHREAD_ONCE_INIT;

static void
libaudit_init(void)
{
    libaudit_handle = dlopen(LIBAUDIT_NAME, RTLD_LAZY);
    if (!libaudit_handle) {
	return;
    }
    audit_open_func = dlsym(libaudit_handle, "audit_open");
    audit_close_func = dlsym(libaudit_handle, "audit_close");
    



    audit_log_user_message_func = dlsym(libaudit_handle,
					"audit_log_user_message");
    if (!audit_log_user_message_func) {
	audit_send_user_message_func = dlsym(libaudit_handle,
					     "audit_send_user_message");
    }
    if (!audit_open_func || !audit_close_func ||
	(!audit_log_user_message_func && !audit_send_user_message_func)) {
	dlclose(libaudit_handle);
	libaudit_handle = NULL;
	audit_open_func = NULL;
	audit_close_func = NULL;
	audit_log_user_message_func = NULL;
	audit_send_user_message_func = NULL;
    }
}
#endif 





static PRBool isLoggedIn = PR_FALSE;
PRBool sftk_fatalError = PR_FALSE;






















static CK_RV sftk_newPinCheck(CK_CHAR_PTR pPin, CK_ULONG ulPinLen) {
    unsigned int i;
    int nchar = 0;      
    int ntrail = 0;     
    int ndigit = 0;     
    int nlower = 0;     
    int nupper = 0;     
    int nnonalnum = 0;  
    int nnonascii = 0;  
    int nclass;         

    for (i = 0; i < ulPinLen; i++) {
	unsigned int byte = pPin[i];

	if (ntrail) {
	    if ((byte & 0xc0) != 0x80) {
		
		nchar = -1;
		break;
	    }
	    if (--ntrail == 0) {
		nchar++;
		nnonascii++;
	    }
	    continue;
	}
	if ((byte & 0x80) == 0x00) {
	    
	    nchar++;
	    if (isdigit(byte)) {
		if (i < ulPinLen - 1) {
		    ndigit++;
		}
	    } else if (islower(byte)) {
		nlower++;
	    } else if (isupper(byte)) {
		if (i > 0) {
		    nupper++;
		}
	    } else {
		nnonalnum++;
	    }
	} else if ((byte & 0xe0) == 0xc0) {
	    
	    ntrail = 1;
	} else if ((byte & 0xf0) == 0xe0) {
	    
	    ntrail = 2;
	} else if ((byte & 0xf8) == 0xf0) {
	    
	    ntrail = 3;
	} else {
	    
	    nchar = -1;
	    break;
	}
    }
    if (nchar == -1) {
	
	return CKR_PIN_INVALID;
    }
    if (nchar < FIPS_MIN_PIN) {
	return CKR_PIN_LEN_RANGE;
    }
    nclass = (ndigit != 0) + (nlower != 0) + (nupper != 0) +
	     (nnonalnum != 0) + (nnonascii != 0);
    if (nclass < 3) {
	return CKR_PIN_LEN_RANGE;
    }
    return CKR_OK;
}



static CK_RV sftk_fipsCheck(void) {
    if (sftk_fatalError) 
	return CKR_DEVICE_ERROR;
    if (!isLoggedIn) 
	return CKR_USER_NOT_LOGGED_IN;
    return CKR_OK;
}


#define SFTK_FIPSCHECK() \
    CK_RV rv; \
    if ((rv = sftk_fipsCheck()) != CKR_OK) return rv;

#define SFTK_FIPSFATALCHECK() \
    if (sftk_fatalError) return CKR_DEVICE_ERROR;



void *
fc_getAttribute(CK_ATTRIBUTE_PTR pTemplate, 
				CK_ULONG ulCount, CK_ATTRIBUTE_TYPE type)
{
    int i;

    for (i=0; i < (int) ulCount; i++) {
	if (pTemplate[i].type == type) {
	    return pTemplate[i].pValue;
	}
    }
    return NULL;
}


#define __PASTE(x,y)	x##y


#undef CK_NEED_ARG_LIST
#undef CK_PKCS11_FUNCTION_INFO

#define CK_PKCS11_FUNCTION_INFO(name) CK_RV __PASTE(NS,name)
#define CK_NEED_ARG_LIST 1

#include "pkcs11f.h"


#undef CK_NEED_ARG_LIST
#undef CK_PKCS11_FUNCTION_INFO

#define CK_PKCS11_FUNCTION_INFO(name) CK_RV __PASTE(F,name)
#define CK_NEED_ARG_LIST 1

#include "pkcs11f.h"


static CK_FUNCTION_LIST sftk_fipsTable = {
    { 1, 10 },

#undef CK_NEED_ARG_LIST
#undef CK_PKCS11_FUNCTION_INFO

#define CK_PKCS11_FUNCTION_INFO(name) __PASTE(F,name),


#include "pkcs11f.h"

};

#undef CK_NEED_ARG_LIST
#undef CK_PKCS11_FUNCTION_INFO


#undef __PASTE


#define CKO_NOT_A_KEY CKO_DATA

#define SFTK_IS_KEY_OBJECT(objClass) \
    (((objClass) == CKO_PUBLIC_KEY) || \
    ((objClass) == CKO_PRIVATE_KEY) || \
    ((objClass) == CKO_SECRET_KEY))

#define SFTK_IS_NONPUBLIC_KEY_OBJECT(objClass) \
    (((objClass) == CKO_PRIVATE_KEY) || ((objClass) == CKO_SECRET_KEY))

static CK_RV
sftk_get_object_class_and_fipsCheck(CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE hObject, CK_OBJECT_CLASS *pObjClass)
{
    CK_RV rv;
    CK_ATTRIBUTE class; 
    class.type = CKA_CLASS;
    class.pValue = pObjClass;
    class.ulValueLen = sizeof(*pObjClass);
    rv = NSC_GetAttributeValue(hSession, hObject, &class, 1);
    if ((rv == CKR_OK) && SFTK_IS_NONPUBLIC_KEY_OBJECT(*pObjClass)) {
	rv = sftk_fipsCheck();
    }
    return rv;
}

#ifdef LINUX

int
sftk_mapLinuxAuditType(NSSAuditSeverity severity, NSSAuditType auditType)
{
    switch (auditType) {
    case NSS_AUDIT_ACCESS_KEY:
    case NSS_AUDIT_CHANGE_KEY: 
    case NSS_AUDIT_COPY_KEY:
    case NSS_AUDIT_DERIVE_KEY:
    case NSS_AUDIT_DESTROY_KEY:
    case NSS_AUDIT_DIGEST_KEY:
    case NSS_AUDIT_GENERATE_KEY: 
    case NSS_AUDIT_LOAD_KEY:
    case NSS_AUDIT_UNWRAP_KEY:
    case NSS_AUDIT_WRAP_KEY:
	return AUDIT_CRYPTO_KEY_USER;
    case NSS_AUDIT_CRYPT:
	return (severity == NSS_AUDIT_ERROR) ? AUDIT_CRYPTO_FAILURE_USER : 
					 AUDIT_CRYPTO_KEY_USER;
    case NSS_AUDIT_FIPS_STATE:
    case NSS_AUDIT_INIT_PIN:
    case NSS_AUDIT_INIT_TOKEN:
    case NSS_AUDIT_SET_PIN:
	return AUDIT_CRYPTO_PARAM_CHANGE_USER;
    case NSS_AUDIT_SELF_TEST: 
	return AUDIT_CRYPTO_TEST_USER;
    case NSS_AUDIT_LOGIN:
	return AUDIT_CRYPTO_LOGIN;
    case NSS_AUDIT_LOGOUT:
	return AUDIT_CRYPTO_LOGOUT;
    


    }
    
    return AUDIT_CRYPTO_PARAM_CHANGE_USER;
} 
#endif








PRBool sftk_audit_enabled = PR_FALSE;















void
sftk_LogAuditMessage(NSSAuditSeverity severity, NSSAuditType auditType,
		     const char *msg)
{
#ifdef NSS_AUDIT_WITH_SYSLOG
    int level;

    switch (severity) {
    case NSS_AUDIT_ERROR:
	level = LOG_ERR;
	break;
    case NSS_AUDIT_WARNING:
	level = LOG_WARNING;
	break;
    default:
	level = LOG_INFO;
	break;
    }
    
    syslog(level | LOG_USER ,
	   "NSS " SOFTOKEN_LIB_NAME "[pid=%d uid=%d]: %s",
	   (int)getpid(), (int)getuid(), msg);
#ifdef LINUX
    if (pthread_once(&libaudit_once_control, libaudit_init) != 0) {
	return;
    }
    if (libaudit_handle) {
	int audit_fd;
	int linuxAuditType;
	int result = (severity != NSS_AUDIT_ERROR); 
	char *message = PR_smprintf("NSS " SOFTOKEN_LIB_NAME ": %s", msg);
	if (!message) {
	    return;
	}
	audit_fd = audit_open_func();
	if (audit_fd < 0) {
	    PR_smprintf_free(message);
	    return;
	}
	linuxAuditType = sftk_mapLinuxAuditType(severity, auditType);
	if (audit_log_user_message_func) {
	    audit_log_user_message_func(audit_fd, linuxAuditType, message,
					NULL, NULL, NULL, result);
	} else {
	    audit_send_user_message_func(audit_fd, linuxAuditType, message);
	}
	audit_close_func(audit_fd);
	PR_smprintf_free(message);
    }
#endif 
#ifdef SOLARIS
    {
        int rd;
        char *message = PR_smprintf("NSS " SOFTOKEN_LIB_NAME ": %s", msg);

        if (!message) {
            return;
        }

        
        if ((rd = au_open()) == -1) {
            PR_smprintf_free(message);
            return;
        }

        
        if (au_write(rd, au_to_text(message))) {
            (void)au_close(rd, AU_TO_NO_WRITE, AUE_FIPS_AUDIT);
            PR_smprintf_free(message);
            return;
        }

        
        (void)au_close(rd, AU_TO_WRITE, AUE_FIPS_AUDIT);

        PR_smprintf_free(message);
    }
#endif 
#else
    
#endif
}








CK_RV FC_GetFunctionList(CK_FUNCTION_LIST_PTR *pFunctionList) {

    CHECK_FORK();

    *pFunctionList = &sftk_fipsTable;
    return CKR_OK;
}


PRBool nsf_init = PR_FALSE;


CK_RV FC_Initialize(CK_VOID_PTR pReserved) {
    const char *envp;
    CK_RV crv;

    sftk_ForkReset(pReserved, &crv);

    if (nsf_init) {
	return CKR_CRYPTOKI_ALREADY_INITIALIZED;
    }

    if ((envp = PR_GetEnv("NSS_ENABLE_AUDIT")) != NULL) {
	sftk_audit_enabled = (atoi(envp) == 1);
    }

    crv = nsc_CommonInitialize(pReserved, PR_TRUE);

    
    if (crv != CKR_OK) {
	sftk_fatalError = PR_TRUE;
	return crv;
    }

    sftk_fatalError = PR_FALSE; 

    crv = sftk_fipsPowerUpSelfTest();
    if (crv != CKR_OK) {
        nsc_CommonFinalize(NULL, PR_TRUE);
	sftk_fatalError = PR_TRUE;
	if (sftk_audit_enabled) {
	    char msg[128];
	    PR_snprintf(msg,sizeof msg,
			"C_Initialize()=0x%08lX "
			"power-up self-tests failed",
			(PRUint32)crv);
	    sftk_LogAuditMessage(NSS_AUDIT_ERROR, NSS_AUDIT_SELF_TEST, msg);
	}
	return crv;
    }
    nsf_init = PR_TRUE;

    return CKR_OK;
}


CK_RV FC_Finalize (CK_VOID_PTR pReserved) {
   CK_RV crv;

   if (sftk_ForkReset(pReserved, &crv)) {
       return crv;
   }

   if (!nsf_init) {
      return CKR_OK;
   }

   crv = nsc_CommonFinalize (pReserved, PR_TRUE);

   nsf_init = (PRBool) !(crv == CKR_OK);
   return crv;
}



CK_RV  FC_GetInfo(CK_INFO_PTR pInfo) {
    CHECK_FORK();

    return NSC_GetInfo(pInfo);
}


CK_RV FC_GetSlotList(CK_BBOOL tokenPresent,
	 		CK_SLOT_ID_PTR pSlotList, CK_ULONG_PTR pulCount) {
    CHECK_FORK();

    return nsc_CommonGetSlotList(tokenPresent,pSlotList,pulCount,
							 NSC_FIPS_MODULE);
}
	

CK_RV FC_GetSlotInfo(CK_SLOT_ID slotID, CK_SLOT_INFO_PTR pInfo) {
    CHECK_FORK();

    return NSC_GetSlotInfo(slotID,pInfo);
}



 CK_RV FC_GetTokenInfo(CK_SLOT_ID slotID,CK_TOKEN_INFO_PTR pInfo) {
    CK_RV crv;

    CHECK_FORK();

    crv = NSC_GetTokenInfo(slotID,pInfo);
    if (crv == CKR_OK) 
       pInfo->flags |= CKF_LOGIN_REQUIRED;
    return crv;

}




 CK_RV FC_GetMechanismList(CK_SLOT_ID slotID,
	CK_MECHANISM_TYPE_PTR pMechanismList, CK_ULONG_PTR pusCount) {
     CHECK_FORK();

    SFTK_FIPSFATALCHECK();
    if (slotID == FIPS_SLOT_ID) slotID = NETSCAPE_SLOT_ID;
    
    return NSC_GetMechanismList(slotID,pMechanismList,pusCount);
}




 CK_RV FC_GetMechanismInfo(CK_SLOT_ID slotID, CK_MECHANISM_TYPE type,
    					CK_MECHANISM_INFO_PTR pInfo) {
    CHECK_FORK();

    SFTK_FIPSFATALCHECK();
    if (slotID == FIPS_SLOT_ID) slotID = NETSCAPE_SLOT_ID;
    
    return NSC_GetMechanismInfo(slotID,type,pInfo);
}



 CK_RV FC_InitToken(CK_SLOT_ID slotID,CK_CHAR_PTR pPin,
 				CK_ULONG usPinLen,CK_CHAR_PTR pLabel) {
    CK_RV crv;

    CHECK_FORK();

    crv = NSC_InitToken(slotID,pPin,usPinLen,pLabel);
    if (sftk_audit_enabled) {
	char msg[128];
	NSSAuditSeverity severity = (crv == CKR_OK) ?
		NSS_AUDIT_INFO : NSS_AUDIT_ERROR;
	
	PR_snprintf(msg,sizeof msg,
		"C_InitToken(slotID=%lu, pLabel=\"%.32s\")=0x%08lX",
		(PRUint32)slotID,pLabel,(PRUint32)crv);
	sftk_LogAuditMessage(severity, NSS_AUDIT_INIT_TOKEN, msg);
    }
    return crv;
}



 CK_RV FC_InitPIN(CK_SESSION_HANDLE hSession,
    					CK_CHAR_PTR pPin, CK_ULONG ulPinLen) {
    CK_RV rv;

    CHECK_FORK();

    if (sftk_fatalError) return CKR_DEVICE_ERROR;
    if ((rv = sftk_newPinCheck(pPin,ulPinLen)) == CKR_OK) {
	rv = NSC_InitPIN(hSession,pPin,ulPinLen);
    }
    if (sftk_audit_enabled) {
	char msg[128];
	NSSAuditSeverity severity = (rv == CKR_OK) ?
		NSS_AUDIT_INFO : NSS_AUDIT_ERROR;
	PR_snprintf(msg,sizeof msg,
		"C_InitPIN(hSession=0x%08lX)=0x%08lX",
		(PRUint32)hSession,(PRUint32)rv);
	sftk_LogAuditMessage(severity, NSS_AUDIT_INIT_PIN, msg);
    }
    return rv;
}




 CK_RV FC_SetPIN(CK_SESSION_HANDLE hSession, CK_CHAR_PTR pOldPin,
    CK_ULONG usOldLen, CK_CHAR_PTR pNewPin, CK_ULONG usNewLen) {
    CK_RV rv;

    CHECK_FORK();

    if ((rv = sftk_fipsCheck()) == CKR_OK &&
	(rv = sftk_newPinCheck(pNewPin,usNewLen)) == CKR_OK) {
	rv = NSC_SetPIN(hSession,pOldPin,usOldLen,pNewPin,usNewLen);
    }
    if (sftk_audit_enabled) {
	char msg[128];
	NSSAuditSeverity severity = (rv == CKR_OK) ?
		NSS_AUDIT_INFO : NSS_AUDIT_ERROR;
	PR_snprintf(msg,sizeof msg,
		"C_SetPIN(hSession=0x%08lX)=0x%08lX",
		(PRUint32)hSession,(PRUint32)rv);
	sftk_LogAuditMessage(severity, NSS_AUDIT_SET_PIN, msg);
    }
    return rv;
}


 CK_RV FC_OpenSession(CK_SLOT_ID slotID, CK_FLAGS flags,
   CK_VOID_PTR pApplication,CK_NOTIFY Notify,CK_SESSION_HANDLE_PTR phSession) {
    SFTK_FIPSFATALCHECK();

    CHECK_FORK();

    return NSC_OpenSession(slotID,flags,pApplication,Notify,phSession);
}



 CK_RV FC_CloseSession(CK_SESSION_HANDLE hSession) {
    CHECK_FORK();

    return NSC_CloseSession(hSession);
}



 CK_RV FC_CloseAllSessions (CK_SLOT_ID slotID) {

    CHECK_FORK();

    return NSC_CloseAllSessions (slotID);
}



 CK_RV FC_GetSessionInfo(CK_SESSION_HANDLE hSession,
						CK_SESSION_INFO_PTR pInfo) {
    CK_RV rv;
    SFTK_FIPSFATALCHECK();

    CHECK_FORK();

    rv = NSC_GetSessionInfo(hSession,pInfo);
    if (rv == CKR_OK) {
	if ((isLoggedIn) && (pInfo->state == CKS_RO_PUBLIC_SESSION)) {
		pInfo->state = CKS_RO_USER_FUNCTIONS;
	}
	if ((isLoggedIn) && (pInfo->state == CKS_RW_PUBLIC_SESSION)) {
		pInfo->state = CKS_RW_USER_FUNCTIONS;
	}
    }
    return rv;
}


 CK_RV FC_Login(CK_SESSION_HANDLE hSession, CK_USER_TYPE userType,
				    CK_CHAR_PTR pPin, CK_ULONG usPinLen) {
    CK_RV rv;
    PRBool successful;
    if (sftk_fatalError) return CKR_DEVICE_ERROR;
    rv = NSC_Login(hSession,userType,pPin,usPinLen);
    successful = (rv == CKR_OK) || (rv == CKR_USER_ALREADY_LOGGED_IN);
    if (successful)
	isLoggedIn = PR_TRUE;
    if (sftk_audit_enabled) {
	char msg[128];
	NSSAuditSeverity severity;
	severity = successful ? NSS_AUDIT_INFO : NSS_AUDIT_ERROR;
	PR_snprintf(msg,sizeof msg,
		    "C_Login(hSession=0x%08lX, userType=%lu)=0x%08lX",
		    (PRUint32)hSession,(PRUint32)userType,(PRUint32)rv);
	sftk_LogAuditMessage(severity, NSS_AUDIT_LOGIN, msg);
    }
    return rv;
}


 CK_RV FC_Logout(CK_SESSION_HANDLE hSession) {
    CK_RV rv;

    CHECK_FORK();

    if ((rv = sftk_fipsCheck()) == CKR_OK) {
	rv = NSC_Logout(hSession);
	isLoggedIn = PR_FALSE;
    }
    if (sftk_audit_enabled) {
	char msg[128];
	NSSAuditSeverity severity = (rv == CKR_OK) ?
		NSS_AUDIT_INFO : NSS_AUDIT_ERROR;
	PR_snprintf(msg,sizeof msg,
		    "C_Logout(hSession=0x%08lX)=0x%08lX",
		    (PRUint32)hSession,(PRUint32)rv);
	sftk_LogAuditMessage(severity, NSS_AUDIT_LOGOUT, msg);
    }
    return rv;
}



 CK_RV FC_CreateObject(CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount, 
					CK_OBJECT_HANDLE_PTR phObject) {
    CK_OBJECT_CLASS * classptr;

    SFTK_FIPSCHECK();
    CHECK_FORK();

    classptr = (CK_OBJECT_CLASS *)fc_getAttribute(pTemplate,ulCount,CKA_CLASS);
    if (classptr == NULL) return CKR_TEMPLATE_INCOMPLETE;

    
    if (SFTK_IS_NONPUBLIC_KEY_OBJECT(*classptr)) {
	rv = CKR_ATTRIBUTE_VALUE_INVALID;
    } else {
	rv = NSC_CreateObject(hSession,pTemplate,ulCount,phObject);
    }
    if (sftk_audit_enabled && SFTK_IS_KEY_OBJECT(*classptr)) {
	sftk_AuditCreateObject(hSession,pTemplate,ulCount,phObject,rv);
    }
    return rv;
}






 CK_RV FC_CopyObject(CK_SESSION_HANDLE hSession,
       CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount,
					CK_OBJECT_HANDLE_PTR phNewObject) {
    CK_RV rv;
    CK_OBJECT_CLASS objClass = CKO_NOT_A_KEY;

    CHECK_FORK();

    SFTK_FIPSFATALCHECK();
    rv = sftk_get_object_class_and_fipsCheck(hSession, hObject, &objClass);
    if (rv == CKR_OK) {
	rv = NSC_CopyObject(hSession,hObject,pTemplate,ulCount,phNewObject);
    }
    if (sftk_audit_enabled && SFTK_IS_KEY_OBJECT(objClass)) {
	sftk_AuditCopyObject(hSession,
	    hObject,pTemplate,ulCount,phNewObject,rv);
    }
    return rv;
}



 CK_RV FC_DestroyObject(CK_SESSION_HANDLE hSession,
		 				CK_OBJECT_HANDLE hObject) {
    CK_RV rv;
    CK_OBJECT_CLASS objClass = CKO_NOT_A_KEY;

    CHECK_FORK();

    SFTK_FIPSFATALCHECK();
    rv = sftk_get_object_class_and_fipsCheck(hSession, hObject, &objClass);
    if (rv == CKR_OK) {
	rv = NSC_DestroyObject(hSession,hObject);
    }
    if (sftk_audit_enabled && SFTK_IS_KEY_OBJECT(objClass)) {
	sftk_AuditDestroyObject(hSession,hObject,rv);
    }
    return rv;
}



 CK_RV FC_GetObjectSize(CK_SESSION_HANDLE hSession,
    			CK_OBJECT_HANDLE hObject, CK_ULONG_PTR pulSize) {
    CK_RV rv;
    CK_OBJECT_CLASS objClass = CKO_NOT_A_KEY;

    CHECK_FORK();

    SFTK_FIPSFATALCHECK();
    rv = sftk_get_object_class_and_fipsCheck(hSession, hObject, &objClass);
    if (rv == CKR_OK) {
	rv = NSC_GetObjectSize(hSession, hObject, pulSize);
    }
    if (sftk_audit_enabled && SFTK_IS_KEY_OBJECT(objClass)) {
	sftk_AuditGetObjectSize(hSession, hObject, pulSize, rv);
    }
    return rv;
}



 CK_RV FC_GetAttributeValue(CK_SESSION_HANDLE hSession,
 CK_OBJECT_HANDLE hObject,CK_ATTRIBUTE_PTR pTemplate,CK_ULONG ulCount) {
    CK_RV rv;
    CK_OBJECT_CLASS objClass = CKO_NOT_A_KEY;

    CHECK_FORK();

    SFTK_FIPSFATALCHECK();
    rv = sftk_get_object_class_and_fipsCheck(hSession, hObject, &objClass);
    if (rv == CKR_OK) {
	rv = NSC_GetAttributeValue(hSession,hObject,pTemplate,ulCount);
    }
    if (sftk_audit_enabled && SFTK_IS_KEY_OBJECT(objClass)) {
	sftk_AuditGetAttributeValue(hSession,hObject,pTemplate,ulCount,rv);
    }
    return rv;
}



 CK_RV FC_SetAttributeValue (CK_SESSION_HANDLE hSession,
 CK_OBJECT_HANDLE hObject,CK_ATTRIBUTE_PTR pTemplate,CK_ULONG ulCount) {
    CK_RV rv;
    CK_OBJECT_CLASS objClass = CKO_NOT_A_KEY;

    CHECK_FORK();

    SFTK_FIPSFATALCHECK();
    rv = sftk_get_object_class_and_fipsCheck(hSession, hObject, &objClass);
    if (rv == CKR_OK) {
	rv = NSC_SetAttributeValue(hSession,hObject,pTemplate,ulCount);
    }
    if (sftk_audit_enabled && SFTK_IS_KEY_OBJECT(objClass)) {
	sftk_AuditSetAttributeValue(hSession,hObject,pTemplate,ulCount,rv);
    }
    return rv;
}





 CK_RV FC_FindObjectsInit(CK_SESSION_HANDLE hSession,
    			CK_ATTRIBUTE_PTR pTemplate,CK_ULONG usCount) {
    
    unsigned int i;
    CK_RV rv;
    PRBool needLogin = PR_FALSE;


    CHECK_FORK();

    SFTK_FIPSFATALCHECK();

    for (i=0; i < usCount; i++) {
	CK_OBJECT_CLASS class;
	if (pTemplate[i].type != CKA_CLASS) {
	    continue;
	}
	if (pTemplate[i].ulValueLen != sizeof(CK_OBJECT_CLASS)) {
	    continue;
	}
	if (pTemplate[i].pValue == NULL) {
	    continue;
	}
	class = *(CK_OBJECT_CLASS *)pTemplate[i].pValue;
	if ((class == CKO_PRIVATE_KEY) || (class == CKO_SECRET_KEY)) {
	    needLogin = PR_TRUE;
	    break;
	}
    }
    if (needLogin) {
	if ((rv = sftk_fipsCheck()) != CKR_OK) return rv;
    }
    return NSC_FindObjectsInit(hSession,pTemplate,usCount);
}




 CK_RV FC_FindObjects(CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE_PTR phObject,CK_ULONG usMaxObjectCount,
    					CK_ULONG_PTR pusObjectCount) {
    CHECK_FORK();

    
    SFTK_FIPSFATALCHECK();
    return NSC_FindObjects(hSession,phObject,usMaxObjectCount,
    							pusObjectCount);
}







 CK_RV FC_EncryptInit(CK_SESSION_HANDLE hSession,
		 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    rv = NSC_EncryptInit(hSession,pMechanism,hKey);
    if (sftk_audit_enabled) {
	sftk_AuditCryptInit("Encrypt",hSession,pMechanism,hKey,rv);
    }
    return rv;
}


 CK_RV FC_Encrypt (CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData,
    		CK_ULONG usDataLen, CK_BYTE_PTR pEncryptedData,
					 CK_ULONG_PTR pusEncryptedDataLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_Encrypt(hSession,pData,usDataLen,pEncryptedData,
							pusEncryptedDataLen);
}



 CK_RV FC_EncryptUpdate(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pPart, CK_ULONG usPartLen, CK_BYTE_PTR pEncryptedPart,	
					CK_ULONG_PTR pusEncryptedPartLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_EncryptUpdate(hSession,pPart,usPartLen,pEncryptedPart,
						pusEncryptedPartLen);
}



 CK_RV FC_EncryptFinal(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pLastEncryptedPart, CK_ULONG_PTR pusLastEncryptedPartLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_EncryptFinal(hSession,pLastEncryptedPart,
						pusLastEncryptedPartLen);
}







 CK_RV FC_DecryptInit( CK_SESSION_HANDLE hSession,
			 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    rv = NSC_DecryptInit(hSession,pMechanism,hKey);
    if (sftk_audit_enabled) {
	sftk_AuditCryptInit("Decrypt",hSession,pMechanism,hKey,rv);
    }
    return rv;
}


 CK_RV FC_Decrypt(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pEncryptedData,CK_ULONG usEncryptedDataLen,CK_BYTE_PTR pData,
    						CK_ULONG_PTR pusDataLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_Decrypt(hSession,pEncryptedData,usEncryptedDataLen,pData,
    								pusDataLen);
}



 CK_RV FC_DecryptUpdate(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pEncryptedPart, CK_ULONG usEncryptedPartLen,
    				CK_BYTE_PTR pPart, CK_ULONG_PTR pusPartLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_DecryptUpdate(hSession,pEncryptedPart,usEncryptedPartLen,
    							pPart,pusPartLen);
}



 CK_RV FC_DecryptFinal(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pLastPart, CK_ULONG_PTR pusLastPartLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_DecryptFinal(hSession,pLastPart,pusLastPartLen);
}







 CK_RV FC_DigestInit(CK_SESSION_HANDLE hSession,
    					CK_MECHANISM_PTR pMechanism) {
    SFTK_FIPSFATALCHECK();
    CHECK_FORK();

    return NSC_DigestInit(hSession, pMechanism);
}



 CK_RV FC_Digest(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pData, CK_ULONG usDataLen, CK_BYTE_PTR pDigest,
    						CK_ULONG_PTR pusDigestLen) {
    SFTK_FIPSFATALCHECK();
    CHECK_FORK();

    return NSC_Digest(hSession,pData,usDataLen,pDigest,pusDigestLen);
}



 CK_RV FC_DigestUpdate(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pPart,
					    CK_ULONG usPartLen) {
    SFTK_FIPSFATALCHECK();
    CHECK_FORK();

    return NSC_DigestUpdate(hSession,pPart,usPartLen);
}



 CK_RV FC_DigestFinal(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pDigest,
    						CK_ULONG_PTR pusDigestLen) {
    SFTK_FIPSFATALCHECK();
    CHECK_FORK();

    return NSC_DigestFinal(hSession,pDigest,pusDigestLen);
}









 CK_RV FC_SignInit(CK_SESSION_HANDLE hSession,
		 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    rv = NSC_SignInit(hSession,pMechanism,hKey);
    if (sftk_audit_enabled) {
	sftk_AuditCryptInit("Sign",hSession,pMechanism,hKey,rv);
    }
    return rv;
}





 CK_RV FC_Sign(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pData,CK_ULONG usDataLen,CK_BYTE_PTR pSignature,
    					CK_ULONG_PTR pusSignatureLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_Sign(hSession,pData,usDataLen,pSignature,pusSignatureLen);
}





 CK_RV FC_SignUpdate(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pPart,
    							CK_ULONG usPartLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_SignUpdate(hSession,pPart,usPartLen);
}




 CK_RV FC_SignFinal(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pSignature,
					    CK_ULONG_PTR pusSignatureLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_SignFinal(hSession,pSignature,pusSignatureLen);
}







 CK_RV FC_SignRecoverInit(CK_SESSION_HANDLE hSession,
			 CK_MECHANISM_PTR pMechanism,CK_OBJECT_HANDLE hKey) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    rv = NSC_SignRecoverInit(hSession,pMechanism,hKey);
    if (sftk_audit_enabled) {
	sftk_AuditCryptInit("SignRecover",hSession,pMechanism,hKey,rv);
    }
    return rv;
}





 CK_RV FC_SignRecover(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData,
  CK_ULONG usDataLen, CK_BYTE_PTR pSignature, CK_ULONG_PTR pusSignatureLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_SignRecover(hSession,pData,usDataLen,pSignature,pusSignatureLen);
}








 CK_RV FC_VerifyInit(CK_SESSION_HANDLE hSession,
			   CK_MECHANISM_PTR pMechanism,CK_OBJECT_HANDLE hKey) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    rv = NSC_VerifyInit(hSession,pMechanism,hKey);
    if (sftk_audit_enabled) {
	sftk_AuditCryptInit("Verify",hSession,pMechanism,hKey,rv);
    }
    return rv;
}





 CK_RV FC_Verify(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData,
    CK_ULONG usDataLen, CK_BYTE_PTR pSignature, CK_ULONG usSignatureLen) {
    
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_Verify(hSession,pData,usDataLen,pSignature,usSignatureLen);
}





 CK_RV FC_VerifyUpdate( CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart,
						CK_ULONG usPartLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_VerifyUpdate(hSession,pPart,usPartLen);
}




 CK_RV FC_VerifyFinal(CK_SESSION_HANDLE hSession,
			CK_BYTE_PTR pSignature,CK_ULONG usSignatureLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_VerifyFinal(hSession,pSignature,usSignatureLen);
}








 CK_RV FC_VerifyRecoverInit(CK_SESSION_HANDLE hSession,
			CK_MECHANISM_PTR pMechanism,CK_OBJECT_HANDLE hKey) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    rv = NSC_VerifyRecoverInit(hSession,pMechanism,hKey);
    if (sftk_audit_enabled) {
	sftk_AuditCryptInit("VerifyRecover",hSession,pMechanism,hKey,rv);
    }
    return rv;
}





 CK_RV FC_VerifyRecover(CK_SESSION_HANDLE hSession,
		 CK_BYTE_PTR pSignature,CK_ULONG usSignatureLen,
    				CK_BYTE_PTR pData,CK_ULONG_PTR pusDataLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_VerifyRecover(hSession,pSignature,usSignatureLen,pData,
								pusDataLen);
}






 CK_RV FC_GenerateKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,CK_ATTRIBUTE_PTR pTemplate,CK_ULONG ulCount,
    						CK_OBJECT_HANDLE_PTR phKey) {
    CK_BBOOL *boolptr;

    SFTK_FIPSCHECK();
    CHECK_FORK();

    

    boolptr = (CK_BBOOL *) fc_getAttribute(pTemplate, ulCount, CKA_SENSITIVE);
    if (boolptr != NULL) {
	if (!(*boolptr)) {
	    return CKR_ATTRIBUTE_VALUE_INVALID;
	}
    }

    rv = NSC_GenerateKey(hSession,pMechanism,pTemplate,ulCount,phKey);
    if (sftk_audit_enabled) {
	sftk_AuditGenerateKey(hSession,pMechanism,pTemplate,ulCount,phKey,rv);
    }
    return rv;
}




 CK_RV FC_GenerateKeyPair (CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_ATTRIBUTE_PTR pPublicKeyTemplate,
    CK_ULONG usPublicKeyAttributeCount, CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
    CK_ULONG usPrivateKeyAttributeCount, CK_OBJECT_HANDLE_PTR phPublicKey,
					CK_OBJECT_HANDLE_PTR phPrivateKey) {
    CK_BBOOL *boolptr;
    CK_RV crv;

    SFTK_FIPSCHECK();
    CHECK_FORK();


    

    boolptr = (CK_BBOOL *) fc_getAttribute(pPrivateKeyTemplate, 
				usPrivateKeyAttributeCount, CKA_SENSITIVE);
    if (boolptr != NULL) {
	if (!(*boolptr)) {
	    return CKR_ATTRIBUTE_VALUE_INVALID;
	}
    }
    crv = NSC_GenerateKeyPair (hSession,pMechanism,pPublicKeyTemplate,
    		usPublicKeyAttributeCount,pPrivateKeyTemplate,
		usPrivateKeyAttributeCount,phPublicKey,phPrivateKey);
    if (crv == CKR_GENERAL_ERROR) {
	
	sftk_fatalError = PR_TRUE;
    }
    if (sftk_audit_enabled) {
	sftk_AuditGenerateKeyPair(hSession,pMechanism,pPublicKeyTemplate,
    		usPublicKeyAttributeCount,pPrivateKeyTemplate,
		usPrivateKeyAttributeCount,phPublicKey,phPrivateKey,crv);
    }
    return crv;
}



 CK_RV FC_WrapKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hWrappingKey,
    CK_OBJECT_HANDLE hKey, CK_BYTE_PTR pWrappedKey,
					 CK_ULONG_PTR pulWrappedKeyLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    rv = NSC_WrapKey(hSession,pMechanism,hWrappingKey,hKey,pWrappedKey,
							pulWrappedKeyLen);
    if (sftk_audit_enabled) {
	sftk_AuditWrapKey(hSession,pMechanism,hWrappingKey,hKey,pWrappedKey,
							pulWrappedKeyLen,rv);
    }
    return rv;
}



 CK_RV FC_UnwrapKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hUnwrappingKey,
    CK_BYTE_PTR pWrappedKey, CK_ULONG ulWrappedKeyLen,
    CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulAttributeCount,
						 CK_OBJECT_HANDLE_PTR phKey) {
    CK_BBOOL *boolptr;

    SFTK_FIPSCHECK();
    CHECK_FORK();

    

    boolptr = (CK_BBOOL *) fc_getAttribute(pTemplate, 
					ulAttributeCount, CKA_SENSITIVE);
    if (boolptr != NULL) {
	if (!(*boolptr)) {
	    return CKR_ATTRIBUTE_VALUE_INVALID;
	}
    }
    rv = NSC_UnwrapKey(hSession,pMechanism,hUnwrappingKey,pWrappedKey,
			ulWrappedKeyLen,pTemplate,ulAttributeCount,phKey);
    if (sftk_audit_enabled) {
	sftk_AuditUnwrapKey(hSession,pMechanism,hUnwrappingKey,pWrappedKey,
			ulWrappedKeyLen,pTemplate,ulAttributeCount,phKey,rv);
    }
    return rv;
}



 CK_RV FC_DeriveKey( CK_SESSION_HANDLE hSession,
	 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hBaseKey,
	 CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulAttributeCount, 
						CK_OBJECT_HANDLE_PTR phKey) {
    CK_BBOOL *boolptr;

    SFTK_FIPSCHECK();
    CHECK_FORK();

    

    boolptr = (CK_BBOOL *) fc_getAttribute(pTemplate, 
					ulAttributeCount, CKA_SENSITIVE);
    if (boolptr != NULL) {
	if (!(*boolptr)) {
	    return CKR_ATTRIBUTE_VALUE_INVALID;
	}
    }
    rv = NSC_DeriveKey(hSession,pMechanism,hBaseKey,pTemplate,
			ulAttributeCount, phKey);
    if (sftk_audit_enabled) {
	sftk_AuditDeriveKey(hSession,pMechanism,hBaseKey,pTemplate,
			ulAttributeCount,phKey,rv);
    }
    return rv;
}







 CK_RV FC_SeedRandom(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pSeed,
    CK_ULONG usSeedLen) {
    CK_RV crv;

    SFTK_FIPSFATALCHECK();
    CHECK_FORK();

    crv = NSC_SeedRandom(hSession,pSeed,usSeedLen);
    if (crv != CKR_OK) {
	sftk_fatalError = PR_TRUE;
    }
    return crv;
}



 CK_RV FC_GenerateRandom(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR	pRandomData, CK_ULONG ulRandomLen) {
    CK_RV crv;

    CHECK_FORK();

    SFTK_FIPSFATALCHECK();
    crv = NSC_GenerateRandom(hSession,pRandomData,ulRandomLen);
    if (crv != CKR_OK) {
	sftk_fatalError = PR_TRUE;
	if (sftk_audit_enabled) {
	    char msg[128];
	    PR_snprintf(msg,sizeof msg,
			"C_GenerateRandom(hSession=0x%08lX, pRandomData=%p, "
			"ulRandomLen=%lu)=0x%08lX "
			"self-test: continuous RNG test failed",
			(PRUint32)hSession,pRandomData,
			(PRUint32)ulRandomLen,(PRUint32)crv);
	    sftk_LogAuditMessage(NSS_AUDIT_ERROR, NSS_AUDIT_SELF_TEST, msg);
	}
    }
    return crv;
}




 CK_RV FC_GetFunctionStatus(CK_SESSION_HANDLE hSession) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_GetFunctionStatus(hSession);
}



 CK_RV FC_CancelFunction(CK_SESSION_HANDLE hSession) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_CancelFunction(hSession);
}







CK_RV FC_GetOperationState(CK_SESSION_HANDLE hSession, 
	CK_BYTE_PTR  pOperationState, CK_ULONG_PTR pulOperationStateLen) {
    SFTK_FIPSFATALCHECK();
    CHECK_FORK();

    return NSC_GetOperationState(hSession,pOperationState,pulOperationStateLen);
}




CK_RV FC_SetOperationState(CK_SESSION_HANDLE hSession, 
	CK_BYTE_PTR  pOperationState, CK_ULONG  ulOperationStateLen,
        CK_OBJECT_HANDLE hEncryptionKey, CK_OBJECT_HANDLE hAuthenticationKey) {
    SFTK_FIPSFATALCHECK();
    CHECK_FORK();

    return NSC_SetOperationState(hSession,pOperationState,ulOperationStateLen,
        				hEncryptionKey,hAuthenticationKey);
}


CK_RV FC_FindObjectsFinal(CK_SESSION_HANDLE hSession) {
    
    SFTK_FIPSFATALCHECK();
    CHECK_FORK();

    return NSC_FindObjectsFinal(hSession);
}






CK_RV FC_DigestEncryptUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR  pPart,
    CK_ULONG  ulPartLen, CK_BYTE_PTR  pEncryptedPart,
					 CK_ULONG_PTR pulEncryptedPartLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_DigestEncryptUpdate(hSession,pPart,ulPartLen,pEncryptedPart,
					 pulEncryptedPartLen);
}




CK_RV FC_DecryptDigestUpdate(CK_SESSION_HANDLE hSession,
     CK_BYTE_PTR  pEncryptedPart, CK_ULONG  ulEncryptedPartLen,
    				CK_BYTE_PTR  pPart, CK_ULONG_PTR pulPartLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_DecryptDigestUpdate(hSession, pEncryptedPart,ulEncryptedPartLen,
    				pPart,pulPartLen);
}



CK_RV FC_SignEncryptUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR  pPart,
	 CK_ULONG  ulPartLen, CK_BYTE_PTR  pEncryptedPart,
					 CK_ULONG_PTR pulEncryptedPartLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_SignEncryptUpdate(hSession,pPart,ulPartLen,pEncryptedPart,
					 pulEncryptedPartLen);
}



CK_RV FC_DecryptVerifyUpdate(CK_SESSION_HANDLE hSession, 
	CK_BYTE_PTR  pEncryptedData, CK_ULONG  ulEncryptedDataLen, 
				CK_BYTE_PTR  pData, CK_ULONG_PTR pulDataLen) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    return NSC_DecryptVerifyUpdate(hSession,pEncryptedData,ulEncryptedDataLen, 
				pData,pulDataLen);
}





CK_RV FC_DigestKey(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hKey) {
    SFTK_FIPSCHECK();
    CHECK_FORK();

    rv = NSC_DigestKey(hSession,hKey);
    if (sftk_audit_enabled) {
	sftk_AuditDigestKey(hSession,hKey,rv);
    }
    return rv;
}


CK_RV FC_WaitForSlotEvent(CK_FLAGS flags, CK_SLOT_ID_PTR pSlot,
							 CK_VOID_PTR pReserved)
{
    CHECK_FORK();

    return NSC_WaitForSlotEvent(flags, pSlot, pReserved);
}
