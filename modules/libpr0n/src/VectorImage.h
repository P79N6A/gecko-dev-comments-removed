





































#ifndef mozilla_imagelib_VectorImage_h_
#define mozilla_imagelib_VectorImage_h_

#include "Image.h"
#include "nsIStreamListener.h"
#include "nsWeakReference.h"

class imgIDecoderObserver;

namespace mozilla {
namespace imagelib {

class SVGDocumentWrapper;
class SVGRootRenderingObserver;

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

  
  void InvalidateObserver();

protected:
  virtual nsresult StartAnimation();
  virtual nsresult StopAnimation();

private:
  nsWeakPtr                          mObserver;   
  nsRefPtr<SVGDocumentWrapper>       mSVGDocumentWrapper;
#ifdef MOZ_ENABLE_LIBXUL
  nsRefPtr<SVGRootRenderingObserver> mRenderingObserver;
#endif 

  nsIntRect      mRestrictedRegion;       
                                          
                                          
                                          

  nsIntSize      mLastRenderedSize;       
                                          
                                          
                                          

  PRUint16       mAnimationMode;          

  PRPackedBool   mIsInitialized:1;        
  PRPackedBool   mIsFullyLoaded:1;        
  PRPackedBool   mHaveAnimations:1;       
                                          
  PRPackedBool   mHaveRestrictedRegion:1; 
                                          

  PRPackedBool   mError:1;                
};

} 
} 

#endif 
