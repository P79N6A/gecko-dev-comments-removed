








































#ifndef __SYSTEM_PREF_SERVICE_H__
#define __SYSTEM_PREF_SERVICE_H__

#include "prlink.h"
#include "nsVoidArray.h"
#include "nsWeakPtr.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"

class GConfProxy;







class nsSystemPrefService : public nsIPrefBranch2
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPREFBRANCH
    NS_DECL_NSIPREFBRANCH2

    nsSystemPrefService();
    virtual ~nsSystemPrefService();
    nsresult Init();

    void OnPrefChange(PRUint32 aPrefAtom, void *aData);

private:
    PRBool mInitialized;
    GConfProxy *mGConf;

    
    nsAutoVoidArray *mObservers;
};

#define NS_SYSTEMPREF_SERVICE_CID                  \
  { /* {94f1de09-d0e5-4ca8-94c2-98b049316b7f} */       \
    0x94f1de09,                                        \
    0xd0e5,                                            \
    0x4ca8,                                            \
    { 0x94, 0xc2, 0x98, 0xb0, 0x49, 0x31, 0x6b, 0x7f } \
  }

#define NS_SYSTEMPREF_SERVICE_CONTRACTID "@mozilla.org/system-preference-service;1"
#define NS_SYSTEMPREF_SERVICE_CLASSNAME "System Preferences Service"

#define NS_SYSTEMPREF_PREFCHANGE_TOPIC_ID "nsSystemPrefService:pref-changed"

#endif  
