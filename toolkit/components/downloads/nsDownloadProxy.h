



 
#ifndef downloadproxy___h___
#define downloadproxy___h___

#include "nsIDownloadManager.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIMIMEInfo.h"
#include "nsIFileURL.h"
#include "nsIDownloadManagerUI.h"

#define PREF_BDM_SHOWWHENSTARTING "browser.download.manager.showWhenStarting"
#define PREF_BDM_FOCUSWHENSTARTING "browser.download.manager.focusWhenStarting"






class nsDownloadProxy : public nsITransfer
{
protected:

  virtual ~nsDownloadProxy() { }

public:

  nsDownloadProxy() { }

  NS_DECL_ISUPPORTS

  NS_IMETHODIMP Init(nsIURI* aSource,
                     nsIURI* aTarget,
                     const nsAString& aDisplayName,
                     nsIMIMEInfo *aMIMEInfo,
                     PRTime aStartTime,
                     nsIFile* aTempFile,
                     nsICancelable* aCancelable,
                     bool aIsPrivate) override {
    nsresult rv;
    nsCOMPtr<nsIDownloadManager> dm = do_GetService("@mozilla.org/download-manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = dm->AddDownload(nsIDownloadManager::DOWNLOAD_TYPE_DOWNLOAD, aSource,
                         aTarget, aDisplayName, aMIMEInfo, aStartTime,
                         aTempFile, aCancelable, aIsPrivate,
                         getter_AddRefs(mInner));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIPrefService> prefs = do_GetService("@mozilla.org/preferences-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIPrefBranch> branch = do_QueryInterface(prefs);

    bool showDM = true;
    if (branch)
      branch->GetBoolPref(PREF_BDM_SHOWWHENSTARTING, &showDM);

    if (showDM) {
      nsCOMPtr<nsIDownloadManagerUI> dmui =
        do_GetService("@mozilla.org/download-manager-ui;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      bool visible;
      rv = dmui->GetVisible(&visible);
      NS_ENSURE_SUCCESS(rv, rv);

      bool focusWhenStarting = true;
      if (branch)
        (void)branch->GetBoolPref(PREF_BDM_FOCUSWHENSTARTING, &focusWhenStarting);

      if (visible && !focusWhenStarting)
        return NS_OK;

      return dmui->Show(nullptr, mInner, nsIDownloadManagerUI::REASON_NEW_DOWNLOAD, aIsPrivate);
    }
    return rv;
  }

  NS_IMETHODIMP OnStateChange(nsIWebProgress* aWebProgress,
                              nsIRequest* aRequest, uint32_t aStateFlags,
                              nsresult aStatus) override
  {
    NS_ENSURE_TRUE(mInner, NS_ERROR_NOT_INITIALIZED);
    return mInner->OnStateChange(aWebProgress, aRequest, aStateFlags, aStatus);
  }
  
  NS_IMETHODIMP OnStatusChange(nsIWebProgress *aWebProgress,
                               nsIRequest *aRequest, nsresult aStatus,
                               const char16_t *aMessage) override
  {
    NS_ENSURE_TRUE(mInner, NS_ERROR_NOT_INITIALIZED);
    return mInner->OnStatusChange(aWebProgress, aRequest, aStatus, aMessage);
  }

  NS_IMETHODIMP OnLocationChange(nsIWebProgress *aWebProgress,
                                 nsIRequest *aRequest, nsIURI *aLocation,
                                 uint32_t aFlags) override
  {
    NS_ENSURE_TRUE(mInner, NS_ERROR_NOT_INITIALIZED);
    return mInner->OnLocationChange(aWebProgress, aRequest, aLocation, aFlags);
  }
  
  NS_IMETHODIMP OnProgressChange(nsIWebProgress *aWebProgress,
                                 nsIRequest *aRequest,
                                 int32_t aCurSelfProgress,
                                 int32_t aMaxSelfProgress,
                                 int32_t aCurTotalProgress,
                                 int32_t aMaxTotalProgress) override
  {
    NS_ENSURE_TRUE(mInner, NS_ERROR_NOT_INITIALIZED);
    return mInner->OnProgressChange(aWebProgress, aRequest,
                                    aCurSelfProgress,
                                    aMaxSelfProgress,
                                    aCurTotalProgress,
                                    aMaxTotalProgress);
  }

  NS_IMETHODIMP OnProgressChange64(nsIWebProgress *aWebProgress,
                                   nsIRequest *aRequest,
                                   int64_t aCurSelfProgress,
                                   int64_t aMaxSelfProgress,
                                   int64_t aCurTotalProgress,
                                   int64_t aMaxTotalProgress) override
  {
    NS_ENSURE_TRUE(mInner, NS_ERROR_NOT_INITIALIZED);
    return mInner->OnProgressChange64(aWebProgress, aRequest,
                                      aCurSelfProgress,
                                      aMaxSelfProgress,
                                      aCurTotalProgress,
                                      aMaxTotalProgress);
  }

  NS_IMETHODIMP OnRefreshAttempted(nsIWebProgress *aWebProgress,
                                   nsIURI *aUri,
                                   int32_t aDelay,
                                   bool aSameUri,
                                   bool *allowRefresh) override
  {
    *allowRefresh = true;
    return NS_OK;
  }

  NS_IMETHODIMP OnSecurityChange(nsIWebProgress *aWebProgress,
                                 nsIRequest *aRequest, uint32_t aState) override
  {
    NS_ENSURE_TRUE(mInner, NS_ERROR_NOT_INITIALIZED);
    return mInner->OnSecurityChange(aWebProgress, aRequest, aState);
  }

  NS_IMETHODIMP SetSha256Hash(const nsACString& aHash) override
  {
    NS_ENSURE_TRUE(mInner, NS_ERROR_NOT_INITIALIZED);
    return mInner->SetSha256Hash(aHash);
  }

  NS_IMETHODIMP SetSignatureInfo(nsIArray* aSignatureInfo) override
  {
    NS_ENSURE_TRUE(mInner, NS_ERROR_NOT_INITIALIZED);
    return mInner->SetSignatureInfo(aSignatureInfo);
  }

  NS_IMETHODIMP SetRedirects(nsIArray* aRedirects) override
  {
    NS_ENSURE_TRUE(mInner, NS_ERROR_NOT_INITIALIZED);
    return mInner->SetRedirects(aRedirects);
  }

private:
  nsCOMPtr<nsIDownload> mInner;
};

NS_IMPL_ISUPPORTS(nsDownloadProxy, nsITransfer,
                  nsIWebProgressListener, nsIWebProgressListener2)

#endif
