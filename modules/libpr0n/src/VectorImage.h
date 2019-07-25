





































#ifndef mozilla_imagelib_VectorImage_h_
#define mozilla_imagelib_VectorImage_h_

#include "Image.h"
#include "nsIStreamListener.h"

namespace mozilla {
namespace imagelib {

class VectorImage : public Image,
                    public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGICONTAINER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  VectorImage(imgStatusTracker* aStatusTracker = nsnull);
  virtual ~VectorImage();

  
  PRUint16 GetType() { return imgIContainer::TYPE_VECTOR; }

  
  nsresult Init(imgIDecoderObserver* aObserver,
                const char* aMimeType,
                PRUint32 aFlags);
  void GetCurrentFrameRect(nsIntRect& aRect);
  PRUint32 GetDataSize();

protected:
  virtual nsresult StartAnimation();
  virtual nsresult StopAnimation();
};

} 
} 

#endif 
