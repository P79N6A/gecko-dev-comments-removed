




































#ifndef nsAboutCache_h__
#define nsAboutCache_h__

#include "nsIAboutModule.h"

#include "nsString.h"
#include "nsIOutputStream.h"

#include "nsICacheVisitor.h"
#include "nsCOMPtr.h"

class nsAboutCache : public nsIAboutModule 
                   , public nsICacheVisitor
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIABOUTMODULE
    NS_DECL_NSICACHEVISITOR

    nsAboutCache() {}
    virtual ~nsAboutCache() {}

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
    nsresult  ParseURI(nsIURI * uri, nsCString &deviceID);

    nsCOMPtr<nsIOutputStream> mStream;
    nsCString                 mDeviceID;
    nsCString mBuffer;
};

#define NS_ABOUT_CACHE_MODULE_CID                    \
{ /* 9158c470-86e4-11d4-9be2-00e09872a416 */         \
    0x9158c470,                                      \
    0x86e4,                                          \
    0x11d4,                                          \
    {0x9b, 0xe2, 0x00, 0xe0, 0x98, 0x72, 0xa4, 0x16} \
}

#endif 
