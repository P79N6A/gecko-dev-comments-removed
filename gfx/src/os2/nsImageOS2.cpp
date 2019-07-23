











































#include "nsGfxDefs.h"
#include <stdlib.h>

#include "nsImageOS2.h"
#include "nsRenderingContextOS2.h"
#include "nsDeviceContextOS2.h"
#include "imgScaler.h"

#define MAX_BUFFER_WIDTH        128
#define MAX_BUFFER_HEIGHT       128

#ifdef XP_OS2_VACPP










extern "C" int zeroStack(int index)
{
  #define CLEAR_BUF_SIZE 1024
  BYTE buf[CLEAR_BUF_SIZE];
  memset(buf, 0, CLEAR_BUF_SIZE * sizeof(BYTE));
  return buf[index];
}
#else
#define zeroStack(x)
#endif

struct MONOBITMAPINFO
{
   BITMAPINFOHEADER2 bmpInfo;
   RGB2 argbColor [2];

   operator PBITMAPINFO2 ()       { return (PBITMAPINFO2) &bmpInfo; }
   operator PBITMAPINFOHEADER2 () { return &bmpInfo; }

   MONOBITMAPINFO( PBITMAPINFO2 pBI)
   {
      memcpy( &bmpInfo, pBI, sizeof( BITMAPINFOHEADER2));
      bmpInfo.cBitCount = 1;

      argbColor [0].bRed      = 0;
      argbColor [0].bGreen    = 0;
      argbColor [0].bBlue     = 0;
      argbColor [0].fcOptions = 0;
      argbColor [1].bRed      = 255;
      argbColor [1].bGreen    = 255;
      argbColor [1].bBlue     = 255;
      argbColor [1].fcOptions = 0;
   }
};


PRUint8 nsImageOS2::gBlenderLookup [65536];    
PRBool  nsImageOS2::gBlenderReady = PR_FALSE;



NS_IMPL_ISUPPORTS1(nsImageOS2, nsIImage)


nsImageOS2::nsImageOS2()
: mInfo(0)
, mDeviceDepth(0)
, mRowBytes(0)
, mImageBits(0)
, mIsOptimized(PR_FALSE)
, mColorMap(0)
, mDecodedRect()
, mAlphaBits(0)
, mAlphaDepth(0)
, mARowBytes(0)
{
   if (gBlenderReady != PR_TRUE)
     BuildBlenderLookup ();
}

nsImageOS2::~nsImageOS2()
{
   CleanUp(PR_TRUE);
}

nsresult nsImageOS2::Init( PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth,
                           nsMaskRequirements aMaskRequirements)
{
   

   
   NS_ASSERTION( aDepth == 24 || aDepth == 8, "Bad image depth");

   
   mRowBytes = RASWIDTH(aWidth,aDepth);

   mImageBits = new PRUint8 [ aHeight * mRowBytes ];

   
   int cols = -1;
   if( aDepth == 8) cols = COLOR_CUBE_SIZE;
   else if( aDepth <= 32) cols = 0;

   int szStruct = sizeof( BITMAPINFOHEADER2) + cols * sizeof( RGB2);

   mInfo = (PBITMAPINFO2) calloc( szStruct, 1);
   mInfo->cbFix = sizeof( BITMAPINFOHEADER2);
   mInfo->cx = aWidth;
   mInfo->cy = aHeight;
   mInfo->cPlanes = 1;
   mInfo->cBitCount = (USHORT) aDepth;

   

   
   
   
   if( aDepth == 8)
   {
      mColorMap = new nsColorMap;
      mColorMap->NumColors = COLOR_CUBE_SIZE;
      mColorMap->Index = new PRUint8[3 * mColorMap->NumColors];
   }

   
   if( aMaskRequirements != nsMaskRequirements_kNoMask)
   {
      if( aMaskRequirements == nsMaskRequirements_kNeeds1Bit)
      {
         mAlphaDepth = 1;
      }
      else
      {
         NS_ASSERTION( nsMaskRequirements_kNeeds8Bit == aMaskRequirements,
                       "unexpected mask depth");
         mAlphaDepth = 8;
      }

      
      mARowBytes = RASWIDTH (aWidth, mAlphaDepth);

      mAlphaBits = new PRUint8 [ aHeight * mARowBytes];
   }

   return NS_OK;
}

void nsImageOS2::CleanUp(PRBool aCleanUpAll)
{
   

   if( mImageBits) {
      delete [] mImageBits; 
      mImageBits = 0;
   }
   if( mInfo) {
      free( mInfo);
      mInfo = 0;
   }
   if( mColorMap) {
      if( mColorMap->Index)
         delete [] mColorMap->Index;
      delete mColorMap;
      mColorMap = 0;
   }
   if( mAlphaBits) {
      delete [] mAlphaBits; 
      mAlphaBits = 0;
   }
}

