




#ifndef mozilla_imagelib_VectorImage_h_
#define mozilla_imagelib_VectorImage_h_

#include "Image.h"
#include "nsIStreamListener.h"
#include "nsIRequest.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/WeakPtr.h"

class imgDecoderObserver;

namespace mozilla {
namespace layers {
class LayerManager;
class ImageContainer;
}
namespace image {

class SVGDocumentWrapper;
class SVGRootRenderingObserver;
class SVGLoadEventListener;
class SVGParseCompleteListener;

class VectorImage : public ImageResource,
                    public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_IMGICONTAINER

  
  virtual ~VectorImage();

  
  nsresult Init(const char* aMimeType,
                uint32_t aFlags);
  virtual nsIntRect FrameRect(uint32_t aWhichFrame) MOZ_OVERRIDE;

  virtual size_t HeapSizeOfSourceWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const;
  virtual size_t HeapSizeOfDecodedWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const;
  virtual size_t NonHeapSizeOfDecoded() const;
  virtual size_t OutOfProcessSizeOfDecoded() const;

  virtual nsresult OnImageDataAvailable(nsIRequest* aRequest,
                                        nsISupports* aContext,
                                        nsIInputStream* aInStr,
                                        uint64_t aSourceOffset,
                                        uint32_t aCount) MOZ_OVERRIDE;
  virtual nsresult OnImageDataComplete(nsIRequest* aRequest,
                                       nsISupports* aContext,
                                       nsresult status) MOZ_OVERRIDE;
  virtual nsresult OnNewSourceData() MOZ_OVERRIDE;

  
  void InvalidateObserver();

  
  void OnSVGDocumentParsed();

  
  void OnSVGDocumentLoaded();
  void OnSVGDocumentError();

protected:
  VectorImage(imgStatusTracker* aStatusTracker = nullptr, nsIURI* aURI = nullptr);

  virtual nsresult StartAnimation();
  virtual nsresult StopAnimation();
  virtual bool     ShouldAnimate();

private:
  void CancelAllListeners();

  nsRefPtr<SVGDocumentWrapper>       mSVGDocumentWrapper;
  nsRefPtr<SVGRootRenderingObserver> mRenderingObserver;
  nsRefPtr<SVGLoadEventListener>     mLoadEventListener;
  nsRefPtr<SVGParseCompleteListener> mParseCompleteListener;

  nsIntRect      mRestrictedRegion;       
                                          
                                          
                                          

  bool           mIsInitialized:1;        
  bool           mIsFullyLoaded:1;        
  bool           mIsDrawing:1;            
  bool           mHaveAnimations:1;       
                                          
  bool           mHaveRestrictedRegion:1; 
                                          

  friend class ImageFactory;
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
