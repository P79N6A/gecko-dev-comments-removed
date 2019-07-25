





































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
namespace imagelib {

class SVGDocumentWrapper;
class SVGRootRenderingObserver;

class VectorImage : public Image,
                    public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  
  
  NS_SCRIPTABLE NS_IMETHOD GetWidth(PRInt32 *aWidth);
  NS_SCRIPTABLE NS_IMETHOD GetHeight(PRInt32 *aHeight);
  NS_SCRIPTABLE NS_IMETHOD GetType(PRUint16 *aType);
  NS_IMETHOD_(PRUint16) GetType(void);
  NS_SCRIPTABLE NS_IMETHOD GetAnimated(bool *aAnimated);
  NS_SCRIPTABLE NS_IMETHOD GetCurrentFrameIsOpaque(bool *aCurrentFrameIsOpaque);
  NS_IMETHOD GetFrame(PRUint32 aWhichFrame, PRUint32 aFlags, gfxASurface **_retval NS_OUTPARAM);
  NS_IMETHOD GetImageContainer(mozilla::layers::LayerManager* aManager, mozilla::layers::ImageContainer **_retval NS_OUTPARAM) { *_retval = NULL; return NS_OK; }
  NS_IMETHOD CopyFrame(PRUint32 aWhichFrame, PRUint32 aFlags, gfxImageSurface **_retval NS_OUTPARAM);
  NS_IMETHOD ExtractFrame(PRUint32 aWhichFrame, const nsIntRect & aRect, PRUint32 aFlags, imgIContainer **_retval NS_OUTPARAM);
  NS_IMETHOD Draw(gfxContext *aContext, gfxPattern::GraphicsFilter aFilter, const gfxMatrix & aUserSpaceToImageSpace, const gfxRect & aFill, const nsIntRect & aSubimage, const nsIntSize & aViewportSize, PRUint32 aFlags);
  NS_IMETHOD_(nsIFrame *) GetRootLayoutFrame(void);
  NS_SCRIPTABLE NS_IMETHOD RequestDecode(void);
  NS_SCRIPTABLE NS_IMETHOD LockImage(void);
  NS_SCRIPTABLE NS_IMETHOD UnlockImage(void);
  NS_SCRIPTABLE NS_IMETHOD ResetAnimation(void);
  NS_IMETHOD_(void) RequestRefresh(const mozilla::TimeStamp& aTime);
  

  VectorImage(imgStatusTracker* aStatusTracker = nsnull);
  virtual ~VectorImage();

  
  nsresult Init(imgIDecoderObserver* aObserver,
                const char* aMimeType,
                const char* aURIString,
                PRUint32 aFlags);
  void GetCurrentFrameRect(nsIntRect& aRect);

  virtual PRUint32 GetDecodedHeapSize();
  virtual PRUint32 GetDecodedNonheapSize();
  virtual PRUint32 GetDecodedOutOfProcessSize();
  virtual PRUint32 GetSourceHeapSize();

  
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
