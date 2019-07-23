





































 
#ifndef downloadmanager___h___
#define downloadmanager___h___

#include "nsIDownloadManager.h"
#include "nsIDownloadProgressListener.h"
#include "nsIDownload.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIRDFService.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEventListener.h"
#include "nsIRDFContainerUtils.h"
#include "nsIURI.h"
#include "nsILocalFile.h"
#include "nsRefPtrHashtable.h"
#include "nsIRequest.h"
#include "nsIObserver.h"
#include "nsIStringBundle.h"
#include "nsIProgressDialog.h"
#include "nsIMIMEInfo.h"
#include "nsISound.h"
#include "nsAutoPtr.h"
 
enum DownloadState { NOTSTARTED = -1, DOWNLOADING, FINISHED, FAILED, CANCELED };

class nsDownload;

class nsDownloadManager : public nsIDownloadManager,
                          public nsIDOMEventListener,
                          public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOWNLOADMANAGER
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_NSIOBSERVER

  nsresult Init();

  nsDownloadManager();
  ~nsDownloadManager();

protected:
  nsresult GetDownloadsContainer(nsIRDFContainer** aResult);
  nsresult GetProfileDownloadsFileURL(nsCString& aDownloadsFileURL);
  nsresult GetInternalListener(nsIDownloadProgressListener** aInternalListener);
  nsresult GetDataSource(nsIRDFDataSource** aDataSource);
  nsresult AssertProgressInfo();
  nsresult AssertProgressInfoFor(const nsACString& aTargetPath);
  nsresult DownloadStarted(const nsACString& aTargetPath);
  nsresult DownloadEnded(const nsACString& aTargetPath, const PRUnichar* aMessage);
  PRBool MustUpdateUI() { if (mDocument) return PR_TRUE; return PR_FALSE; }

  nsCOMPtr<nsISound> mSoundInterface;

private:
  nsCOMPtr<nsIRDFDataSource> mDataSource;
  nsCOMPtr<nsIDOMDocument> mDocument;
  nsCOMPtr<nsIRDFContainer> mDownloadsContainer;
  nsCOMPtr<nsIDownloadProgressListener> mListener;
  nsCOMPtr<nsIRDFContainerUtils> mRDFContainerUtils;
  nsCOMPtr<nsIStringBundle> mBundle;
  PRInt32 mBatches;
  nsRefPtrHashtable<nsCStringHashKey, nsDownload> mCurrDownloads;

  friend class nsDownload;
};

class nsDownload : public nsIDownload,
                   public nsIObserver
{
public:
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSIWEBPROGRESSLISTENER2
  NS_DECL_NSITRANSFER
  NS_DECL_NSIDOWNLOAD
  NS_DECL_NSIOBSERVER
  NS_DECL_ISUPPORTS

  nsDownload(nsDownloadManager* aManager, nsIURI* aTarget, nsIURI* aSource,
             nsICancelable* aCancelable);
  ~nsDownload();

  nsresult Cancel();
  nsresult Suspend();
  nsresult SetDisplayName(const PRUnichar* aDisplayName);
  nsresult SetTempFile(nsILocalFile* aTempFile);
  nsresult Resume();
  void DisplayDownloadFinishedAlert();

  void SetDialogListener(nsIDownloadProgressListener* aInternalListener) {
    mDialogListener = aInternalListener;
  }
  void SetDialog(nsIProgressDialog* aDialog) {
    mDialog = aDialog;
  }
  
  nsIProgressDialog* GetDialog() {
    return mDialog;
  }

  struct TransferInformation {
    PRInt64 mCurrBytes, mMaxBytes;
    TransferInformation(PRInt64 aCurr, PRInt64 aMax) :
      mCurrBytes(aCurr),
      mMaxBytes(aMax)
      {}
  };

  TransferInformation GetTransferInformation() {
    return TransferInformation(mCurrBytes, mMaxBytes);
  }
  DownloadState GetDownloadState() {
    return mDownloadState;
  }
  void SetDownloadState(DownloadState aState) {
    mDownloadState = aState;
  }
  void SetMIMEInfo(nsIMIMEInfo* aMIMEInfo) {
    mMIMEInfo = aMIMEInfo;
  }
  void SetStartTime(PRInt64 aStartTime) {
    mStartTime = aStartTime;
    mLastUpdate = aStartTime;
  }
private:
  nsRefPtr<nsDownloadManager> mDownloadManager;

  nsString mDisplayName;

  nsCOMPtr<nsIURI> mTarget;
  nsCOMPtr<nsIURI> mSource;
  nsCOMPtr<nsIDownloadProgressListener> mDialogListener;
  nsCOMPtr<nsICancelable> mCancelable;
  nsCOMPtr<nsIRequest> mRequest;
  nsCOMPtr<nsIProgressDialog> mDialog;
  nsCOMPtr<nsIMIMEInfo> mMIMEInfo;
  nsCOMPtr<nsILocalFile> mTempFile;
  DownloadState mDownloadState;

  PRInt32 mPercentComplete;
  PRUint64 mCurrBytes;
  PRUint64 mMaxBytes;
  PRTime mStartTime;
  PRTime mLastUpdate;
  double mSpeed;
};

#endif
