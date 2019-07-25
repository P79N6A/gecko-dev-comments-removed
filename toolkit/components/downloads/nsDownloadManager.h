




#ifndef downloadmanager___h___
#define downloadmanager___h___

#if defined(XP_WIN)
#define DOWNLOAD_SCANNER
#endif

#include "nsIDownload.h"
#include "nsIDownloadManager.h"
#include "nsIDownloadProgressListener.h"
#include "nsIFile.h"
#include "nsIMIMEInfo.h"
#include "nsINavHistoryService.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIStringBundle.h"
#include "nsISupportsPrimitives.h"
#include "nsWeakReference.h"
#include "nsITimer.h"
#include "nsString.h"

#include "mozStorageHelper.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"

typedef int16_t DownloadState;
typedef int16_t DownloadType;

class nsDownload;

#ifdef DOWNLOAD_SCANNER
#include "nsDownloadScanner.h"
#endif

class nsDownloadManager : public nsIDownloadManager,
                          public nsINavHistoryObserver,
                          public nsIObserver,
                          public nsSupportsWeakReference
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
    , mInPrivateBrowsing(false)
#ifdef DOWNLOAD_SCANNER
    , mScanner(nullptr)
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
  void CloseDB();
  nsresult InitMemoryDB();
  already_AddRefed<mozIStorageConnection> GetFileDBConnection(nsIFile *dbFile) const;
  already_AddRefed<mozIStorageConnection> GetMemoryDBConnection() const;
  nsresult SwitchDatabaseTypeTo(enum DatabaseType aType);
  nsresult CreateTable();

  




  nsresult RestoreDatabaseState();

  



  nsresult RestoreActiveDownloads();

  nsresult GetDownloadFromDB(uint32_t aID, nsDownload **retVal);

  



  nsresult AddToCurrentDownloads(nsDownload *aDl);

  void SendEvent(nsDownload *aDownload, const char *aTopic);

  




  int64_t AddDownloadToDB(const nsAString &aName,
                          const nsACString &aSource,
                          const nsACString &aTarget,
                          const nsAString &aTempPath,
                          int64_t aStartTime,
                          int64_t aEndTime,
                          const nsACString &aMimeType,
                          const nsACString &aPreferredApp,
                          nsHandlerInfoAction aPreferredAction);

  void NotifyListenersOnDownloadStateChange(int16_t aOldState,
                                            nsIDownload *aDownload);
  void NotifyListenersOnProgressChange(nsIWebProgress *aProgress,
                                       nsIRequest *aRequest,
                                       int64_t aCurSelfProgress,
                                       int64_t aMaxSelfProgress,
                                       int64_t aCurTotalProgress,
                                       int64_t aMaxTotalProgress,
                                       nsIDownload *aDownload);
  void NotifyListenersOnStateChange(nsIWebProgress *aProgress,
                                    nsIRequest *aRequest,
                                    uint32_t aStateFlags,
                                    nsresult aStatus,
                                    nsIDownload *aDownload);

  nsDownload *FindDownload(uint32_t aID);

  




  nsresult ResumeRetry(nsDownload *aDl);

  






  nsresult PauseAllDownloads(bool aSetResume);

  






  nsresult ResumeAllDownloads(bool aResumeAll);

  





  nsresult RemoveAllDownloads();

  





  nsresult RemoveDownloadsForURI(nsIURI *aURI);

  







  static void ResumeOnWakeCallback(nsITimer *aTimer, void *aClosure);
  nsCOMPtr<nsITimer> mResumeOnWakeTimer;

  void ConfirmCancelDownloads(int32_t aCount,
                              nsISupportsPRBool *aCancelDownloads,
                              const PRUnichar *aTitle,
                              const PRUnichar *aCancelMessageMultiple,
                              const PRUnichar *aCancelMessageSingle,
                              const PRUnichar *aDontCancelButton);

  int32_t GetRetentionBehavior();

  







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
  bool mInPrivateBrowsing;

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

  


  void SetStartTime(int64_t aStartTime);

  



  void SetProgressBytes(int64_t aCurrBytes, int64_t aMaxBytes);

  



  nsresult Pause();

  



  nsresult Cancel();

  


  nsresult Resume();

  


  bool IsPaused();

  


  bool IsResumable();

  


  bool WasResumed();

  


  bool ShouldAutoResume();

  


  bool IsFinishable();

  


  bool IsFinished();

  




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
  nsCOMPtr<nsIFile> mTempFile;
  nsCOMPtr<nsIMIMEInfo> mMIMEInfo;

  DownloadState mDownloadState;
  DownloadType mDownloadType;

  uint32_t mID;
  int32_t mPercentComplete;

  




  int64_t mCurrBytes;
  int64_t mMaxBytes;

  PRTime mStartTime;
  PRTime mLastUpdate;
  int64_t mResumedAt;
  double mSpeed;

  bool mHasMultipleFiles;

  






  enum AutoResume { DONT_RESUME, AUTO_RESUME };
  AutoResume mAutoResume;

  friend class nsDownloadManager;
};

#endif
