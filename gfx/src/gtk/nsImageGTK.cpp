






































#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "imgScaler.h"

#include "nsImageGTK.h"
#include "nsRenderingContextGTK.h"

#include "nspr.h"

#define IsFlagSet(a,b) ((a) & (b))

#define NS_GET_BIT(rowptr, x) (rowptr[(x)>>3] &  (1<<(7-(x)&0x7)))
#define NS_SET_BIT(rowptr, x) (rowptr[(x)>>3] |= (1<<(7-(x)&0x7)))
#define NS_CLEAR_BIT(rowptr, x) (rowptr[(x)>>3] &= ~(1<<(7-(x)&0x7)))















static GdkGC *s1bitGC = nsnull;
static GdkGC *sXbitGC = nsnull;



static PRBool sNeedSlowTile = PR_FALSE;

#ifdef MOZ_WIDGET_GTK2
NS_IMPL_ISUPPORTS2(nsImageGTK, nsIImage, nsIGdkPixbufImage)
#else
NS_IMPL_ISUPPORTS1(nsImageGTK, nsIImage)
#endif



nsImageGTK::nsImageGTK()
  : mImageBits(nsnull)
  , mImagePixmap(nsnull)
  , mTrueAlphaBits(nsnull)
  , mAlphaBits(nsnull)
  , mAlphaPixmap(nsnull)
  , mAlphaXImage(nsnull)
  , mWidth(0)
  , mHeight(0)
  , mRowBytes(0)
  , mSizeImage(0)
  , mDecodedX1(PR_INT32_MAX)
  , mDecodedY1(PR_INT32_MAX)
  , mDecodedX2(0)
  , mDecodedY2(0)
  , mAlphaDepth(0)
  , mTrueAlphaDepth(0)
  , mIsSpacer(PR_TRUE)
  , mPendingUpdate(PR_FALSE)
  , mDepth(0)
  , mOptimized(PR_FALSE)
{
#ifdef TRACE_IMAGE_ALLOCATION
  printf("nsImageGTK::nsImageGTK(this=%p)\n",
         this);
#endif
}



nsImageGTK::~nsImageGTK()
{
  if(nsnull != mImageBits) {
    free(mImageBits);
    mImageBits = nsnull;
  }

  if (nsnull != mAlphaBits) {
    free(mAlphaBits);
    mAlphaBits = nsnull;
  }

  if (nsnull != mTrueAlphaBits) {
    free(mTrueAlphaBits);
    mTrueAlphaBits = nsnull;
  }

  if (mAlphaPixmap) {
    gdk_pixmap_unref(mAlphaPixmap);
  }

  if (mImagePixmap) {
    gdk_pixmap_unref(mImagePixmap);
  }

  if (mAlphaXImage) {
    mAlphaXImage->data = 0;
    XDestroyImage(mAlphaXImage);
  }

#ifdef TRACE_IMAGE_ALLOCATION
  printf("nsImageGTK::~nsImageGTK(this=%p)\n",
         this);
#endif
}

 void
nsImageGTK::Startup()
{
  Display *dpy = GDK_DISPLAY();

  if (strstr(ServerVendor(dpy), "XFree86") && VendorRelease(dpy) < 40400000)
    sNeedSlowTile = PR_TRUE;
}

 void
nsImageGTK::Shutdown()
{
  if (s1bitGC) {
    gdk_gc_unref(s1bitGC);
    s1bitGC = nsnull;
  }
  if (sXbitGC) {
    gdk_gc_unref(sXbitGC);
    sXbitGC = nsnull;
  }
}



nsresult nsImageGTK::Init(PRInt32 aWidth, PRInt32 aHeight,
                          PRInt32 aDepth, nsMaskRequirements aMaskRequirements)
{
  
  g_return_val_if_fail ((aWidth != 0) || (aHeight != 0), NS_ERROR_FAILURE);

  
  
  if (aWidth > SHRT_MAX || aHeight > SHRT_MAX)
    return NS_ERROR_FAILURE;

  if (24 == aDepth) {
    mNumBytesPixel = 3;
  } else {
    NS_ASSERTION(PR_FALSE, "unexpected image depth");
    return NS_ERROR_UNEXPECTED;
  }

  mWidth = aWidth;
  mHeight = aHeight;
  mDepth = aDepth;

#ifdef TRACE_IMAGE_ALLOCATION
  printf("nsImageGTK::Init(this=%p,%d,%d,%d,%d)\n",
         this,
         aWidth,
         aHeight,
         aDepth,
         aMaskRequirements);
#endif

  
  ComputeMetrics();

  mImageBits = (PRUint8*)malloc(mSizeImage);
  if (!mImageBits)
    return NS_ERROR_OUT_OF_MEMORY;

  switch(aMaskRequirements)
  {
    case nsMaskRequirements_kNeeds8Bit:
      mTrueAlphaRowBytes = aWidth;
      mTrueAlphaDepth = 8;

      
      mTrueAlphaRowBytes = (mTrueAlphaRowBytes + 3) & ~0x3;
      mTrueAlphaBits = (PRUint8*)calloc(mTrueAlphaRowBytes * aHeight, 1);
      if (!mTrueAlphaBits)
        return NS_ERROR_OUT_OF_MEMORY;

      

    case nsMaskRequirements_kNeeds1Bit:
      mAlphaRowBytes = (aWidth + 7) / 8;
      mAlphaDepth = 1;

      
      mAlphaRowBytes = (mAlphaRowBytes + 3) & ~0x3;

      mAlphaBits = (PRUint8*)calloc(mAlphaRowBytes * aHeight, 1);
      if (!mAlphaBits)
        return NS_ERROR_OUT_OF_MEMORY;
      break;

    default:
      break; 
  }

  if (aMaskRequirements == nsMaskRequirements_kNeeds8Bit)
    mAlphaDepth = 0;
  
  return NS_OK;
}



PRInt32 nsImageGTK::GetHeight()
{
  return mHeight;
}

PRInt32 nsImageGTK::GetWidth()
{
  return mWidth;
}

PRUint8 *nsImageGTK::GetBits()
{
  return mImageBits;
}

void *nsImageGTK::GetBitInfo()
{
  return nsnull;
}

PRInt32 nsImageGTK::GetLineStride()
{
  return mRowBytes;
}

nsColorMap *nsImageGTK::GetColorMap()
{
  return nsnull;
}

PRUint8 *nsImageGTK::GetAlphaBits()
{
  if (mTrueAlphaBits)
    return mTrueAlphaBits;
  else
    return mAlphaBits;
}

PRInt32
nsImageGTK::GetAlphaLineStride()
{
  if (mTrueAlphaBits)
    return mTrueAlphaRowBytes;
  else
    return mAlphaRowBytes;
}

void nsImageGTK::ImageUpdated(nsIDeviceContext *aContext,
                              PRUint8 aFlags,
                              nsRect *aUpdateRect)
{
  mPendingUpdate = PR_TRUE;
  mUpdateRegion.Or(mUpdateRegion, *aUpdateRect);

  mDecodedX1 = PR_MIN(mDecodedX1, aUpdateRect->x);
  mDecodedY1 = PR_MIN(mDecodedY1, aUpdateRect->y);

  if (aUpdateRect->YMost() > mDecodedY2)
    mDecodedY2 = aUpdateRect->YMost();
  if (aUpdateRect->XMost() > mDecodedX2)
    mDecodedX2 = aUpdateRect->XMost();
}




PRBool nsImageGTK::GetIsImageComplete() {
  return mDecodedX1 == 0 &&
         mDecodedY1 == 0 &&
         mDecodedX2 == mWidth &&
         mDecodedY2 == mHeight;
}