void nsImageOS2::ImageUpdated( nsIDeviceContext *aContext,
                               PRUint8 aFlags, nsRect *aUpdateRect)
{
   mDecodedRect.UnionRect(mDecodedRect, *aUpdateRect);

   if (!aContext) {
      return;
   } 
   
   
   
   

   aContext->GetDepth( mDeviceDepth);

   if( (aFlags & nsImageUpdateFlags_kColorMapChanged) && mInfo->cBitCount == 8)
   {
      PRGB2 pBmpEntry  = mInfo->argbColor;
      PRUint8 *pMapByte = mColorMap->Index;

      for( PRInt32 i = 0; i < mColorMap->NumColors; i++, pBmpEntry++)
      {
         pBmpEntry->bRed   = *pMapByte++;
         pBmpEntry->bGreen = *pMapByte++;
         pBmpEntry->bBlue  = *pMapByte++;
      }
   }
   else if( aFlags & nsImageUpdateFlags_kBitsChanged)
   {
      
   }
}




PRBool nsImageOS2::GetIsImageComplete() {
  return mInfo &&
         mDecodedRect.x == 0 &&
         mDecodedRect.y == 0 &&
         mDecodedRect.width == mInfo->cx &&
         mDecodedRect.height == mInfo->cy;
}

void nsImageOS2::BuildBlenderLookup (void)
{
  for (int y = 0 ; y < 256 ; y++)
    for (int x = 0 ; x < 256 ; x++)
      gBlenderLookup [y * 256 + x] = y * x / 255;

  gBlenderReady = PR_TRUE;
}

nsresult nsImageOS2::Draw( nsIRenderingContext &aContext,
                           nsIDrawingSurface* aSurface,
                           PRInt32 aX, PRInt32 aY,
                           PRInt32 aWidth, PRInt32 aHeight)
{
   return Draw( aContext, aSurface,
                0, 0, mInfo->cx, mInfo->cy,
                aX, aY, aWidth, aHeight);
}




void nsImageOS2 :: DrawComposited24(unsigned char *aBits,
			     PRUint8 *aImageRGB, PRUint32 aStrideRGB,
			     PRUint8 *aImageAlpha, PRUint32 aStrideAlpha,
			     int aWidth, int aHeight)
{
  PRInt32 targetRowBytes = ((aWidth * 3) + 3) & ~3;

  for (int y = 0; y < aHeight; y++) {
    unsigned char *targetRow = aBits + y * targetRowBytes;
    unsigned char *imageRow = aImageRGB + y * aStrideRGB;
    unsigned char *alphaRow = aImageAlpha + y * aStrideAlpha;

    for (int x = 0; x < aWidth;
         x++, targetRow += 3, imageRow += 3, alphaRow++) {
      unsigned alpha = *alphaRow;
      MOZ_BLEND(targetRow[0], targetRow[0], imageRow[0], alpha);
      MOZ_BLEND(targetRow[1], targetRow[1], imageRow[1], alpha);
      MOZ_BLEND(targetRow[2], targetRow[2], imageRow[2], alpha);
    }
  }
}

