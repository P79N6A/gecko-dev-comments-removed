











































#include <stdlib.h>
#include <string.h>
#include "prtypes.h"
#include "jscntxt.h"
#include "jsdbgapi.h"
#include "nsCSecurityContext.h"
#include "nsIScriptContext.h"
#include "jvmmgr.h"
#include "jsjava.h"



#include "nsIScriptSecurityManager.h"
#include "nsIServiceManager.h"
#include "nsCRT.h"

#include "nsTraceRefcnt.h"

static NS_DEFINE_IID(kISecurityContextIID, NS_ISECURITYCONTEXT_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);





NS_IMPL_ISUPPORTS1(nsCSecurityContext, nsISecurityContext)




NS_METHOD 
nsCSecurityContext::Implies(const char* target, const char* action, PRBool *bAllowedAccess)
{
    if(!bAllowedAccess) {
        return NS_ERROR_FAILURE;
    }
  
    if(!nsCRT::strcmp(target,"UniversalBrowserRead")) {
        
        
        
        
        
        
        
        
        if (JSJ_IsJSCallApplet()) {
            *bAllowedAccess = PR_TRUE;
        }
        else {
            *bAllowedAccess = m_HasUniversalBrowserReadCapability;
        }
    } else if(!nsCRT::strcmp(target,"UniversalJavaPermission")) {
        *bAllowedAccess = m_HasUniversalJavaCapability;
    } else {
        *bAllowedAccess = PR_FALSE;
    }

    return NS_OK;
}


NS_METHOD 
nsCSecurityContext::GetOrigin(char* buf, int buflen)
{
    if (!m_pPrincipal) {
        
        nsresult rv = NS_OK;
        nsCOMPtr<nsIScriptSecurityManager> secMan =
             do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
        if (NS_FAILED(rv) || !secMan) {
            return NS_ERROR_FAILURE;
        }

        secMan->GetSubjectPrincipal(getter_AddRefs(m_pPrincipal));
        if (!m_pPrincipal) {
            return NS_ERROR_FAILURE;
        }
    }

    nsXPIDLCString origin;
    m_pPrincipal->GetOrigin(getter_Copies(origin));

    PRInt32 originlen = origin.Length();
    if (origin.IsEmpty() || originlen > buflen - 1) {
        return NS_ERROR_FAILURE;
    }

    
    

    memcpy(buf, origin, originlen);
    buf[originlen] = nsnull; 

    return NS_OK;
}

NS_METHOD 
nsCSecurityContext::GetCertificateID(char* buf, int buflen)
{
    nsCOMPtr<nsIPrincipal> principal;
  
    

    nsresult rv      = NS_OK;
    nsCOMPtr<nsIScriptSecurityManager> secMan = 
             do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv) || !secMan) return NS_ERROR_FAILURE;

    secMan->GetSubjectPrincipal(getter_AddRefs(principal));
    if (!principal) {
        return NS_ERROR_FAILURE;
    }

    nsCAutoString certificate;
    principal->GetFingerprint(certificate);

    PRInt32 certlen = certificate.Length();
    if (buflen <= certlen) {
        return NS_ERROR_FAILURE;
    }

    memcpy(buf, certificate.get(), certlen);
    buf[certlen] = nsnull;

    return NS_OK;
}



extern PRUintn tlsIndex3_g;

nsCSecurityContext::nsCSecurityContext(JSContext* cx)
                   : m_pJStoJavaFrame(NULL), m_pJSCX(cx),
                     m_pPrincipal(NULL),
                     m_HasUniversalJavaCapability(PR_FALSE),
                     m_HasUniversalBrowserReadCapability(PR_FALSE)
{
    MOZ_COUNT_CTOR(nsCSecurityContext);

      

    nsresult rv = NS_OK;
    nsCOMPtr<nsIScriptSecurityManager> secMan = 
             do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv) || !secMan) return;

    
    nsCOMPtr<nsIPrincipal> principal;
    if (NS_FAILED(secMan->GetSubjectPrincipal(getter_AddRefs(principal))))
        
        ; 
          
          

    nsCOMPtr<nsIPrincipal> sysprincipal;
    if (NS_FAILED(secMan->GetSystemPrincipal(getter_AddRefs(sysprincipal))))
        return;

    

    PRBool equals;
    if (!principal || 
        NS_SUCCEEDED(principal->Equals(sysprincipal, &equals)) && equals) {
        
        m_HasUniversalBrowserReadCapability = PR_TRUE;
        m_HasUniversalJavaCapability = PR_TRUE;
    }
    else {
        
        secMan->IsCapabilityEnabled("UniversalBrowserRead",&m_HasUniversalBrowserReadCapability);
        secMan->IsCapabilityEnabled("UniversalJavaPermission",&m_HasUniversalJavaCapability);
    }
}

nsCSecurityContext::nsCSecurityContext(nsIPrincipal *principal)
                   : m_pJStoJavaFrame(NULL), m_pJSCX(NULL),
                     m_pPrincipal(principal),
                     m_HasUniversalJavaCapability(PR_FALSE),
                     m_HasUniversalBrowserReadCapability(PR_FALSE)
{
    MOZ_COUNT_CTOR(nsCSecurityContext);

      

    nsresult rv = NS_OK;
    nsCOMPtr<nsIScriptSecurityManager> secMan = 
             do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv) || !secMan) return;

    nsCOMPtr<nsIPrincipal> sysprincipal;
    if (NS_FAILED(secMan->GetSystemPrincipal(getter_AddRefs(sysprincipal))))
        return;

    

    if (!m_pPrincipal || m_pPrincipal == sysprincipal) {
        
        m_HasUniversalBrowserReadCapability = PR_TRUE;
        m_HasUniversalJavaCapability = PR_TRUE;
    }
    else {
        
        secMan->IsCapabilityEnabled("UniversalBrowserRead",&m_HasUniversalBrowserReadCapability);
        secMan->IsCapabilityEnabled("UniversalJavaPermission",&m_HasUniversalJavaCapability);
    }
}

nsCSecurityContext::~nsCSecurityContext()
{
  MOZ_COUNT_DTOR(nsCSecurityContext);
}