void nsImageGTK::UpdateCachedImage()
{
#ifdef TRACE_IMAGE_ALLOCATION
  printf("nsImageGTK::ImageUpdated(this=%p)\n",
         this);
#endif

  nsRegionRectIterator ri(mUpdateRegion);
  const nsRect *rect;

  while ((rect = ri.Next()) != nsnull) {




    unsigned bottom, left, right;
    bottom = rect->y + rect->height;
    left   = rect->x;
    right  = left + rect->width;

    
    if ((mTrueAlphaDepth==8) && (mAlphaDepth<mTrueAlphaDepth)) {
      for (unsigned y=rect->y; 
           (y<bottom) && (mAlphaDepth<mTrueAlphaDepth); 
           y++) {
        unsigned char *alpha = mTrueAlphaBits + mTrueAlphaRowBytes*y + left;
        unsigned char *mask = mAlphaBits + mAlphaRowBytes*y;
        for (unsigned x=left; x<right; x++) {
          switch (*(alpha++)) {
          case 255:
            NS_SET_BIT(mask,x);
            break;
          case 0:
            NS_CLEAR_BIT(mask,x);
            if (mAlphaDepth == 0) {
              mAlphaDepth=1;

              
              
              CreateOffscreenPixmap(mWidth, mHeight);

              XFillRectangle(GDK_WINDOW_XDISPLAY(mAlphaPixmap),
                             GDK_WINDOW_XWINDOW(mAlphaPixmap),
                             GDK_GC_XGC(s1bitGC),
                             mDecodedX1, mDecodedY1,
                             mDecodedX2 - mDecodedX1 + 1,
                             mDecodedY2 - mDecodedY1 + 1);
            }
            break;
          default:
            mAlphaDepth=8;
            break;
          }
        }
      }
      
      if (mAlphaDepth==8) {
        if (mImagePixmap) {
          gdk_pixmap_unref(mImagePixmap);
          mImagePixmap = 0;
        }
        if (mAlphaPixmap) {
          gdk_pixmap_unref(mAlphaPixmap);
          mAlphaPixmap = 0;
        }
        if (mAlphaBits) {
          free(mAlphaBits);
          mAlphaBits = mTrueAlphaBits;
          mAlphaRowBytes = mTrueAlphaRowBytes;
          mTrueAlphaBits = 0;
        }
      }
    }

    
    if ((mAlphaDepth==1) && mIsSpacer) {
      
      PRUint8  leftmask   = 0xff  >> (left & 0x7);
      PRUint8  rightmask  = 0xff  << (7 - ((right-1) & 0x7));

      
      PRUint32 leftindex  = left      >> 3;
      PRUint32 rightindex = (right-1) >> 3;

      
      
      if (leftindex == rightindex) {
        leftmask &= rightmask;
        rightmask = 0xff;
      }

      
      if (leftmask != 0xff) {
        PRUint8 *ptr = mAlphaBits + mAlphaRowBytes * rect->y + leftindex;
        for (unsigned y=rect->y; y<bottom; y++, ptr+=mAlphaRowBytes) {
          if (*ptr & leftmask) {
            mIsSpacer = PR_FALSE;
            break;
          }
        }
        
        leftindex++;
      }

      
      if (mIsSpacer && (rightmask != 0xff)) {
        PRUint8 *ptr = mAlphaBits + mAlphaRowBytes * rect->y + rightindex;
        for (unsigned y=rect->y; y<bottom; y++, ptr+=mAlphaRowBytes) {
          if (*ptr & rightmask) {
            mIsSpacer = PR_FALSE;
            break;
          }
        }
        
        rightindex--;
      }
    
      
      if (mIsSpacer && (leftindex <= rightindex)) {
        for (unsigned y=rect->y; (y<bottom) && mIsSpacer; y++) {
          unsigned char *alpha = mAlphaBits + mAlphaRowBytes*y + leftindex;
          for (unsigned x=leftindex; x<=rightindex; x++) {
            if (*(alpha++)!=0) {
              mIsSpacer = PR_FALSE;
              break;
            }
          }
        }
      }
    }

    if (mAlphaDepth != 8) {
      CreateOffscreenPixmap(mWidth, mHeight);

      gdk_draw_rgb_image_dithalign(mImagePixmap, sXbitGC, 
                                   rect->x, rect->y,
                                   rect->width, rect->height,
                                   GDK_RGB_DITHER_MAX,
                                   mImageBits + mRowBytes*rect->y + 3*rect->x,
                                   mRowBytes,
                                   0, 0);
    }

    if (mAlphaDepth==1) {
      XPutImage(GDK_WINDOW_XDISPLAY(mAlphaPixmap),
                GDK_WINDOW_XWINDOW(mAlphaPixmap),
                GDK_GC_XGC(s1bitGC),
                mAlphaXImage,
                rect->x, rect->y, 
                rect->x, rect->y,
                rect->width, rect->height);
    }
  }
  
  mUpdateRegion.SetEmpty();
  mPendingUpdate = PR_FALSE;
  mFlags = nsImageUpdateFlags_kBitsChanged; 
}

#ifdef CHEAP_PERFORMANCE_MEASURMENT
static PRTime gConvertTime, gAlphaTime, gCopyStart, gCopyEnd, gStartTime, gPixmapTime, gEndTime;
#endif



#define sign(x) ((x)>0 ? 1:-1)

static void XlibStretchHorizontal(long x1,long x2,long y1,long y2,
                                  long ymin,long ymax,
                                  long startColumn, long endColumn,
                                  long offsetX, long offsetY,
                                  GdkPixmap *aSrcImage, GdkPixmap *aDstImage, GdkGC *gc);














void
XlibRectStretch(PRInt32 srcWidth, PRInt32 srcHeight,
                PRInt32 dstWidth, PRInt32 dstHeight,
                PRInt32 dstOrigX, PRInt32 dstOrigY,
                PRInt32 aDX, PRInt32 aDY,
                PRInt32 aDWidth, PRInt32 aDHeight,
                GdkPixmap *aSrcImage, GdkPixmap *aDstImage,
                GdkGC *gc, GdkGC *copygc, PRInt32 aDepth)
{
  long dx,dy,e,d,dx2;
  short sx,sy;
  GdkPixmap *aTmpImage = 0;
  PRBool skipHorizontal=PR_FALSE, skipVertical=PR_FALSE;
  long startColumn, startRow, endColumn, endRow;
  long xs1, ys1, xs2, ys2, xd1, yd1, xd2, yd2;

  xs1 = ys1 = xd1 = yd1 = 0;
  xs2 = srcWidth-1;
  ys2 = srcHeight-1;
  xd2 = dstWidth-1;
  yd2 = dstHeight-1;



  
  startColumn = aDX-dstOrigX;
  startRow    = aDY-dstOrigY;
  endColumn   = aDX+aDWidth-dstOrigX;
  endRow      = aDY+aDHeight-dstOrigY;





  long scaleStartY, scaleEndY;
  scaleStartY = startRow * (ys2-ys1+1) / (yd2-yd1+1);
  scaleEndY   = 1 + endRow * (ys2-ys1+1) / (yd2-yd1+1);

  if (xd2-xd1 == xs2-xs1) {

    skipHorizontal = PR_TRUE;
    aTmpImage = aSrcImage;
    scaleStartY = 0;
    scaleEndY = ys2;
  }

  if (yd2-yd1 == ys2-ys1) {

    skipVertical = PR_TRUE;
    aTmpImage = aDstImage;
  }

  if (skipVertical && skipHorizontal) {
    gdk_draw_pixmap(aDstImage, gc, aSrcImage,
                    0, 0, srcWidth, srcHeight,
                    dstOrigX, dstOrigY);
    return;
  }



  if (!skipHorizontal && !skipVertical) {
    aTmpImage = gdk_pixmap_new(nsnull,
                               endColumn-startColumn,
                               scaleEndY-scaleStartY,
                               aDepth);
#ifdef MOZ_WIDGET_GTK2
    if (aDepth != 1)
      gdk_drawable_set_colormap(GDK_DRAWABLE(aTmpImage),
                                gdk_rgb_get_colormap());
#endif
  }
 
  dx = abs((int)(yd2-yd1));
  dy = abs((int)(ys2-ys1));
  sx = sign(yd2-yd1);
  sy = sign(ys2-ys1);
  e = dy-dx;
  dx2 = dx;
  dy += 1;
  if (!dx2) dx2=1;

  if (!skipHorizontal)
    XlibStretchHorizontal(xd1, xd2, xs1, xs2, scaleStartY, scaleEndY,
                          startColumn, endColumn,
                          skipVertical?dstOrigX:-startColumn, skipVertical?dstOrigY:-scaleStartY,
                          aSrcImage, aTmpImage, (skipVertical?gc:copygc));
  
  if (!skipVertical) {
    for (d=0; d<=dx; d++) {
      if ((yd1 >= startRow) && (yd1 <= endRow)) {
        gdk_draw_pixmap(aDstImage, gc, aTmpImage,
                        (skipHorizontal?startColumn:0), ys1-scaleStartY,
                        aDX, dstOrigY+yd1,
                        endColumn-startColumn, 1);
      }
      while (e>=0) {
	      ys1 += sy;
	      e -= dx2;
      }
      yd1 += sx;
      e += dy;
    }
  }

  if (!skipHorizontal && !skipVertical)
    gdk_pixmap_unref(aTmpImage);
}











static void
XlibStretchHorizontal(long x1, long x2, long y1, long y2,
                      long ymin, long ymax,
                      long startColumn, long endColumn,
                      long offsetX, long offsetY,
                      GdkPixmap *aSrcImage, GdkPixmap *aDstImage, GdkGC *gc)
{
  long dx,dy,e,d,dx2;
  short sx,sy;

  dx = abs((int)(x2-x1));
  dy = abs((int)(y2-y1));
  sx = sign(x2-x1);
  sy = sign(y2-y1);
  e = dy-dx;
  dx2 = dx;
  dy += 1;
  if (!dx2) dx2=1;
  for (d=0; d<=dx; d++) {
    if ((x1 >= startColumn) && (x1 <= endColumn)) {
      gdk_draw_pixmap(aDstImage, gc, aSrcImage,
                      y1, ymin, x1+offsetX, ymin+offsetY,
                      1, ymax-ymin);
    }
    while (e>=0) {
      y1 += sy;
      e -= dx2;
    }
    x1 += sx;
    e += dy;
  }
}

#undef sign




