








































#include "nsStubImageDecoderObserver.h"

class nsIFrame;
class nsIURI;

#include "imgIRequest.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"







class nsImageLoadNotifier : public nsStubImageDecoderObserver
{
private:
  nsImageLoadNotifier(nsIFrame *aFrame, PRBool aReflowOnLoad,
                      nsImageLoadNotifier *aNextLoader);
  virtual ~nsImageLoadNotifier();

public:
  static already_AddRefed<nsImageLoadNotifier>
    Create(nsIFrame *aFrame, imgIRequest *aRequest,
           PRBool aReflowOnLoad, nsImageLoadNotifier *aNextLoader);

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage);
  NS_IMETHOD OnStopFrame(imgIRequest *aRequest, gfxIImageFrame *aFrame);
  
  
  
  
  

  
  NS_IMETHOD FrameChanged(imgIContainer *aContainer, gfxIImageFrame *newframe,
                          nsRect * dirtyRect);


  void Destroy();

  imgIRequest *GetRequest() { return mRequest; }
  nsImageLoadNotifier *GetNextLoader() { return mNextLoader; }

private:
  nsresult Load(imgIRequest *aImage);
  void RedrawDirtyFrame(const nsRect* aDamageRect);

  nsIFrame *mFrame;
  nsCOMPtr<imgIRequest> mRequest;
  PRBool mReflowOnLoad;
  nsRefPtr<nsImageLoadNotifier> mNextLoader;
};