NS_IMETHODIMP 
nsImageOS2 :: Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
                  PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                  PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight)
{
  nsresult rv = NS_OK;

   PRInt32 origSHeight = aSHeight, origDHeight = aDHeight;
   PRInt32 origSWidth = aSWidth, origDWidth = aDWidth;

   if (mInfo == nsnull || aSWidth < 0 || aDWidth < 0 || aSHeight < 0 || aDHeight < 0) 
      return NS_ERROR_FAILURE;

   if (0 == aSWidth || 0 == aDWidth || 0 == aSHeight || 0 == aDHeight)
      return NS_OK;

   
   PRInt32 aSX2 = aSX + aSWidth;

   if (aSX2 > mDecodedRect.XMost()) 
      aSX2 = mDecodedRect.XMost();

   if (aSX < mDecodedRect.x) {
      aDX += (mDecodedRect.x - aSX) * origDWidth / origSWidth;
      aSX = mDecodedRect.x;
   }
  
   aSWidth = aSX2 - aSX;
   aDWidth -= (origSWidth - aSWidth) * origDWidth / origSWidth;
  
   if (aSWidth <= 0 || aDWidth <= 0)
      return NS_OK;

   PRInt32 aSY2 = aSY + aSHeight;

   if (aSY2 > mDecodedRect.YMost())
      aSY2 = mDecodedRect.YMost();

   if (aSY < mDecodedRect.y) {
      aDY += (mDecodedRect.y - aSY) * origDHeight / origSHeight;
      aSY = mDecodedRect.y;
   }

   aSHeight = aSY2 - aSY;
   aDHeight -= (origSHeight - aSHeight) * origDHeight / origSHeight;

   if (aSHeight <= 0 || aDHeight <= 0)
      return NS_OK;

   nsDrawingSurfaceOS2 *surf = (nsDrawingSurfaceOS2*) aSurface;
 

   nsRect trect( aDX, aDY, aDWidth, aDHeight);
   RECTL  rcl;
   surf->NS2PM_ININ (trect, rcl);

   
   POINTL aptl[ 4] = { { rcl.xLeft, rcl.yBottom },              
                       { rcl.xRight, rcl.yTop },                
                       { aSX, mInfo->cy - (aSY + aSHeight) },   
                       { aSX + aSWidth, mInfo->cy - aSY } };    
                       
   PRBool fPrinting = PR_FALSE;
   nsCOMPtr<nsIDeviceContext>  context;
   aContext.GetDeviceContext(*getter_AddRefs(context));
   if (((nsDeviceContextOS2 *)context.get())->mPrintDC) {
      fPrinting = PR_TRUE;
   }

   if( mAlphaDepth == 0)
   {
      
      GFX (::GpiDrawBits (surf->GetPS (), mImageBits, mInfo, 4, aptl, ROP_SRCCOPY, BBO_IGNORE), GPI_ERROR);
   }
   else if( mAlphaDepth == 1)
   {
      if (!fPrinting) {
         
         
         
         
         
         
         
         
         
         
         
         
         
   
         
         MONOBITMAPINFO MaskBitmapInfo (mInfo);
         GFX (::GpiDrawBits (surf->GetPS (), mAlphaBits, MaskBitmapInfo, 4, aptl, ROP_SRCAND, BBO_IGNORE), GPI_ERROR);
   
         
         GFX (::GpiDrawBits (surf->GetPS (), mImageBits, mInfo, 4, aptl, ROP_SRCPAINT, BBO_IGNORE), GPI_ERROR);
      } else {
         
         HDC hdcCompat = GFX (::GpiQueryDevice (surf->GetPS ()), HDC_ERROR);
   
         rv = NS_ERROR_FAILURE;
       
         
         RECTL dest;
         surf->NS2PM_INEX (trect, dest);
   
         DEVOPENSTRUC dop = { 0, 0, 0, 0, 0 };
         HDC MemDC = ::DevOpenDC( (HAB)0, OD_MEMORY, "*", 5, (PDEVOPENDATA) &dop, hdcCompat);
   
         if( MemDC != DEV_ERROR )
         {   
           
           SIZEL sizel = { 0, 0 };
           HPS MemPS = GFX (::GpiCreatePS (0, MemDC, &sizel, PU_PELS | GPIT_MICRO | GPIA_ASSOC), GPI_ERROR);
   
           if( MemPS != GPI_ERROR )
           {
             GFX (::GpiCreateLogColorTable (MemPS, 0, LCOLF_RGB, 0, 0, 0), FALSE);
   
             
             HBITMAP hMemBmp;
             BITMAPINFOHEADER2 bihMem = { 0 };
   
             bihMem.cbFix = sizeof (BITMAPINFOHEADER2);
             bihMem.cx = aSWidth;
             bihMem.cy = aSHeight;
             bihMem.cPlanes = 1;
             LONG lBitCount = 0;
             GFX (::DevQueryCaps( hdcCompat, CAPS_COLOR_BITCOUNT, 1, &lBitCount), FALSE);
             lBitCount = 24; 
             bihMem.cBitCount = lBitCount;
             
             hMemBmp = GFX (::GpiCreateBitmap (MemPS, &bihMem, 0, 0, 0), GPI_ERROR);
   
             if( hMemBmp != GPI_ERROR )
             {
               GFX (::GpiSetBitmap (MemPS, hMemBmp), HBM_ERROR);
               zeroStack(10);
               GpiErase(MemPS);
   
               
               
               POINTL aptlNew[ 4] = { { 0, 0 },              
                                      { bihMem.cx, bihMem.cy },                
                                      { aSX, mInfo->cy - (aSY + aSHeight) },   
                                      { aSX + aSWidth+1, mInfo->cy - aSY+1 } };    
   
               
               MONOBITMAPINFO MaskBitmapInfo (mInfo);                                   
               GFX (::GpiDrawBits (MemPS, mAlphaBits, MaskBitmapInfo, 4, aptlNew, ROP_SRCAND, BBO_IGNORE), GPI_ERROR);
          
               
               GFX (::GpiDrawBits (MemPS, mImageBits, mInfo, 4, aptlNew, ROP_SRCPAINT, BBO_IGNORE), GPI_ERROR);
   
               
               POINTL aptlMemToDev [4] = { {dest.xLeft, dest.yBottom},   
                                           {dest.xRight, dest.yTop},     
                                           {0, 0},                       
                                           {bihMem.cx, bihMem.cy} };      
   
               GFX (::GpiBitBlt (surf->GetPS (), MemPS, 4, aptlMemToDev, ROP_SRCCOPY, BBO_IGNORE), GPI_ERROR);
               
               rv = NS_OK;
                                                                   
               GFX (::GpiSetBitmap (MemPS, NULLHANDLE), HBM_ERROR);
               GFX (::GpiDeleteBitmap (hMemBmp), FALSE);
             }
           
             GFX (::GpiDestroyPS (MemPS), FALSE);
           }
       
           ::DevCloseDC (MemDC);
         }
      }
   } else
   {
      
      HDC hdcCompat = GFX (::GpiQueryDevice (surf->GetPS ()), HDC_ERROR);

      rv = NS_ERROR_FAILURE;
    
       
      RECTL dest;
      surf->NS2PM_INEX (trect, dest);

      DEVOPENSTRUC dop = { 0, 0, 0, 0, 0 };
      HDC MemDC = ::DevOpenDC( (HAB)0, OD_MEMORY, "*", 5, (PDEVOPENDATA) &dop, hdcCompat);

      if( MemDC != DEV_ERROR )
      {   
        
        SIZEL sizel = { 0, 0 };
        HPS MemPS = GFX (::GpiCreatePS (0, MemDC, &sizel, PU_PELS | GPIT_MICRO | GPIA_ASSOC), GPI_ERROR);

        if( MemPS != GPI_ERROR )
        {
          GFX (::GpiCreateLogColorTable (MemPS, 0, LCOLF_RGB, 0, 0, 0), FALSE);

          
          HBITMAP hMemBmp;
          BITMAPINFOHEADER2 bihMem = { 0 };

          bihMem.cbFix = sizeof (BITMAPINFOHEADER2);
          bihMem.cx = aDWidth;
          bihMem.cy = aDHeight;
          bihMem.cPlanes = 1;
          LONG lBitCount = 0;
          GFX (::DevQueryCaps( hdcCompat, CAPS_COLOR_BITCOUNT, 1, &lBitCount), FALSE);
          if (!fPrinting)
          {
            bihMem.cBitCount = (USHORT) lBitCount;
          }
          else  
          {
            
            bihMem.cBitCount = 24;
          }

          hMemBmp = GFX (::GpiCreateBitmap (MemPS, &bihMem, 0, 0, 0), GPI_ERROR);

          if( hMemBmp != GPI_ERROR )
          {
            GFX (::GpiSetBitmap (MemPS, hMemBmp), HBM_ERROR);

            POINTL aptlDevToMem [4] = { {0, 0},                       
                                        {bihMem.cx, bihMem.cy},       
                                        {dest.xLeft, dest.yBottom},   
                                        {dest.xRight, dest.yTop} };   

            GFX (::GpiBitBlt (MemPS, surf->GetPS (), 4, aptlDevToMem, ROP_SRCCOPY, BBO_IGNORE), GPI_ERROR);

            
            
            
            BITMAPINFOHEADER2 bihDirect = { 0 };
            bihDirect.cbFix   = sizeof (BITMAPINFOHEADER2);
            bihDirect.cPlanes = 1;
            bihDirect.cBitCount = 24;

            int RawDataSize = bihMem.cy * RASWIDTH (bihMem.cx, 24);
            PRUint8* pRawBitData = (PRUint8*)malloc (RawDataSize);

            if( pRawBitData )
            {
              LONG rc = GFX (::GpiQueryBitmapBits (MemPS, 0, bihMem.cy, (PBYTE)pRawBitData, (PBITMAPINFO2)&bihDirect), GPI_ALTERROR);

              if( rc != GPI_ALTERROR )
              {
		PRUint8 *imageRGB, *imageAlpha;
		PRUint32 strideRGB, strideAlpha;

		

		if ((aSWidth != aDWidth) || (aSHeight != aDHeight)) {
		  
		  imageRGB = (PRUint8 *)nsMemory::Alloc(3*aDWidth*aDHeight);
		  imageAlpha = (PRUint8 *)nsMemory::Alloc(aDWidth*aDHeight);
		      
		  if (!imageRGB || !imageAlpha) {
		    if (imageRGB)
		      nsMemory::Free(imageRGB);
		    if (imageAlpha)
		      nsMemory::Free(imageAlpha);

		    free(pRawBitData);
		    GFX (::GpiSetBitmap (MemPS, NULLHANDLE), HBM_ERROR);
		    GFX (::GpiDeleteBitmap (hMemBmp), FALSE);
		    GFX (::GpiDestroyPS (MemPS), FALSE);
		    ::DevCloseDC (MemDC);

		    return NS_ERROR_FAILURE;
		  }
		      
		  strideRGB = 3 * aDWidth;
		  strideAlpha = aDWidth;
		  RectStretch(aSWidth, aSHeight, aDWidth, aDHeight, 0, 0, aDWidth-1, aDHeight-1,
			      mImageBits, mRowBytes, imageRGB, strideRGB, 24);
		  RectStretch(aSWidth, aSHeight, aDWidth, aDHeight, 0, 0, aDWidth-1, aDHeight-1,
			      mAlphaBits, mARowBytes, imageAlpha, strideAlpha, 8);
		} else {
                  PRUint32 srcy = mInfo->cy - (aSY + aSHeight);
		  imageRGB = mImageBits + srcy * mRowBytes + aSX * 3;
		  imageAlpha = mAlphaBits + srcy * mARowBytes + aSX;
		  strideRGB = mRowBytes;
		  strideAlpha = mARowBytes;
		}

		
		DrawComposited24(pRawBitData, imageRGB, strideRGB, imageAlpha, strideAlpha,
				 aDWidth, aDHeight);
		  
		if ((aSWidth != aDWidth) || (aSHeight != aDHeight)) {
		  
		  nsMemory::Free(imageRGB);
		  nsMemory::Free(imageAlpha);
		}
        
                
                GFX (::GpiSetBitmapBits (MemPS, 0, bihMem.cy, (PBYTE)pRawBitData, (PBITMAPINFO2)&bihDirect), GPI_ALTERROR);
              
                
                POINTL aptlMemToDev [4] = { {dest.xLeft, dest.yBottom},   
                                            {dest.xRight, dest.yTop},     
                                            {0, 0},                       
                                            {bihMem.cx, bihMem.cy} };      

                GFX (::GpiBitBlt (surf->GetPS (), MemPS, 4, aptlMemToDev, ROP_SRCCOPY, BBO_IGNORE), GPI_ERROR);
            
                rv = NS_OK;
              }

              free (pRawBitData);
            }

            GFX (::GpiSetBitmap (MemPS, NULLHANDLE), HBM_ERROR);
            GFX (::GpiDeleteBitmap (hMemBmp), FALSE);
          }
        
          GFX (::GpiDestroyPS (MemPS), FALSE);
        }
    
        ::DevCloseDC (MemDC);
      }
   }

   return rv;
}

