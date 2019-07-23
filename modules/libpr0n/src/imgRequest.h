






































#ifndef imgRequest_h__
#define imgRequest_h__

#include "imgILoad.h"

#include "imgIContainer.h"
#include "imgIDecoder.h"
#include "imgIDecoderObserver.h"

#include "nsIContentSniffer.h"
#include "nsIRequest.h"
#include "nsIProperties.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"
#include "nsIPrincipal.h"

#include "nsCategoryCache.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTObserverArray.h"
#include "nsWeakReference.h"

class imgCacheValidator;

class imgRequestProxy;
class imgCacheEntry;

enum {
  onStartRequest   = PR_BIT(0),
  onStartDecode    = PR_BIT(1),
  onStartContainer = PR_BIT(2),
  onStopContainer  = PR_BIT(3),
  onStopDecode     = PR_BIT(4),
  onStopRequest    = PR_BIT(5)
};

class imgRequest : public imgILoad,
                   public imgIDecoderObserver,
                   public nsIStreamListener,
                   public nsSupportsWeakReference
{
public:
  imgRequest();
  virtual ~imgRequest();

  NS_DECL_ISUPPORTS

  nsresult Init(nsIURI *aURI,
                nsIRequest *aRequest,
                imgCacheEntry *aCacheEntry,
                void *aCacheId,
                void *aLoadId);

  
  nsresult AddProxy(imgRequestProxy *proxy);

  
  nsresult RemoveProxy(imgRequestProxy *proxy, nsresult aStatus, PRBool aNotify);
  nsresult NotifyProxyListener(imgRequestProxy *proxy);

  void SniffMimeType(const char *buf, PRUint32 len);

  
  
  
  PRBool IsReusable(void *aCacheId) { return !mLoading || (aCacheId == mCacheId); }

  
  
  nsresult GetNetworkStatus();

private:
  friend class imgCacheEntry;
  friend class imgRequestProxy;
  friend class imgLoader;
  friend class imgCacheValidator;

  inline void SetLoadId(void *aLoadId) {
    mLoadId = aLoadId;
    mLoadTime = PR_Now();
  }
  inline PRUint32 GetImageStatus() const { return mImageStatus; }
  inline nsresult GetResultFromImageStatus(PRUint32 aStatus) const;
  void Cancel(nsresult aStatus);
  nsresult GetURI(nsIURI **aURI);
  nsresult GetPrincipal(nsIPrincipal **aPrincipal);
  nsresult GetSecurityInfo(nsISupports **aSecurityInfo);
  void RemoveFromCache();
  inline const char *GetMimeType() const {
    return mContentType.get();
  }
  inline nsIProperties *Properties() {
    return mProperties;
  }

  
  
  PRBool HaveProxyWithObserver(imgRequestProxy* aProxyToIgnore) const;

  
  
  PRInt32 Priority() const;

  
  
  void AdjustPriority(imgRequestProxy *aProxy, PRInt32 aDelta);

public:
  NS_DECL_IMGILOAD
  NS_DECL_IMGIDECODEROBSERVER
  NS_DECL_IMGICONTAINEROBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER

private:
  nsCOMPtr<nsIRequest> mRequest;
  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<imgIContainer> mImage;
  nsCOMPtr<imgIDecoder> mDecoder;
  nsCOMPtr<nsIProperties> mProperties;
  nsCOMPtr<nsISupports> mSecurityInfo;

  nsTObserverArray<imgRequestProxy*> mObservers;

  PRPackedBool mLoading;
  PRPackedBool mProcessing;
  PRPackedBool mHadLastPart;
  PRUint32 mNetworkStatus;
  PRUint32 mImageStatus;
  PRUint32 mState;
  nsCString mContentType;

  nsRefPtr<imgCacheEntry> mCacheEntry; 

  void *mCacheId;

  void *mLoadId;
  PRTime mLoadTime;

  imgCacheValidator *mValidator;
  PRBool   mIsMultiPartChannel;

  nsCategoryCache<nsIContentSniffer> mImageSniffers;
};

#endif
