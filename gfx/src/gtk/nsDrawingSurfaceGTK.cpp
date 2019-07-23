





































#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>
#include "nsDrawingSurfaceGTK.h"

NS_IMPL_ISUPPORTS1(nsDrawingSurfaceGTK, nsIDrawingSurface)



#ifdef CHEAP_PERFORMANCE_MEASUREMENT
static PRTime mLockTime, mUnlockTime;
#endif

#ifdef MOZ_ENABLE_XFT
#include <X11/Xft/Xft.h>
#endif

nsDrawingSurfaceGTK :: nsDrawingSurfaceGTK()
{
  GdkVisual *v;

  mPixmap = nsnull;
  mGC = nsnull;
  mDepth = 0;
  mWidth = 0;
  mHeight = 0;
  mFlags = 0;

  mImage = nsnull;
  mLockWidth = 0;
  mLockHeight = 0;
  mLockFlags = 0;
  mLockX = 0;
  mLockY = 0;
  mLocked = PR_FALSE;

  v = ::gdk_rgb_get_visual();

  mPixFormat.mRedMask = v->red_mask;
  mPixFormat.mGreenMask = v->green_mask;
  mPixFormat.mBlueMask = v->blue_mask;
  
  mPixFormat.mAlphaMask = 0;

  mPixFormat.mRedCount = ConvertMaskToCount(v->red_mask);
  mPixFormat.mGreenCount = ConvertMaskToCount(v->green_mask);
  mPixFormat.mBlueCount = ConvertMaskToCount(v->blue_mask);;


  mPixFormat.mRedShift = v->red_shift;
  mPixFormat.mGreenShift = v->green_shift;
  mPixFormat.mBlueShift = v->blue_shift;
  
  mPixFormat.mAlphaShift = 0;

  mDepth = v->depth;

#ifdef MOZ_ENABLE_XFT
  mXftDraw = nsnull;
#endif
}

nsDrawingSurfaceGTK :: ~nsDrawingSurfaceGTK()
{
  if (mPixmap)
    ::gdk_pixmap_unref(mPixmap);

  if (mImage)
    ::gdk_image_destroy(mImage);

  if (mGC)
    gdk_gc_unref(mGC);

#ifdef MOZ_ENABLE_XFT
  if (mXftDraw)
    XftDrawDestroy(mXftDraw);
#endif
}


















