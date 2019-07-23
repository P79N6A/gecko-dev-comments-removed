








































#include "nsStubImageDecoderObserver.h"

class nsIFrame;
class nsIURI;

#include "imgIRequest.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"







class nsImageLoader : public nsStubImageDecoderObserver
{
private:
  nsImageLoader(nsIFrame *aFrame, PRBool aReflowOnLoad,
                nsImageLoader *aNextLoader);
  virtual ~nsImageLoader();

public:
  static already_AddRefed<nsImageLoader>
    Create(nsIFrame *aFrame, imgIRequest *aRequest,
           PRBool aReflowOnLoad, nsImageLoader *aNextLoader);

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage);
  NS_IMETHOD OnStopFrame(imgIRequest *aRequest, PRUint32 aFrame);
  
  
  
  
  

  
  NS_IMETHOD FrameChanged(imgIContainer *aContainer, nsIntRect *dirtyRect);

  void Destroy();

  imgIRequest *GetRequest() { return mRequest; }
  nsImageLoader *GetNextLoader() { return mNextLoader; }

private:
  nsresult Load(imgIRequest *aImage);
  void RedrawDirtyFrame(const nsRect* aDamageRect);

  nsIFrame *mFrame;
  nsCOMPtr<imgIRequest> mRequest;
  PRBool mReflowOnLoad;
  nsRefPtr<nsImageLoader> mNextLoader;
};
