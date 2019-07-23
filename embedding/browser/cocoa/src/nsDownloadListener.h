





































 
 
#import <Foundation/Foundation.h>
#import <Appkit/Appkit.h>

#import "CHDownloadProgressDisplay.h"

#include "nsString.h"
#include "nsIDownload.h"
#include "nsIWebBrowserPersist.h"
#include "nsIURI.h"
#include "nsILocalFile.h"




class nsDownloadListener :  public CHDownloader,
                            public nsIDownload
{
public:
            nsDownloadListener(DownloadControllerFactory* inDownloadControllerFactory);
    virtual ~nsDownloadListener();

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIDOWNLOAD
    NS_DECL_NSIWEBPROGRESSLISTENER
  
public:
    
    void InitDialog();
    
    virtual void PauseDownload();
    virtual void ResumeDownload();
    virtual void CancelDownload();
    virtual void DownloadDone();
    virtual void DetachDownloadDisplay();
    
private:

    nsCOMPtr<nsIWebBrowserPersist>  mWebPersist;        
    nsCOMPtr<nsIURI>                mURI;               
    nsCOMPtr<nsILocalFile>          mDestination;       
    PRInt64                         mStartTime;         
    PRPackedBool                    mBypassCache;       
    PRPackedBool                    mNetworkTransfer;     
    PRPackedBool                    mGotFirstStateChange; 
    PRPackedBool                    mUserCanceled;        
};

