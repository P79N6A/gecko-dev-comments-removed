






































 
#ifndef downloadmanager___h___
#define downloadmanager___h___

#include "nsIDownloadManager.h"
#include "nsIXPInstallManagerUI.h"
#include "nsIDownloadProgressListener.h"
#include "nsIDownload.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEventListener.h"
#include "nsIWebProgressListener.h"
#include "nsIWebProgressListener2.h"
#include "nsIXPIProgressDialog.h"
#include "nsIURI.h"
#include "nsIWebBrowserPersist.h"
#include "nsILocalFile.h"
#include "nsIRequest.h"
#include "nsIObserver.h"
#include "nsString.h"
#include "nsIStringBundle.h"
#include "nsISupportsPrimitives.h"
#include "nsIProgressDialog.h"
#include "nsIMIMEInfo.h"
#include "nsITimer.h"
#include "mozIStorageConnection.h"
#include "nsISupportsArray.h"
#include "nsCOMArray.h"
#include "nsArrayEnumerator.h"
#include "nsAutoPtr.h"
#include "nsIObserverService.h"

typedef PRInt16 DownloadState;
typedef PRInt16 DownloadType;

class nsXPIProgressListener;
class nsDownload;

class nsDownloadManager : public nsIDownloadManager,
                          public nsIXPInstallManagerUI,
                          public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOWNLOADMANAGER
  NS_DECL_NSIXPINSTALLMANAGERUI
  NS_DECL_NSIOBSERVER

  nsresult Init();

  virtual ~nsDownloadManager();

protected:
  struct TimerParams {
    nsRefPtr<nsDownload> download;
    nsCOMPtr<nsIDOMWindow> parent;
  };
  nsresult InitDB(PRBool *aDoImport);
  nsresult CreateTable();
  nsresult ImportDownloadHistory();
  nsresult GetDownloadFromDB(PRUint32 aID, nsDownload **retVal);
  nsresult AddToCurrentDownloads(nsDownload *aDl);

  




  PRInt64 AddDownloadToDB(const nsAString &aName,
                          const nsACString &aSource,
                          const nsACString &aTarget,
                          const nsAString &aIconURL,
                          PRInt64 aStartTime,
                          PRInt64 aEndTime,
                          PRInt32 aState);

  void NotifyListenersOnDownloadStateChange(PRInt16 aOldState,
                                            nsIDownload *aDownload);
  void NotifyListenersOnProgressChange(nsIWebProgress *aProgress,
                                       nsIRequest *aRequest,
                                       PRInt64 aCurSelfProgress,
                                       PRInt64 aMaxSelfProgress,
                                       PRInt64 aCurTotalProgress,
                                       PRInt64 aMaxTotalProgress,
                                       nsIDownload *aDownload);
  void NotifyListenersOnStateChange(nsIWebProgress *aProgress,
                                    nsIRequest *aRequest,
                                    PRUint32 aStateFlags,
                                    nsresult aStatus,
                                    nsIDownload *aDownload);

  nsDownload *FindDownload(PRUint32 aID);
  nsresult PauseResumeDownload(PRUint32 aID, PRBool aPause);
  nsresult CancelAllDownloads();
  inline PRBool RemoveDownloadFromCurrent(nsDownload *aDownload)
  {
    return mCurrentDownloads.RemoveObject(aDownload);
  }


  void     ConfirmCancelDownloads(PRInt32 aCount,
                                  nsISupportsPRBool* aCancelDownloads,
                                  const PRUnichar* aTitle, 
                                  const PRUnichar* aCancelMessageMultiple, 
                                  const PRUnichar* aCancelMessageSingle,
                                  const PRUnichar* aDontCancelButton);

  static void OpenTimerCallback(nsITimer* aTimer, void* aClosure);
  static nsresult OpenDownloadManager(PRBool aShouldFocus, PRInt32 aFlashCount,
                                      nsIDownload* aDownload,
                                      nsIDOMWindow* aParent);

  PRInt32  GetRetentionBehavior();

  static PRBool IsInFinalStage(DownloadState aState)
  {
    return aState == nsIDownloadManager::DOWNLOAD_NOTSTARTED ||
           aState == nsIDownloadManager::DOWNLOAD_DOWNLOADING ||
           aState == nsIXPInstallManagerUI::INSTALL_INSTALLING;
  }

  static PRBool IsInProgress(DownloadState aState) 
  {
    return aState == nsIDownloadManager::DOWNLOAD_NOTSTARTED || 
           aState == nsIDownloadManager::DOWNLOAD_DOWNLOADING || 
           aState == nsIDownloadManager::DOWNLOAD_PAUSED || 
           aState == nsIXPInstallManagerUI::INSTALL_DOWNLOADING ||
           aState == nsIXPInstallManagerUI::INSTALL_INSTALLING;
  }

  static PRBool CompletedSuccessfully(DownloadState aState)
  {
    return aState == nsIDownloadManager::DOWNLOAD_FINISHED || 
           aState == nsIXPInstallManagerUI::INSTALL_FINISHED;
  }

private:
  nsCOMArray<nsIDownloadProgressListener> mListeners;
  nsCOMPtr<nsIXPIProgressDialog> mXPIProgress;
  nsCOMPtr<nsIStringBundle> mBundle;
  nsCOMPtr<nsITimer> mDMOpenTimer;
  nsCOMPtr<mozIStorageConnection> mDBConn;
  nsCOMArray<nsDownload> mCurrentDownloads;
  nsCOMPtr<nsIObserverService> mObserverService;

  friend class nsDownload;
};

class nsXPIProgressListener : public nsIXPIProgressDialog
{
public:
  NS_DECL_NSIXPIPROGRESSDIALOG
  NS_DECL_ISUPPORTS

  nsXPIProgressListener() { }
  nsXPIProgressListener(nsDownloadManager* aManager);
  virtual ~nsXPIProgressListener();

  void AddDownload(nsIDownload* aDownload);

  PRBool HasActiveXPIOperations();

protected:
  void RemoveDownloadAtIndex(PRUint32 aIndex);

private:
  nsDownloadManager* mDownloadManager;
  nsCOMPtr<nsISupportsArray> mDownloads;
};

class nsDownload : public nsIDownload
{
public:
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSIWEBPROGRESSLISTENER2
  NS_DECL_NSITRANSFER
  NS_DECL_NSIDOWNLOAD
  NS_DECL_ISUPPORTS

  nsDownload();
  virtual ~nsDownload();

public:
  




  nsresult SetState(DownloadState aState);

  DownloadType GetDownloadType();
  void SetDownloadType(DownloadType aType);

  nsresult UpdateDB();

protected:
  void SetStartTime(PRInt64 aStartTime);

  nsresult PauseResume(PRBool aPause);

  nsDownloadManager* mDownloadManager;
  nsCOMPtr<nsIURI> mTarget;

private:
  nsString mDisplayName;

  nsCOMPtr<nsIURI> mSource;
  nsCOMPtr<nsICancelable> mCancelable;
  nsCOMPtr<nsIRequest> mRequest;
  nsCOMPtr<nsIProgressDialog> mDialog;
  nsCOMPtr<nsILocalFile> mTempFile;
  nsCOMPtr<nsIMIMEInfo> mMIMEInfo;
  
  DownloadState mDownloadState;
  DownloadType  mDownloadType;

  PRUint32 mID;
  PRInt32 mPercentComplete;
  PRUint64 mCurrBytes;
  PRUint64 mMaxBytes;
  PRTime mStartTime;
  PRTime mLastUpdate;
  PRBool mPaused;
  double mSpeed;

  friend class nsDownloadManager;
};

#endif