NS_IMETHODIMP
nsImageGTK::Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
                 PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                 PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight)
{
  g_return_val_if_fail ((aSurface != nsnull), NS_ERROR_FAILURE);

  if (mPendingUpdate)
    UpdateCachedImage();

  if ((mAlphaDepth==1) && mIsSpacer)
    return NS_OK;

  if (mDecodedX2 < mDecodedX1 || mDecodedY2 < mDecodedY1)
    return NS_OK;

#ifdef TRACE_IMAGE_ALLOCATION
  fprintf(stderr, "nsImageGTK::Draw(%p) s=(%4d %4d %4d %4d) d=(%4d %4d %4d %4d)\n",
         this,
         aSX, aSY, aSWidth, aSHeight,
         aDX, aDY, aDWidth, aDHeight);
#endif

  if (aSWidth <= 0 || aDWidth <= 0 || aSHeight <= 0 || aDHeight <= 0) {
    return NS_OK;
  }

  

  PRInt32 srcWidth, srcHeight, dstWidth, dstHeight;
  PRInt32 dstOrigX, dstOrigY;

  srcWidth = aSWidth;
  srcHeight = aSHeight;
  dstWidth = aDWidth;
  dstHeight = aDHeight;
  dstOrigX = aDX;
  dstOrigY = aDY;

  
  PRInt32 j = aSX + aSWidth;
  PRInt32 z;
  if (j > mDecodedX2) {
    z = j - mDecodedX2;
    aDWidth -= z*dstWidth/srcWidth;
    aSWidth -= z;
  }
  if (aSX < mDecodedX1) {
    aDX += (mDecodedX1 - aSX)*dstWidth/srcWidth;
    aSX = mDecodedX1;
  }

  j = aSY + aSHeight;
  if (j > mDecodedY2) {
    z = j - mDecodedY2;
    aDHeight -= z*dstHeight/srcHeight;
    aSHeight -= z;
  }
  if (aSY < mDecodedY1) {
    aDY += (mDecodedY1 - aSY)*dstHeight/srcHeight;
    aSY = mDecodedY1;
  }

  if (aDWidth <= 0 || aDHeight <= 0 || aSWidth <= 0 || aSHeight <= 0) {
    return NS_OK;
  }

  
  nsDrawingSurfaceGTK *drawing = (nsDrawingSurfaceGTK*)aSurface;
  PRUint32 surfaceWidth, surfaceHeight;
  drawing->GetDimensions(&surfaceWidth, &surfaceHeight);

  if (aDX + aDWidth > (PRInt32)surfaceWidth) {
    z = aDX + aDWidth - surfaceWidth;
    aDWidth -= z;
    aSWidth -= z*srcWidth/dstWidth;
  }

  if (aDX < 0) {
    aDWidth += aDX;
    aSWidth += aDX*srcWidth/dstWidth;
    aSX -= aDX*srcWidth/dstWidth;
    aDX = 0;
  }

  if (aDY + aDHeight > (PRInt32)surfaceHeight) {
    z = aDY + aDHeight - surfaceHeight;
    aDHeight -= z;
    aSHeight -= z*srcHeight/dstHeight;
  }

  if (aDY < 0) {
    aDHeight += aDY;
    aSHeight += aDY*srcHeight/dstHeight;
    aSY -= aDY*srcHeight/dstHeight;
    aDY = 0;
  }

  if (aDWidth <= 0 || aDHeight <= 0 || aSWidth <= 0 || aSHeight <= 0) {
    return NS_OK;
  }

  if ((srcWidth != dstWidth) || (srcHeight != dstHeight)) {
    GdkPixmap *pixmap = 0;
    GdkGC *gc = 0;
    nsRegionGTK clipRgn;

    switch (mAlphaDepth) {
    case 8:
      DrawComposited(aContext, aSurface,
                     srcWidth, srcHeight,
                     dstWidth, dstHeight,
                     dstOrigX, dstOrigY,
                     aDX, aDY,
                     aDWidth, aDHeight);
      break;
    case 1:
      pixmap = gdk_pixmap_new(nsnull, dstWidth, dstHeight, 1);
      if (pixmap) {
        XlibRectStretch(srcWidth, srcHeight,
                        dstWidth, dstHeight,
                        0, 0,
                        0, 0,
                        dstWidth, dstHeight,
                        mAlphaPixmap, pixmap,
                        s1bitGC, s1bitGC, 1);
        gc = gdk_gc_new(drawing->GetDrawable());
        if (gc) {
          gdk_gc_set_clip_origin(gc, dstOrigX, dstOrigY);
          gdk_gc_set_clip_mask(gc, pixmap);
        }
      }

      if (gdk_rgb_get_visual()->depth <= 8) {
        PRUint8 *scaledRGB = (PRUint8 *)nsMemory::Alloc(3*dstWidth*dstHeight);

        if (!scaledRGB)
          return NS_ERROR_OUT_OF_MEMORY;

        RectStretch(mWidth, mHeight,
                    dstWidth, dstHeight,
                    0, 0, dstWidth-1, dstHeight-1,
                    mImageBits, mRowBytes, scaledRGB, 3*dstWidth, 24);

        if (NS_SUCCEEDED(((nsRenderingContextGTK&)aContext).CopyClipRegion(clipRgn))) {
          
          
          nsRegionRectSet *rectSet = nsnull;
          clipRgn.Intersect(aDX, aDY, aDWidth, aDHeight);
          clipRgn.GetRects(&rectSet);
          for (PRUint32 i=0; i<rectSet->mRectsLen; i++) {
            nsRegionRect *rect = &(rectSet->mRects[i]);

            gdk_draw_rgb_image_dithalign(drawing->GetDrawable(), gc,
                                         rect->x, rect->y, rect->width, rect->height,
                                         GDK_RGB_DITHER_MAX, 
                                         scaledRGB + 3*((rect->y-dstOrigY)*dstWidth+(rect->x-dstOrigX)),
                                         3*dstWidth,
                                         (rect->x-dstOrigX), (rect->y-dstOrigY));
          }
          clipRgn.FreeRects(rectSet);
        } else {
          gdk_draw_rgb_image_dithalign(drawing->GetDrawable(), gc,
                                       aDX, aDY, aDWidth, aDHeight,
                                       GDK_RGB_DITHER_MAX, 
                                       scaledRGB + 3*((aDY-dstOrigY)*dstWidth+(aDX-dstOrigX)),
                                       3*dstWidth,
                                       (aDX-dstOrigX), (aDY-dstOrigY));
        }
        nsMemory::Free(scaledRGB);
      } else {
        if (NS_SUCCEEDED(((nsRenderingContextGTK&)aContext).CopyClipRegion(clipRgn))) {
          
          
          nsRegionRectSet *rectSet = nsnull;
          clipRgn.Intersect(aDX, aDY, aDWidth, aDHeight);
          clipRgn.GetRects(&rectSet);
          for (PRUint32 i=0; i<rectSet->mRectsLen; i++) {
            nsRegionRect *rect = &(rectSet->mRects[i]);
            
            XlibRectStretch(srcWidth, srcHeight,
                            dstWidth, dstHeight,
                            dstOrigX, dstOrigY,
                            rect->x, rect->y,
                            rect->width, rect->height,
                            mImagePixmap, drawing->GetDrawable(),
                            gc, sXbitGC, gdk_rgb_get_visual()->depth);
          }
          clipRgn.FreeRects(rectSet);
        } else {
          
          XlibRectStretch(srcWidth, srcHeight,
                          dstWidth, dstHeight,
                          dstOrigX, dstOrigY,
                          aDX, aDY,
                          aDWidth, aDHeight,
                          mImagePixmap, drawing->GetDrawable(),
                          gc, sXbitGC, gdk_rgb_get_visual()->depth);
        }
      }

      break;
    case 0:
      if (!gc)
        gc = ((nsRenderingContextGTK&)aContext).GetGC();

      if (gdk_rgb_get_visual()->depth <= 8) {
        PRUint8 *scaledRGB = (PRUint8 *)nsMemory::Alloc(3*dstWidth*dstHeight);
        if (!scaledRGB)
          break;
        RectStretch(mWidth, mHeight,
                    dstWidth, dstHeight,
                    0, 0, dstWidth-1, dstHeight-1,
                    mImageBits, mRowBytes, scaledRGB, 3*dstWidth, 24);
    
        gdk_draw_rgb_image_dithalign(drawing->GetDrawable(), gc,
                                     aDX, aDY, aDWidth, aDHeight,
                                     GDK_RGB_DITHER_MAX, 
                                     scaledRGB + 3*((aDY-dstOrigY)*dstWidth+(aDX-dstOrigX)),
                                     3*dstWidth,
                                     (aDX-dstOrigX), (aDY-dstOrigY));

        nsMemory::Free(scaledRGB);
      }
      else
        XlibRectStretch(srcWidth, srcHeight,
                        dstWidth, dstHeight,
                        dstOrigX, dstOrigY,
                        aDX, aDY,
                        aDWidth, aDHeight,
                        mImagePixmap, drawing->GetDrawable(),
                        gc, sXbitGC, gdk_rgb_get_visual()->depth);
      break;
    }
    if (gc)
      gdk_gc_unref(gc);
    if (pixmap)
      gdk_pixmap_unref(pixmap);

    mFlags = 0;
    return NS_OK;
  }

  

  if (mAlphaDepth==8) {
    DrawComposited(aContext, aSurface, 
                   srcWidth, srcHeight,
                   dstWidth, dstHeight,
                   aDX-aSX, aDY-aSY,
                   aDX, aDY,
                   aDWidth, aDHeight);
    return NS_OK;
  }

  GdkGC *copyGC;
  if (mAlphaPixmap) {
    copyGC = gdk_gc_new(drawing->GetDrawable());
    GdkGC *gc = ((nsRenderingContextGTK&)aContext).GetGC();
    gdk_gc_copy(copyGC, gc);
    gdk_gc_unref(gc); 
    
    SetupGCForAlpha(copyGC, aDX-aSX, aDY-aSY);
  } else {
    
    copyGC = ((nsRenderingContextGTK&)aContext).GetGC();
  }

  nsRegionGTK clipRgn;
  if (mAlphaPixmap &&
      NS_SUCCEEDED(((nsRenderingContextGTK&)aContext).CopyClipRegion(clipRgn))) {
    
    
    nsRegionRectSet *rectSet = nsnull;
    clipRgn.Intersect(aDX, aDY, aSWidth, aSHeight);
    clipRgn.GetRects(&rectSet);
    for (PRUint32 i=0; i<rectSet->mRectsLen; i++) {
      nsRegionRect *rect = &(rectSet->mRects[i]);
      gdk_window_copy_area(drawing->GetDrawable(),      
                           copyGC,                      
                           rect->x,                     
                           rect->y,                     
                           mImagePixmap,                
                           aSX+(rect->x-aDX),           
                           aSY+(rect->y-aDY),           
                           rect->width,                 
                           rect->height);               
    }
    clipRgn.FreeRects(rectSet);
  } else {
    
    gdk_window_copy_area(drawing->GetDrawable(),      
                         copyGC,                      
                         aDX,                         
                         aDY,                         
                         mImagePixmap,                
                         aSX,                         
                         aSY,                         
                         aSWidth,                     
                         aSHeight);                   
  }
 
  gdk_gc_unref(copyGC);
  mFlags = 0;

  return NS_OK;
}


















