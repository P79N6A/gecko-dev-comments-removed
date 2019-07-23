




































#ifndef nsIDrawingSurface_h___
#define nsIDrawingSurface_h___

#include "nscore.h"
#include "nsISupports.h"



typedef struct
{
  PRUint32  mRedZeroMask;     
  PRUint32  mGreenZeroMask;   
  PRUint32  mBlueZeroMask;    
  PRUint32  mAlphaZeroMask;   
  PRUint32  mRedMask;         
  PRUint32  mGreenMask;       
  PRUint32  mBlueMask;        
  PRUint32  mAlphaMask;       
  PRUint8   mRedCount;        
  PRUint8   mGreenCount;      
  PRUint8   mBlueCount;       
  PRUint8   mAlphaCount;      
  PRUint8   mRedShift;        
  PRUint8   mGreenShift;      
  PRUint8   mBlueShift;       
  PRUint8   mAlphaShift;      
} nsPixelFormat;

#define RASWIDTH(width, bpp) ((((width) * (bpp) + 31) >> 5) << 2)

#define NS_IDRAWING_SURFACE_IID   \
{ 0x61cc77e0, 0xcaac, 0x11d2, \
{ 0xa8, 0x49, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

class nsIDrawingSurface : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDRAWING_SURFACE_IID)
  
















  NS_IMETHOD Lock(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                  void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                  PRUint32 aFlags) = 0;

  





  NS_IMETHOD Unlock(void) = 0;

  






  NS_IMETHOD GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight) = 0;

  








  NS_IMETHOD IsOffscreen(PRBool *aOffScreen) = 0;

  











  NS_IMETHOD IsPixelAddressable(PRBool *aAddressable) = 0;

  






  NS_IMETHOD GetPixelFormat(nsPixelFormat *aFormat) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDrawingSurface, NS_IDRAWING_SURFACE_IID)







#define NS_CREATEDRAWINGSURFACE_FOR_PIXEL_ACCESS  0x0001




#define NS_CREATEDRAWINGSURFACE_SHORTLIVED        0x0002



#define NS_LOCK_SURFACE_READ_ONLY       0x0001
#define NS_LOCK_SURFACE_WRITE_ONLY      0x0002

#endif  
