





































#include "nsBlender.h"
#include "nsCRT.h"





nsBlender :: nsBlender()
{
}





nsBlender::~nsBlender() 
{
}

NS_IMPL_ISUPPORTS1(nsBlender, nsIBlender)






#if defined(XP_MAC) || defined(XP_MACOSX)

#define BLEND_RED_MASK        0x7c00
#define BLEND_GREEN_MASK      0x03e0
#define BLEND_BLUE_MASK       0x001f
#define BLEND_RED_SET_MASK    0xf8
#define BLEND_GREEN_SET_MASK  0xf8
#define BLEND_BLUE_SET_MASK   0xf8
#define BLEND_RED_SHIFT       7
#define BLEND_GREEN_SHIFT     2
#define BLEND_BLUE_SHIFT      3
#define BLEND_GREEN_BITS      5

#else  

#define BLEND_RED_MASK        0xf800
#define BLEND_GREEN_MASK      0x07e0
#define BLEND_BLUE_MASK       0x001f
#define BLEND_RED_SET_MASK    0xf8
#define BLEND_GREEN_SET_MASK  0xfC
#define BLEND_BLUE_SET_MASK   0xf8
#define BLEND_RED_SHIFT       8
#define BLEND_GREEN_SHIFT     3
#define BLEND_BLUE_SHIFT      3
#define BLEND_GREEN_BITS      6

#endif





NS_IMETHODIMP
nsBlender::Init(nsIDeviceContext *aContext)
{
  return NS_OK;
}

#define RED16(x)    (((x) & BLEND_RED_MASK) >> BLEND_RED_SHIFT)
#define GREEN16(x)  (((x) & BLEND_GREEN_MASK) >> BLEND_GREEN_SHIFT)
#define BLUE16(x)   (((x) & BLEND_BLUE_MASK) << BLEND_BLUE_SHIFT)

static void rangeCheck(nsIDrawingSurface* surface, PRInt32& aX, PRInt32& aY, PRInt32& aWidth, PRInt32& aHeight)
{
  PRUint32 width, height;
  surface->GetDimensions(&width, &height);
  
  
  if (aX < 0)
    aX = 0;
  else if (aX > (PRInt32)width)
    aX = width;
  if (aY < 0)
    aY = 0;
  else if (aY > (PRInt32)height)
    aY = height;
  
  
  if (aX + aWidth > (PRInt32)width)
    aWidth = width - aX;
  if (aY + aHeight > (PRInt32)height)
    aHeight = height - aY;
}





NS_IMETHODIMP
nsBlender::Blend(PRInt32 aSX, PRInt32 aSY, PRInt32 aWidth, PRInt32 aHeight, nsIDrawingSurface* aSrc,
                 nsIDrawingSurface* aDst, PRInt32 aDX, PRInt32 aDY, float aSrcOpacity,
                 nsIDrawingSurface* aSecondSrc, nscolor aSrcBackColor,
                 nscolor aSecondSrcBackColor)
{
  NS_ASSERTION(aSrc, "nsBlender::Blend() called with nsnull aSrc");
  NS_ASSERTION(aDst, "nsBlender::Blend() called with nsnull aDst");
  NS_ENSURE_ARG_POINTER(aSrc);
  NS_ENSURE_ARG_POINTER(aDst);

  if (aSecondSrc) {
    
    NS_ASSERTION(aSrcBackColor == NS_RGB(0, 0, 0),
      "Background color for primary source must be black");
    NS_ASSERTION(aSecondSrcBackColor == NS_RGB(255, 255, 255),
      "Background color for secondary source must be white");
    if (aSrcBackColor != NS_RGB(0, 0, 0) ||
        aSecondSrcBackColor != NS_RGB(255, 255, 255)) {
      
      
      aSecondSrc = nsnull;
    }
  }
 
  nsresult result = NS_ERROR_FAILURE;

  
  rangeCheck(aSrc, aSX, aSY, aWidth, aHeight);
  rangeCheck(aDst, aDX, aDY, aWidth, aHeight);

  if (aWidth <= 0 || aHeight <= 0)
    return NS_OK;

  PRUint8* srcBytes = nsnull;
  PRUint8* secondSrcBytes = nsnull;
  PRUint8* destBytes = nsnull;
  PRInt32 srcSpan, destSpan, secondSrcSpan;
  PRInt32 srcRowBytes, destRowBytes, secondSrcRowBytes;

  result = aSrc->Lock(aSX, aSY, aWidth, aHeight, (void**)&srcBytes, &srcSpan, &srcRowBytes, NS_LOCK_SURFACE_READ_ONLY);
  if (NS_SUCCEEDED(result)) {

    
    
    
    PRUint32 depth = (srcRowBytes / aWidth) * 8;

    result = aDst->Lock(aDX, aDY, aWidth, aHeight, (void**)&destBytes, &destSpan, &destRowBytes, 0);
    if (NS_SUCCEEDED(result)) {
      NS_ASSERTION(srcRowBytes == destRowBytes, "Mismatched lock-bitmap sizes (src/dest) in Blender");
      if (srcRowBytes == destRowBytes) {
        if (aSecondSrc) {
          result = aSecondSrc->Lock(aSX, aSY, aWidth, aHeight, (void**)&secondSrcBytes, &secondSrcSpan, &secondSrcRowBytes, NS_LOCK_SURFACE_READ_ONLY);
          if (NS_SUCCEEDED(result)) {
            NS_ASSERTION(srcSpan == secondSrcSpan && srcRowBytes == secondSrcRowBytes,
                         "Mismatched bitmap formats (src/secondSrc) in Blender");                         
            if (srcSpan == secondSrcSpan && srcRowBytes == secondSrcRowBytes) {
              result = Blend(srcBytes, srcSpan,
                             destBytes, destSpan,
                             secondSrcBytes,
                             srcRowBytes, aHeight, aSrcOpacity, depth);
            }
            
            aSecondSrc->Unlock();
          }
        }
        else
        {
          result = Blend(srcBytes, srcSpan,
                         destBytes, destSpan,
                         secondSrcBytes,
                         srcRowBytes, aHeight, aSrcOpacity, depth);
        }
      }

      aDst->Unlock();
    }

    aSrc->Unlock();
  }

  return result;
}





