




































#pragma once

#include "nsISecurityContext.h"

class nsILiveconnect;

class MRJSecurityContext : public nsISecurityContext {
public:
	MRJSecurityContext(const char* location);
	~MRJSecurityContext();

	NS_DECL_ISUPPORTS
    
	NS_IMETHOD Implies(const char* target, const char* action, PRBool *bAllowedAccess);
    NS_IMETHOD GetOrigin(char* buf, int len);
    NS_IMETHOD GetCertificateID(char* buf, int len);

    nsILiveconnect* getConnection() { return mConnection; }

private:
    char* mLocation;
    nsILiveconnect* mConnection;
};
