





































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
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  
  
  NS_SCRIPTABLE NS_IMETHOD GetWidth(PRInt32 *aWidth);
  NS_SCRIPTABLE NS_IMETHOD GetHeight(PRInt32 *aHeight);
  NS_SCRIPTABLE NS_IMETHOD GetType(PRUint16 *aType);
  NS_SCRIPTABLE NS_IMETHOD GetAnimated(PRBool *aAnimated);
  NS_SCRIPTABLE NS_IMETHOD GetCurrentFrameIsOpaque(PRBool *aCurrentFrameIsOpaque);
  NS_IMETHOD GetFrame(PRUint32 aWhichFrame, PRUint32 aFlags, gfxASurface **_retval NS_OUTPARAM);
  NS_IMETHOD CopyFrame(PRUint32 aWhichFrame, PRUint32 aFlags, gfxImageSurface **_retval NS_OUTPARAM);
  NS_IMETHOD ExtractFrame(PRUint32 aWhichFrame, const nsIntRect & aRect, PRUint32 aFlags, imgIContainer **_retval NS_OUTPARAM);
  NS_IMETHOD Draw(gfxContext *aContext, gfxPattern::GraphicsFilter aFilter, const gfxMatrix & aUserSpaceToImageSpace, const gfxRect & aFill, const nsIntRect & aSubimage, const nsIntSize & aViewportSize, PRUint32 aFlags);
  NS_IMETHOD_(nsIFrame *) GetRootLayoutFrame(void);
  NS_SCRIPTABLE NS_IMETHOD RequestDecode(void);
  NS_SCRIPTABLE NS_IMETHOD LockImage(void);
  NS_SCRIPTABLE NS_IMETHOD UnlockImage(void);
  NS_SCRIPTABLE NS_IMETHOD GetAnimationMode(PRUint16 *aAnimationMode);
  NS_SCRIPTABLE NS_IMETHOD SetAnimationMode(PRUint16 aAnimationMode);
  NS_SCRIPTABLE NS_IMETHOD ResetAnimation(void);
  

  VectorImage(imgStatusTracker* aStatusTracker = nsnull);
  virtual ~VectorImage();

  
  PRUint16 GetType() { return imgIContainer::TYPE_VECTOR; }

  
  nsresult Init(imgIDecoderObserver* aObserver,
                const char* aMimeType,
                const char* aURIString,
                PRUint32 aFlags);
  void GetCurrentFrameRect(nsIntRect& aRect);
  PRUint32 GetDecodedDataSize();
  PRUint32 GetSourceDataSize();

  
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
                                          
};

} 
} 

#endif 
