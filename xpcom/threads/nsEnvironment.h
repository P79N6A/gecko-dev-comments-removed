






































#ifndef nsEnvironment_h__
#define nsEnvironment_h__

#include "nsIEnvironment.h"
#include "prlock.h"

#define NS_ENVIRONMENT_CID \
  { 0X3D68F92UL, 0X9513, 0X4E25, \
  { 0X9B, 0XE9, 0X7C, 0XB2, 0X39, 0X87, 0X41, 0X72 } }
#define NS_ENVIRONMENT_CONTRACTID "@mozilla.org/process/environment;1"

class nsEnvironment : public nsIEnvironment
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIENVIRONMENT

    static NS_METHOD Create(nsISupports *aOuter, REFNSIID aIID,
                           void **aResult);

private:
    nsEnvironment() { }
    ~nsEnvironment();

    PRLock *mLock;
};

#endif 