NS_IMETHODIMP nsBlender::Blend(PRInt32 aSX, PRInt32 aSY, PRInt32 aWidth, PRInt32 aHeight, nsIRenderingContext *aSrc,
                               nsIRenderingContext *aDest, PRInt32 aDX, PRInt32 aDY, float aSrcOpacity,
                               nsIRenderingContext *aSecondSrc, nscolor aSrcBackColor,
                               nscolor aSecondSrcBackColor)
{
  
  nsIDrawingSurface* srcSurface, *destSurface, *secondSrcSurface = nsnull;
  aSrc->GetDrawingSurface(&srcSurface);
  aDest->GetDrawingSurface(&destSurface);
  if (aSecondSrc != nsnull)
    aSecondSrc->GetDrawingSurface(&secondSrcSurface);
  return Blend(aSX, aSY, aWidth, aHeight, srcSurface, destSurface,
               aDX, aDY, aSrcOpacity, secondSrcSurface, aSrcBackColor,
               aSecondSrcBackColor);
}

#ifndef MOZ_XUL
NS_IMETHODIMP nsBlender::GetAlphas(const nsRect& aRect, nsIDrawingSurface* aBlack,
                                   nsIDrawingSurface* aWhite, PRUint8** aAlphas) {
  NS_ERROR("GetAlphas not implemented because XUL support not built");
  return NS_ERROR_NOT_IMPLEMENTED;
}
#else







static void ComputeAlphasByByte(PRInt32 aNumLines, PRInt32 aBytesPerLine,
                                PRInt32 aBytesPerPixel,
                                PRUint8 *aOnBlackImage, PRUint8 *aOnWhiteImage,
                                PRInt32 aBytesLineSpan, PRUint8 *aAlphas,
                                PRUint32 aAlphasSize)
{
  NS_ASSERTION(aBytesPerPixel == 3 || aBytesPerPixel == 4,
               "Only 24 or 32 bits per pixel supported here");

  PRIntn y;
  PRUint8* alphas = aAlphas;
  for (y = 0; y < aNumLines; y++) {
    
    
    PRUint8 *s1 = aOnBlackImage + 1;
    PRUint8 *s2 = aOnWhiteImage + 1;
    
    PRIntn i;
    for (i = 1; i < aBytesPerLine; i += aBytesPerPixel) {
      *alphas++ = (PRUint8)(255 - (*s2 - *s1));
      s1 += aBytesPerPixel;
      s2 += aBytesPerPixel;
    }
  
    aOnBlackImage += aBytesLineSpan;
    aOnWhiteImage += aBytesLineSpan;
  }

  NS_ASSERTION(alphas - aAlphas == aAlphasSize, "alpha24/32 calculation error");
}








