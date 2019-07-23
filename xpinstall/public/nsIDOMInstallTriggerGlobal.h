




































#ifndef nsIDOMInstallTriggerGlobal_h__
#define nsIDOMInstallTriggerGlobal_h__

#include "nsISupports.h"
#include "nsString.h"
#include "nsIScriptContext.h"
#include "nsXPITriggerInfo.h"
#include "nsIXPIInstallInfo.h"


#define NS_IDOMINSTALLTRIGGERGLOBAL_IID \
 { 0xe8c7941c, 0xaaa0, 0x4faf, \
  {0x83, 0xe8, 0x01, 0xbe, 0x8b, 0xbe, 0x8a, 0x57}}

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

  NS_IMETHOD    UpdateEnabled(nsIScriptGlobalObject* aGlobalObject, PRBool aUseWhitelist, PRBool* aReturn)=0;

  NS_IMETHOD    UpdateEnabled(nsIURI* aURI, PRBool aUseWhitelist, PRBool* aReturn)=0;

  NS_IMETHOD    StartInstall(nsIXPIInstallInfo* aInstallInfo, PRBool* aReturn)=0;

  NS_IMETHOD    Install(nsIScriptGlobalObject* globalObject, nsXPITriggerInfo* aInfo, PRBool* aReturn)=0;

  NS_IMETHOD    InstallChrome(nsIScriptGlobalObject* globalObject, PRUint32 aType, nsXPITriggerItem* aItem, PRBool* aReturn)=0;

  NS_IMETHOD    StartSoftwareUpdate(nsIScriptGlobalObject* globalObject, const nsString& aURL, PRInt32 aFlags, PRBool* aReturn)=0;

  NS_IMETHOD    CompareVersion(const nsString& aRegName, PRInt32 aMajor, PRInt32 aMinor, PRInt32 aRelease, PRInt32 aBuild, PRInt32* aReturn)=0;
  NS_IMETHOD    CompareVersion(const nsString& aRegName, const nsString& aVersion, PRInt32* aReturn)=0;
  NS_IMETHOD    CompareVersion(const nsString& aRegName, nsIDOMInstallVersion* aVersion, PRInt32* aReturn)=0;

  NS_IMETHOD    GetVersion(const nsString& component, nsString& version)=0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMInstallTriggerGlobal,
                              NS_IDOMINSTALLTRIGGERGLOBAL_IID)

extern nsresult NS_InitInstallTriggerGlobalClass(nsIScriptContext *aContext, void **aPrototype);

extern "C" nsresult NS_NewScriptInstallTriggerGlobal(nsIScriptContext *aContext, nsISupports *aSupports, nsISupports *aParent, void **aReturn);

#endif 