nsresult nsImageOS2::Optimize( nsIDeviceContext* aContext)
{
   
   mIsOptimized = PR_TRUE;
   return NS_OK;
}



NS_IMETHODIMP
nsImageOS2::LockImagePixels(PRBool aMaskPixels)
{
  return NS_OK;
}



NS_IMETHODIMP
nsImageOS2::UnlockImagePixels(PRBool aMaskPixels)
{
  return NS_OK;
} 

void
nsImageOS2::BuildTile (HPS hpsTile, PRUint8* pImageBits, PBITMAPINFO2 pBitmapInfo,
                       nscoord aTileWidth, nscoord aTileHeight, float scale)
{
   
   if (nsRect (0, 0, mInfo->cx, mInfo->cy) != mDecodedRect)
   {
      POINTL pt1 = { 0, 0 };                                                   
      POINTL pt2 = { mInfo->cx, mInfo->cy };                                   

#ifdef DEBUG
      GFX (::GpiSetColor (hpsTile, MK_RGB (255, 255, 0)), FALSE);              
#else
      GFX (::GpiSetColor (hpsTile, MK_RGB (255, 255, 255)), FALSE);
#endif
      GFX (::GpiMove (hpsTile, &pt1), FALSE);
      GFX (::GpiBox (hpsTile, DRO_FILL, &pt2, 0, 0), GPI_ERROR);
   }

   
   POINTL aptl [4] = { {mDecodedRect.x, mDecodedRect.y},                         
                       {mDecodedRect.XMost () - 1, mDecodedRect.YMost () - 1},   
                       {mDecodedRect.x, mDecodedRect.y},                         
                       {mDecodedRect.XMost (), mDecodedRect.YMost ()} };         

   
   aptl[0].x = (LONG)(aptl[0].x * scale);
   aptl[0].y = (LONG)(aptl[0].y * scale);
   aptl[1].x = (LONG)(mDecodedRect.XMost() * scale) - 1;
   aptl[1].y = (LONG)(mDecodedRect.YMost() * scale) - 1;
   
   
   GFX (::GpiDrawBits (hpsTile, (PBYTE)pImageBits, pBitmapInfo, 4, aptl, ROP_SRCCOPY, BBO_IGNORE), GPI_ERROR);

   PRInt32 DestWidth  = (PRInt32)(mInfo->cx * scale);
   PRInt32 DestHeight = (PRInt32)(mInfo->cy * scale);

   
   if (DestWidth > 0) {
      while (DestWidth < aTileWidth)
      {
         POINTL aptlCopy [3] = { {DestWidth, 0},                     
                                 {2 * DestWidth, DestHeight},        
                                 {0, 0} };                           

         GFX (::GpiBitBlt (hpsTile, hpsTile, 3, aptlCopy, ROP_SRCCOPY, 0L), GPI_ERROR);
         DestWidth *= 2;
      }
   }

   
   if (DestHeight > 0) {
      while (DestHeight < aTileHeight)
      {
         POINTL aptlCopy [4] = { {0, DestHeight},                    
                                 {DestWidth, 2 * DestHeight},        
                                 {0, 0} };                           

         GFX (::GpiBitBlt (hpsTile, hpsTile, 3, aptlCopy, ROP_SRCCOPY, 0L), GPI_ERROR);
         DestHeight *= 2;
      }
   }
}





