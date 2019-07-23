




































#ifndef NSLOCALHANDLERAPPMAC_H_
#define NSLOCALHANDLERAPPMAC_H_

#include "nsLocalHandlerApp.h"

class nsLocalHandlerAppMac : public nsLocalHandlerApp {

  public:
    nsLocalHandlerAppMac() { }

    nsLocalHandlerAppMac(const PRUnichar *aName, nsIFile *aExecutable)
      : nsLocalHandlerApp(aName, aExecutable) {} 

    nsLocalHandlerAppMac(const nsAString & aName, nsIFile *aExecutable) 
      : nsLocalHandlerApp(aName, aExecutable) {}
    virtual ~nsLocalHandlerAppMac() { }

    NS_IMETHOD LaunchWithURI(nsIURI* aURI, nsIInterfaceRequestor* aWindowContext);
    NS_IMETHOD GetName(nsAString& aName);
};

#endif 
