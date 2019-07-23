






































#include "imgICache.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"

#include "prtypes.h"

class imgRequest;
class nsIURI;
class nsICacheEntryDescriptor;

#define NS_IMGCACHE_CID \
{ /* fb4fd28a-1dd1-11b2-8391-e14242c59a41 */         \
     0xfb4fd28a,                                     \
     0x1dd1,                                         \
     0x11b2,                                         \
    {0x83, 0x91, 0xe1, 0x42, 0x42, 0xc5, 0x9a, 0x41} \
}

class imgCache : public imgICache, 
                 public nsIObserver,
                 public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGICACHE
  NS_DECL_NSIOBSERVER

  imgCache();
  virtual ~imgCache();

  static nsresult Init();

  static void Shutdown(); 

  
  static PRBool Put(nsIURI *aKey, imgRequest *request, nsICacheEntryDescriptor **aEntry);
  static PRBool Get(nsIURI *aKey, PRBool *aHasExpired, imgRequest **aRequest, nsICacheEntryDescriptor **aEntry);
  static PRBool Remove(nsIURI *aKey);

  static nsresult ClearChromeImageCache();
  static nsresult ClearImageCache();
};

