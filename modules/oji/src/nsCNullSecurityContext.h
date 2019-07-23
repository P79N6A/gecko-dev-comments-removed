





































#ifndef nsCNullSecurityContext_h___
#define nsCNullSecurityContext_h___

#include "nsISecurityContext.h"

class nsCNullSecurityContext : public nsISecurityContext
{
public:

    
    

    NS_DECL_ISUPPORTS

    
    

    








    NS_IMETHOD Implies(const char* target, const char* action, 
		       PRBool* bActionAllowed);

    







    NS_IMETHOD GetOrigin(char* buf, int len);

    







    NS_IMETHOD GetCertificateID(char* buf, int len);

   
    
    

    nsCNullSecurityContext() { }
};

#endif 





