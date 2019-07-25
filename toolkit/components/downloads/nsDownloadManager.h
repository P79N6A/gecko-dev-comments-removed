










































#ifndef downloadmanager___h___
#define downloadmanager___h___

#if defined(XP_WIN) && (MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN)
#define DOWNLOAD_SCANNER
#endif

#include "nsIDownload.h"
#include "nsIDownloadManager.h"
#include "nsIDownloadProgressListener.h"
#include "nsILocalFile.h"
#include "nsIMIMEInfo.h"
#include "nsINavHistoryService.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIStringBundle.h"
#include "nsISupportsPrimitives.h"
#include "nsITimer.h"

#include "mozStorageHelper.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"

typedef PRInt16 DownloadState;
typedef PRInt16 DownloadType;

class nsDownload;

#ifdef DOWNLOAD_SCANNER
#include "nsDownloadScanner.h"
#endif

class nsDownloadManager : public nsIDownloadManager,
                          public nsINavHistoryObserver,
                          public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOWNLOADMANAGER
  NS_DECL_NSINAVHISTORYOBSERVER
  NS_DECL_NSIOBSERVER

  nsresult Init();

  static nsDownloadManager *GetSingleton();

  virtual ~nsDownloadManager();
  nsDownloadManager() :
      mDBType(DATABASE_DISK)
    , mInPrivateBrowsing(PR_FALSE)
#ifdef DOWNLOAD_SCANNER
    , mScanner(nsnull)
#endif
  {
  }

protected:
  enum DatabaseType
  {
    DATABASE_DISK = 0, 
    DATABASE_MEMORY
  };

  nsresult InitDB();
  nsresult InitFileDB();
  nsresult InitMemoryDB();
  already_AddRefed<mozIStorageConnection> GetFileDBConnection(nsIFile *dbFile) const;
  already_AddRefed<mozIStorageConnection> GetMemoryDBConnection() const;
  nsresult SwitchDatabaseTypeTo(enum DatabaseType aType);
  nsresult CreateTable();

  




  nsresult RestoreDatabaseState();

  



  nsresult RestoreActiveDownloads();

  nsresult GetDownloadFromDB(PRUint32 aID, nsDownload **retVal);

  



  nsresult AddToCurrentDownloads(nsDownload *aDl);

  void SendEvent(nsDownload *aDownload, const char *aTopic);

  




  PRInt64 AddDownloadToDB(const nsAString &aName,
                          const nsACString &aSource,
                          const nsACString &aTarget,
                          const nsAString &aTempPath,
                          PRInt64 aStartTime,
                          PRInt64 aEndTime,
                          const nsACString &aMimeType,
                          const nsACString &aPreferredApp,
                          nsHandlerInfoAction aPreferredAction);

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

  




  nsresult ResumeRetry(nsDownload *aDl);

  






  nsresult PauseAllDownloads(PRBool aSetResume);

  






  nsresult ResumeAllDownloads(PRBool aResumeAll);

  





  nsresult RemoveAllDownloads();

  





  nsresult RemoveDownloadsForURI(nsIURI *aURI);

  







  static void ResumeOnWakeCallback(nsITimer *aTimer, void *aClosure);
  nsCOMPtr<nsITimer> mResumeOnWakeTimer;

  void ConfirmCancelDownloads(PRInt32 aCount,
                              nsISupportsPRBool *aCancelDownloads,
                              const PRUnichar *aTitle,
                              const PRUnichar *aCancelMessageMultiple,
                              const PRUnichar *aCancelMessageSingle,
                              const PRUnichar *aDontCancelButton);

  PRInt32 GetRetentionBehavior();

  







  enum QuitBehavior {
    QUIT_AND_RESUME = 0, 
    QUIT_AND_PAUSE = 1, 
    QUIT_AND_CANCEL = 2
  };

  




  enum QuitBehavior GetQuitBehavior();

  void OnEnterPrivateBrowsingMode();
  void OnLeavePrivateBrowsingMode();

  
#ifdef DOWNLOAD_SCANNER
private:
  nsDownloadScanner* mScanner;
#endif

private:
  nsCOMArray<nsIDownloadProgressListener> mListeners;
  nsCOMPtr<nsIStringBundle> mBundle;
  nsCOMPtr<mozIStorageConnection> mDBConn;
  nsCOMArray<nsDownload> mCurrentDownloads;
  nsCOMPtr<nsIObserverService> mObserverService;
  nsCOMPtr<mozIStorageStatement> mUpdateDownloadStatement;
  nsCOMPtr<mozIStorageStatement> mGetIdsForURIStatement;
  nsAutoPtr<mozStorageTransaction> mHistoryTransaction;

  enum DatabaseType mDBType;
  PRBool mInPrivateBrowsing;

  static nsDownloadManager *gDownloadManagerService;

  friend class nsDownload;
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

  




  nsresult SetState(DownloadState aState);

protected:
  







  void Finalize();

  



  nsresult ExecuteDesiredAction();

  



  nsresult MoveTempToTarget();

  


  void SetStartTime(PRInt64 aStartTime);

  



  void SetProgressBytes(PRInt64 aCurrBytes, PRInt64 aMaxBytes);

  



  nsresult Pause();

  



  nsresult Cancel();

  


  nsresult Resume();

  


  PRBool IsPaused();

  


  PRBool IsResumable();

  


  PRBool WasResumed();

  


  PRBool ShouldAutoResume();

  


  PRBool IsFinishable();

  


  PRBool IsFinished();

  




  nsresult UpdateDB();

  



  nsresult FailDownload(nsresult aStatus, const PRUnichar *aMessage);

  









  nsresult OpenWithApplication();

  nsDownloadManager *mDownloadManager;
  nsCOMPtr<nsIURI> mTarget;

private:
  nsString mDisplayName;
  nsCString mEntityID;

  nsCOMPtr<nsIURI> mSource;
  nsCOMPtr<nsIURI> mReferrer;
  nsCOMPtr<nsICancelable> mCancelable;
  nsCOMPtr<nsIRequest> mRequest;
  nsCOMPtr<nsILocalFile> mTempFile;
  nsCOMPtr<nsIMIMEInfo> mMIMEInfo;

  DownloadState mDownloadState;
  DownloadType mDownloadType;

  PRUint32 mID;
  PRInt32 mPercentComplete;

  




  PRInt64 mCurrBytes;
  PRInt64 mMaxBytes;

  PRTime mStartTime;
  PRTime mLastUpdate;
  PRInt64 mResumedAt;
  double mSpeed;

  PRBool mHasMultipleFiles;

  






  enum AutoResume { DONT_RESUME, AUTO_RESUME };
  AutoResume mAutoResume;

  friend class nsDownloadManager;
};

#endif