NS_IMETHODIMP nsImageOS2::DrawTile(nsIRenderingContext &aContext,
                                   nsIDrawingSurface* aSurface,
                                   PRInt32 aSXOffset, PRInt32 aSYOffset,
                                   PRInt32 aPadX, PRInt32 aPadY,
                                   const nsRect &aTileRect)
{
   if (aTileRect.IsEmpty ())
      return NS_OK;

   PRBool didTile = PR_FALSE;
   PRInt32 ImageWidth = mInfo->cx;
   PRInt32 ImageHeight = mInfo->cy;
   PRBool padded = (aPadX || aPadY);

   
   nsCOMPtr<nsIDeviceContext> theDeviceContext;
   float scale;
   aContext.GetDeviceContext(*getter_AddRefs(theDeviceContext));
   theDeviceContext->GetCanonicalPixelScale(scale);

   nsRect ValidRect (0, 0, ImageWidth, ImageHeight);
   ValidRect.IntersectRect (ValidRect, mDecodedRect);
   PRInt32 DestScaledWidth = PR_MAX(PRInt32(ValidRect.width * scale), 1);
   PRInt32 DestScaledHeight = PR_MAX(PRInt32(ValidRect.height * scale), 1);

   nsRect DrawRect = aTileRect;
   DrawRect.MoveBy (-aSXOffset, -aSYOffset);
   DrawRect.SizeBy (aSXOffset, aSYOffset);

   
   
   if ((ImageWidth < DrawRect.width / 2 || ImageHeight < DrawRect.height / 2) &&
       (ImageWidth <= MAX_BUFFER_WIDTH) && (ImageHeight <= MAX_BUFFER_HEIGHT) &&
       mAlphaDepth <= 1 &&
       !padded)
   {
      nsDrawingSurfaceOS2 *surf = (nsDrawingSurfaceOS2*) aSurface;

      
      HDC hdcCompat = GFX (::GpiQueryDevice (surf->GetPS ()), HDC_ERROR);

      DEVOPENSTRUC dop = { 0, 0, 0, 0, 0 };
      HDC MemDC = GFX (::DevOpenDC( (HAB)0, OD_MEMORY, "*", 5, (PDEVOPENDATA) &dop, hdcCompat), DEV_ERROR);

      if( DEV_ERROR != MemDC)
      {
         
         SIZEL sizel = { 0, 0 };
         HPS MemPS = GFX (::GpiCreatePS (0, MemDC, &sizel, PU_PELS | GPIT_MICRO | GPIA_ASSOC), GPI_ERROR);

         if( GPI_ERROR != MemPS)
         {
            GFX (::GpiCreateLogColorTable (MemPS, 0, LCOLF_RGB, 0, 0, 0), FALSE);

            
            BITMAPINFOHEADER2 hdr = { 0 };

            hdr.cbFix = sizeof( BITMAPINFOHEADER2);
            
            PRInt32 endWidth = DestScaledWidth;
            while( endWidth < DrawRect.width)
               endWidth *= 2;

            PRInt32 endHeight = DestScaledHeight;
            while( endHeight < DrawRect.height)
               endHeight *= 2;

            hdr.cx = endWidth;
            hdr.cy = endHeight;
            hdr.cPlanes = 1;

            
            LONG lBitCount = 0;
            GFX (::DevQueryCaps( hdcCompat, CAPS_COLOR_BITCOUNT, 1, &lBitCount), FALSE);
            hdr.cBitCount = (USHORT) lBitCount;

            RECTL  rcl;
            surf->NS2PM_INEX (aTileRect, rcl);

            POINTL aptlTile [3] = { {rcl.xLeft, rcl.yBottom},                                 
                                    {rcl.xRight, rcl.yTop},                                   
                                    {aSXOffset, endHeight - aTileRect.height - aSYOffset} };  
            
            if (scale > 1.0)
            {
               aptlTile[2].x = 0;
               aptlTile[2].y = endHeight - aTileRect.height;
            }
            HBITMAP hMemBmp = GFX (::GpiCreateBitmap (MemPS, &hdr, 0, 0, 0), GPI_ERROR);
            if (hMemBmp != GPI_ERROR)
            {
               LONG ImageROP = ROP_SRCCOPY;

               GFX (::GpiSetBitmap (MemPS, hMemBmp), HBM_ERROR);

               if (mAlphaDepth == 1)
               {
                  LONG BlackColor = GFX (::GpiQueryColorIndex (MemPS, 0, MK_RGB (0x00, 0x00, 0x00)), GPI_ALTERROR);    
                  LONG WhiteColor = GFX (::GpiQueryColorIndex (MemPS, 0, MK_RGB (0xFF, 0xFF, 0xFF)), GPI_ALTERROR);    

                  
                  
                  

                  if (BlackColor == GPI_ALTERROR) BlackColor = MK_RGB (0x00, 0x00, 0x00);
                  if (WhiteColor == GPI_ALTERROR) WhiteColor = MK_RGB (0xFF, 0xFF, 0xFF);

                  
                  
                  IMAGEBUNDLE ib;
                  ib.lColor     = BlackColor;        
                  ib.lBackColor = WhiteColor;        
                  ib.usMixMode  = FM_OVERPAINT;
                  ib.usBackMixMode = BM_OVERPAINT;
                  GFX (::GpiSetAttrs (MemPS, PRIM_IMAGE, IBB_COLOR | IBB_BACK_COLOR | IBB_MIX_MODE | IBB_BACK_MIX_MODE, 0, (PBUNDLE)&ib), FALSE);

                  MONOBITMAPINFO MaskBitmapInfo (mInfo);
                  BuildTile (MemPS, mAlphaBits, MaskBitmapInfo, DrawRect.width, DrawRect.height, scale);

                  
                  GFX (::GpiBitBlt (surf->GetPS (), MemPS, 3, aptlTile, ROP_SRCAND, 0L), GPI_ERROR);

                  ImageROP = ROP_SRCPAINT;    
               }

               BuildTile (MemPS, mImageBits, mInfo, DrawRect.width, DrawRect.height, scale);

               GFX (::GpiBitBlt (surf->GetPS (), MemPS, 3, aptlTile, ImageROP, 0L), GPI_ERROR);

               didTile = PR_TRUE;

               
               GFX (::GpiSetBitmap (MemPS, NULLHANDLE), HBM_ERROR);
               GFX (::GpiDeleteBitmap (hMemBmp), FALSE);
            }
            GFX (::GpiDestroyPS (MemPS), FALSE);
         }
         GFX (::DevCloseDC (MemDC), DEV_ERROR);
      }
   }

   
   if( didTile == PR_FALSE)
   {
      
      PRInt32 y0 = aTileRect.y - aSYOffset;
      PRInt32 x0 = aTileRect.x - aSXOffset;
      PRInt32 y1 = aTileRect.y + aTileRect.height;
      PRInt32 x1 = aTileRect.x + aTileRect.width;

      
      
      nscoord ScaledTileWidth = PR_MAX(PRInt32(ImageWidth*scale), 1);
      nscoord ScaledTileHeight = PR_MAX(PRInt32(ImageHeight*scale), 1);

      for (PRInt32 y = y0; y < y1; y += (PRInt32)(ScaledTileHeight + aPadY * scale))
      {
        for (PRInt32 x = x0; x < x1;  x += (PRInt32)(ScaledTileWidth + aPadX * scale))
        {
          Draw(aContext, aSurface,
               0, 0, PR_MIN(ValidRect.width, x1 - x), PR_MIN(ValidRect.height, y1 - y),
               x, y, PR_MIN(DestScaledWidth, x1-x), PR_MIN(DestScaledHeight, y1 - y));
        }
      }
   }
   return NS_OK;
}


