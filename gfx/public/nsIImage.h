




































#ifndef nsIImage_h___
#define nsIImage_h___

#include "nsISupports.h"
#include "nsIRenderingContext.h"
#include "nsRect.h"
#include "gfxRect.h"

#ifdef MOZ_CAIRO_GFX
class gfxASurface;
#endif

class nsIDeviceContext;

struct nsColorMap
{
  
  
  
  PRInt32 NumColors;  
                      
                      
  
  PRUint8 *Index;     
                      
};

typedef enum {
    nsMaskRequirements_kNoMask,
    nsMaskRequirements_kNeeds1Bit,
    nsMaskRequirements_kNeeds8Bit
} nsMaskRequirements;


#define  nsImageUpdateFlags_kColorMapChanged 0x1
#define  nsImageUpdateFlags_kBitsChanged     0x2

#ifndef MOZ_CAIRO_GFX

#if defined(XP_WIN) || defined(XP_OS2) || defined(XP_MACOSX)
#define MOZ_PLATFORM_IMAGES_BOTTOM_TO_TOP
#endif
#endif



#define NS_IIMAGE_IID \
{ 0xfd31e1f2, 0xbd46, 0x47f1, \
  { 0xb8, 0xb6, 0xb9, 0x4c, 0xe9, 0x54, 0xf9, 0xce } }


class nsIImage : public nsISupports
{

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IIMAGE_IID)

  






  virtual nsresult Init(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth, nsMaskRequirements aMaskRequirements) = 0;

  




  virtual PRInt32 GetBytesPix() = 0;

  




  virtual PRBool GetIsRowOrderTopToBottom() = 0;

  




  virtual PRInt32 GetWidth() = 0;

  




  virtual PRInt32 GetHeight() = 0;

  




  virtual PRUint8 * GetBits() = 0;

  




  virtual PRInt32 GetLineStride() = 0;

  





  virtual PRBool GetHasAlphaMask() = 0;

  




  virtual PRUint8 * GetAlphaBits() = 0;

  




  virtual PRInt32 GetAlphaLineStride() = 0;

  





  virtual void ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags, nsIntRect *aUpdateRect) = 0;
  
  




  virtual PRBool GetIsImageComplete() = 0;

  





  virtual nsresult Optimize(nsIDeviceContext* aContext) = 0;

  




  virtual nsColorMap * GetColorMap() = 0;

  




  NS_IMETHOD Draw(nsIRenderingContext &aContext,
                  const gfxRect &aSourceRect,
                  const gfxRect &aDestRect) = 0;

  









  NS_IMETHOD DrawToImage(nsIImage* aDstImage, PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight) = 0;

  




  virtual PRInt8 GetAlphaDepth() = 0;

  




  virtual void* GetBitInfo() = 0;


  











  NS_IMETHOD LockImagePixels(PRBool aMaskPixels) = 0;
  
  










  NS_IMETHOD UnlockImagePixels(PRBool aMaskPixels) = 0;

  






  NS_IMETHOD GetSurface(gfxASurface **aSurface) = 0;

  





  virtual void SetHasNoAlpha() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIImage, NS_IIMAGE_IID)

#endif
