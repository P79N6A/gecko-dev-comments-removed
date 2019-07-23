





































 
#import <Foundation/Foundation.h>
#import <Appkit/Appkit.h>

#include "nsString.h"
#include "nsIWebProgressListener.h"
#include "nsIWebBrowserPersist.h"
#include "nsIURI.h"
#include "nsILocalFile.h"
#include "nsIInputStream.h"
#include "nsIDOMDocument.h"



class nsHeaderSniffer : public nsIWebProgressListener
{
public:
    nsHeaderSniffer(nsIWebBrowserPersist* aPersist, nsIFile* aFile, nsIURI* aURL,
                    nsIDOMDocument* aDocument, nsIInputStream* aPostData,
                    const nsCString& aSuggestedFilename, PRBool aBypassCache,
                    NSView* aFilterView, NSPopUpButton* aFilterList);
    virtual ~nsHeaderSniffer();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESSLISTENER
  
protected:

    nsresult  PerformSave(nsIURI* inOriginalURI);
    nsresult  InitiateDownload(nsISupports* inSourceData, nsString& inFileName, nsIURI* inOriginalURI);

private:

    nsIWebBrowserPersist*     mPersist; 
    nsCOMPtr<nsIFile>         mTmpFile;
    nsCOMPtr<nsIURI>          mURL;
    nsCOMPtr<nsIDOMDocument>  mDocument;
    nsCOMPtr<nsIInputStream>  mPostData;
    nsCString                 mDefaultFilename;
    PRBool                    mBypassCache;
    nsCString                 mContentType;
    nsCString                 mContentDisposition;
    NSView*                   mFilterView;
    NSPopUpButton*            mFilterList;
};