static void ComputeAlphas16(PRInt32 aNumLines, PRInt32 aBytesPerLine,
                            PRUint8 *aOnBlackImage, PRUint8 *aOnWhiteImage,
                            PRInt32 aBytesLineSpan, PRUint8 *aAlphas,
                            PRUint32 aAlphasSize)
{
  PRIntn y;
  PRUint8* alphas = aAlphas;
  for (y = 0; y < aNumLines; y++) {
    PRUint16 *s1 = (PRUint16*)aOnBlackImage;
    PRUint16 *s2 = (PRUint16*)aOnWhiteImage;
    
      
      
      
      
      
      
      
      
      
    const PRUint32 SCALE_DENOMINATOR =   
      ((1 << BLEND_GREEN_BITS) - 1) << (8 - BLEND_GREEN_BITS);

    PRIntn i;
    for (i = 0; i < aBytesPerLine; i += 2) {
      PRUint32 pix1 = GREEN16(*s1);
      PRUint32 pix2 = GREEN16(*s2);
      *alphas++ = (PRUint8)(255 - ((pix2 - pix1)*255)/SCALE_DENOMINATOR);
      s1++;
      s2++;
    }
    
    aOnBlackImage += aBytesLineSpan;
    aOnWhiteImage += aBytesLineSpan;
  }

  NS_ASSERTION(alphas - aAlphas == aAlphasSize, "alpha16 calculation error");
}

static void ComputeAlphas(PRInt32 aNumLines, PRInt32 aBytesPerLine,
                          PRInt32 aDepth,
                          PRUint8 *aOnBlackImage, PRUint8 *aOnWhiteImage,
                          PRInt32 aBytesLineSpan, PRUint8 *aAlphas,
                          PRUint32 aAlphasSize)
{
  switch (aDepth) {
    case 32:
    case 24:
      ComputeAlphasByByte(aNumLines, aBytesPerLine, aDepth/8,
                          aOnBlackImage, aOnWhiteImage,
                          aBytesLineSpan, aAlphas, aAlphasSize);
      break;

    case 16:
      ComputeAlphas16(aNumLines, aBytesPerLine, aOnBlackImage, aOnWhiteImage,
                      aBytesLineSpan, aAlphas, aAlphasSize);
      break;
    
    default:
      NS_ERROR("Unknown depth for alpha calculation");
      
      memset(aAlphas, 255, aAlphasSize);
  }
}

NS_IMETHODIMP nsBlender::GetAlphas(const nsRect& aRect, nsIDrawingSurface* aBlack,
                                   nsIDrawingSurface* aWhite, PRUint8** aAlphas) {
  nsresult result;

  nsIDrawingSurface* blackSurface = (nsIDrawingSurface *)aBlack;
  nsIDrawingSurface* whiteSurface = (nsIDrawingSurface *)aWhite;

  nsRect r = aRect;

  rangeCheck(blackSurface, r.x, r.y, r.width, r.height);
  rangeCheck(whiteSurface, r.x, r.y, r.width, r.height);

  PRUint8* blackBytes = nsnull;
  PRUint8* whiteBytes = nsnull;
  PRInt32 blackSpan, whiteSpan;
  PRInt32 blackBytesPerLine, whiteBytesPerLine;

  result = blackSurface->Lock(r.x, r.y, r.width, r.height,
                              (void**)&blackBytes, &blackSpan,
                              &blackBytesPerLine, NS_LOCK_SURFACE_READ_ONLY);
  if (NS_SUCCEEDED(result)) {
    result = whiteSurface->Lock(r.x, r.y, r.width, r.height,
                                (void**)&whiteBytes, &whiteSpan,
                                &whiteBytesPerLine, NS_LOCK_SURFACE_READ_ONLY);
    if (NS_SUCCEEDED(result)) {
      NS_ASSERTION(blackSpan == whiteSpan &&
                   blackBytesPerLine == whiteBytesPerLine,
                   "Mismatched bitmap formats (black/white) in Blender");
      if (blackSpan == whiteSpan && blackBytesPerLine == whiteBytesPerLine) {
        *aAlphas = new PRUint8[r.width*r.height];
        if (*aAlphas) {
          
          
          
          PRUint32 depth = (blackBytesPerLine/r.width)*8;
          ComputeAlphas(r.height, blackBytesPerLine, depth,
                        blackBytes, whiteBytes, blackSpan, 
                        *aAlphas, r.width*r.height);
        } else {
          result = NS_ERROR_FAILURE;
        }
      } else {
        result = NS_ERROR_FAILURE;
      }

      whiteSurface->Unlock();
    }

    blackSurface->Unlock();
  }
  
  return result;
}
#endif 




