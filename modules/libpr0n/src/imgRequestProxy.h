






































#include "imgIRequest.h"
#include "imgIDecoderObserver.h"

#include "imgIContainer.h"
#include "imgIDecoder.h"
#include "nsIRequestObserver.h"
#include "nsIChannel.h"
#include "nsILoadGroup.h"
#include "nsISupportsPriority.h"
#include "nsCOMPtr.h"

#include "imgRequest.h"

#define NS_IMGREQUESTPROXY_CID \
{ /* 20557898-1dd2-11b2-8f65-9c462ee2bc95 */         \
     0x20557898,                                     \
     0x1dd2,                                         \
     0x11b2,                                         \
    {0x8f, 0x65, 0x9c, 0x46, 0x2e, 0xe2, 0xbc, 0x95} \
}

class imgRequestProxy : public imgIRequest, public nsISupportsPriority
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIREQUEST
  NS_DECL_NSIREQUEST
  NS_DECL_NSISUPPORTSPRIORITY

  imgRequestProxy();
  virtual ~imgRequestProxy();

  
  
  
  nsresult Init(imgRequest *request, nsILoadGroup *aLoadGroup, imgIDecoderObserver *aObserver);
  nsresult ChangeOwner(imgRequest *aNewOwner); 
                                               

  void AddToLoadGroup();
  void RemoveFromLoadGroup(PRBool releaseLoadGroup);

protected:
  friend class imgRequest;

  
  void OnStartDecode   ();
  void OnStartContainer(imgIContainer *aContainer);
  void OnStartFrame    (gfxIImageFrame *aFrame);
  void OnDataAvailable (gfxIImageFrame *aFrame, const nsIntRect * aRect);
  void OnStopFrame     (gfxIImageFrame *aFrame);
  void OnStopContainer (imgIContainer *aContainer);
  void OnStopDecode    (nsresult status, const PRUnichar *statusArg); 

  
  void FrameChanged(imgIContainer *aContainer, gfxIImageFrame *aFrame, nsIntRect * aDirtyRect);

  
  void OnStartRequest(nsIRequest *request, nsISupports *ctxt);
  void OnStopRequest(nsIRequest *request, nsISupports *ctxt, nsresult statusCode, PRBool aLastPart); 

  inline PRBool HasObserver() const {
    return mListener != nsnull;
  }
  
private:
  friend class imgCacheValidator;

  imgRequest *mOwner;

  imgIDecoderObserver* mListener;  
  nsCOMPtr<nsILoadGroup> mLoadGroup;

  nsLoadFlags mLoadFlags;
  PRPackedBool mCanceled;
  PRPackedBool mIsInLoadGroup;
};
