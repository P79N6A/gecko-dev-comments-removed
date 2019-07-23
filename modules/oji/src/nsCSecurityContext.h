











































#ifndef nsCSecurityContext_h___
#define nsCSecurityContext_h___

#include "jscntxt.h"
#include "jsdbgapi.h"
#include "nsISecurityContext.h"
#include "nsIPrincipal.h"
#include "nsCOMPtr.h"

struct JSContext;





class nsCSecurityContext :public nsISecurityContext {
public:
    
    

    NS_DECL_ISUPPORTS

    
    

    








    NS_IMETHOD Implies(const char* target, const char* action, PRBool *bAllowedAccess);

    







    NS_IMETHOD GetOrigin(char* buf, int len);

    







    NS_IMETHOD GetCertificateID(char* buf, int len);

    
    

    nsCSecurityContext(JSContext* cx);
    nsCSecurityContext(nsIPrincipal* principal);
    virtual ~nsCSecurityContext(void);

protected:
    JSStackFrame *m_pJStoJavaFrame;
    JSContext    *m_pJSCX;
private:
    nsCOMPtr<nsIPrincipal> m_pPrincipal;
    PRBool        m_HasUniversalJavaCapability;
    PRBool        m_HasUniversalBrowserReadCapability;
};

#endif 
