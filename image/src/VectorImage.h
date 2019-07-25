




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

  
  
  NS_IMETHOD GetWidth(int32_t *aWidth);
  NS_IMETHOD GetHeight(int32_t *aHeight);
  NS_IMETHOD GetType(uint16_t *aType);
  NS_IMETHOD_(uint16_t) GetType(void);
  NS_IMETHOD GetAnimated(bool *aAnimated);
  NS_IMETHOD GetCurrentFrameIsOpaque(bool *aCurrentFrameIsOpaque);
  NS_IMETHOD GetFrame(uint32_t aWhichFrame, uint32_t aFlags, gfxASurface **_retval);
  NS_IMETHOD GetImageContainer(mozilla::layers::ImageContainer **_retval) { *_retval = NULL; return NS_OK; }
  NS_IMETHOD CopyFrame(uint32_t aWhichFrame, uint32_t aFlags, gfxImageSurface **_retval);
  NS_IMETHOD ExtractFrame(uint32_t aWhichFrame, const nsIntRect & aRect, uint32_t aFlags, imgIContainer **_retval);
  NS_IMETHOD Draw(gfxContext *aContext, gfxPattern::GraphicsFilter aFilter, const gfxMatrix & aUserSpaceToImageSpace, const gfxRect & aFill, const nsIntRect & aSubimage, const nsIntSize & aViewportSize, uint32_t aFlags);
  NS_IMETHOD_(nsIFrame *) GetRootLayoutFrame(void);
  NS_IMETHOD RequestDecode(void);
  NS_IMETHOD LockImage(void);
  NS_IMETHOD UnlockImage(void);
  NS_IMETHOD RequestDiscard(void);
  NS_IMETHOD ResetAnimation(void);
  NS_IMETHOD_(void) RequestRefresh(const mozilla::TimeStamp& aTime);
  

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

} 
} 

#endif 
