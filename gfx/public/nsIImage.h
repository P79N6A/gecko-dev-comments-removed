




































#ifndef nsIImage_h___
#define nsIImage_h___

#include "nsISupports.h"
#include "nsIRenderingContext.h"
#include "nsRect.h"
#include "gfxRect.h"

class gfxASurface;

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



#define NS_IIMAGE_IID \
{ 0x79fc5cde, 0x09d8, 0x4134, \
  { 0x93, 0x5b, 0x4d, 0xcb, 0x6d, 0x76, 0xd6, 0x22 } }


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
                  const gfxRect &aSubimageRect,
                  const gfxRect &aDestRect) = 0;

  




  virtual PRInt8 GetAlphaDepth() = 0;

  




  virtual void* GetBitInfo() = 0;


  











  NS_IMETHOD LockImagePixels(PRBool aMaskPixels) = 0;
  
  










  NS_IMETHOD UnlockImagePixels(PRBool aMaskPixels) = 0;

  






  NS_IMETHOD GetSurface(gfxASurface **aSurface) = 0;

  





  virtual void SetHasNoAlpha() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIImage, NS_IIMAGE_IID)

#endif
