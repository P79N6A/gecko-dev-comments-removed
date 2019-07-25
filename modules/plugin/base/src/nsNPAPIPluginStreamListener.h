




































#ifndef nsNPAPIPluginStreamListener_h_
#define nsNPAPIPluginStreamListener_h_

#include "nsIPluginStreamListener.h"
#include "nsIPluginStreamInfo.h"
#include "nsIHTTPHeaderListener.h"
#include "nsIRequest.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"

#define MAX_PLUGIN_NECKO_BUFFER 16384

class nsNPAPIPluginInstance;
class nsINPAPIPluginStreamInfo;

class nsNPAPIPluginStreamListener : public nsIPluginStreamListener,
                                 public nsITimerCallback,
                                 public nsIHTTPHeaderListener
{
private:
  typedef mozilla::PluginLibrary PluginLibrary;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGINSTREAMLISTENER
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIHTTPHEADERLISTENER

  
  nsNPAPIPluginStreamListener(nsNPAPIPluginInstance* inst, void* notifyData,
                              const char* aURL);
  virtual ~nsNPAPIPluginStreamListener();
  PRBool IsStarted();
  nsresult CleanUpStream(NPReason reason);
  void CallURLNotify(NPReason reason);
  void SetCallNotify(PRBool aCallNotify)
  {
    mCallNotify = aCallNotify;
  }
  nsresult SuspendRequest();
  void ResumeRequest();
  nsresult StartDataPump();
  void StopDataPump();

  PRBool PluginInitJSLoadInProgress();

protected:
  void* mNotifyData;
  char* mStreamBuffer;
  char* mNotifyURL;
  nsNPAPIPluginInstance* mInst;
  NPStream mNPStream;
  PRUint32 mStreamBufferSize;
  PRInt32 mStreamBufferByteCount;
  PRInt32 mStreamType;
  PRPackedBool mStreamStarted;
  PRPackedBool mStreamCleanedUp;
  PRPackedBool mCallNotify;
  PRPackedBool mIsSuspended;
  PRPackedBool mIsPluginInitJSStream;
  nsCString mResponseHeaders;
  char* mResponseHeaderBuf;

  nsCOMPtr<nsITimer> mDataPumpTimer;

public:
  nsCOMPtr<nsIPluginStreamInfo> mStreamInfo;
};



#define NS_INPAPIPLUGINSTREAMINFO_IID       \
{ 0x097fdaaa, 0xa2a3, 0x49c2, \
  {0x91, 0xee, 0xeb, 0xc5, 0x7d, 0x6c, 0x9c, 0x97} }

class nsINPAPIPluginStreamInfo : public nsIPluginStreamInfo
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INPAPIPLUGINSTREAMINFO_IID)

  nsIRequest *GetRequest()
  {
    return mRequest;
  }

protected:
  nsCOMPtr<nsIRequest> mRequest;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsINPAPIPluginStreamInfo,
                              NS_INPAPIPLUGINSTREAMINFO_IID)

#endif 
