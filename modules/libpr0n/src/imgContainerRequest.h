





































#ifndef imgContainerRequest_h__
#define imgContainerRequest_h__

#include "imgIRequest.h"
#include "imgIContainer.h"
#include "imgIDecoderObserver.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"

class imgContainerRequest : public imgIRequest
{
public:
  imgContainerRequest(imgIContainer* aImage,
                      nsIURI* aURI,
                      PRUint32 aImageStatus,
                      PRUint32 aState,
                      nsIPrincipal* aPrincipal);
  virtual ~imgContainerRequest();

  NS_DECL_ISUPPORTS
  NS_DECL_IMGIREQUEST
  NS_DECL_NSIREQUEST

protected:
  nsCOMPtr<imgIContainer>       mImage;
  nsCOMPtr<nsIURI>              mURI;
  nsCOMPtr<nsIPrincipal>        mPrincipal;
  nsCOMPtr<imgIDecoderObserver> mObserver;
  PRUint32                      mImageStatus;
  PRUint32                      mState;
  PRUint32                      mLocksHeld;
};
#endif
