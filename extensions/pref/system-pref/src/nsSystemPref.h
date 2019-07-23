








































#ifndef __SYSTEM_PREF_H__
#define __SYSTEM_PREF_H__

#include "nsCOMPtr.h"
#include "nsXPCOM.h"
#include "nsCRT.h"
#include "nsIAppStartupNotifier.h"
#include "nsICategoryManager.h"
#include "nsIServiceManager.h"
#include "nsWeakReference.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"

#include <nsIObserver.h>

union MozPrefValue;
struct SysPrefItem;
















class nsSystemPref : public nsIObserver,
                     public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    nsSystemPref();
    virtual ~nsSystemPref();
    nsresult Init(void);

private:
    
    nsresult UseSystemPrefs();
    nsresult ReadSystemPref(const char *aPrefName);
    nsresult SaveMozDefaultPref(const char *aPrefName,
                                MozPrefValue *aPrefVal,
                                PRBool *aLocked);

    
    nsresult UseMozillaPrefs();
    nsresult RestoreMozDefaultPref(const char *aPrefName,
                                   MozPrefValue *aPrefVal,
                                   PRBool aLocked);

    nsCOMPtr<nsIPrefBranch2>  mSysPrefService;
    PRBool mEnabled;  
    SysPrefItem *mSysPrefs;
};

#define NS_SYSTEMPREF_CID                  \
  { /* {549abb24-7c9d-4aba-915e-7ce0b716b32f} */       \
    0x549abb24,                                        \
    0x7c9d,                                            \
    0x4aba,                                            \
    { 0x91, 0x5e, 0x7c, 0xe0, 0xb7, 0x16, 0xb3, 0x2f } \
  }

#define NS_SYSTEMPREF_CONTRACTID "@mozilla.org/system-preferences;1"
#define NS_SYSTEMPREF_CLASSNAME "System Preferences"

#endif  
