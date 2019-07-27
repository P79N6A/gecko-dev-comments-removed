




#ifndef nsAutoConfig_h
#define nsAutoConfig_h

#include "nsIAutoConfig.h"
#include "nsITimer.h"
#include "nsIFile.h"
#include "nsIObserver.h"
#include "nsIStreamListener.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsWeakReference.h"
#include "nsString.h"

class nsAutoConfig : public nsIAutoConfig,
                     public nsITimerCallback,
                     public nsIStreamListener,
                     public nsIObserver,
                     public nsSupportsWeakReference

{
    public:

        NS_DECL_THREADSAFE_ISUPPORTS
        NS_DECL_NSIAUTOCONFIG
        NS_DECL_NSIREQUESTOBSERVER
        NS_DECL_NSISTREAMLISTENER
        NS_DECL_NSIOBSERVER
        NS_DECL_NSITIMERCALLBACK

        nsAutoConfig();
        nsresult Init();
  
    protected:
  
        virtual ~nsAutoConfig();
        nsresult downloadAutoConfig();
        nsresult readOfflineFile();
        nsresult evaluateLocalFile(nsIFile *file);
        nsresult writeFailoverFile();
        nsresult getEmailAddr(nsACString & emailAddr);
        nsresult PromptForEMailAddress(nsACString &emailAddress);
        nsCString mBuf;
        nsCOMPtr<nsIPrefBranch> mPrefBranch;
        bool mLoaded;
        nsCOMPtr<nsITimer> mTimer;
        nsCString mConfigURL;
};

#endif
