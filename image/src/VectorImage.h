




#ifndef mozilla_imagelib_VectorImage_h_
#define mozilla_imagelib_VectorImage_h_

#include "Image.h"
#include "nsIStreamListener.h"
#include "nsWeakReference.h"
#include "mozilla/TimeStamp.h"

class imgIDecoderObserver;

namespace mozilla {
namespace layers {
class LayerManager;
class ImageContainer;
}
namespace image {

class SVGDocumentWrapper;
class SVGRootRenderingObserver;

class VectorImage : public Image,
                    public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_IMGICONTAINER

  VectorImage(imgStatusTracker* aStatusTracker = nullptr);
  virtual ~VectorImage();

  
  nsresult Init(imgIDecoderObserver* aObserver,
                const char* aMimeType,
                const char* aURIString,
                uint32_t aFlags);
  void GetCurrentFrameRect(nsIntRect& aRect);

  virtual size_t HeapSizeOfSourceWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const;
  virtual size_t HeapSizeOfDecodedWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const;
  virtual size_t NonHeapSizeOfDecoded() const;
  virtual size_t OutOfProcessSizeOfDecoded() const;

  
  void InvalidateObserver();

protected:
  virtual nsresult StartAnimation();
  virtual nsresult StopAnimation();
  virtual bool     ShouldAnimate();

private:
  nsWeakPtr                          mObserver;   
  nsRefPtr<SVGDocumentWrapper>       mSVGDocumentWrapper;
  nsRefPtr<SVGRootRenderingObserver> mRenderingObserver;

  nsIntRect      mRestrictedRegion;       
                                          
                                          
                                          

  nsIntSize      mLastRenderedSize;       
                                          
                                          
                                          

  bool           mIsInitialized:1;        
  bool           mIsFullyLoaded:1;        
  bool           mIsDrawing:1;            
  bool           mHaveAnimations:1;       
                                          
  bool           mHaveRestrictedRegion:1; 
                                          
};

inline NS_IMETHODIMP VectorImage::GetAnimationMode(uint16_t *aAnimationMode) {
  return GetAnimationModeInternal(aAnimationMode);
}

inline NS_IMETHODIMP VectorImage::SetAnimationMode(uint16_t aAnimationMode) {
  return SetAnimationModeInternal(aAnimationMode);
}

} 
} 

#endif 