static unsigned
findIndex32(unsigned mask)
{
  switch (mask) {
  case 0xff:
    return 3;
  case 0xff00:
    return 2;
  case 0xff0000:
    return 1;
  case 0xff000000:
    return 0;
  default:
    return 0;
  }
}

static unsigned
findIndex24(unsigned mask)
{
  switch (mask) {
  case 0xff:
    return 2;
  case 0xff00:
    return 1;
  case 0xff0000:
    return 0;
  default:
    return 0;
  }
}



void
nsImageGTK::DrawComposited32(PRBool isLSB, PRBool flipBytes,
                             PRUint8 *imageOrigin, PRUint32 imageStride,
                             PRUint8 *alphaOrigin, PRUint32 alphaStride,
                             unsigned width, unsigned height,
                             XImage *ximage, unsigned char *readData, unsigned char *srcData)
{
  GdkVisual *visual   = gdk_rgb_get_visual();
  unsigned redIndex   = findIndex32(visual->red_mask);
  unsigned greenIndex = findIndex32(visual->green_mask);
  unsigned blueIndex  = findIndex32(visual->blue_mask);

  if (flipBytes^isLSB) {
    redIndex   = 3-redIndex;
    greenIndex = 3-greenIndex;
    blueIndex  = 3-blueIndex;
  }





  for (unsigned y=0; y<height; y++) {
    unsigned char *baseRow   = srcData     + y*ximage->bytes_per_line;
    unsigned char *targetRow = readData    + 3*(y*ximage->width);
    unsigned char *imageRow  = imageOrigin + y*imageStride;
    unsigned char *alphaRow  = alphaOrigin + y*alphaStride;

    for (unsigned i=0; i<width;
         i++, baseRow+=4, targetRow+=3, imageRow+=3, alphaRow++) {
      unsigned alpha = *alphaRow;
      MOZ_BLEND(targetRow[0], baseRow[redIndex],   imageRow[0], alpha);
      MOZ_BLEND(targetRow[1], baseRow[greenIndex], imageRow[1], alpha);
      MOZ_BLEND(targetRow[2], baseRow[blueIndex],  imageRow[2], alpha);
    }
  }
}


void
nsImageGTK::DrawComposited24(PRBool isLSB, PRBool flipBytes,
                             PRUint8 *imageOrigin, PRUint32 imageStride,
                             PRUint8 *alphaOrigin, PRUint32 alphaStride,
                             unsigned width, unsigned height,
                             XImage *ximage, unsigned char *readData, unsigned char *srcData)
{
  GdkVisual *visual   = gdk_rgb_get_visual();
  unsigned redIndex   = findIndex24(visual->red_mask);
  unsigned greenIndex = findIndex24(visual->green_mask);
  unsigned blueIndex  = findIndex24(visual->blue_mask);

  if (flipBytes^isLSB) {
    redIndex   = 2-redIndex;
    greenIndex = 2-greenIndex;
    blueIndex  = 2-blueIndex;
  }

  for (unsigned y=0; y<height; y++) {
    unsigned char *baseRow   = srcData     + y*ximage->bytes_per_line;
    unsigned char *targetRow = readData    + 3*(y*ximage->width);
    unsigned char *imageRow  = imageOrigin + y*imageStride;
    unsigned char *alphaRow  = alphaOrigin + y*alphaStride;

    for (unsigned i=0; i<width;
         i++, baseRow+=3, targetRow+=3, imageRow+=3, alphaRow++) {
      unsigned alpha = *alphaRow;
      MOZ_BLEND(targetRow[0], baseRow[redIndex],   imageRow[0], alpha);
      MOZ_BLEND(targetRow[1], baseRow[greenIndex], imageRow[1], alpha);
      MOZ_BLEND(targetRow[2], baseRow[blueIndex],  imageRow[2], alpha);
    }
  }
}

unsigned nsImageGTK::scaled6[1<<6] = {
  3,   7,  11,  15,  19,  23,  27,  31,  35,  39,  43,  47,  51,  55,  59,  63,
 67,  71,  75,  79,  83,  87,  91,  95,  99, 103, 107, 111, 115, 119, 123, 127,
131, 135, 139, 143, 147, 151, 155, 159, 163, 167, 171, 175, 179, 183, 187, 191,
195, 199, 203, 207, 211, 215, 219, 223, 227, 231, 235, 239, 243, 247, 251, 255
};

unsigned nsImageGTK::scaled5[1<<5] = {
  7,  15,  23,  31,  39,  47,  55,  63,  71,  79,  87,  95, 103, 111, 119, 127,
135, 143, 151, 159, 167, 175, 183, 191, 199, 207, 215, 223, 231, 239, 247, 255
};


void
nsImageGTK::DrawComposited16(PRBool isLSB, PRBool flipBytes,
                             PRUint8 *imageOrigin, PRUint32 imageStride,
                             PRUint8 *alphaOrigin, PRUint32 alphaStride,
                             unsigned width, unsigned height,
                             XImage *ximage, unsigned char *readData, unsigned char *srcData)
{
  GdkVisual *visual   = gdk_rgb_get_visual();

  unsigned *redScale   = (visual->red_prec   == 5) ? scaled5 : scaled6;
  unsigned *greenScale = (visual->green_prec == 5) ? scaled5 : scaled6;
  unsigned *blueScale  = (visual->blue_prec  == 5) ? scaled5 : scaled6;

  for (unsigned y=0; y<height; y++) {
    unsigned char *baseRow   = srcData     + y*ximage->bytes_per_line;
    unsigned char *targetRow = readData    + 3*(y*ximage->width);
    unsigned char *imageRow  = imageOrigin + y*imageStride;
    unsigned char *alphaRow  = alphaOrigin + y*alphaStride;

    for (unsigned i=0; i<width;
         i++, baseRow+=2, targetRow+=3, imageRow+=3, alphaRow++) {
      unsigned pix;
      if (flipBytes) {
        unsigned char tmp[2];
        tmp[0] = baseRow[1];
        tmp[1] = baseRow[0]; 
        pix = *((short *)tmp); 
      } else
        pix = *((short *)baseRow);
      unsigned alpha = *alphaRow;
      MOZ_BLEND(targetRow[0],
                redScale[(pix&visual->red_mask)>>visual->red_shift], 
                imageRow[0], alpha);
      MOZ_BLEND(targetRow[1],
                greenScale[(pix&visual->green_mask)>>visual->green_shift], 
                imageRow[1], alpha);
      MOZ_BLEND(targetRow[2],
                blueScale[(pix&visual->blue_mask)>>visual->blue_shift], 
                imageRow[2], alpha);
    }
  }
}


void
nsImageGTK::DrawCompositedGeneral(PRBool isLSB, PRBool flipBytes,
                                  PRUint8 *imageOrigin, PRUint32 imageStride,
                                  PRUint8 *alphaOrigin, PRUint32 alphaStride,
                                  unsigned width, unsigned height,
                                  XImage *ximage, unsigned char *readData, unsigned char *srcData)
{
  GdkVisual *visual     = gdk_rgb_get_visual();
  GdkColormap *colormap = gdk_rgb_get_cmap();

  
  if (flipBytes && (ximage->bits_per_pixel>=16)) {
    for (int row=0; row<ximage->height; row++) {
      unsigned char *ptr = srcData + row*ximage->bytes_per_line;
      if (ximage->bits_per_pixel==24) {  
        for (int col=0;
             col<ximage->bytes_per_line;
             col+=(ximage->bits_per_pixel/8)) {
          unsigned char tmp;
          tmp = *ptr;
          *ptr = *(ptr+2);
          *(ptr+2) = tmp;
          ptr+=3;
        }
        continue;
      }
      
      for (int col=0; 
               col<ximage->bytes_per_line;
               col+=(ximage->bits_per_pixel/8)) {
        unsigned char tmp;
        switch (ximage->bits_per_pixel) {
        case 16:
          tmp = *ptr;
          *ptr = *(ptr+1);
          *(ptr+1) = tmp;
          ptr+=2;
          break; 
        case 32:
          tmp = *ptr;
          *ptr = *(ptr+3);
          *(ptr+3) = tmp;
          tmp = *(ptr+1);
          *(ptr+1) = *(ptr+2);
          *(ptr+2) = tmp;
          ptr+=4;
          break;
        }
      }
    }
  }

  unsigned redScale, greenScale, blueScale, redFill, greenFill, blueFill;
  redScale =   8-visual->red_prec;
  greenScale = 8-visual->green_prec;
  blueScale =  8-visual->blue_prec;
  redFill =   0xff>>visual->red_prec;
  greenFill = 0xff>>visual->green_prec;
  blueFill =  0xff>>visual->blue_prec;

  for (unsigned row=0; row<height; row++) {
    unsigned char *ptr = srcData + row*ximage->bytes_per_line;
    unsigned char *target = readData+3*row*ximage->width;
    for (unsigned col=0; col<width; col++) {
      unsigned pix;
      switch (ximage->bits_per_pixel) {
      case 1:
        pix = (*ptr>>(col%8))&1;
        if ((col%8)==7)
          ptr++;
        break;
      case 4:
        pix = (col&1)?(*ptr>>4):(*ptr&0xf);
        if (col&1)
          ptr++;
        break;
      case 8:
        pix = *ptr++;
        break;
      case 16:
        pix = *((short *)ptr);
        ptr+=2;
        break;
      case 24:
        if (isLSB)
          pix = (*(ptr+2)<<16) | (*(ptr+1)<<8) | *ptr;
        else
          pix = (*ptr<<16) | (*(ptr+1)<<8) | *(ptr+2);
        ptr+=3;
        break;
      case 32:
        pix = *((unsigned *)ptr);
        ptr+=4;
        break;
      }
      switch (visual->type) {
      case GDK_VISUAL_STATIC_GRAY:
      case GDK_VISUAL_GRAYSCALE:
      case GDK_VISUAL_STATIC_COLOR:
      case GDK_VISUAL_PSEUDO_COLOR:
        *target++ = colormap->colors[pix].red   >>8;
        *target++ = colormap->colors[pix].green >>8;
        *target++ = colormap->colors[pix].blue  >>8;
        break;
        
      case GDK_VISUAL_DIRECT_COLOR:
        *target++ = 
          colormap->colors[(pix&visual->red_mask)>>visual->red_shift].red       >> 8;
        *target++ = 
          colormap->colors[(pix&visual->green_mask)>>visual->green_shift].green >> 8;
        *target++ =
          colormap->colors[(pix&visual->blue_mask)>>visual->blue_shift].blue    >> 8;
        break;
        
      case GDK_VISUAL_TRUE_COLOR:
        *target++ = 
          redFill|((pix&visual->red_mask)>>visual->red_shift)<<redScale;
        *target++ = 
          greenFill|((pix&visual->green_mask)>>visual->green_shift)<<greenScale;
        *target++ = 
          blueFill|((pix&visual->blue_mask)>>visual->blue_shift)<<blueScale;
        break;
      }
    }
  }

  
  for (unsigned y=0; y<height; y++) {
    unsigned char *targetRow = readData+3*y*ximage->width;
    unsigned char *imageRow  = imageOrigin + y*imageStride;
    unsigned char *alphaRow  = alphaOrigin + y*alphaStride;
    
    for (unsigned i=0; i<width; i++) {
      unsigned alpha = alphaRow[i];
      MOZ_BLEND(targetRow[3*i],   targetRow[3*i],   imageRow[3*i],   alpha);
      MOZ_BLEND(targetRow[3*i+1], targetRow[3*i+1], imageRow[3*i+1], alpha);
      MOZ_BLEND(targetRow[3*i+2], targetRow[3*i+2], imageRow[3*i+2], alpha);
    }
  }
}