static void Do8Blend(float aOpacity, PRInt32 aNumLines, PRInt32 aNumBytes,
                     PRUint8 *aSImage, PRUint8 *aS2Image, PRUint8 *aDImage,
                     PRInt32 aSLSpan, PRInt32 aDLSpan)
{
  if (aOpacity <= 0.0) {
    return;
  }

  
  
  PRIntn y;
  if (!aS2Image) {
    for (y = 0; y < aNumLines; y++) {
      memcpy(aDImage, aSImage, aNumBytes);
      aSImage += aSLSpan;
      aDImage += aDLSpan;
    }
  } else {
    for (y = 0; y < aNumLines; y++) {
      for (int i = 0; i < aNumBytes; i++) {
        if (aSImage[i] == aS2Image[i]) {
          aDImage[i] = aSImage[i];
        }
      }
      aSImage += aSLSpan;
      aS2Image += aSLSpan;
      aDImage += aDLSpan;
    }
  }
}





nsresult nsBlender::Blend(PRUint8 *aSrcBits, PRInt32 aSrcStride,
                          PRUint8 *aDestBits, PRInt32 aDestStride,
                          PRUint8 *aSecondSrcBits,
                          PRInt32 aSrcBytes, PRInt32 aLines, float aOpacity,
                          PRUint8 aDepth)
{
  nsresult result = NS_OK;
  switch (aDepth) {
    case 32:
        Do32Blend(aOpacity, aLines, aSrcBytes, aSrcBits, aDestBits,
                  aSecondSrcBits, aSrcStride, aDestStride, nsHighQual);
        break;

    case 24:
        Do24Blend(aOpacity, aLines, aSrcBytes, aSrcBits, aDestBits,
                  aSecondSrcBits, aSrcStride, aDestStride, nsHighQual);
        break;

    case 16:
        Do16Blend(aOpacity, aLines, aSrcBytes, aSrcBits, aDestBits,
                  aSecondSrcBits, aSrcStride, aDestStride, nsHighQual);
        break;

    default:
        Do8Blend(aOpacity, aLines, aSrcBytes, aSrcBits, aSecondSrcBits,
               aDestBits, aSrcStride, aDestStride);
        break;
  }

  return result;
}
























static void DoSingleImageBlend(PRUint32 aOpacity256, PRInt32 aNumLines, PRInt32 aNumBytes,
                               PRUint8 *aSImage, PRUint8 *aDImage,
                               PRInt32 aSLSpan, PRInt32 aDLSpan)
{
  PRIntn y;

  for (y = 0; y < aNumLines; y++) {
    PRUint8 *s2 = aSImage;
    PRUint8 *d2 = aDImage;
    
    PRIntn i;
    for (i = 0; i < aNumBytes; i++) {
      PRUint32 destPix = *d2;
      
      *d2 = (PRUint8)(destPix + (((*s2 - destPix)*aOpacity256) >> 8));
      
      d2++;
      s2++;
    }
    
    aSImage += aSLSpan;
    aDImage += aDLSpan;
  }
}







































