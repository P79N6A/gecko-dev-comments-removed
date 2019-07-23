




































#ifndef ns4xPluginStreamListener_h__
#define ns4xPluginStreamListener_h__

#include "nsIPluginStreamListener.h"
#include "nsIPluginStreamInfo.h"
#include "nsIHTTPHeaderListener.h"
#include "nsIRequest.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"

#define MAX_PLUGIN_NECKO_BUFFER 16384

class ns4xPluginInstance;
class nsI4xPluginStreamInfo;

class ns4xPluginStreamListener : public nsIPluginStreamListener,
                                 public nsITimerCallback,
                                 public nsIHTTPHeaderListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGINSTREAMLISTENER
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIHTTPHEADERLISTENER

  
  ns4xPluginStreamListener(nsIPluginInstance* inst, void* notifyData,
                           const char* aURL);
  virtual ~ns4xPluginStreamListener();
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

protected:
  void* mNotifyData;
  char* mStreamBuffer;
  char* mNotifyURL;
  ns4xPluginInstance* mInst;
  NPStream mNPStream;
  PRUint32 mStreamBufferSize;
  PRInt32 mStreamBufferByteCount;
  nsPluginStreamType mStreamType;
  PRPackedBool mStreamStarted;
  PRPackedBool mStreamCleanedUp;
  PRPackedBool mCallNotify;
  PRPackedBool mIsSuspended;
  nsCString mResponseHeaders;
  char* mResponseHeaderBuf;

  nsCOMPtr<nsITimer> mDataPumpTimer;

public:
  nsCOMPtr<nsIPluginStreamInfo> mStreamInfo;
};



#define NS_I4XPLUGINSTREAMINFO_IID       \
{ 0x097fdaaa, 0xa2a3, 0x49c2, \
  {0x91, 0xee, 0xeb, 0xc5, 0x7d, 0x6c, 0x9c, 0x97} }

class nsI4xPluginStreamInfo : public nsIPluginStreamInfo
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_I4XPLUGINSTREAMINFO_IID)

  nsIRequest *GetRequest()
  {
    return mRequest;
  }

protected:
  nsCOMPtr<nsIRequest> mRequest;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsI4xPluginStreamInfo,
                              NS_I4XPLUGINSTREAMINFO_IID)

#endif
