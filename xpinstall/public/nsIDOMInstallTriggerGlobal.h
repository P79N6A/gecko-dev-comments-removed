





































#ifndef nsIDOMInstallTriggerGlobal_h__
#define nsIDOMInstallTriggerGlobal_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsXPITriggerInfo.h"
#include "nsIXPIInstallInfo.h"


#define NS_IDOMINSTALLTRIGGERGLOBAL_IID \
 { 0x23bb93a4, 0xdaee, 0x4a47, \
  {0x87, 0x76, 0xb1, 0x72, 0x35, 0x86, 0x2d, 0xac}}

class nsIDOMInstallTriggerGlobal : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMINSTALLTRIGGERGLOBAL_IID)
  enum {
    NOT_FOUND = -5,
    MAJOR_DIFF = 4,
    MINOR_DIFF = 3,
    REL_DIFF = 2,
    BLD_DIFF = 1,
    EQUAL = 0
  };

  NS_IMETHOD    GetOriginatingURI(nsIScriptGlobalObject* aGlobalObject, nsIURI * *aUri)=0;

  NS_IMETHOD    UpdateEnabled(nsIScriptGlobalObject* aGlobalObject, bool aUseWhitelist, bool* aReturn)=0;

  NS_IMETHOD    UpdateEnabled(nsIURI* aURI, bool aUseWhitelist, bool* aReturn)=0;

  NS_IMETHOD    StartInstall(nsIXPIInstallInfo* aInstallInfo, bool* aReturn)=0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMInstallTriggerGlobal,
                              NS_IDOMINSTALLTRIGGERGLOBAL_IID)

extern nsresult NS_InitInstallTriggerGlobalClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" nsresult NS_NewScriptInstallTriggerGlobal(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif 