void
nsBlender::Do32Blend(float aOpacity, PRInt32 aNumLines, PRInt32 aNumBytes,
                     PRUint8 *aSImage, PRUint8 *aDImage, PRUint8 *aSecondSImage,
                     PRInt32 aSLSpan, PRInt32 aDLSpan, nsBlendQuality aBlendQuality)
{
  

  PRUint32 opacity256 = (PRUint32)(aOpacity*256);

  
  if (opacity256 <= 0) {
    return;
  }
  if (nsnull == aSecondSImage) {
    DoSingleImageBlend(opacity256, aNumLines, aNumBytes, aSImage, aDImage, aSLSpan, aDLSpan);
    return;
  }

  PRIntn numPixels = aNumBytes/4;

  PRIntn y;
  for (y = 0; y < aNumLines; y++) {
    PRUint8 *s2 = aSImage;
    PRUint8 *d2 = aDImage;
    PRUint8 *ss2 = aSecondSImage;

    PRIntn x;
    for (x = 0; x < numPixels; x++) {
      PRUint32 pixSColor  = *((PRUint32*)(s2))&0xFFFFFF;
      PRUint32 pixSSColor = *((PRUint32*)(ss2))&0xFFFFFF;
      
      if ((pixSColor != 0x000000) || (pixSSColor != 0xFFFFFF)) {
        if (pixSColor != pixSSColor) {
          PRIntn i;
          
          
          for (i = 0; i < 4; i++) {
            PRUint32 destPix = *d2;
            PRUint32 onBlack = *s2;
            PRUint32 imageAlphaTimesDestPix = (255 + onBlack - *ss2)*destPix;
            PRUint32 adjustedDestPix;
            FAST_DIVIDE_BY_255(adjustedDestPix, imageAlphaTimesDestPix);
            
            *d2 = (PRUint8)(destPix + (((onBlack - adjustedDestPix)*opacity256) >> 8));
            
            d2++;
            s2++;
            ss2++;
          }
        } else {
          PRIntn i;
          for (i = 0; i < 4; i++) {
            PRUint32 destPix = *d2;
            PRUint32 onBlack = *s2;
            
            *d2 = (PRUint8)(destPix + (((onBlack - destPix)*opacity256) >> 8));
            
            d2++;
            s2++;
          }

          ss2 += 4;
        }
      } else {
        d2 += 4;
        s2 += 4;
        ss2 += 4;
      }
    }
    
    aSImage += aSLSpan;
    aDImage += aDLSpan;
    aSecondSImage += aSLSpan;
  }
}





void
nsBlender::Do24Blend(float aOpacity, PRInt32 aNumLines, PRInt32 aNumBytes,
                     PRUint8 *aSImage, PRUint8 *aDImage, PRUint8 *aSecondSImage,
                     PRInt32 aSLSpan, PRInt32 aDLSpan, nsBlendQuality aBlendQuality)
{
  

  PRUint32 opacity256 = (PRUint32)(aOpacity*256);

  
  if (opacity256 <= 0) {
    return;
  }
  if (nsnull == aSecondSImage) {
    DoSingleImageBlend(opacity256, aNumLines, aNumBytes, aSImage, aDImage, aSLSpan, aDLSpan);
    return;
  }

  PRIntn numPixels = aNumBytes/3;

  PRIntn y;
  for (y = 0; y < aNumLines; y++) {
    PRUint8 *s2 = aSImage;
    PRUint8 *d2 = aDImage;
    PRUint8 *ss2 = aSecondSImage;

    PRIntn x;
    for (x = 0; x < numPixels; x++) {
      PRUint32 pixSColor  = s2[0] | (s2[1] << 8) | (s2[2] << 16);
      PRUint32 pixSSColor = ss2[0] | (ss2[1] << 8) | (ss2[2] << 16);
      
      if ((pixSColor != 0x000000) || (pixSSColor != 0xFFFFFF)) {
        if (pixSColor != pixSSColor) {
          PRIntn i;
          
          
          for (i = 0; i < 3; i++) {
            PRUint32 destPix = *d2;
            PRUint32 onBlack = *s2;
            PRUint32 imageAlphaTimesDestPix = (255 + onBlack - *ss2)*destPix;
            PRUint32 adjustedDestPix;
            FAST_DIVIDE_BY_255(adjustedDestPix, imageAlphaTimesDestPix);
            
            *d2 = (PRUint8)(destPix + (((onBlack - adjustedDestPix)*opacity256) >> 8));
            
            d2++;
            s2++;
            ss2++;
          }
        } else {
          PRIntn i;
          for (i = 0; i < 3; i++) {
            PRUint32 destPix = *d2;
            PRUint32 onBlack = *s2;
            
            *d2 = (PRUint8)(destPix + (((onBlack - destPix)*opacity256) >> 8));
            
            d2++;
            s2++;
          }

          ss2 += 3;
        }
      } else {
        d2 += 3;
        s2 += 3;
        ss2 += 3;
      }
    }
    
    aSImage += aSLSpan;
    aDImage += aDLSpan;
    aSecondSImage += aSLSpan;
  }
}





#define MAKE16(r, g, b)                                              \
        (PRUint16)(((r) & BLEND_RED_SET_MASK) << BLEND_RED_SHIFT)    \
          | (((g) & BLEND_GREEN_SET_MASK) << BLEND_GREEN_SHIFT)      \
          | (((b) & BLEND_BLUE_SET_MASK) >> BLEND_BLUE_SHIFT)





