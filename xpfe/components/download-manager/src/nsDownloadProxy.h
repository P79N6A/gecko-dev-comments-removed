




































 
#ifndef downloadproxy___h___
#define downloadproxy___h___

#include "nsCOMPtr.h"
#include "nsIDownload.h"
#include "nsIDownloadManager.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIMIMEInfo.h"

#define DOWNLOAD_MANAGER_BEHAVIOR_PREF "browser.downloadmanager.behavior"

class nsDownloadProxy : public nsITransfer
{
public:

  nsDownloadProxy() { }
  virtual ~nsDownloadProxy() { }

  NS_DECL_ISUPPORTS
  NS_FORWARD_SAFE_NSIWEBPROGRESSLISTENER(mInner)
  NS_FORWARD_SAFE_NSIWEBPROGRESSLISTENER2(mInner)

  NS_IMETHODIMP Init(nsIURI* aSource,
                     nsIURI* aTarget,
                     const nsAString& aDisplayName,
                     nsIMIMEInfo *aMIMEInfo,
                     PRTime aStartTime,
                     nsILocalFile* aTempFile,
                     nsICancelable* aCancelable) {
    nsresult rv;
    nsCOMPtr<nsIDownloadManager> dm = do_GetService("@mozilla.org/download-manager;1", &rv);
    if (NS_FAILED(rv))
      return rv;
    
    rv = dm->AddDownload(aSource, aTarget, aDisplayName, aMIMEInfo, aStartTime,
                         aTempFile, aCancelable, getter_AddRefs(mInner));
    if (NS_FAILED(rv))
      return rv;

    PRInt32 behavior;
    nsCOMPtr<nsIPrefBranch> branch = do_GetService("@mozilla.org/preferences-service;1", &rv);
    if (NS_SUCCEEDED(rv))
      rv = branch->GetIntPref(DOWNLOAD_MANAGER_BEHAVIOR_PREF, &behavior);
    if (NS_FAILED(rv))
      behavior = 0;

    if (behavior == 0)
      rv = dm->Open(nsnull, mInner);
    else if (behavior == 1)
      rv = dm->OpenProgressDialogFor(mInner, nsnull, PR_TRUE);
    return rv;
  }
private:
  nsCOMPtr<nsIDownload> mInner;
};

NS_IMPL_ISUPPORTS3(nsDownloadProxy, nsITransfer,
                   nsIWebProgressListener, nsIWebProgressListener2)

#endif
