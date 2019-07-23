








































#include "nsStubImageDecoderObserver.h"

class nsPresContext;
class nsIFrame;
class nsIURI;

#include "imgIRequest.h"
#include "nsCOMPtr.h"

class nsImageLoader : public nsStubImageDecoderObserver
{
public:
  nsImageLoader();
  virtual ~nsImageLoader();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage);
  NS_IMETHOD OnStopFrame(imgIRequest *aRequest, gfxIImageFrame *aFrame);
  
  
  
  
  

  
  NS_IMETHOD FrameChanged(imgIContainer *aContainer, gfxIImageFrame *newframe,
                          nsRect * dirtyRect);

  void Init(nsIFrame *aFrame, nsPresContext *aPresContext);
  nsresult Load(imgIRequest *aImage);

  void Destroy();

  nsIFrame *GetFrame() { return mFrame; }
  imgIRequest *GetRequest() { return mRequest; }

private:
  void RedrawDirtyFrame(const nsRect* aDamageRect);

private:
  nsIFrame *mFrame;
  nsPresContext *mPresContext;
  nsCOMPtr<imgIRequest> mRequest;
};
