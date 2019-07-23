







































#ifndef nsISoftwareUpdate_h__
#define nsISoftwareUpdate_h__

#include "nsISupports.h"
#include "nsIFactory.h"

#include "nsIXPINotifier.h"
#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsILocalFile.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"

#define NS_IXPINSTALLCOMPONENT_CONTRACTID  "@mozilla.org/xpinstall;1"
#define NS_IXPINSTALLCOMPONENT_CLASSNAME "Mozilla XPInstall Component"

#define XPINSTALL_ENABLE_PREF            "xpinstall.enabled"
#define XPINSTALL_WHITELIST_ADD          "xpinstall.whitelist.add"
#define XPINSTALL_WHITELIST_ADD_103      "xpinstall.whitelist.add.103"
#define XPINSTALL_WHITELIST_REQUIRED     "xpinstall.whitelist.required"
#define XPINSTALL_BLACKLIST_ADD          "xpinstall.blacklist.add"


#define XPI_NO_NEW_THREAD   0x1000

#define NS_ISOFTWAREUPDATE_IID                   \
{ 0x18c2f992, 									 \
  0xb09f, 										 \
  0x11d2,						                 \
 {0xbc, 0xde, 0x00, 0x80, 0x5f, 0x0e, 0x13, 0x53}\
}

class nsIPrincipal;

class nsISoftwareUpdate : public nsISupports
{
    public:
            NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISOFTWAREUPDATE_IID)

            NS_IMETHOD InstallJar(nsIFile* localFile,
                                  const PRUnichar* URL,
                                  const PRUnichar* arguments,
                                  nsIPrincipal* aPrincipalDisplayed,
                                  PRUint32 flags,
                                  nsIXPIListener* aListener = 0) = 0;

            NS_IMETHOD InstallChrome(PRUint32 aType,
                                     nsIFile* aFile,
                                     const PRUnichar* URL,
                                     const PRUnichar* aName,
                                     PRBool aSelect,
                                     nsIXPIListener* aListener = 0) = 0;

            NS_IMETHOD RegisterListener(nsIXPIListener *aListener) = 0;

            
            NS_IMETHOD InstallJarCallBack()                   = 0;
            NS_IMETHOD GetMasterListener(nsIXPIListener **aListener) = 0;
            NS_IMETHOD SetActiveListener(nsIXPIListener *aListener) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISoftwareUpdate, NS_ISOFTWAREUPDATE_IID)

#endif 

