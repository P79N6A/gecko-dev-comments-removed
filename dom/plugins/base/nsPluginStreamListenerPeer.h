







































#ifndef nsPluginStreamListenerPeer_h_
#define nsPluginStreamListenerPeer_h_

#include "nscore.h"
#include "nsIFile.h"
#include "nsIStreamListener.h"
#include "nsIProgressEventSink.h"
#include "nsIHttpHeaderVisitor.h"
#include "nsWeakReference.h"
#include "nsNPAPIPluginStreamListener.h"
#include "nsHashtable.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIInterfaceRequestor.h"
#include "nsIChannelEventSink.h"

class nsIChannel;







class CachedFileHolder
{
public:
  CachedFileHolder(nsIFile* cacheFile);
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
public nsINPAPIPluginStreamInfo,
public nsIInterfaceRequestor,
public nsIChannelEventSink
{
public:
  nsPluginStreamListenerPeer();
  virtual ~nsPluginStreamListenerPeer();
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROGRESSEVENTSINK
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIHTTPHEADERVISITOR
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

  
  NS_DECL_NSIPLUGINSTREAMINFO
  
  
  void
  MakeByteRangeString(NPByteRange* aRangeList, nsACString &string, PRInt32 *numRequests);
  
  PRBool UseExistingPluginCacheFile(nsPluginStreamListenerPeer* psi);
  
  
  nsresult Initialize(nsIURI *aURL,
                      nsNPAPIPluginInstance *aInstance,
                      nsIPluginStreamListener *aListener);
  
  nsresult InitializeEmbedded(nsIURI *aURL,
                              nsNPAPIPluginInstance* aInstance,
                              nsIPluginInstanceOwner *aOwner = nsnull);
  
  nsresult InitializeFullPage(nsIURI* aURL, nsNPAPIPluginInstance *aInstance);

  nsresult OnFileAvailable(nsIFile* aFile);
  
  nsresult ServeStreamAsFile(nsIRequest *request, nsISupports *ctxt);
  
  nsNPAPIPluginInstance *GetPluginInstance() { return mPluginInstance; }
  
private:
  nsresult SetUpStreamListener(nsIRequest* request, nsIURI* aURL);
  nsresult SetupPluginCacheFile(nsIChannel* channel);
  nsresult GetInterfaceGlobal(const nsIID& aIID, void** result);

  nsCOMPtr<nsIURI> mURL;
  nsCString mURLSpec; 
  nsCOMPtr<nsIPluginInstanceOwner> mOwner;
  nsRefPtr<nsNPAPIPluginStreamListener> mPStreamListener;

  
  PRPackedBool            mRequestFailed;
  
  





  PRPackedBool      mStartBinding;
  PRPackedBool      mHaveFiredOnStartRequest;
  
  PRUint32                mLength;
  PRInt32                 mStreamType;
  
  
  
  nsRefPtr<CachedFileHolder> mLocalCachedFileHolder;
  nsCOMPtr<nsIOutputStream> mFileCacheOutputStream;
  nsHashtable             *mDataForwardToRequest;
  
  nsCString mContentType;
  PRBool mSeekable;
  PRUint32 mModified;
  nsRefPtr<nsNPAPIPluginInstance> mPluginInstance;
  PRInt32 mStreamOffset;
  PRBool mStreamComplete;
  
public:
  PRBool                  mAbort;
  PRInt32                 mPendingRequests;
  nsWeakPtr               mWeakPtrChannelCallbacks;
  nsWeakPtr               mWeakPtrChannelLoadGroup;
};

#endif 
