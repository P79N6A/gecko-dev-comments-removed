




































#ifndef nsJARProtocolHandler_h__
#define nsJARProtocolHandler_h__

#include "nsIJARProtocolHandler.h"
#include "nsIProtocolHandler.h"
#include "nsIJARURI.h"
#include "nsIZipReader.h"
#include "nsIMIMEService.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"

class nsJARProtocolHandler : public nsIJARProtocolHandler
                           , public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_NSIJARPROTOCOLHANDLER

    
    nsJARProtocolHandler();
    virtual ~nsJARProtocolHandler();

    static nsJARProtocolHandler *GetSingleton();

    nsresult Init();

    
    nsIMIMEService    *MimeService();
    nsIZipReaderCache *JarCache() { return mJARCache; }

protected:
    nsCOMPtr<nsIZipReaderCache> mJARCache;
    nsCOMPtr<nsIMIMEService> mMimeService;
};

extern nsJARProtocolHandler *gJarHandler;

#define NS_JARPROTOCOLHANDLER_CLASSNAME \
    "nsJarProtocolHandler"
#define NS_JARPROTOCOLHANDLER_CID                    \
{ /* 0xc7e410d4-0x85f2-11d3-9f63-006008a6efe9 */     \
    0xc7e410d4,                                      \
    0x85f2,                                          \
    0x11d3,                                          \
    {0x9f, 0x63, 0x00, 0x60, 0x08, 0xa6, 0xef, 0xe9} \
}

#endif 
