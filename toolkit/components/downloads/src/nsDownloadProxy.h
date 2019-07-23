




































 
#ifndef downloadproxy___h___
#define downloadproxy___h___

#include "nsIDownloadManager.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIMIMEInfo.h"
#include "nsIFileURL.h"

#define PREF_BDM_SHOWWHENSTARTING "browser.download.manager.showWhenStarting"
#define PREF_BDM_USEWINDOW "browser.download.manager.useWindow"

class nsDownloadProxy : public nsITransfer
{
public:

  nsDownloadProxy() { }
  virtual ~nsDownloadProxy() { }

  NS_DECL_ISUPPORTS

  NS_IMETHODIMP Init(nsIURI* aSource,
                     nsIURI* aTarget,
                     const nsAString& aDisplayName,
                     nsIMIMEInfo *aMIMEInfo,
                     PRTime aStartTime,
                     nsILocalFile* aTempFile,
                     nsICancelable* aCancelable) {
    nsresult rv;
    nsCOMPtr<nsIDownloadManager> dm = do_GetService("@mozilla.org/download-manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = dm->AddDownload(nsIDownloadManager::DOWNLOAD_TYPE_DOWNLOAD, aSource,
                         aTarget, aDisplayName, EmptyString(), aMIMEInfo,
                         aStartTime, aTempFile, aCancelable,
                         getter_AddRefs(mInner));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIPrefService> prefs = do_GetService("@mozilla.org/preferences-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIPrefBranch> branch = do_QueryInterface(prefs);

    PRBool showDM = PR_TRUE;
    if (branch)
      branch->GetBoolPref(PREF_BDM_SHOWWHENSTARTING , &showDM);

    PRBool useWindow = PR_TRUE;
    if (branch)
      branch->GetBoolPref(PREF_BDM_USEWINDOW, &useWindow);
    
    if (showDM && useWindow) {
      PRUint32 id;
      mInner->GetId(&id);

      return dm->Open(nsnull, id);
    }
    return rv;
  }

  NS_IMETHODIMP OnStateChange(nsIWebProgress* aWebProgress,
                              nsIRequest* aRequest, PRUint32 aStateFlags,
                              PRUint32 aStatus)
  {
    nsCOMPtr<nsIWebProgressListener> listener = do_QueryInterface(mInner);
    if (listener)
      return listener->OnStateChange(aWebProgress, aRequest, aStateFlags, aStatus);
    return NS_OK;
  }
  
  NS_IMETHODIMP OnStatusChange(nsIWebProgress *aWebProgress,
                               nsIRequest *aRequest, nsresult aStatus,
                               const PRUnichar *aMessage)
  {
    nsCOMPtr<nsIWebProgressListener> listener = do_QueryInterface(mInner);
    if (listener)
      return listener->OnStatusChange(aWebProgress, aRequest, aStatus, aMessage);
    return NS_OK;
  }

  NS_IMETHODIMP OnLocationChange(nsIWebProgress *aWebProgress,
                                 nsIRequest *aRequest, nsIURI *aLocation)
  {
    nsCOMPtr<nsIWebProgressListener> listener = do_QueryInterface(mInner);
    if (listener)
      return listener->OnLocationChange(aWebProgress, aRequest, aLocation);
    return NS_OK;
  }
  
  NS_IMETHODIMP OnProgressChange(nsIWebProgress *aWebProgress,
                                 nsIRequest *aRequest,
                                 PRInt32 aCurSelfProgress,
                                 PRInt32 aMaxSelfProgress,
                                 PRInt32 aCurTotalProgress,
                                 PRInt32 aMaxTotalProgress)
  {
    nsCOMPtr<nsIWebProgressListener> listener = do_QueryInterface(mInner);
    if (listener)
      return listener->OnProgressChange(aWebProgress, aRequest,
                                        aCurSelfProgress,
                                        aMaxSelfProgress,
                                        aCurTotalProgress,
                                        aMaxTotalProgress);
    return NS_OK;
  }

  NS_IMETHODIMP OnProgressChange64(nsIWebProgress *aWebProgress,
                                   nsIRequest *aRequest,
                                   PRInt64 aCurSelfProgress,
                                   PRInt64 aMaxSelfProgress,
                                   PRInt64 aCurTotalProgress,
                                   PRInt64 aMaxTotalProgress)
  {
    nsCOMPtr<nsIWebProgressListener2> listener = do_QueryInterface(mInner);
    if (listener)
      return listener->OnProgressChange64(aWebProgress, aRequest,
                                          aCurSelfProgress,
                                          aMaxSelfProgress,
                                          aCurTotalProgress,
                                          aMaxTotalProgress);
    return NS_OK;
  }

  NS_IMETHODIMP OnRefreshAttempted(nsIWebProgress *aWebProgress,
                                   nsIURI *aUri,
                                   PRInt32 aDelay,
                                   PRBool aSameUri,
                                   PRBool *allowRefresh)
  {
    *allowRefresh = PR_TRUE;
    return NS_OK;
  }

  NS_IMETHODIMP OnSecurityChange(nsIWebProgress *aWebProgress,
                                 nsIRequest *aRequest, PRUint32 aState)
  {
    nsCOMPtr<nsIWebProgressListener> listener = do_QueryInterface(mInner);
    if (listener)
      return listener->OnSecurityChange(aWebProgress, aRequest, aState);
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDownload> mInner;
};

NS_IMPL_ISUPPORTS3(nsDownloadProxy, nsITransfer,
                   nsIWebProgressListener, nsIWebProgressListener2)

#endif
