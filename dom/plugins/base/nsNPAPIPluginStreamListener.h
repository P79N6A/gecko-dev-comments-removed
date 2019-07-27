




#ifndef nsNPAPIPluginStreamListener_h_
#define nsNPAPIPluginStreamListener_h_

#include "nscore.h"
#include "nsIHTTPHeaderListener.h"
#include "nsIRequest.h"
#include "nsITimer.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsIOutputStream.h"
#include "nsIPluginInstanceOwner.h"
#include "nsString.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "mozilla/PluginLibrary.h"

#define MAX_PLUGIN_NECKO_BUFFER 16384

class nsPluginStreamListenerPeer;
class nsNPAPIPluginStreamListener;
class nsNPAPIPluginInstance;
class nsIChannel;

class nsNPAPIStreamWrapper
{
public:
  nsNPAPIStreamWrapper(nsIOutputStream *outputStream,
                       nsNPAPIPluginStreamListener *streamListener);
  ~nsNPAPIStreamWrapper();

  nsIOutputStream* GetOutputStream() { return mOutputStream.get(); }
  nsNPAPIPluginStreamListener* GetStreamListener() { return mStreamListener; }

  NPStream                              mNPStream;
protected:
  nsCOMPtr<nsIOutputStream>             mOutputStream; 
  nsNPAPIPluginStreamListener*          mStreamListener; 
};




class nsPluginStreamToFile : public nsIOutputStream
{
public:
  nsPluginStreamToFile(const char* target, nsIPluginInstanceOwner* owner);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOUTPUTSTREAM
protected:
  virtual ~nsPluginStreamToFile();
  char* mTarget;
  nsCString mFileURL;
  nsCOMPtr<nsIFile> mTempFile;
  nsCOMPtr<nsIOutputStream> mOutputStream;
  nsIPluginInstanceOwner* mOwner;
};

class nsNPAPIPluginStreamListener : public nsITimerCallback,
                                    public nsIHTTPHeaderListener
{
private:
  typedef mozilla::PluginLibrary PluginLibrary;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIHTTPHEADERLISTENER

  nsNPAPIPluginStreamListener(nsNPAPIPluginInstance* inst, void* notifyData,
                              const char* aURL);

  nsresult OnStartBinding(nsPluginStreamListenerPeer* streamPeer);
  nsresult OnDataAvailable(nsPluginStreamListenerPeer* streamPeer,
                           nsIInputStream* input,
                           uint32_t length);
  nsresult OnFileAvailable(nsPluginStreamListenerPeer* streamPeer, 
                           const char* fileName);
  nsresult OnStopBinding(nsPluginStreamListenerPeer* streamPeer, 
                         nsresult status);
  nsresult GetStreamType(int32_t *result);
  bool SetStreamType(uint16_t aType, bool aNeedsResume = true);

  bool IsStarted();
  nsresult CleanUpStream(NPReason reason);
  void CallURLNotify(NPReason reason);
  void SetCallNotify(bool aCallNotify) { mCallNotify = aCallNotify; }
  void SuspendRequest();
  void ResumeRequest();
  nsresult StartDataPump();
  void StopDataPump();
  bool PluginInitJSLoadInProgress();

  void* GetNotifyData();
  nsPluginStreamListenerPeer* GetStreamListenerPeer() { return mStreamListenerPeer; }
  void SetStreamListenerPeer(nsPluginStreamListenerPeer* aPeer) { mStreamListenerPeer = aPeer; }

  
  bool HandleRedirectNotification(nsIChannel *oldChannel, nsIChannel *newChannel,
                                  nsIAsyncVerifyRedirectCallback* callback);
  void URLRedirectResponse(NPBool allow);

protected:

  enum StreamState
  {
    eStreamStopped = 0, 
    eNewStreamCalled,   
    eStreamTypeSet      
  };

  enum StreamStopMode
  {
    eNormalStop = 0,
    eDoDeferredStop,
    eStopPending
  };

  virtual ~nsNPAPIPluginStreamListener();
  bool MaybeRunStopBinding();

  char* mStreamBuffer;
  char* mNotifyURL;
  nsRefPtr<nsNPAPIPluginInstance> mInst;
  nsNPAPIStreamWrapper *mNPStreamWrapper;
  uint32_t mStreamBufferSize;
  int32_t mStreamBufferByteCount;
  int32_t mStreamType;
  StreamState mStreamState;
  bool mStreamCleanedUp;
  bool mCallNotify;
  bool mIsSuspended;
  bool mIsPluginInitJSStream;
  bool mRedirectDenied;
  nsCString mResponseHeaders;
  char* mResponseHeaderBuf;
  nsCOMPtr<nsITimer> mDataPumpTimer;
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mHTTPRedirectCallback;
  StreamStopMode mStreamStopMode;
  nsresult mPendingStopBindingStatus;

public:
  nsRefPtr<nsPluginStreamListenerPeer> mStreamListenerPeer;
};

#endif
