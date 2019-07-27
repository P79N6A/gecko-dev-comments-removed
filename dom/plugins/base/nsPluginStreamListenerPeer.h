




#ifndef nsPluginStreamListenerPeer_h_
#define nsPluginStreamListenerPeer_h_

#include "nscore.h"
#include "nsIFile.h"
#include "nsIStreamListener.h"
#include "nsIProgressEventSink.h"
#include "nsIHttpHeaderVisitor.h"
#include "nsWeakReference.h"
#include "nsNPAPIPluginStreamListener.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIInterfaceRequestor.h"
#include "nsIChannelEventSink.h"

class nsIChannel;







class CachedFileHolder
{
public:
  explicit CachedFileHolder(nsIFile* cacheFile);
  ~CachedFileHolder();

  void AddRef();
  void Release();

  nsIFile* file() const { return mFile; }

private:
  nsAutoRefCnt mRefCnt;
  nsCOMPtr<nsIFile> mFile;
};

class nsPluginStreamListenerPeer : public nsIStreamListener,
public nsIProgressEventSink,
public nsIHttpHeaderVisitor,
public nsSupportsWeakReference,
public nsIInterfaceRequestor,
public nsIChannelEventSink
{
  virtual ~nsPluginStreamListenerPeer();

public:
  nsPluginStreamListenerPeer();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROGRESSEVENTSINK
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIHTTPHEADERVISITOR
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

  
  void
  MakeByteRangeString(NPByteRange* aRangeList, nsACString &string, int32_t *numRequests);

  bool UseExistingPluginCacheFile(nsPluginStreamListenerPeer* psi);

  
  
  nsresult Initialize(nsIURI *aURL,
                      nsNPAPIPluginInstance *aInstance,
                      nsNPAPIPluginStreamListener *aListener);

  nsresult OnFileAvailable(nsIFile* aFile);

  nsresult ServeStreamAsFile(nsIRequest *request, nsISupports *ctxt);

  nsNPAPIPluginInstance *GetPluginInstance() { return mPluginInstance; }

  nsresult RequestRead(NPByteRange* rangeList);
  nsresult GetLength(uint32_t* result);
  nsresult GetURL(const char** result);
  nsresult GetLastModified(uint32_t* result);
  nsresult IsSeekable(bool* result);
  nsresult GetContentType(char** result);
  nsresult GetStreamOffset(int32_t* result);
  nsresult SetStreamOffset(int32_t value);

  void TrackRequest(nsIRequest* request)
  {
    mRequests.AppendObject(request);
  }

  void ReplaceRequest(nsIRequest* oldRequest, nsIRequest* newRequest)
  {
    int32_t i = mRequests.IndexOfObject(oldRequest);
    if (i == -1) {
      NS_ASSERTION(mRequests.Count() == 0,
                   "Only our initial stream should be unknown!");
      mRequests.AppendObject(oldRequest);
    }
    else {
      mRequests.ReplaceObjectAt(newRequest, i);
    }
  }

  void CancelRequests(nsresult status)
  {
    
    nsCOMArray<nsIRequest> requestsCopy(mRequests);
    for (int32_t i = 0; i < requestsCopy.Count(); ++i)
      requestsCopy[i]->Cancel(status);
  }

  void SuspendRequests() {
    nsCOMArray<nsIRequest> requestsCopy(mRequests);
    for (int32_t i = 0; i < requestsCopy.Count(); ++i)
      requestsCopy[i]->Suspend();
  }

  void ResumeRequests() {
    nsCOMArray<nsIRequest> requestsCopy(mRequests);
    for (int32_t i = 0; i < requestsCopy.Count(); ++i)
      requestsCopy[i]->Resume();
  }

private:
  nsresult SetUpStreamListener(nsIRequest* request, nsIURI* aURL);
  nsresult SetupPluginCacheFile(nsIChannel* channel);
  nsresult GetInterfaceGlobal(const nsIID& aIID, void** result);

  nsCOMPtr<nsIURI> mURL;
  nsCString mURLSpec; 
  nsRefPtr<nsNPAPIPluginStreamListener> mPStreamListener;

  
  bool                    mRequestFailed;

  





  bool              mStartBinding;
  bool              mHaveFiredOnStartRequest;
  
  uint32_t                mLength;
  int32_t                 mStreamType;

  
  
  nsRefPtr<CachedFileHolder> mLocalCachedFileHolder;
  nsCOMPtr<nsIOutputStream> mFileCacheOutputStream;
  nsDataHashtable<nsUint32HashKey, uint32_t>* mDataForwardToRequest;

  nsCString mContentType;
  bool mSeekable;
  uint32_t mModified;
  nsRefPtr<nsNPAPIPluginInstance> mPluginInstance;
  int32_t mStreamOffset;
  bool mStreamComplete;

public:
  bool                    mAbort;
  int32_t                 mPendingRequests;
  nsWeakPtr               mWeakPtrChannelCallbacks;
  nsWeakPtr               mWeakPtrChannelLoadGroup;
  nsCOMArray<nsIRequest> mRequests;
};

#endif 
