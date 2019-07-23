





































 #pragma once

#include "nsString.h"
#include "nsIWebProgressListener.h"
#include "nsIWebBrowserPersist.h"
#include "nsIURI.h"
#include "nsILocalFile.h"
#include "nsIInputStream.h"
#include "nsIDOMDocument.h"


typedef enum
{
  eSaveFormatUnspecified = 0,
  eSaveFormatPlainText,       
  eSaveFormatHTML,
  eSaveFormatHTMLComplete
} ESaveFormat;


class CHeaderSniffer : public nsIWebProgressListener
{
public:
    CHeaderSniffer(nsIWebBrowserPersist* aPersist, nsIFile* aFile, nsIURI* aURL,
                    nsIDOMDocument* aDocument, nsIInputStream* aPostData,
                    const nsAString& aSuggestedFilename, PRBool aBypassCache,
                    ESaveFormat aSaveFormat = eSaveFormatUnspecified);
    virtual ~CHeaderSniffer();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESSLISTENER
  
protected:

    nsresult  PerformSave(nsIURI* inOriginalURI, const ESaveFormat inSaveFormat);
    nsresult  InitiateDownload(nsISupports* inSourceData, nsILocalFile* inDestFile, nsIURI* inOriginalURI);

private:

    nsIWebBrowserPersist*     mPersist; 
    nsCOMPtr<nsIFile>         mTmpFile;
    nsCOMPtr<nsIURI>          mURL;
    nsCOMPtr<nsIDOMDocument>  mDocument;
    nsCOMPtr<nsIInputStream>  mPostData;
    nsString                  mDefaultFilename;
    PRBool                    mBypassCache;
    ESaveFormat               mSaveFormat;
    nsCString                 mContentType;
    nsCString                 mContentDisposition;
};

