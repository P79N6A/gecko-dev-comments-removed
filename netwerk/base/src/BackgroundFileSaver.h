










#ifndef BackgroundFileSaver_h__
#define BackgroundFileSaver_h__

#include "mozilla/Mutex.h"
#include "nsCOMPtr.h"
#include "nsNSSShutDown.h"
#include "nsIAsyncOutputStream.h"
#include "nsIBackgroundFileSaver.h"
#include "nsIStreamListener.h"
#include "nsStreamUtils.h"
#include "ScopedNSSTypes.h"

class nsIAsyncInputStream;
class nsIThread;

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

  


  void virtualDestroyNSSReference();

protected:
  virtual ~BackgroundFileSaver();

  


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

  




  nsCOMPtr<nsIFile> mAssignedTarget;

  



  bool mAssignedTargetKeepPartial;

  



  nsCOMPtr<nsISupports> mAsyncCopyContext;

  



  nsAutoCString mSha256;

  



  bool mSha256Enabled;

  
  

  


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
};




class BackgroundFileSaverOutputStream : public BackgroundFileSaver
                                      , public nsIAsyncOutputStream
                                      , public nsIOutputStreamCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOUTPUTSTREAM
  NS_DECL_NSIASYNCOUTPUTSTREAM
  NS_DECL_NSIOUTPUTSTREAMCALLBACK

  BackgroundFileSaverOutputStream();

protected:
  virtual bool HasInfiniteBuffer() MOZ_OVERRIDE;
  virtual nsAsyncCopyProgressFun GetProgressCallback() MOZ_OVERRIDE;

private:
  ~BackgroundFileSaverOutputStream();

  


  nsCOMPtr<nsIOutputStreamCallback> mAsyncWaitCallback;
};





class BackgroundFileSaverStreamListener : public BackgroundFileSaver
                                        , public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  BackgroundFileSaverStreamListener();

protected:
  virtual bool HasInfiniteBuffer() MOZ_OVERRIDE;
  virtual nsAsyncCopyProgressFun GetProgressCallback() MOZ_OVERRIDE;

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
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOUTPUTSTREAM
  
  DigestOutputStream(nsIOutputStream* outputStream, PK11Context* aContext);
  ~DigestOutputStream();

  
  void virtualDestroyNSSReference() { }

private:
  
  nsCOMPtr<nsIOutputStream> mOutputStream;
  
  PK11Context* mDigestContext;

  
  DigestOutputStream(const DigestOutputStream& d);
};
} 
} 

#endif