void nsImageOS2::NS2PM_ININ( const nsRect &in, RECTL &rcl)
{
  PRUint32 ulHeight = GetHeight ();

  rcl.xLeft = in.x;
  rcl.xRight = in.x + in.width - 1;
  rcl.yTop = ulHeight - in.y - 1;
  rcl.yBottom = rcl.yTop - in.height + 1;
}





NS_IMETHODIMP nsImageOS2::UpdateImageBits( HPS aPS )
{
  BITMAPINFOHEADER2 rawInfo = { 0 };
  rawInfo.cbFix   = sizeof (BITMAPINFOHEADER2);
  rawInfo.cPlanes = 1;
  rawInfo.cBitCount = mInfo->cBitCount;

  int RawDataSize = mInfo->cy * RASWIDTH (mInfo->cx, mInfo->cBitCount);
  PRUint8* pRawBitData = new PRUint8 [RawDataSize];

  if (pRawBitData)
  {
    GFX (::GpiQueryBitmapBits (aPS, 0, mInfo->cy, (PBYTE)pRawBitData, (PBITMAPINFO2)&rawInfo), GPI_ALTERROR);
    delete [] mImageBits;
    mImageBits = pRawBitData;
    return NS_OK;
  }
  else
    return NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP nsImageOS2::DrawToImage(nsIImage* aDstImage,
                                      nscoord aDX, nscoord aDY,
                                      nscoord aDWidth, nscoord aDHeight)
{
  nsresult rc = NS_OK;

  DEVOPENSTRUC dop = { 0, 0, 0, 0, 0 };
  SIZEL sizel = { 0, 0 };

  if (mInfo == nsnull || aDWidth < 0 || aDHeight < 0) 
    return NS_ERROR_FAILURE;

  if (0 == aDWidth || 0 == aDHeight)
    return NS_OK;

  
  HDC MemDC = GFX (::DevOpenDC( 0, OD_MEMORY, "*", 5,
                   (PDEVOPENDATA) &dop, (HDC)0), DEV_ERROR);

  
  HPS MemPS = GFX (::GpiCreatePS (0, MemDC, &sizel, PU_PELS | GPIT_MICRO | GPIA_ASSOC), GPI_ERROR);

  GFX (::GpiCreateLogColorTable (MemPS, 0, LCOLF_RGB, 0, 0, 0), FALSE);

  nsImageOS2* destImg = NS_STATIC_CAST(nsImageOS2*, aDstImage); 

  HBITMAP hTmpBitmap = GFX (::GpiCreateBitmap (MemPS, (PBITMAPINFOHEADER2)destImg->mInfo,
                                               CBM_INIT, (PBYTE)destImg->mImageBits,
                                               destImg->mInfo), GPI_ERROR);
  GFX (::GpiSetBitmap (MemPS, hTmpBitmap), HBM_ERROR);
  
  nsRect trect( aDX, aDY, aDWidth, aDHeight);
  RECTL  rcl;
  destImg->NS2PM_ININ (trect, rcl);

  
  POINTL aptl [4] = { {rcl.xLeft, rcl.yBottom},              
                      {rcl.xRight, rcl.yTop},                
                      {0, 0},                                
                      {mInfo->cx, mInfo->cy} };              

  if( 1==mAlphaDepth && mAlphaBits)
  {
    LONG BlackColor = GFX (::GpiQueryColorIndex (MemPS, 0, MK_RGB (0x00, 0x00, 0x00)), GPI_ALTERROR);    
    LONG WhiteColor = GFX (::GpiQueryColorIndex (MemPS, 0, MK_RGB (0xFF, 0xFF, 0xFF)), GPI_ALTERROR);    
    if (BlackColor == GPI_ALTERROR) BlackColor = MK_RGB (0x00, 0x00, 0x00);
    if (WhiteColor == GPI_ALTERROR) WhiteColor = MK_RGB (0xFF, 0xFF, 0xFF);

    
    
    IMAGEBUNDLE ib;
    ib.lColor     = BlackColor;        
    ib.lBackColor = WhiteColor;        
    ib.usMixMode  = FM_OVERPAINT;
    ib.usBackMixMode = BM_OVERPAINT;
    GFX (::GpiSetAttrs (MemPS, PRIM_IMAGE, IBB_COLOR | IBB_BACK_COLOR | IBB_MIX_MODE | IBB_BACK_MIX_MODE, 0, (PBUNDLE)&ib), FALSE);

    
    MONOBITMAPINFO MaskBitmapInfo (mInfo);
    GFX (::GpiDrawBits (MemPS, mAlphaBits, MaskBitmapInfo, 4, aptl, ROP_SRCAND,
                        BBO_IGNORE), GPI_ERROR);

    
    GFX (::GpiDrawBits (MemPS, mImageBits, mInfo, 4, aptl, ROP_SRCPAINT, 
                        BBO_IGNORE), GPI_ERROR);
  } else {
    
    NS_ASSERTION( mAlphaDepth != 8, "Alpha depth of 8 not implemented in DrawToImage" );

    
    GFX (::GpiDrawBits (MemPS, mImageBits, mInfo, 4, aptl, ROP_SRCCOPY,
                        BBO_IGNORE), GPI_ERROR);
  }

  rc = destImg->UpdateImageBits (MemPS);

  GFX (::GpiSetBitmap (MemPS, NULLHANDLE), HBM_ERROR);
  GFX (::GpiDeleteBitmap (hTmpBitmap), FALSE);
  GFX (::GpiDestroyPS (MemPS), FALSE);
  GFX (::DevCloseDC (MemDC), DEV_ERROR);

  return rc;
}