void
nsImageGTK::DrawComposited(nsIRenderingContext &aContext,
                           nsIDrawingSurface* aSurface,
                           PRInt32 srcWidth, PRInt32 srcHeight,
                           PRInt32 dstWidth, PRInt32 dstHeight,
                           PRInt32 dstOrigX, PRInt32 dstOrigY,
                           PRInt32 aDX, PRInt32 aDY,
                           PRInt32 aDWidth, PRInt32 aDHeight)
{
  nsDrawingSurfaceGTK* drawing = (nsDrawingSurfaceGTK*) aSurface;
  GdkVisual *visual = gdk_rgb_get_visual();
    
  Display *dpy = GDK_WINDOW_XDISPLAY(drawing->GetDrawable());
  Drawable drawable = GDK_WINDOW_XWINDOW(drawing->GetDrawable());

  int readX, readY;
  unsigned readWidth, readHeight, destX, destY;

  destX = aDX-dstOrigX;
  destY = aDY-dstOrigY;
  readX = aDX;
  readY = aDY;
  readWidth = aDWidth;
  readHeight = aDHeight;






  XImage *ximage = XGetImage(dpy, drawable,
                             readX, readY, readWidth, readHeight, 
                             AllPlanes, ZPixmap);

  NS_ASSERTION((ximage!=NULL), "XGetImage() failed");
  if (!ximage)
    return;

  unsigned char *readData = 
    (unsigned char *)nsMemory::Alloc(3*readWidth*readHeight);
  if (!readData) {
    XDestroyImage(ximage);
    return;
  }

  PRUint8 *scaledImage = 0;
  PRUint8 *scaledAlpha = 0;
  PRUint8 *imageOrigin, *alphaOrigin;
  PRUint32 imageStride, alphaStride;
  if ((srcWidth!=dstWidth) || (srcHeight!=dstHeight)) {
    PRUint32 x1, y1, x2, y2;
    x1 = destX*srcWidth/dstWidth;
    y1 = destY*srcHeight/dstHeight;
    x2 = (destX+aDWidth)*srcWidth/dstWidth;
    y2 = (destY+aDHeight)*srcHeight/dstHeight;

    scaledImage = (PRUint8 *)nsMemory::Alloc(3*aDWidth*aDHeight);
    scaledAlpha = (PRUint8 *)nsMemory::Alloc(aDWidth*aDHeight);
    if (!scaledImage || !scaledAlpha) {
      XDestroyImage(ximage);
      nsMemory::Free(readData);
      if (scaledImage)
        nsMemory::Free(scaledImage);
      if (scaledAlpha)
        nsMemory::Free(scaledAlpha);
      return;
    }
    RectStretch(srcWidth, srcHeight,
                dstWidth, dstHeight,
                destX, destY,
                destX+aDWidth-1, destY+aDHeight-1,
                mImageBits, mRowBytes, scaledImage, 3*readWidth, 24);
    RectStretch(srcWidth, srcHeight,
                dstWidth, dstHeight,
                destX, destY,
                destX+aDWidth-1, destY+aDHeight-1,
                mAlphaBits, mAlphaRowBytes, scaledAlpha, readWidth, 8);
    imageOrigin = scaledImage;
    imageStride = 3*readWidth;
    alphaOrigin = scaledAlpha;
    alphaStride = readWidth;
  } else {
    imageOrigin = mImageBits + destY*mRowBytes + 3*destX;
    imageStride = mRowBytes;
    alphaOrigin = mAlphaBits + destY*mAlphaRowBytes + destX;
    alphaStride = mAlphaRowBytes;
  }

  PRBool isLSB;
  unsigned test = 1;
  isLSB = (((char *)&test)[0]) ? 1 : 0;

  PRBool flipBytes = 
    ( isLSB && ximage->byte_order != LSBFirst) ||
    (!isLSB && ximage->byte_order == LSBFirst);

  if ((ximage->bits_per_pixel==32) &&
      (visual->red_prec == 8) &&
      (visual->green_prec == 8) &&
      (visual->blue_prec == 8))
    DrawComposited32(isLSB, flipBytes, 
                     imageOrigin, imageStride,
                     alphaOrigin, alphaStride, 
                     readWidth, readHeight, ximage, readData, (unsigned char *)ximage->data);
  else if ((ximage->bits_per_pixel==24) &&
           (visual->red_prec == 8) && 
           (visual->green_prec == 8) &&
           (visual->blue_prec == 8))
    DrawComposited24(isLSB, flipBytes, 
                     imageOrigin, imageStride,
                     alphaOrigin, alphaStride, 
                     readWidth, readHeight, ximage, readData, (unsigned char *)ximage->data);
  else if ((ximage->bits_per_pixel==16) &&
           ((visual->red_prec == 5)   || (visual->red_prec == 6)) &&
           ((visual->green_prec == 5) || (visual->green_prec == 6)) &&
           ((visual->blue_prec == 5)  || (visual->blue_prec == 6)))
    DrawComposited16(isLSB, flipBytes,
                     imageOrigin, imageStride,
                     alphaOrigin, alphaStride, 
                     readWidth, readHeight, ximage, readData, (unsigned char *)ximage->data);
  else
    DrawCompositedGeneral(isLSB, flipBytes,
                     imageOrigin, imageStride,
                     alphaOrigin, alphaStride, 
                     readWidth, readHeight, ximage, readData, (unsigned char *)ximage->data);

  GdkGC *imageGC = ((nsRenderingContextGTK&)aContext).GetGC();
  gdk_draw_rgb_image(drawing->GetDrawable(), imageGC,
                     readX, readY, readWidth, readHeight,
                     GDK_RGB_DITHER_MAX,
                     readData, 3*readWidth);
  gdk_gc_unref(imageGC);

  XDestroyImage(ximage);
  nsMemory::Free(readData);
  if (scaledImage)
    nsMemory::Free(scaledImage);
  if (scaledAlpha)
    nsMemory::Free(scaledAlpha);
  mFlags = 0;
}


