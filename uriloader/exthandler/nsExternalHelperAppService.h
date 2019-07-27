




#ifndef nsExternalHelperAppService_h__
#define nsExternalHelperAppService_h__

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif
#include "prlog.h"
#include "prtime.h"

#include "nsIExternalHelperAppService.h"
#include "nsIExternalProtocolService.h"
#include "nsIWebProgressListener2.h"
#include "nsIHelperAppLauncherDialog.h"

#include "nsIMIMEInfo.h"
#include "nsIMIMEService.h"
#include "nsIStreamListener.h"
#include "nsIFile.h"
#include "nsIFileStreams.h"
#include "nsIOutputStream.h"
#include "nsString.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIChannel.h"
#include "nsITimer.h"
#include "nsIBackgroundFileSaver.h"

#include "nsIHandlerService.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsCOMArray.h"
#include "nsWeakReference.h"
#include "nsIPrompt.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"

class nsExternalAppHandler;
class nsIMIMEInfo;
class nsITransfer;
class nsIDOMWindow;





class nsExternalHelperAppService
: public nsIExternalHelperAppService,
  public nsPIExternalAppLauncher,
  public nsIExternalProtocolService,
  public nsIMIMEService,
  public nsIObserver,
  public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEXTERNALHELPERAPPSERVICE
  NS_DECL_NSPIEXTERNALAPPLAUNCHER
  NS_DECL_NSIEXTERNALPROTOCOLSERVICE
  NS_DECL_NSIMIMESERVICE
  NS_DECL_NSIOBSERVER

  nsExternalHelperAppService();

  



  nsresult Init();
 
  














  virtual already_AddRefed<nsIMIMEInfo> GetMIMEInfoFromOS(const nsACString& aMIMEType,
                                                          const nsACString& aFileExt,
                                                          bool       * aFound) = 0;

  













  virtual nsresult GetFileTokenForPath(const char16_t * platformAppPath,
                                       nsIFile ** aFile);

  virtual nsresult OSProtocolHandlerExists(const char *aScheme,
                                                       bool *aExists) = 0;

protected:
  virtual ~nsExternalHelperAppService();

  







  nsresult FillMIMEInfoForMimeTypeFromExtras(
    const nsACString& aContentType, nsIMIMEInfo * aMIMEInfo);
  







  nsresult FillMIMEInfoForExtensionFromExtras(
    const nsACString& aExtension, nsIMIMEInfo * aMIMEInfo);

  





  bool GetTypeFromExtras(const nsACString& aExtension,
                                       nsACString& aMIMEType);

#ifdef PR_LOGGING
  




  static PRLogModuleInfo* mLog;

#endif
  
  friend class nsExternalAppHandler;

  


  static void ExpungeTemporaryFilesHelper(nsCOMArray<nsIFile> &fileList);
  


  static nsresult DeleteTemporaryFileHelper(nsIFile* aTemporaryFile,
                                            nsCOMArray<nsIFile> &aFileList);
  



  void ExpungeTemporaryFiles();
  




  void ExpungeTemporaryPrivateFiles();

  


  nsCOMArray<nsIFile> mTemporaryFilesList;
  



  nsCOMArray<nsIFile> mTemporaryPrivateFilesList;

private:
  nsresult DoContentContentProcessHelper(const nsACString& aMimeContentType,
                                         nsIRequest *aRequest,
                                         nsIInterfaceRequestor *aContentContext,
                                         bool aForceSave,
                                         nsIInterfaceRequestor *aWindowContext,
                                         nsIStreamListener ** aStreamListener);
};









class nsExternalAppHandler MOZ_FINAL : public nsIStreamListener,
                                       public nsIHelperAppLauncher,
                                       public nsITimerCallback,
                                       public nsIBackgroundFileSaverObserver
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSIHELPERAPPLAUNCHER
  NS_DECL_NSICANCELABLE
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIBACKGROUNDFILESAVEROBSERVER

  














  nsExternalAppHandler(nsIMIMEInfo * aMIMEInfo, const nsCSubstring& aFileExtension,
                       nsIInterfaceRequestor * aContentContext,
                       nsIInterfaceRequestor * aWindowContext,
                       nsExternalHelperAppService * aExtProtSvc,
                       const nsAString& aFilename,
                       uint32_t aReason, bool aForceSave);

  


  void DidDivertRequest(nsIRequest *request);

protected:
  ~nsExternalAppHandler();

  nsIInterfaceRequestor* GetDialogParent() {
    return mWindowContext ? mWindowContext : mContentContext;
  }

  nsCOMPtr<nsIFile> mTempFile;
  nsCOMPtr<nsIURI> mSourceUrl;
  nsString mTempFileExtension;
  nsString mTempLeafName;

  


  nsCOMPtr<nsIMIMEInfo> mMimeInfo;

  


  nsCOMPtr<nsIInterfaceRequestor> mContentContext;

  



  nsCOMPtr<nsIInterfaceRequestor> mWindowContext;

  



  nsCOMPtr<nsIDOMWindow> mWindowToClose;
  nsCOMPtr<nsITimer> mTimer;

  




  nsString mSuggestedFileName;

  




  bool mForceSave;
  
  



  bool mCanceled;

  




  bool mShouldCloseWindow;

  


  bool mStopRequestIssued; 

  bool mIsFileChannel;

  




  uint32_t mReason;

  


  bool mTempFileIsExecutable;

  PRTime mTimeDownloadStarted;
  int64_t mContentLength;
  int64_t mProgress; 

  




  nsCOMPtr<nsIFile> mFinalFileDestination;

  uint32_t mBufferSize;

  




  nsCOMPtr<nsIBackgroundFileSaver> mSaver;

  


  nsAutoCString mHash;
  




  nsCOMPtr<nsIArray> mSignatureInfo;
  


  nsCOMPtr<nsIArray> mRedirects;
  



  nsresult SetUpTempFile(nsIChannel * aChannel);
  





  void RetargetLoadNotifications(nsIRequest *request);
  




  nsresult CreateTransfer();

  



  nsresult CreateFailedTransfer(bool aIsPrivateBrowsing);

  










  



  void RequestSaveDestination(const nsAFlatString &aDefaultFile,
                              const nsAFlatString &aDefaultFileExt);

  






  nsresult ContinueSave(nsIFile* aFile);

  






  void ProcessAnyRefreshTags();

  







  void NotifyTransfer(nsresult aStatus);

  


  bool GetNeverAskFlagFromPref(const char * prefName, const char * aContentType);

  




  void EnsureSuggestedFileName();

  typedef enum { kReadError, kWriteError, kLaunchError } ErrorType;
  


  void SendStatusChange(ErrorType type, nsresult aStatus, nsIRequest *aRequest, const nsAFlatString &path);

  




  nsresult MaybeCloseWindow();

  



  nsCOMPtr<nsIWebProgressListener2> mDialogProgressListener;
  



  nsCOMPtr<nsITransfer> mTransfer;

  nsCOMPtr<nsIChannel> mOriginalChannel; 
  nsCOMPtr<nsIHelperAppLauncherDialog> mDialog;

  



  bool mKeepRequestAlive;

  




  nsCOMPtr<nsIRequest> mRequest;

  nsRefPtr<nsExternalHelperAppService> mExtProtSvc;
};

#endif 
