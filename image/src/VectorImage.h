




#ifndef mozilla_imagelib_VectorImage_h_
#define mozilla_imagelib_VectorImage_h_

#include "Image.h"
#include "nsIStreamListener.h"
#include "mozilla/MemoryReporting.h"

class nsIRequest;
class gfxDrawable;

namespace mozilla {
namespace layers {
class LayerManager;
class ImageContainer;
}
namespace image {

struct SVGDrawingParameters;
class  SVGDocumentWrapper;
class  SVGRootRenderingObserver;
class  SVGLoadEventListener;
class  SVGParseCompleteListener;

class VectorImage MOZ_FINAL : public ImageResource,
                              public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_IMGICONTAINER

  

  
  nsresult Init(const char* aMimeType,
                uint32_t aFlags);
  virtual nsIntRect FrameRect(uint32_t aWhichFrame) MOZ_OVERRIDE;

  virtual size_t HeapSizeOfSourceWithComputedFallback(MallocSizeOf aMallocSizeOf) const;
  virtual size_t HeapSizeOfDecodedWithComputedFallback(MallocSizeOf aMallocSizeOf) const;
  virtual size_t NonHeapSizeOfDecoded() const;
  virtual size_t OutOfProcessSizeOfDecoded() const;

  virtual size_t HeapSizeOfVectorImageDocument(nsACString* aDocURL = nullptr) const MOZ_OVERRIDE;

  virtual nsresult OnImageDataAvailable(nsIRequest* aRequest,
                                        nsISupports* aContext,
                                        nsIInputStream* aInStr,
                                        uint64_t aSourceOffset,
                                        uint32_t aCount) MOZ_OVERRIDE;
  virtual nsresult OnImageDataComplete(nsIRequest* aRequest,
                                       nsISupports* aContext,
                                       nsresult aResult,
                                       bool aLastPart) MOZ_OVERRIDE;
  virtual nsresult OnNewSourceData() MOZ_OVERRIDE;

  







  void InvalidateObserversOnNextRefreshDriverTick();

  
  void OnSVGDocumentParsed();

  
  void OnSVGDocumentLoaded();
  void OnSVGDocumentError();

protected:
  explicit VectorImage(imgStatusTracker* aStatusTracker = nullptr,
                       ImageURL* aURI = nullptr);
  virtual ~VectorImage();

  virtual nsresult StartAnimation();
  virtual nsresult StopAnimation();
  virtual bool     ShouldAnimate();

  void CreateSurfaceAndShow(const SVGDrawingParameters& aParams);
  void Show(gfxDrawable* aDrawable, const SVGDrawingParameters& aParams);

private:
  void CancelAllListeners();
  void SendInvalidationNotifications();

  nsRefPtr<SVGDocumentWrapper>       mSVGDocumentWrapper;
  nsRefPtr<SVGRootRenderingObserver> mRenderingObserver;
  nsRefPtr<SVGLoadEventListener>     mLoadEventListener;
  nsRefPtr<SVGParseCompleteListener> mParseCompleteListener;

  bool           mIsInitialized;          
  bool           mIsFullyLoaded;          
  bool           mIsDrawing;              
  bool           mHaveAnimations;         
                                          
  bool           mHasPendingInvalidation; 
                                          

  
  nsAutoPtr<imgStatusTrackerInit> mStatusTrackerInit;

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
