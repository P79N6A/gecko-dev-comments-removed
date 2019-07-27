




#ifndef nsJARProtocolHandler_h__
#define nsJARProtocolHandler_h__

#include "nsIJARProtocolHandler.h"
#include "nsIProtocolHandler.h"
#include "nsIJARURI.h"
#include "nsIZipReader.h"
#include "nsIMIMEService.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "nsTArrayForwardDeclare.h"

class nsIHashable;
class nsIRemoteOpenFileListener;

class nsJARProtocolHandler final : public nsIJARProtocolHandler
                                 , public nsSupportsWeakReference
{
    typedef nsAutoTArray<nsCOMPtr<nsIRemoteOpenFileListener>, 5>
            RemoteFileListenerArray;

public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_NSIJARPROTOCOLHANDLER

    
    nsJARProtocolHandler();

    static nsJARProtocolHandler *GetSingleton();

    nsresult Init();

    
    nsIMIMEService    *MimeService();
    nsIZipReaderCache *JarCache() { return mJARCache; }

    bool IsMainProcess() const { return mIsMainProcess; }

    bool RemoteOpenFileInProgress(nsIHashable *aRemoteFile,
                                  nsIRemoteOpenFileListener *aListener);
    void RemoteOpenFileComplete(nsIHashable *aRemoteFile, nsresult aStatus);

protected:
    virtual ~nsJARProtocolHandler();

    nsCOMPtr<nsIZipReaderCache> mJARCache;
    nsCOMPtr<nsIMIMEService> mMimeService;

    
    
    nsClassHashtable<nsHashableHashKey, RemoteFileListenerArray>
        mRemoteFileListeners;

    bool mIsMainProcess;
};

extern nsJARProtocolHandler *gJarHandler;

#define NS_JARPROTOCOLHANDLER_CID                    \
{ /* 0xc7e410d4-0x85f2-11d3-9f63-006008a6efe9 */     \
    0xc7e410d4,                                      \
    0x85f2,                                          \
    0x11d3,                                          \
    {0x9f, 0x63, 0x00, 0x60, 0x08, 0xa6, 0xef, 0xe9} \
}

#endif 
