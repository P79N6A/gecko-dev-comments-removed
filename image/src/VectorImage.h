




#ifndef mozilla_image_src_VectorImage_h
#define mozilla_image_src_VectorImage_h

#include "Image.h"
#include "nsIStreamListener.h"
#include "mozilla/MemoryReporting.h"

class nsIRequest;
class gfxDrawable;

namespace mozilla {
namespace image {

struct SVGDrawingParameters;
class  SVGDocumentWrapper;
class  SVGRootRenderingObserver;
class  SVGLoadEventListener;
class  SVGParseCompleteListener;

class VectorImage final : public ImageResource,
                          public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_IMGICONTAINER

  

  
  nsresult Init(const char* aMimeType,
                uint32_t aFlags) override;

  virtual size_t SizeOfSourceWithComputedFallback(MallocSizeOf aMallocSizeOf)
    const override;
  virtual size_t SizeOfDecoded(gfxMemoryLocation aLocation,
                               MallocSizeOf aMallocSizeOf) const override;

  virtual nsresult OnImageDataAvailable(nsIRequest* aRequest,
                                        nsISupports* aContext,
                                        nsIInputStream* aInStr,
                                        uint64_t aSourceOffset,
                                        uint32_t aCount) override;
  virtual nsresult OnImageDataComplete(nsIRequest* aRequest,
                                       nsISupports* aContext,
                                       nsresult aResult,
                                       bool aLastPart) override;

  void OnSurfaceDiscarded() override;

  







  void InvalidateObserversOnNextRefreshDriverTick();

  
  void OnSVGDocumentParsed();

  
  void OnSVGDocumentLoaded();
  void OnSVGDocumentError();

protected:
  explicit VectorImage(ProgressTracker* aProgressTracker = nullptr,
                       ImageURL* aURI = nullptr);
  virtual ~VectorImage();

  virtual nsresult StartAnimation() override;
  virtual nsresult StopAnimation() override;
  virtual bool     ShouldAnimate() override;

  void CreateSurfaceAndShow(const SVGDrawingParameters& aParams);
  void Show(gfxDrawable* aDrawable, const SVGDrawingParameters& aParams);

private:
  




  void RecoverFromLossOfSurfaces();

  void CancelAllListeners();
  void SendInvalidationNotifications();

  nsRefPtr<SVGDocumentWrapper>       mSVGDocumentWrapper;
  nsRefPtr<SVGRootRenderingObserver> mRenderingObserver;
  nsRefPtr<SVGLoadEventListener>     mLoadEventListener;
  nsRefPtr<SVGParseCompleteListener> mParseCompleteListener;

  
  uint32_t mLockCount;

  bool           mIsInitialized;          
  bool           mDiscardable;            
  bool           mIsFullyLoaded;          
                                          
  bool           mIsDrawing;              
  bool           mHaveAnimations;         
                                          
  bool           mHasPendingInvalidation; 
                                          

  
  nsAutoPtr<ProgressTrackerInit> mProgressTrackerInit;

  friend class ImageFactory;
};

inline NS_IMETHODIMP VectorImage::GetAnimationMode(uint16_t* aAnimationMode) {
  return GetAnimationModeInternal(aAnimationMode);
}

inline NS_IMETHODIMP VectorImage::SetAnimationMode(uint16_t aAnimationMode) {
  return SetAnimationModeInternal(aAnimationMode);
}

} 
} 

#endif 
