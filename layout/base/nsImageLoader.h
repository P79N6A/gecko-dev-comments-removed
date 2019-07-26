







#include "nsStubImageDecoderObserver.h"

class nsIFrame;
class nsIURI;

#include "imgIRequest.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"







class nsImageLoader : public nsStubImageDecoderObserver
{
private:
  nsImageLoader(nsIFrame *aFrame, PRUint32 aActions,
                nsImageLoader *aNextLoader);
  virtual ~nsImageLoader();

public:
  





  enum {
    ACTION_REDRAW_ON_DECODE = 0x01,
    ACTION_REDRAW_ON_LOAD   = 0x02,
  };
  static already_AddRefed<nsImageLoader>
    Create(nsIFrame *aFrame, imgIRequest *aRequest,
           PRUint32 aActions, nsImageLoader *aNextLoader);

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage);
  NS_IMETHOD OnStopFrame(imgIRequest *aRequest, PRUint32 aFrame);
  NS_IMETHOD OnStopRequest(imgIRequest *aRequest, bool aLastPart);
  NS_IMETHOD OnImageIsAnimated(imgIRequest *aRequest);
  
  
  
  
  

  
  NS_IMETHOD FrameChanged(imgIRequest *aRequest,
                          imgIContainer *aContainer,
                          const nsIntRect *aDirtyRect);

  void Destroy();

  imgIRequest *GetRequest() { return mRequest; }
  nsImageLoader *GetNextLoader() { return mNextLoader; }

private:
  nsresult Load(imgIRequest *aImage);
  
  void DoRedraw(const nsRect* aDamageRect);

  nsIFrame *mFrame;
  nsCOMPtr<imgIRequest> mRequest;
  PRUint32 mActions;
  nsRefPtr<nsImageLoader> mNextLoader;

  
  
  bool mRequestRegistered;
};