void
nsImageGTK::DrawCompositeTile(nsIRenderingContext &aContext,
                              nsIDrawingSurface* aSurface,
                              PRInt32 aSX, PRInt32 aSY,
                              PRInt32 aSWidth, PRInt32 aSHeight,
                              PRInt32 aDX, PRInt32 aDY,
                              PRInt32 aDWidth, PRInt32 aDHeight)
{
  if ((aDWidth==0) || (aDHeight==0))
    return;

  nsDrawingSurfaceGTK* drawing = (nsDrawingSurfaceGTK*) aSurface;
  GdkVisual *visual = gdk_rgb_get_visual();
    
  Display *dpy = GDK_WINDOW_XDISPLAY(drawing->GetDrawable());
  Drawable drawable = GDK_WINDOW_XWINDOW(drawing->GetDrawable());

  
  PRUint32 surfaceWidth, surfaceHeight;
  drawing->GetDimensions(&surfaceWidth, &surfaceHeight);
  
  int readX, readY;
  unsigned readWidth, readHeight;
  PRInt32 destX, destY;

  if ((aDY>=(int)surfaceHeight) || (aDX>=(int)surfaceWidth) ||
      (aDY+aDHeight<=0) || (aDX+aDWidth<=0)) {
    
    
    
    return;
  }

  if (aDX<0) {
    readX = 0;   readWidth = aDWidth+aDX;    destX = aSX-aDX;
  } else {
    readX = aDX;  readWidth = aDWidth;       destX = aSX;
  }
  if (aDY<0) {
    readY = 0;   readHeight = aDHeight+aDY;  destY = aSY-aDY;
  } else {
    readY = aDY;  readHeight = aDHeight;     destY = aSY;
  }

  if (readX+readWidth > surfaceWidth)
    readWidth = surfaceWidth-readX;
  if (readY+readHeight > surfaceHeight)
    readHeight = surfaceHeight-readY;

  if ((readHeight <= 0) || (readWidth <= 0))
    return;

  XImage *ximage = XGetImage(dpy, drawable,
                             readX, readY, readWidth, readHeight, 
                             AllPlanes, ZPixmap);

  NS_ASSERTION((ximage!=NULL), "XGetImage() failed");
  if (!ximage)
    return;

  unsigned char *readData = 
    (unsigned char *)nsMemory::Alloc(3*readWidth*readHeight);
  if (!readData) {
    XDestroyImage(ximage);
    return;
  }

  PRBool isLSB;
  unsigned test = 1;
  isLSB = (((char *)&test)[0]) ? 1 : 0;

  PRBool flipBytes = 
    ( isLSB && ximage->byte_order != LSBFirst) ||
    (!isLSB && ximage->byte_order == LSBFirst);


  PRUint8 *imageOrigin, *alphaOrigin;
  PRUint32 imageStride, alphaStride;
  PRUint32 compX, compY;
  PRUint8 *compTarget, *compSource;

  imageStride = mRowBytes;
  alphaStride = mAlphaRowBytes;

  if (destX==mWidth)
    destX = 0;
  if (destY==mHeight)
    destY = 0;

  for (unsigned y=0; y<readHeight; y+=compY) {
    if (y==0) {
      compY = PR_MIN(mHeight-destY, readHeight-y);
    } else {
      destY = 0;
      compY = PR_MIN(mHeight, readHeight-y);
    }

    compTarget = readData + 3*y*ximage->width;
    compSource = (unsigned char *)ximage->data + y*ximage->bytes_per_line;

    for (unsigned x=0; x<readWidth; x+=compX) {
      if (x==0) {
        compX = PR_MIN(mWidth-destX, readWidth-x);
        imageOrigin = mImageBits + destY*mRowBytes + 3*destX;
        alphaOrigin = mAlphaBits + destY*mAlphaRowBytes + destX;
      } else {
        compX = PR_MIN(mWidth, readWidth-x);
        imageOrigin = mImageBits + destY*mRowBytes;
        alphaOrigin = mAlphaBits + destY*mAlphaRowBytes;
      }

      if ((ximage->bits_per_pixel==32) &&
          (visual->red_prec == 8) &&
          (visual->green_prec == 8) &&
          (visual->blue_prec == 8))
        DrawComposited32(isLSB, flipBytes, 
                         imageOrigin, imageStride,
                         alphaOrigin, alphaStride, 
                         compX, compY, ximage, compTarget, compSource);
      else if ((ximage->bits_per_pixel==24) &&
               (visual->red_prec == 8) && 
               (visual->green_prec == 8) &&
               (visual->blue_prec == 8))
        DrawComposited24(isLSB, flipBytes, 
                         imageOrigin, imageStride,
                         alphaOrigin, alphaStride, 
                         compX, compY, ximage, compTarget, compSource);
      else if ((ximage->bits_per_pixel==16) &&
               ((visual->red_prec == 5)   || (visual->red_prec == 6)) &&
               ((visual->green_prec == 5) || (visual->green_prec == 6)) &&
               ((visual->blue_prec == 5)  || (visual->blue_prec == 6)))
        DrawComposited16(isLSB, flipBytes,
                         imageOrigin, imageStride,
                         alphaOrigin, alphaStride, 
                         compX, compY, ximage, compTarget, compSource);
      else
        DrawCompositedGeneral(isLSB, flipBytes,
                              imageOrigin, imageStride,
                              alphaOrigin, alphaStride, 
                              compX, compY, ximage, compTarget, compSource);

      compTarget += 3*compX;
      compSource += (ximage->bits_per_pixel*compX)/8;
    }
  }

  GdkGC *imageGC = ((nsRenderingContextGTK&)aContext).GetGC();
  gdk_draw_rgb_image(drawing->GetDrawable(), imageGC,
                     readX, readY, readWidth, readHeight,
                     GDK_RGB_DITHER_MAX,
                     readData, 3*readWidth);
  gdk_gc_unref(imageGC);

  XDestroyImage(ximage);
  nsMemory::Free(readData);
  mFlags = 0;
}


void nsImageGTK::CreateOffscreenPixmap(PRInt32 aWidth, PRInt32 aHeight)
{
  
  
  
  if (!mImagePixmap) {
#ifdef TRACE_IMAGE_ALLOCATION
    printf("nsImageGTK::Draw(this=%p) gdk_pixmap_new(nsnull,width=%d,height=%d,depth=%d)\n",
           this,
           aWidth, aHeight,
           mDepth);
#endif

    
    mImagePixmap = gdk_pixmap_new(nsnull, aWidth, aHeight,
                                  gdk_rgb_get_visual()->depth);
#ifdef MOZ_WIDGET_GTK2
    gdk_drawable_set_colormap(GDK_DRAWABLE(mImagePixmap), gdk_rgb_get_colormap());
#endif
  }

    
  if ((!mAlphaPixmap) && (mAlphaDepth==1)) {
    mAlphaPixmap = gdk_pixmap_new(nsnull, aWidth, aHeight, 1);

    
    mAlphaXImage = XCreateImage(GDK_WINDOW_XDISPLAY(mAlphaPixmap),
                                GDK_VISUAL_XVISUAL(gdk_rgb_get_visual()),
                                1, 
                                XYPixmap,
                                0, 
                                (char *)mAlphaBits,  
                                aWidth,
                                aHeight,
                                32,
                                mAlphaRowBytes); 

    mAlphaXImage->bits_per_pixel=1;

    
    mAlphaXImage->bitmap_bit_order = MSBFirst;

    



    mAlphaXImage->byte_order = MSBFirst;

    if (!s1bitGC) {
      GdkColor fg = { 1, 0, 0, 0 };
      s1bitGC = gdk_gc_new(mAlphaPixmap);
      gdk_gc_set_foreground(s1bitGC, &fg);
    }
  }

  if (!sXbitGC)
    sXbitGC = gdk_gc_new(mImagePixmap);
}


void nsImageGTK::SetupGCForAlpha(GdkGC *aGC, PRInt32 aX, PRInt32 aY)
{
  
  if (mAlphaPixmap) {
    
    XGCValues xvalues;
    memset(&xvalues, 0, sizeof(XGCValues));
    unsigned long xvalues_mask = 0;
    xvalues.clip_x_origin = aX;
    xvalues.clip_y_origin = aY;
    xvalues_mask = GCClipXOrigin | GCClipYOrigin;

    xvalues.clip_mask = GDK_WINDOW_XWINDOW(mAlphaPixmap);
    xvalues_mask |= GCClipMask;

    XChangeGC(GDK_DISPLAY(), GDK_GC_XGC(aGC), xvalues_mask, &xvalues);
  }
}


NS_IMETHODIMP
nsImageGTK::Draw(nsIRenderingContext &aContext,
                 nsIDrawingSurface* aSurface,
                 PRInt32 aX, PRInt32 aY,
                 PRInt32 aWidth, PRInt32 aHeight)
{
#ifdef TRACE_IMAGE_ALLOCATION
  printf("nsImageGTK::Draw(this=%p,x=%d,y=%d,width=%d,height=%d)\n",
         this,
         aX, aY,
         aWidth, aHeight);
#endif

  return Draw(aContext, aSurface,
              0, 0, mWidth, mHeight,
              aX, aY, mWidth, mHeight);
}


void nsImageGTK::TilePixmap(GdkPixmap *src, GdkPixmap *dest, 
                            PRInt32 aSXOffset, PRInt32 aSYOffset,
                            const nsRect &destRect, 
                            const nsRect &clipRect, PRBool aHaveClip)
{
  GdkGC *gc;
  GdkGCValues values;
  GdkGCValuesMask valuesMask;
  memset(&values, 0, sizeof(GdkGCValues));
  values.fill = GDK_TILED;
  values.tile = src;
  values.ts_x_origin = destRect.x - aSXOffset;
  values.ts_y_origin = destRect.y - aSYOffset;
  valuesMask = GdkGCValuesMask(GDK_GC_FILL | GDK_GC_TILE | GDK_GC_TS_X_ORIGIN | GDK_GC_TS_Y_ORIGIN);
  gc = gdk_gc_new_with_values(src, &values, valuesMask);

  if (aHaveClip) {
    GdkRectangle gdkrect = {clipRect.x, clipRect.y, clipRect.width, clipRect.height};
    gdk_gc_set_clip_rectangle(gc, &gdkrect);
  }

  
  #ifdef DEBUG_TILING
  printf("nsImageGTK::TilePixmap(..., %d, %d, %d, %d)\n",
         destRect.x, destRect.y, 
         destRect.width, destRect.height);
  #endif

  gdk_draw_rectangle(dest, gc, PR_TRUE,
                     destRect.x, destRect.y,
                     destRect.width, destRect.height);

  gdk_gc_unref(gc);
}

