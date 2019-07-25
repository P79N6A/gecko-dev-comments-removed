




































#ifndef nsDownloader_h__
#define nsDownloader_h__

#include "nsIDownloader.h"
#include "nsIOutputStream.h"
#include "nsIFile.h"
#include "nsCOMPtr.h"

class nsDownloader : public nsIDownloader
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOWNLOADER
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER

    nsDownloader() : mLocationIsTemp(PR_FALSE) {}

protected:
    virtual ~nsDownloader();

    static NS_METHOD ConsumeData(nsIInputStream *in,
                                 void           *closure,
                                 const char     *fromRawSegment,
                                 PRUint32        toOffset,
                                 PRUint32        count,
                                 PRUint32       *writeCount);

    nsCOMPtr<nsIDownloadObserver> mObserver;
    nsCOMPtr<nsIFile>             mLocation;
    nsCOMPtr<nsIOutputStream>     mSink;
    nsCOMPtr<nsISupports>         mCacheToken;
    PRBool                        mLocationIsTemp;
};

#endif 
