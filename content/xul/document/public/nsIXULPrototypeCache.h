





































#ifndef nsIXULPrototypeCache_h__
#define nsIXULPrototypeCache_h__

#include "nsISupports.h"
class nsIURI;


#define NS_XULPROTOTYPECACHE_CID \
{ 0x3a0a0fc1, 0x8349, 0x11d3, { 0xbe, 0x47, 0x0, 0x10, 0x4b, 0xde, 0x60, 0x48 } }


#define NS_IXULPROTOTYPECACHE_IID \
{ 0xf8bee3d7, 0x4be8, 0x46ae, \
  { 0x92, 0xc2, 0x60, 0xc2, 0x5d, 0x5c, 0xd6, 0x47 } }




class nsIXULPrototypeCache : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXULPROTOTYPECACHE_IID)

    


    virtual PRBool IsCached(nsIURI* aURI) = 0;

    


    virtual void AbortFastLoads() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXULPrototypeCache, NS_IXULPROTOTYPECACHE_IID)

NS_IMETHODIMP
NS_NewXULPrototypeCache(nsISupports* aOuter, REFNSIID aIID, void** aResult);


const char XUL_FASTLOAD_FILE_BASENAME[] = "XUL";





#define XUL_FASTLOAD_FILE_VERSION       (0xfeedbeef - 22)

#define XUL_SERIALIZATION_BUFFER_SIZE   (64 * 1024)
#define XUL_DESERIALIZATION_BUFFER_SIZE (8 * 1024)


#endif 