void nsImageGTK::SlowTile(nsDrawingSurfaceGTK *aSurface,
                          const nsRect &aTileRect,
                          PRInt32 aSXOffset, PRInt32 aSYOffset,
                          const nsRect& aClipRect, PRBool aHaveClip)
{
  GdkPixmap *tileImg;
  GdkPixmap *tileMask;

  nsRect tmpRect(0,0,aTileRect.width, aTileRect.height);

  tileImg = gdk_pixmap_new(nsnull, aTileRect.width, 
                           aTileRect.height, aSurface->GetDepth());
#ifdef MOZ_WIDGET_GTK2
  gdk_drawable_set_colormap(GDK_DRAWABLE(tileImg), gdk_rgb_get_colormap());
#endif

  TilePixmap(mImagePixmap, tileImg, aSXOffset, aSYOffset, tmpRect,
             tmpRect, PR_FALSE);

  
  tileMask = gdk_pixmap_new(nsnull, aTileRect.width, aTileRect.height,
                            mAlphaDepth);
  TilePixmap(mAlphaPixmap, tileMask, aSXOffset, aSYOffset, tmpRect,
             tmpRect, PR_FALSE);

  GdkGC *fgc = gdk_gc_new(aSurface->GetDrawable());
  gdk_gc_set_clip_mask(fgc, (GdkBitmap*)tileMask);
  gdk_gc_set_clip_origin(fgc, aTileRect.x, aTileRect.y);

  nsRect drawRect = aTileRect;
  if (aHaveClip) {
    drawRect.IntersectRect(drawRect, aClipRect);
  }

  
  gdk_window_copy_area(aSurface->GetDrawable(), fgc, drawRect.x,
                       drawRect.y, tileImg,
                       drawRect.x - aTileRect.x, drawRect.y - aTileRect.y,
                       drawRect.width, drawRect.height);
  gdk_gc_unref(fgc);

  gdk_pixmap_unref(tileImg);
  gdk_pixmap_unref(tileMask);
}


NS_IMETHODIMP nsImageGTK::DrawTile(nsIRenderingContext &aContext,
                                   nsIDrawingSurface* aSurface,
                                   PRInt32 aSXOffset, PRInt32 aSYOffset,
                                   PRInt32 aPadX, PRInt32 aPadY,
                                   const nsRect &aTileRect)
{
#ifdef DEBUG_TILING
  printf("nsImageGTK::DrawTile: mWidth=%d, mHeight=%d\n", mWidth, mHeight);
  printf("nsImageGTK::DrawTile((src: %d, %d), (tile: %d,%d, %d, %d) %p\n", aSXOffset, aSYOffset,
         aTileRect.x, aTileRect.y,
         aTileRect.width, aTileRect.height, this);
#endif
  if (mPendingUpdate)
    UpdateCachedImage();

  if ((mAlphaDepth==1) && mIsSpacer)
    return NS_OK;

  if (mDecodedX2 < mDecodedX1 || mDecodedY2 < mDecodedY1)
    return NS_OK;

  nsDrawingSurfaceGTK *drawing = (nsDrawingSurfaceGTK*)aSurface;
  PRBool partial = PR_FALSE;

  PRInt32
    validX = 0,
    validY = 0,
    validWidth  = mWidth,
    validHeight = mHeight;
  
  
  
  if (mDecodedY2 < mHeight) {
    validHeight = mDecodedY2 - mDecodedY1;
    partial = PR_TRUE;
  }
  if (mDecodedX2 < mWidth) {
    validWidth = mDecodedX2 - mDecodedX1;
    partial = PR_TRUE;
  }
  if (mDecodedY1 > 0) {   
    validHeight -= mDecodedY1;
    validY = mDecodedY1;
    partial = PR_TRUE;
  }
  if (mDecodedX1 > 0) {
    validWidth -= mDecodedX1;
    validX = mDecodedX1; 
    partial = PR_TRUE;
  }

  if (aTileRect.width == 0 || aTileRect.height == 0 ||
      validWidth == 0 || validHeight == 0) {
    return NS_OK;
  }

  if (partial || (mAlphaDepth == 8) || (aPadX || aPadY)) {
    PRInt32 aY0 = aTileRect.y - aSYOffset,
            aX0 = aTileRect.x - aSXOffset,
            aY1 = aTileRect.y + aTileRect.height,
            aX1 = aTileRect.x + aTileRect.width;

    
    aContext.PushState();
    ((nsRenderingContextGTK&)aContext).SetClipRectInPixels(
      aTileRect, nsClipCombine_kIntersect);
    ((nsRenderingContextGTK&)aContext).UpdateGC();

    if (mAlphaDepth==8) {
      DrawCompositeTile(aContext, aSurface,
                        aSXOffset, aSYOffset, mWidth, mHeight,
                        aTileRect.x, aTileRect.y,
                        aTileRect.width, aTileRect.height);
    } else {
#ifdef DEBUG_TILING
      printf("Warning: using slow tiling\n");
#endif
      for (PRInt32 y = aY0; y < aY1; y += mHeight + aPadY)
        for (PRInt32 x = aX0; x < aX1; x += mWidth + aPadX)
          Draw(aContext,aSurface, x,y,
               PR_MIN(validWidth, aX1-x),
               PR_MIN(validHeight, aY1-y));
    }

    aContext.PopState();

    return NS_OK;
  }

  nsRect clipRect;
  PRBool isNonEmpty;
  PRBool haveClip = NS_SUCCEEDED(aContext.GetClipRect(clipRect, isNonEmpty));
  if (haveClip && !isNonEmpty) {
    return NS_OK;
  }
    
  if (mAlphaDepth == 1) {
    if (sNeedSlowTile) {
      SlowTile(drawing, aTileRect, aSXOffset, aSYOffset, clipRect, haveClip);
      return NS_OK;
    }

    GdkGC *tileGC;
    GdkGCValues values;
    GdkGCValuesMask valuesMask;

    memset(&values, 0, sizeof(GdkGCValues));
    values.fill = GDK_STIPPLED;
    values.function = GDK_AND;
    values.stipple = mAlphaPixmap;
    values.ts_x_origin = aTileRect.x - aSXOffset;
    values.ts_y_origin = aTileRect.y - aSYOffset;
    valuesMask = GdkGCValuesMask(GDK_GC_FOREGROUND | GDK_GC_FUNCTION | 
                                 GDK_GC_FILL | GDK_GC_STIPPLE | 
                                 GDK_GC_TS_X_ORIGIN | GDK_GC_TS_Y_ORIGIN);
    tileGC = gdk_gc_new_with_values(drawing->GetDrawable(), &values, valuesMask);
    
    if (haveClip) {
      GdkRectangle gdkrect = {clipRect.x, clipRect.y,
                              clipRect.width, clipRect.height};
      gdk_gc_set_clip_rectangle(tileGC, &gdkrect);
    }

    gdk_draw_rectangle(drawing->GetDrawable(), tileGC, PR_TRUE,
                       aTileRect.x, aTileRect.y,
                       aTileRect.width, aTileRect.height);

    gdk_gc_set_fill(tileGC, GDK_TILED);
    gdk_gc_set_function(tileGC, GDK_OR);
    gdk_gc_set_tile(tileGC, mImagePixmap);

    gdk_draw_rectangle(drawing->GetDrawable(), tileGC, PR_TRUE,
                       aTileRect.x, aTileRect.y,
                       aTileRect.width, aTileRect.height);

    gdk_gc_unref(tileGC);
  } else {
    
    TilePixmap(mImagePixmap, drawing->GetDrawable(), aSXOffset, aSYOffset,
               aTileRect, clipRect, haveClip);
  }

  mFlags = 0;
  return NS_OK;
}





nsresult nsImageGTK::Optimize(nsIDeviceContext* aContext)
{
  if (!mOptimized)
    UpdateCachedImage();

  if (mAlphaBits && mTrueAlphaBits) {
    
    
    for (PRInt32 y = 0; y < mHeight; y++)
      for (PRInt32 x = 0; x < mWidth; x++)
        if (!mTrueAlphaBits[y * mTrueAlphaRowBytes + x]) {
          mImageBits[y * mRowBytes + 3 * x]     = 0;
          mImageBits[y * mRowBytes + 3 * x + 1] = 0;
          mImageBits[y * mRowBytes + 3 * x + 2] = 0;
        }
    nsRect rect(0, 0, mWidth, mHeight);
    ImageUpdated(nsnull, 0, &rect);
    UpdateCachedImage();
  }

  if ((gdk_rgb_get_visual()->depth > 8) && (mAlphaDepth != 8)) {
    if(nsnull != mImageBits) {
      free(mImageBits);
      mImageBits = nsnull;
    }

    if (nsnull != mAlphaBits) {
      free(mAlphaBits);
      mAlphaBits = nsnull;
    }
  }
    
  if (mTrueAlphaBits) {
    free(mTrueAlphaBits);
    mTrueAlphaBits = nsnull;
  }

  if ((mAlphaDepth==0) && mAlphaPixmap) {
    gdk_pixmap_unref(mAlphaPixmap);
    mAlphaPixmap = nsnull;
  }

  mOptimized = PR_TRUE;

  return NS_OK;
}



