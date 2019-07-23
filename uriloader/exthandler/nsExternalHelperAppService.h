






































#ifndef nsExternalHelperAppService_h__
#define nsExternalHelperAppService_h__

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif
#include "prlog.h"
#include "prtime.h"

#include "nsInt64.h"

#include "nsIExternalHelperAppService.h"
#include "nsIExternalProtocolService.h"
#include "nsIWebProgressListener2.h"
#include "nsIHelperAppLauncherDialog.h"

#include "nsIMIMEInfo.h"
#include "nsMIMEInfoImpl.h"
#include "nsIMIMEService.h"
#include "nsIStreamListener.h"
#include "nsIFile.h"
#include "nsIFileStreams.h"
#include "nsIOutputStream.h"
#include "nsString.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsILocalFile.h"
#include "nsIChannel.h"
#include "nsITimer.h"

#ifdef MOZ_RDF
#include "nsIRDFDataSource.h"
#include "nsIRDFResource.h"
#endif
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsCOMArray.h"
#include "nsWeakReference.h"
#include "nsIPrompt.h"

class nsExternalAppHandler;
class nsIMIMEInfo;
class nsIRDFService;
class nsITransfer;
class nsIDOMWindowInternal;





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
  virtual ~nsExternalHelperAppService();
  





  NS_HIDDEN_(nsresult) InitDataSource();

  



  NS_HIDDEN_(nsresult) Init();
 
  





  NS_HIDDEN_(nsresult) GetMIMEInfoForMimeTypeFromDS(const nsACString& aContentType,
                                                    nsIMIMEInfo * aMIMEInfo);
  
  








  NS_HIDDEN_(nsresult) GetMIMEInfoForExtensionFromDS(const nsACString& aFileExtension,
                                                     nsIMIMEInfo * aMIMEInfo);

  





  NS_HIDDEN_(PRBool) GetTypeFromDS(const nsACString& aFileExtension,
                                   nsACString& aType);

  














  virtual already_AddRefed<nsIMIMEInfo> GetMIMEInfoFromOS(const nsACString& aMIMEType,
                                                          const nsACString& aFileExt,
                                                          PRBool     * aFound) = 0;

  













  virtual nsresult GetFileTokenForPath(const PRUnichar * platformAppPath,
                                       nsIFile ** aFile);

  



  NS_HIDDEN_(PRBool) MIMETypeIsInDataSource(const char * aContentType);

protected:
  



#ifdef MOZ_RDF
  nsCOMPtr<nsIRDFDataSource> mOverRideDataSource;

  nsCOMPtr<nsIRDFResource> kNC_Description;
  nsCOMPtr<nsIRDFResource> kNC_Value;
  nsCOMPtr<nsIRDFResource> kNC_FileExtensions;
  nsCOMPtr<nsIRDFResource> kNC_Path;
  nsCOMPtr<nsIRDFResource> kNC_UseSystemDefault;
  nsCOMPtr<nsIRDFResource> kNC_SaveToDisk;
  nsCOMPtr<nsIRDFResource> kNC_AlwaysAsk;
  nsCOMPtr<nsIRDFResource> kNC_HandleInternal;
  nsCOMPtr<nsIRDFResource> kNC_PrettyName;
#endif

  


  PRBool mDataSourceInitialized;

  




#ifdef MOZ_RDF
  NS_HIDDEN_(nsresult) FillTopLevelProperties(nsIRDFResource * aContentTypeNodeResource, 
                                              nsIRDFService * aRDFService,
                                              nsIMIMEInfo * aMIMEInfo);
  


  NS_HIDDEN_(nsresult) FillContentHandlerProperties(const char * aContentType,
                                                    nsIRDFResource * aContentTypeNodeResource,
                                                    nsIRDFService * aRDFService,
                                                    nsIMIMEInfo * aMIMEInfo);

  




  NS_HIDDEN_(nsresult) FillLiteralValueFromTarget(nsIRDFResource * aSource,
                                                  nsIRDFResource * aProperty,
                                                  const PRUnichar ** aLiteralValue);