NS_IMETHODIMP nsDrawingSurfaceGTK :: Lock(PRInt32 aX, PRInt32 aY,
                                          PRUint32 aWidth, PRUint32 aHeight,
                                          void **aBits, PRInt32 *aStride,
                                          PRInt32 *aWidthBytes, PRUint32 aFlags)
{
#ifdef CHEAP_PERFORMANCE_MEASUREMENT
  mLockTime = PR_Now();
  
  
#endif

#if 0
  g_print("nsDrawingSurfaceGTK::Lock() called\n" \
          "  aX = %i, aY = %i,\n" \
          "  aWidth = %i, aHeight = %i,\n" \
          "  aBits, aStride, aWidthBytes,\n" \
          "  aFlags = %i\n", aX, aY, aWidth, aHeight, aFlags);
#endif

  if (mLocked)
  {
    NS_ASSERTION(0, "nested lock attempt");
    return NS_ERROR_FAILURE;
  }
  mLocked = PR_TRUE;

  mLockX = aX;
  mLockY = aY;
  mLockWidth = aWidth;
  mLockHeight = aHeight;
  mLockFlags = aFlags;

  
  mImage = ::gdk_image_get(mPixmap, mLockX, mLockY, mLockWidth, mLockHeight);

  if (!mImage) {
    mLocked = PR_FALSE;
    return NS_ERROR_FAILURE;
  }

  *aBits = GDK_IMAGE_XIMAGE(mImage)->data;

  
  
  
  *aWidthBytes = aWidth * ((GDK_IMAGE_XIMAGE(mImage)->bits_per_pixel + 7) / 8);
  *aStride = GDK_IMAGE_XIMAGE(mImage)->bytes_per_line;

#ifdef CHEAP_PERFORMANCE_MEASUREMENT
  
  
  
  printf("Time taken to lock:   %d\n", PR_Now() - mLockTime);
#endif

  return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceGTK :: Unlock(void)
{

#ifdef CHEAP_PERFORMANCE_MEASUREMENT
  mUnlockTime = PR_Now();
#endif

  
  if (!mLocked)
  {
    NS_ASSERTION(0, "attempting to unlock an DS that isn't locked");
    return NS_ERROR_FAILURE;
  }

  
  if (!(mLockFlags & NS_LOCK_SURFACE_READ_ONLY))
  {
#if 0
    g_print("%p gdk_draw_image(pixmap=%p,lockx=%d,locky=%d,lockw=%d,lockh=%d)\n",
            this,
            mPixmap,
            mLockX, mLockY,
            mLockWidth, mLockHeight);
#endif

    gdk_draw_image(mPixmap,
                   mGC,
                   mImage,
                   0, 0,
                   mLockX, mLockY,
                   mLockWidth, mLockHeight);
  }

  if (mImage)
    ::gdk_image_destroy(mImage);
  mImage = nsnull;

  mLocked = PR_FALSE;


#ifdef CHEAP_PERFORMANCE_MEASUREMENT
  printf("Time taken to unlock: %d\n", PR_Now() - mUnlockTime);
#endif

  return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceGTK :: GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight)
{
  *aWidth = mWidth;
  *aHeight = mHeight;

  return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceGTK :: IsOffscreen(PRBool *aOffScreen)
{
  *aOffScreen = mIsOffscreen;
  return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceGTK :: IsPixelAddressable(PRBool *aAddressable)
{

  *aAddressable = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsDrawingSurfaceGTK :: GetPixelFormat(nsPixelFormat *aFormat)
{
  *aFormat = mPixFormat;

  return NS_OK;
}

nsresult nsDrawingSurfaceGTK :: Init(GdkDrawable *aDrawable, GdkGC *aGC)
{
  if (mGC)
    gdk_gc_unref(mGC);

  mGC = gdk_gc_ref(aGC);
  mPixmap = aDrawable;

#ifdef MOZ_WIDGET_GTK
  mWidth  = ((GdkWindowPrivate*)aDrawable)->width;
  mHeight = ((GdkWindowPrivate*)aDrawable)->height;
#endif 

#ifdef MOZ_WIDGET_GTK2
  gint width = 0;
  gint height = 0;
  gdk_drawable_get_size(aDrawable, &width, &height);
  mWidth = width;
  mHeight = height;
#endif 

  
  
  
  mIsOffscreen = PR_FALSE;

  if (mImage)
    gdk_image_destroy(mImage);
  mImage = nsnull;

  g_return_val_if_fail(mPixmap != nsnull, NS_ERROR_FAILURE);
  
  return NS_OK;
}

nsresult nsDrawingSurfaceGTK :: Init(GdkGC *aGC, PRUint32 aWidth,
                                     PRUint32 aHeight, PRUint32 aFlags)
{
  
  
  if (mGC)
    gdk_gc_unref(mGC);

  mGC = gdk_gc_ref(aGC);
  mWidth = aWidth;
  mHeight = aHeight;
  mFlags = aFlags;

  
  mIsOffscreen = PR_TRUE;

  mPixmap = ::gdk_pixmap_new(nsnull, mWidth, mHeight, mDepth);
#ifdef MOZ_WIDGET_GTK2
  gdk_drawable_set_colormap(GDK_DRAWABLE(mPixmap), gdk_rgb_get_colormap());
#endif

  if (mImage)
    gdk_image_destroy(mImage);
  mImage = nsnull;

  return mPixmap ? NS_OK : NS_ERROR_FAILURE;
}

#ifdef MOZ_ENABLE_XFT
XftDraw *nsDrawingSurfaceGTK :: GetXftDraw(void)
{
  if (!mXftDraw) {
    mXftDraw = XftDrawCreate(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(mPixmap),
                             GDK_VISUAL_XVISUAL(::gdk_rgb_get_visual()),
                             GDK_COLORMAP_XCOLORMAP(::gdk_rgb_get_cmap()));
  }

  return mXftDraw;
}

void nsDrawingSurfaceGTK :: GetLastXftClip(nsIRegion **aLastRegion)
{
  *aLastRegion = mLastXftClip.get();
  NS_IF_ADDREF(*aLastRegion);
}

void nsDrawingSurfaceGTK :: SetLastXftClip(nsIRegion  *aLastRegion)
{
  mLastXftClip = aLastRegion;
}

#endif 


PRUint8 
nsDrawingSurfaceGTK::ConvertMaskToCount(unsigned long val)
{
  PRUint8 retval = 0;
  PRUint8 cur_bit = 0;
  
  
  while (cur_bit < (sizeof(unsigned long) * 8)) {
    if ((val >> cur_bit) & 0x1) {
      retval++;
    }
    cur_bit++;
  }
  return retval;
}
