





































#include "nsIAutoConfig.h"
#include "nsITimer.h"
#include "nsIFile.h"
#include "nsIObserver.h"
#include "nsNetUtil.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsWeakReference.h"

class nsAutoConfig : public nsIAutoConfig,
                     public nsITimerCallback,
                     public nsIStreamListener,
                     public nsIObserver,
                     public nsSupportsWeakReference

{
    public:

        NS_DECL_ISUPPORTS
        NS_DECL_NSIAUTOCONFIG
        NS_DECL_NSIREQUESTOBSERVER
        NS_DECL_NSISTREAMLISTENER
        NS_DECL_NSIOBSERVER
        NS_DECL_NSITIMERCALLBACK

        nsAutoConfig();
        virtual ~nsAutoConfig();
        nsresult Init();
  
    protected:
  
        nsresult downloadAutoConfig();
        nsresult readOfflineFile();
        nsresult evaluateLocalFile(nsIFile *file);
        nsresult writeFailoverFile();
        nsresult getEmailAddr(nsACString & emailAddr);
        nsresult PromptForEMailAddress(nsACString &emailAddress);
        nsCString mBuf, mCurrProfile;
        nsCOMPtr<nsIPrefBranch> mPrefBranch;
        PRBool mLoaded;
        nsCOMPtr<nsITimer> mTimer;
        nsCString mConfigURL;
};