#endif

  







  NS_HIDDEN_(nsresult) GetMIMEInfoForMimeTypeFromExtras(const nsACString& aContentType,
                                                        nsIMIMEInfo * aMIMEInfo);
  







  NS_HIDDEN_(nsresult) GetMIMEInfoForExtensionFromExtras(const nsACString& aExtension,
                                                         nsIMIMEInfo * aMIMEInfo);

  





  NS_HIDDEN_(PRBool) GetTypeFromExtras(const nsACString& aExtension,
                                       nsACString& aMIMEType);

  




  virtual void FixFilePermissions(nsILocalFile* aFile);

#ifdef PR_LOGGING
  




  static PRLogModuleInfo* mLog;

#endif
  
  friend class nsExternalAppHandler;
  friend class nsExternalLoadRequest;

  



  NS_HIDDEN_(nsresult) ExpungeTemporaryFiles();
  


  nsCOMArray<nsILocalFile> mTemporaryFilesList;

  


  virtual NS_HIDDEN_(nsresult) LoadUriInternal(nsIURI * aURL) = 0;
  NS_HIDDEN_(PRBool) isExternalLoadOK(nsIURI* aURI, nsIPrompt* aPrompt);
  NS_HIDDEN_(PRBool) promptForScheme(nsIURI* aURI, nsIPrompt* aPrompt, PRBool *aRemember);
};






#define DATA_BUFFER_SIZE (4096*2) 









class nsExternalAppHandler : public nsIStreamListener,
                             public nsIHelperAppLauncher,
                             public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSIHELPERAPPLAUNCHER
  NS_DECL_NSICANCELABLE
  NS_DECL_NSITIMERCALLBACK

  









  nsExternalAppHandler(nsIMIMEInfo * aMIMEInfo, const nsCSubstring& aFileExtension,
                       nsIInterfaceRequestor * aWindowContext,
                       const nsAString& aFilename,
                       PRUint32 aReason);

  ~nsExternalAppHandler();

protected:
  nsCOMPtr<nsIFile> mTempFile;
  nsCOMPtr<nsIURI> mSourceUrl;
  nsString mTempFileExtension;
  


  nsCOMPtr<nsIMIMEInfo> mMimeInfo;
  nsCOMPtr<nsIOutputStream> mOutStream; 
  nsCOMPtr<nsIInterfaceRequestor> mWindowContext;

  



  nsCOMPtr<nsIDOMWindowInternal> mWindowToClose;
  nsCOMPtr<nsITimer> mTimer;

  




  nsString mSuggestedFileName;

  



  PRPackedBool mCanceled;

  




  PRPackedBool mShouldCloseWindow;

  



  PRPackedBool mReceivedDispositionInfo;
  PRPackedBool mStopRequestIssued; 
  PRPackedBool mProgressListenerInitialized;

  PRPackedBool mIsFileChannel;

  




  PRUint32 mReason;

  PRTime mTimeDownloadStarted;
  nsInt64 mContentLength;
  nsInt64 mProgress; 

  




  nsCOMPtr<nsIFile> mFinalFileDestination;

  char mDataBuffer[DATA_BUFFER_SIZE];

  



  nsresult SetUpTempFile(nsIChannel * aChannel);
  





  void RetargetLoadNotifications(nsIRequest *request); 
  





  nsresult CreateProgressListener();
  nsresult PromptForSaveToFile(nsILocalFile ** aNewFile,
                               const nsAFlatString &aDefaultFile,
                               const nsAFlatString &aDefaultFileExt);

  






  void ProcessAnyRefreshTags();

  



  nsresult MoveFile(nsIFile * aNewFileLocation);
  




  nsresult OpenWithApplication();
  
  



  nsresult ExecuteDesiredAction();
  


  PRBool GetNeverAskFlagFromPref(const char * prefName, const char * aContentType);

  


  nsresult InitializeDownload(nsITransfer*);
  
  




  void EnsureSuggestedFileName();

  typedef enum { kReadError, kWriteError, kLaunchError } ErrorType;
  


  void SendStatusChange(ErrorType type, nsresult aStatus, nsIRequest *aRequest, const nsAFlatString &path);

  




  nsresult MaybeCloseWindow();

  nsCOMPtr<nsIWebProgressListener2> mWebProgressListener;
  nsCOMPtr<nsIChannel> mOriginalChannel; 
  nsCOMPtr<nsIHelperAppLauncherDialog> mDialog;

  



  nsIRequest*  mRequest;
};

#endif 