NS_IMETHODIMP
nsImageGTK::LockImagePixels(PRBool aMaskPixels)
{
  if (!mOptimized)
    return NS_OK;

  if (aMaskPixels) {
    if (mAlphaDepth != 1 || !mAlphaPixmap)
      return NS_OK;

    XImage *xmask = XGetImage(GDK_WINDOW_XDISPLAY(mAlphaPixmap),
                              GDK_WINDOW_XWINDOW(mAlphaPixmap),
                              0, 0, mWidth, mHeight,
                              AllPlanes, XYPixmap);

    mAlphaBits = (PRUint8*)calloc(mAlphaRowBytes * mHeight, 1);
    if (!mAlphaBits)
      return NS_ERROR_OUT_OF_MEMORY;

    for (PRInt32 y = 0; y < mHeight; ++y) {
      PRUint8 *alphaTarget = mAlphaBits + y*mAlphaRowBytes;
      PRUint32 alphaBitPos = 7;

      for (PRInt32 x = 0; x < mWidth; ++x) {
        *alphaTarget |= (XGetPixel(xmask, x, y) << alphaBitPos);
        if (alphaBitPos-- == 0) {
          ++alphaTarget;
          alphaBitPos = 7;
        }
      }
    }

    XDestroyImage(xmask);
    return NS_OK;
  }

  if (!mImagePixmap)
    return NS_OK;

  XImage *ximage, *xmask=0;
  unsigned pix;

  ximage = XGetImage(GDK_WINDOW_XDISPLAY(mImagePixmap),
                     GDK_WINDOW_XWINDOW(mImagePixmap),
                     0, 0, mWidth, mHeight,
                     AllPlanes, ZPixmap);

  if ((mAlphaDepth==1) && mAlphaPixmap)
    xmask = XGetImage(GDK_WINDOW_XDISPLAY(mAlphaPixmap),
                      GDK_WINDOW_XWINDOW(mAlphaPixmap),
                      0, 0, mWidth, mHeight,
                      AllPlanes, XYPixmap);

  mImageBits = (PRUint8*)malloc(mSizeImage);
  if (!mImageBits)
    return NS_ERROR_OUT_OF_MEMORY;

  GdkVisual *visual = gdk_rgb_get_visual();
  GdkColormap *colormap = gdk_rgb_get_cmap();

  unsigned redScale, greenScale, blueScale, redFill, greenFill, blueFill;
  redScale   = 8 - visual->red_prec;
  greenScale = 8 - visual->green_prec;
  blueScale  = 8 - visual->blue_prec;
  redFill    = 0xff >> visual->red_prec;
  greenFill  = 0xff >> visual->green_prec;
  blueFill   = 0xff >> visual->blue_prec;

  
  for (PRInt32 y=0; y<mHeight; y++) {
    PRUint8 *target = mImageBits + y*mRowBytes;
    for (PRInt32 x=0; x<mWidth; x++) {
      if (xmask && !XGetPixel(xmask, x, y)) {
        *target++ = 0xFF;
        *target++ = 0xFF;
        *target++ = 0xFF;
      } else {
        pix = XGetPixel(ximage, x, y);
        switch (visual->type) {
        case GDK_VISUAL_STATIC_GRAY:
        case GDK_VISUAL_GRAYSCALE:
        case GDK_VISUAL_STATIC_COLOR:
        case GDK_VISUAL_PSEUDO_COLOR:
          *target++ = colormap->colors[pix].red   >>8;
          *target++ = colormap->colors[pix].green >>8;
          *target++ = colormap->colors[pix].blue  >>8;
          break;

        case GDK_VISUAL_DIRECT_COLOR:
          *target++ = 
            colormap->colors[(pix&visual->red_mask)>>visual->red_shift].red       >> 8;
          *target++ = 
            colormap->colors[(pix&visual->green_mask)>>visual->green_shift].green >> 8;
          *target++ =
            colormap->colors[(pix&visual->blue_mask)>>visual->blue_shift].blue    >> 8;
          break;

        case GDK_VISUAL_TRUE_COLOR:
          *target++ = 
            redFill|((pix&visual->red_mask)>>visual->red_shift)<<redScale;
          *target++ = 
            greenFill|((pix&visual->green_mask)>>visual->green_shift)<<greenScale;
          *target++ = 
            blueFill|((pix&visual->blue_mask)>>visual->blue_shift)<<blueScale;
          break;
        }
      }
    }
  }

  XDestroyImage(ximage);
  if (xmask)
    XDestroyImage(xmask);

  return NS_OK;
}



NS_IMETHODIMP
nsImageGTK::UnlockImagePixels(PRBool aMaskPixels)
{
  if (mOptimized)
    Optimize(nsnull);

  return NS_OK;
} 

NS_IMETHODIMP nsImageGTK::DrawToImage(nsIImage* aDstImage,
                                      nscoord aDX, nscoord aDY,
                                      nscoord aDWidth, nscoord aDHeight)
{
  nsImageGTK *dest = NS_STATIC_CAST(nsImageGTK *, aDstImage);

  if (!dest)
    return NS_ERROR_FAILURE;
    
  if (aDX >= dest->mWidth || aDY >= dest->mHeight)
    return NS_OK;

  PRUint8 *rgbPtr=0, *alphaPtr=0;
  PRUint32 rgbStride, alphaStride;

  rgbPtr = mImageBits;
  rgbStride = mRowBytes;
  alphaPtr = mAlphaBits;
  alphaStride = mAlphaRowBytes;

  PRInt32 y;
  PRInt32 ValidWidth = ( aDWidth < ( dest->mWidth - aDX ) ) ? aDWidth : ( dest->mWidth - aDX ); 
  PRInt32 ValidHeight = ( aDHeight < ( dest->mHeight - aDY ) ) ? aDHeight : ( dest->mHeight - aDY );

  
  switch (mAlphaDepth) {
  case 1:
    {
      PRUint8 *dst = dest->mImageBits + aDY*dest->mRowBytes + 3*aDX;
      PRUint8 *dstAlpha = dest->mAlphaBits + aDY*dest->mAlphaRowBytes;
      PRUint8 *src = rgbPtr;
      PRUint8 *alpha = alphaPtr;
      PRUint8 offset = aDX & 0x7; 
      int iterations = (ValidWidth+7)/8; 

      for (y=0; y<ValidHeight; y++) {
        for (int x=0; x<ValidWidth; x += 8, dst += 3*8, src += 3*8) {
          PRUint8 alphaPixels = *alpha++;
          if (alphaPixels == 0) {
            
            continue;
          }

          
          
          if (x+7 >= ValidWidth) {
            alphaPixels &= 0xff << (8 - (ValidWidth-x)); 
            if (alphaPixels == 0)
              continue;  
          }
          if (offset == 0) {
            dstAlpha[(aDX+x)>>3] |= alphaPixels; 
          }
          else {
            dstAlpha[(aDX+x)>>3]       |= alphaPixels >> offset;
            
            PRUint8 alphaTemp = alphaPixels << (8U - offset);
            if (alphaTemp & 0xff)
              dstAlpha[((aDX+x)>>3) + 1] |= alphaTemp;
          }
          
          if (alphaPixels == 0xff) {
            
            
            memcpy(dst,src,8*3);
            continue;
          }
          else {
            
            
            PRUint8 *d = dst, *s = src;
            for (PRUint8 aMask = 1<<7, j = 0; aMask && j < ValidWidth-x; aMask >>= 1, j++) {
              
              if (alphaPixels & aMask) {
                
                d[0] = s[0];
                d[1] = s[1];
                d[2] = s[2];
                
              }
              d += 3;
              s += 3;
            }
          }
        }
        
        
        dst = (dst - 3*8*iterations) + dest->mRowBytes;
        src = (src - 3*8*iterations) + rgbStride;
        alpha = (alpha - iterations) + alphaStride;
        dstAlpha += dest->mAlphaRowBytes;
      }
    }
    break;
  case 0:
  default:
    for (y=0; y<ValidHeight; y++)
      memcpy(dest->mImageBits + (y+aDY)*dest->mRowBytes + 3*aDX, 
             rgbPtr + y*rgbStride,
             3*ValidWidth);
  }

  nsRect rect(aDX, aDY, ValidWidth, ValidHeight);
  dest->ImageUpdated(nsnull, 0, &rect);

  return NS_OK;
}

#ifdef MOZ_WIDGET_GTK2
static void pixbuf_free(guchar* data, gpointer) {
  nsMemory::Free(data);
}

NS_IMETHODIMP_(GdkPixbuf*)
nsImageGTK::GetGdkPixbuf() {
  
  NS_ASSERTION(mNumBytesPixel == 3, "Unexpected color depth");

  nsresult rv = LockImagePixels(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, nsnull);

  
  
  guchar* pixels = NS_STATIC_CAST(guchar*,
      nsMemory::Clone(mImageBits, mRowBytes * mHeight));
  UnlockImagePixels(PR_FALSE);
  if (!pixels)
    return nsnull;

  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data(pixels,
      GDK_COLORSPACE_RGB,
      PR_FALSE,
      8,
      mWidth,
      mHeight,
      mRowBytes,
      pixbuf_free,
      nsnull);
  if (!pixbuf)
    return nsnull;

  if (!GetHasAlphaMask()) {
    
    return pixbuf;
  }

  GdkPixbuf *alphaPixbuf = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
  g_object_unref(pixbuf);
  if (!alphaPixbuf)
    return nsnull;

  LockImagePixels(PR_TRUE);

  PRInt32 alphaBytesPerRow = GetAlphaLineStride();
  PRUint8 *alphaBits = GetAlphaBits();

  
  
  PRUint8 *maskRow = alphaBits;
  PRUint8 *pixbufRow = gdk_pixbuf_get_pixels(alphaPixbuf);

  gint pixbufRowStride = gdk_pixbuf_get_rowstride(alphaPixbuf);
  gint pixbufChannels = gdk_pixbuf_get_n_channels(alphaPixbuf);

  for (PRInt32 y = 0; y < mHeight; ++y) {
    PRUint8 *pixbufPixel = pixbufRow;
    PRUint8 *maskPixel = maskRow;

    
    PRUint32 bitPos = 7;

    for (PRInt32 x = 0; x < mWidth; ++x) {
      if (mAlphaDepth == 1) {
        pixbufPixel[pixbufChannels - 1] = ((*maskPixel >> bitPos) & 1) ? 255 : 0;
        if (bitPos-- == 0) { 
          ++maskPixel;
          bitPos = 7;
        }
      } else {
        pixbufPixel[pixbufChannels - 1] = *maskPixel++;
      }

      pixbufPixel += pixbufChannels;
    }

    pixbufRow += pixbufRowStride;
    maskRow += alphaBytesPerRow;
  }

  UnlockImagePixels(PR_TRUE);
  return alphaPixbuf;
}

#endif
