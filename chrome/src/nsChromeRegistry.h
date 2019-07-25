






































#ifndef nsChromeRegistry_h
#define nsChromeRegistry_h

#include "nsIChromeRegistry.h"
#include "nsIToolkitChromeRegistry.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsIPrefBranch.h"

#ifdef MOZ_XUL
#include "nsIXULOverlayProvider.h"
#endif

#include "pldhash.h"

#include "nsCOMArray.h"
#include "nsString.h"
#include "nsTHashtable.h"
#include "nsURIHashKey.h"
#include "nsInterfaceHashtable.h"

class nsIDOMWindowInternal;
class nsIURL;








#define NS_CHROMEREGISTRY_CID \
{ 0x47049e42, 0x1d87, 0x482a, { 0x98, 0x4d, 0x56, 0xae, 0x18, 0x5e, 0x36, 0x7a } }

class nsChromeRegistry : public nsIToolkitChromeRegistry,
#ifdef MOZ_XUL
                         public nsIXULOverlayProvider,
#endif
                         public nsIObserver,
                         public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD ReloadChrome();
  NS_IMETHOD RefreshSkins();
  NS_IMETHOD AllowScriptsForPackage(nsIURI* url,
                                    PRBool* _retval NS_OUTPARAM);
  NS_IMETHOD AllowContentToAccess(nsIURI* url,
                                  PRBool* _retval NS_OUTPARAM);

  
  NS_IMETHOD_(PRBool) WrappersEnabled(nsIURI *aURI);
  NS_IMETHOD ConvertChromeURL(nsIURI* aChromeURI, nsIURI* *aResult);

  
  nsChromeRegistry() : mInitialized(PR_FALSE) { }
  virtual ~nsChromeRegistry();

  virtual nsresult Init();

  static already_AddRefed<nsIChromeRegistry> GetService();

  static nsChromeRegistry* gChromeRegistry;

  static nsresult Canonify(nsIURL* aChromeURL);

protected:
  void FlushSkinCaches();
  void FlushAllCaches();

  static void LogMessage(const char* aMsg, ...);
  static void LogMessageWithContext(nsIURI* aURL, PRUint32 aLineNumber, PRUint32 flags,
                                    const char* aMsg, ...);

  virtual nsresult GetBaseURIFromPackage(const nsCString& aPackage,
                                         const nsCString& aProvider,
                                         const nsCString& aPath,
                                         nsIURI* *aResult) = 0;
  virtual nsresult GetFlagsFromPackage(const nsCString& aPackage,
                                       PRUint32* aFlags) = 0;

  nsresult SelectLocaleFromPref(nsIPrefBranch* prefs);

  static nsresult RefreshWindow(nsIDOMWindowInternal* aWindow);
  static nsresult GetProviderAndPath(nsIURL* aChromeURL,
                                     nsACString& aProvider, nsACString& aPath);

  
  enum {
    
    
    PLATFORM_PACKAGE = 1 << 0,
    
    
    
    XPCNATIVEWRAPPERS = 1 << 1,

    
    CONTENT_ACCESSIBLE = 1 << 2
  };

  PRBool mInitialized;

  
  nsInterfaceHashtable<nsURIHashKey, nsIURI> mOverrideTable;


};

#endif 
