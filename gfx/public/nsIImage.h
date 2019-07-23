




































#ifndef nsIImage_h___
#define nsIImage_h___

#include "nsISupports.h"
#include "nsMargin.h"
#include "nsRect.h"

class gfxASurface;
class gfxPattern;
struct gfxMatrix;
struct gfxRect;
class gfxContext;

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
  { 0xc942f66c, 0x97d0, 0x470e, \
    { 0x99, 0xde, 0xa1, 0xef, 0xb4, 0x58, 0x6a, 0xfd } }


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

  






  virtual nsresult ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags, nsIntRect *aUpdateRect) = 0;
  
  




  virtual PRBool GetIsImageComplete() = 0;

  





  virtual nsresult Optimize(nsIDeviceContext* aContext) = 0;

  




  virtual nsColorMap * GetColorMap() = 0;

  



















  virtual void Draw(gfxContext*        aContext,
                    const gfxMatrix&   aUserSpaceToImageSpace,
                    const gfxRect&     aFill,
                    const nsIntMargin& aPadding,
                    const nsIntRect&   aSubimage) = 0;

  




  virtual PRInt8 GetAlphaDepth() = 0;

  




  virtual void* GetBitInfo() = 0;


  















  NS_IMETHOD LockImagePixels(PRBool aMaskPixels) = 0;
  
  










  NS_IMETHOD UnlockImagePixels(PRBool aMaskPixels) = 0;

  







  NS_IMETHOD GetSurface(gfxASurface **aSurface) = 0;

  







  NS_IMETHOD GetPattern(gfxPattern **aPattern) = 0;

  





  virtual void SetHasNoAlpha() = 0;

  





  NS_IMETHOD Extract(const nsIntRect& aSubimage,
                     nsIImage** aResult NS_OUTPARAM) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIImage, NS_IIMAGE_IID)

#endif