void
nsBlender::Do16Blend(float aOpacity, PRInt32 aNumLines, PRInt32 aNumBytes,
                     PRUint8 *aSImage, PRUint8 *aDImage, PRUint8 *aSecondSImage,
                     PRInt32 aSLSpan, PRInt32 aDLSpan, nsBlendQuality aBlendQuality)
{
  PRUint32 opacity256 = (PRUint32)(aOpacity*256);

  
  if (opacity256 <= 0) {
    return;
  }

  PRIntn numPixels = aNumBytes/2;
  
  if (nsnull == aSecondSImage) {
    PRIntn y;
    for (y = 0; y < aNumLines; y++) {
      PRUint16 *s2 = (PRUint16*)aSImage;
      PRUint16 *d2 = (PRUint16*)aDImage;
      
      PRIntn i;
      for (i = 0; i < numPixels; i++) {
        PRUint32 destPix = *d2;
        PRUint32 destPixR = RED16(destPix);
        PRUint32 destPixG = GREEN16(destPix);
        PRUint32 destPixB = BLUE16(destPix);
        PRUint32 srcPix = *s2;
        
        *d2 = MAKE16(destPixR + (((RED16(srcPix) - destPixR)*opacity256) >> 8),
                     destPixG + (((GREEN16(srcPix) - destPixG)*opacity256) >> 8),
                     destPixB + (((BLUE16(srcPix) - destPixB)*opacity256) >> 8));
        d2++;
        s2++;
      }
      
      aSImage += aSLSpan;
      aDImage += aDLSpan;
    }
    return;
  }

  PRUint32 srcBackgroundColor = MAKE16(0x00, 0x00, 0x00);
  PRUint32 src2BackgroundColor = MAKE16(0xFF, 0xFF, 0xFF);

  PRIntn y;
  for (y = 0; y < aNumLines; y++) {
    PRUint16 *s2 = (PRUint16*)aSImage;
    PRUint16 *d2 = (PRUint16*)aDImage;
    PRUint16 *ss2 = (PRUint16*)aSecondSImage;

    PRIntn x;
    for (x = 0; x < numPixels; x++) {
      PRUint32 srcPix = *s2;
      PRUint32 src2Pix = *ss2;

      if ((srcPix != srcBackgroundColor) || (src2Pix != src2BackgroundColor)) {
        PRUint32 destPix = *d2;
        PRUint32 destPixR = RED16(destPix);
        PRUint32 destPixG = GREEN16(destPix);
        PRUint32 destPixB = BLUE16(destPix);
        PRUint32 srcPixR = RED16(srcPix);
        PRUint32 srcPixG = GREEN16(srcPix);
        PRUint32 srcPixB = BLUE16(srcPix);
          
        if (srcPix != src2Pix) {
          PRUint32 imageAlphaTimesDestPixR = (255 + srcPixR - RED16(src2Pix))*destPixR;
          PRUint32 imageAlphaTimesDestPixG = (255 + srcPixG - GREEN16(src2Pix))*destPixG;
          PRUint32 imageAlphaTimesDestPixB = (255 + srcPixB - BLUE16(src2Pix))*destPixB;
          PRUint32 adjustedDestPixR;
          FAST_DIVIDE_BY_255(adjustedDestPixR, imageAlphaTimesDestPixR);
          PRUint32 adjustedDestPixG;
          FAST_DIVIDE_BY_255(adjustedDestPixG, imageAlphaTimesDestPixG);
          PRUint32 adjustedDestPixB;
          FAST_DIVIDE_BY_255(adjustedDestPixB, imageAlphaTimesDestPixB);
            
          *d2 = MAKE16(destPixR + (((srcPixR - adjustedDestPixR)*opacity256) >> 8),
            destPixG + (((srcPixG - adjustedDestPixG)*opacity256) >> 8),
            destPixB + (((srcPixB - adjustedDestPixB)*opacity256) >> 8));
        } else {
          *d2 = MAKE16(destPixR + (((srcPixR - destPixR)*opacity256) >> 8),
            destPixG + (((srcPixG - destPixG)*opacity256) >> 8),
            destPixB + (((srcPixB - destPixB)*opacity256) >> 8));
        }
      }

      d2++;
      s2++;
      ss2++;
    }
    
    aSImage += aSLSpan;
    aDImage += aDLSpan;
    aSecondSImage += aSLSpan;
  }
}


