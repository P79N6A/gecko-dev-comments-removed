










#ifndef BackgroundFileSaver_h__
#define BackgroundFileSaver_h__

#include "mozilla/Mutex.h"
#include "nsCOMPtr.h"
#include "nsIAsyncOutputStream.h"
#include "nsIBackgroundFileSaver.h"
#include "nsIStreamListener.h"
#include "nsStreamUtils.h"

class nsIAsyncInputStream;
class nsIThread;

namespace mozilla {
namespace net {




class BackgroundFileSaver : public nsIBackgroundFileSaver
{
public:
  NS_DECL_NSIBACKGROUNDFILESAVER

  BackgroundFileSaver();

  





  nsresult Init();

protected:
  virtual ~BackgroundFileSaver();

  


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

  
  

  


  nsCOMPtr<nsIFile> mActualTarget;

  



  bool mActualTargetKeepPartial;

  
  

  







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

} 
} 

#endif
