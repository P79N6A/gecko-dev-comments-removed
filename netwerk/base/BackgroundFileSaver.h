










#ifndef BackgroundFileSaver_h__
#define BackgroundFileSaver_h__

#include "mozilla/Mutex.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsNSSShutDown.h"
#include "nsIAsyncOutputStream.h"
#include "nsIBackgroundFileSaver.h"
#include "nsIStreamListener.h"
#include "nsStreamUtils.h"
#include "ScopedNSSTypes.h"

class nsIAsyncInputStream;
class nsIThread;
class nsIX509CertList;
struct PRLogModuleInfo;

namespace mozilla {
namespace net {

class DigestOutputStream;




class BackgroundFileSaver : public nsIBackgroundFileSaver,
                            public nsNSSShutDownObject
{
public:
  NS_DECL_NSIBACKGROUNDFILESAVER

  BackgroundFileSaver();

  





  nsresult Init();

  


  void virtualDestroyNSSReference() override;

  


  static uint32_t sThreadCount;

  






  static uint32_t sTelemetryMaxThreadCount;


protected:
  virtual ~BackgroundFileSaver();

  static PRLogModuleInfo *prlog;

  


  void destructorSafeDestroyNSSReference();

  


  nsCOMPtr<nsIThread> mControlThread;

  


  nsCOMPtr<nsIThread> mWorkerThread;

  




  nsCOMPtr<nsIAsyncOutputStream> mPipeOutputStream;

  





  virtual bool HasInfiniteBuffer() = 0;

  


  virtual nsAsyncCopyProgressFun GetProgressCallback() = 0;

  


  nsCOMPtr<nsIAsyncInputStream> mPipeInputStream;

private:
  friend class NotifyTargetChangeRunnable;

  





  nsCOMPtr<nsIBackgroundFileSaverObserver> mObserver;

  
  

  



  mozilla::Mutex mLock;

  


  bool mWorkerThreadAttentionRequested;

  


  bool mFinishRequested;

  


  bool mComplete;

  





  nsresult mStatus;

  



  bool mAppend;

  





  nsCOMPtr<nsIFile> mInitialTarget;

  




  bool mInitialTargetKeepPartial;

  








  nsCOMPtr<nsIFile> mRenamedTarget;

  




  bool mRenamedTargetKeepPartial;

  



  nsCOMPtr<nsISupports> mAsyncCopyContext;

  



  nsAutoCString mSha256;

  



  bool mSha256Enabled;

  


  nsCOMArray<nsIX509CertList> mSignatureInfo;

  



  bool mSignatureInfoEnabled;

  
  

  


  nsCOMPtr<nsIFile> mActualTarget;

  



  bool mActualTargetKeepPartial;

  



  ScopedPK11Context mDigestContext;

  
  

  







  static void AsyncCopyCallback(void *aClosure, nsresult aStatus);

  






  nsresult GetWorkerThreadAttention(bool aShouldInterruptCopy);

  


  nsresult ProcessAttention();

  




  nsresult ProcessStateChange();

  



  bool CheckCompletion();

  



  nsresult NotifyTargetChange(nsIFile *aTarget);

  


  nsresult NotifySaveComplete();

  




  nsresult ExtractSignatureInfo(const nsAString& filePath);
};




class BackgroundFileSaverOutputStream : public BackgroundFileSaver
                                      , public nsIAsyncOutputStream
                                      , public nsIOutputStreamCallback
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOUTPUTSTREAM
  NS_DECL_NSIASYNCOUTPUTSTREAM
  NS_DECL_NSIOUTPUTSTREAMCALLBACK

  BackgroundFileSaverOutputStream();

protected:
  virtual bool HasInfiniteBuffer() override;
  virtual nsAsyncCopyProgressFun GetProgressCallback() override;

private:
  ~BackgroundFileSaverOutputStream();

  


  nsCOMPtr<nsIOutputStreamCallback> mAsyncWaitCallback;
};





class BackgroundFileSaverStreamListener final : public BackgroundFileSaver
                                              , public nsIStreamListener
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  BackgroundFileSaverStreamListener();

protected:
  virtual bool HasInfiniteBuffer() override;
  virtual nsAsyncCopyProgressFun GetProgressCallback() override;

private:
  ~BackgroundFileSaverStreamListener();

  


  mozilla::Mutex mSuspensionLock;

  


  bool mReceivedTooMuchData;

  



  nsCOMPtr<nsIRequest> mRequest;

  


  bool mRequestSuspended;

  


  static void AsyncCopyProgressCallback(void *aClosure, uint32_t aCount);

  


  nsresult NotifySuspendOrResume();
};




class DigestOutputStream : public nsNSSShutDownObject,
                           public nsIOutputStream
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOUTPUTSTREAM
  
  DigestOutputStream(nsIOutputStream* outputStream, PK11Context* aContext);

  
  void virtualDestroyNSSReference() override { }

private:
  ~DigestOutputStream();

  
  nsCOMPtr<nsIOutputStream> mOutputStream;
  
  PK11Context* mDigestContext;

  
  DigestOutputStream(const DigestOutputStream& d);
};

} 
} 

#endif
