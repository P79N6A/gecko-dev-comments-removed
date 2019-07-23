




































#include "nsGfxDefs.h"
#include "nsHashtable.h"
#include "nsIWidget.h"
#include "nsDrawingSurfaceOS2.h"
#include "nsFontMetricsOS2.h"
#include "nsPaletteOS2.h"

#define LCID_START 2




NS_IMPL_ISUPPORTS1(nsDrawingSurfaceOS2, nsIDrawingSurface)




nsDrawingSurfaceOS2::nsDrawingSurfaceOS2()
                    : mNextID(LCID_START), mTopID(1), mPS(0), mOwnPS(PR_FALSE),
                      mWidth (0), mHeight (0)
{
   mHTFonts = new nsHashtable;
}

nsDrawingSurfaceOS2::~nsDrawingSurfaceOS2()
{
   DisposeFonts();
}

void nsDrawingSurfaceOS2::DisposeFonts()
{
   if( mHTFonts)
   {
      
      GFX (::GpiSetCharSet(mPS, LCID_DEFAULT), FALSE);

      for (int i = LCID_START; i <= mTopID; i++) {
         GFX (::GpiDeleteSetId(mPS, i), FALSE);
      }
      delete mHTFonts;
      mHTFonts = 0;
   }
}


typedef nsVoidKey FontHandleKey;

void nsDrawingSurfaceOS2::SelectFont(nsFontOS2* aFont)
{
   FontHandleKey key((void*)aFont->mHashMe);

   long lcid = (long) mHTFonts->Get(&key);
   if (lcid == 0) {
      if (mNextID == 255) {
         
         FlushFontCache();
      }

      lcid = mNextID;
      mNextID++;
      CHK_SUCCESS (::GpiCreateLogFont(mPS, 0, lcid, &aFont->mFattrs),
                   FONT_MATCH);
      mHTFonts->Put(&key, (void *) lcid);
      if (mTopID < 254) {
         mTopID++;
      }
   }

   aFont->SelectIntoPS(mPS, lcid);
}

void nsDrawingSurfaceOS2::FlushFontCache()
{
   mHTFonts->Reset();
   mNextID = LCID_START;
   
}




void nsDrawingSurfaceOS2::NS2PM_ININ( const nsRect &in, RECTL &rcl)
{
   const static nscoord kBottomLeftLimit = -8192;
   const static nscoord kTopRightLimit   =  16384;

   PRInt32 ulHeight = GetHeight ();

   rcl.xLeft    = PR_MAX(kBottomLeftLimit, in.x);
   rcl.xRight   = PR_MIN(in.x+in.width-1, kTopRightLimit);
   rcl.yTop     = PR_MIN(ulHeight-in.y-1, kTopRightLimit);
   rcl.yBottom  = PR_MAX(rcl.yTop-in.height+1, kBottomLeftLimit);
   return;
}

void nsDrawingSurfaceOS2::PM2NS_ININ( const RECTL &in, nsRect &out)
{
   PRInt32 ulHeight = GetHeight ();

   out.x = in.xLeft;
   out.width = in.xRight - in.xLeft + 1;
   out.y = ulHeight - in.yTop - 1;
   out.height = in.yTop - in.yBottom + 1;
}


void nsDrawingSurfaceOS2::NS2PM_INEX( const nsRect &in, RECTL &rcl)
{
   NS2PM_ININ( in, rcl);
   rcl.xRight++;
   rcl.yTop++;
}

void nsDrawingSurfaceOS2::NS2PM( PPOINTL aPointl, ULONG cPointls)
{
   PRInt32 ulHeight = GetHeight ();

   for( ULONG i = 0; i < cPointls; i++)
      aPointl[ i].y = ulHeight - aPointl[ i].y - 1;
}

nsresult nsDrawingSurfaceOS2::GetDimensions( PRUint32 *aWidth, PRUint32 *aHeight)
{
   if( !aWidth || !aHeight)
      return NS_ERROR_NULL_POINTER;

   *aWidth = mWidth;
   *aHeight = mHeight;

   return NS_OK;
}




nsOffscreenSurface::nsOffscreenSurface() : mDC(0), mBitmap(0),
                                           mInfoHeader(0), mBits(0),
                                           mYPels(0), mScans(0)
{
}

NS_IMETHODIMP nsOffscreenSurface :: Init(HPS aPS)
{
  mPS = aPS;

  return NS_OK;
}



nsresult nsOffscreenSurface::Init( HPS     aCompatiblePS,
                                   PRInt32 aWidth, PRInt32 aHeight, PRUint32 aFlags)
{
   nsresult rc = NS_ERROR_FAILURE;

   
   HDC hdcCompat = GFX (::GpiQueryDevice (aCompatiblePS), HDC_ERROR);
   DEVOPENSTRUC dop = { 0, 0, 0, 0, 0 };
   mDC = GFX (::DevOpenDC( 0, OD_MEMORY, "*", 5, (PDEVOPENDATA) &dop, hdcCompat), DEV_ERROR);

   if( DEV_ERROR != mDC)
   {
      
      SIZEL sizel = { 0, 0 };
      mPS = GFX (::GpiCreatePS (0, mDC, &sizel,
                 PU_PELS | GPIT_MICRO | GPIA_ASSOC), GPI_ERROR);

      if( GPI_ERROR != mPS)
      {
         mOwnPS = PR_TRUE;

         nsPaletteOS2::SelectGlobalPalette(mPS);

         
         BITMAPINFOHEADER2 hdr = { 0 };
      
         hdr.cbFix = sizeof( BITMAPINFOHEADER2);
         hdr.cx = aWidth;
         hdr.cy = aHeight;
         hdr.cPlanes = 1;

         
         LONG lBitCount = 0;
         GFX (::DevQueryCaps( hdcCompat, CAPS_COLOR_BITCOUNT, 1, &lBitCount), FALSE);
         hdr.cBitCount = (USHORT) lBitCount;

         mBitmap = GFX (::GpiCreateBitmap (mPS, &hdr, 0, 0, 0), GPI_ERROR);

         if( GPI_ERROR != mBitmap)
         {
            
            mHeight = aHeight;
            mWidth = aWidth;
            GFX (::GpiSetBitmap (mPS, mBitmap), HBM_ERROR);
            rc = NS_OK;
         }
      }
   }

   return rc;
}

nsOffscreenSurface::~nsOffscreenSurface()
{
   DisposeFonts();
   
   if (mBitmap) {
      GFX (::GpiSetBitmap (mPS, 0), HBM_ERROR);
      GFX (::GpiDeleteBitmap (mBitmap), FALSE);
   }
   if (mOwnPS) {
      GFX (::GpiDestroyPS (mPS), FALSE);
   }
   if (mDC) {
      ::DevCloseDC(mDC);
   }

   if( mInfoHeader)
      free( mInfoHeader);
   delete [] mBits;
}










nsresult nsOffscreenSurface::Lock( PRInt32 aX, PRInt32 aY,
                                   PRUint32 aWidth, PRUint32 aHeight,
                                   void **aBits, PRInt32 *aStride,
                                   PRInt32 *aWidthBytes,
                                   PRUint32 aFlags)
{
   
   PRInt32 lStride = 0;
   ULONG   rc = 0;

   
   
   
   
   
   
   
   
   
   if( !mBits)
   {
      BITMAPINFOHEADER bih = { sizeof( BITMAPINFOHEADER), 0, 0, 0, 0 };
   
      rc = GFX (::GpiQueryBitmapInfoHeader (mBitmap, (PBITMAPINFOHEADER2) &bih), FALSE);
   
      
      lStride = RASWIDTH( bih.cx, bih.cBitCount);
      mBits = new PRUint8 [ lStride * bih.cy ];
   
      
      int cols = -1;
      if( bih.cBitCount >= 24) cols = 0;
      else cols = 1 << bih.cBitCount;
   
      int szStruct = sizeof( BITMAPINFOHEADER2) + cols * sizeof( RGB2);
   
      mInfoHeader = (PBITMAPINFOHEADER2) calloc( szStruct, 1);
      mInfoHeader->cbFix = sizeof( BITMAPINFOHEADER2);
      mInfoHeader->cx = bih.cx;
      mInfoHeader->cy = bih.cy;
      mInfoHeader->cPlanes = 1;
      mInfoHeader->cBitCount = (USHORT) bih.cBitCount;
      
      mInfoHeader->ulCompression = BCA_UNCOMP;
      mInfoHeader->usRecording = BRA_BOTTOMUP;
      mInfoHeader->usRendering = BRH_NOTHALFTONED; 
      mInfoHeader->ulColorEncoding = BCE_RGB;
   }
   else
      lStride = RASWIDTH( mInfoHeader->cx, mInfoHeader->cBitCount);

   
   mYPels = mInfoHeader->cy - aY - aHeight;
   mScans = aHeight;

   rc = GFX (::GpiQueryBitmapBits (mPS, mYPels, mScans, (PBYTE)mBits,
                                   (PBITMAPINFO2)mInfoHeader), GPI_ALTERROR);

#ifdef DEBUG
   if( rc != mScans) {
     PMERROR( "GpiQueryBitmapBits");
     printf( "Lock, requested %d x %d and got %d x %d\n",
             aWidth, aHeight, (int) mInfoHeader->cx, aHeight);
   }
#endif

   
   *aStride = lStride;
   *aBits = (void*) (mBits + (aX * (mInfoHeader->cBitCount >> 3)));
   *aWidthBytes = aWidth * (mInfoHeader->cBitCount >> 3);

   return NS_OK;
}

nsresult nsOffscreenSurface::Unlock()
{
   GFX (::GpiSetBitmapBits (mPS, mYPels, mScans, (PBYTE)mBits,
                            (PBITMAPINFO2)mInfoHeader), GPI_ALTERROR);

   return NS_OK;
}

nsresult nsOffscreenSurface::IsOffscreen( PRBool *aOffScreen)
{
   if( !aOffScreen)
      return NS_ERROR_NULL_POINTER;

   *aOffScreen = PR_TRUE;

   return NS_OK;
}

nsresult nsOffscreenSurface::IsPixelAddressable( PRBool *aAddressable)
{
   if( !aAddressable)
      return NS_ERROR_NULL_POINTER;

   *aAddressable = PR_TRUE;

   return NS_OK;
}

nsresult nsOffscreenSurface::GetPixelFormat( nsPixelFormat *aFormat)
{
   if( !aFormat)
      return NS_ERROR_NULL_POINTER;

   
   
   
   
   
   
   BITMAPINFOHEADER bih = { sizeof( BITMAPINFOHEADER), 0, 0, 0, 0 };
   GFX (::GpiQueryBitmapInfoHeader (mBitmap, (PBITMAPINFOHEADER2)&bih), FALSE);

   switch( bih.cBitCount)
   {
      case 8:
         memset( aFormat, 0, sizeof(nsPixelFormat));
         break;

      case 16:
         aFormat->mRedZeroMask   = 0x001F;
         aFormat->mGreenZeroMask = 0x003F;
         aFormat->mBlueZeroMask  = 0x001F;
         aFormat->mAlphaZeroMask = 0;
         aFormat->mRedMask       = 0xF800;
         aFormat->mGreenMask     = 0x07E0;
         aFormat->mBlueMask      = 0x001F;
         aFormat->mAlphaMask     = 0;
         aFormat->mRedCount      = 5;
         aFormat->mGreenCount    = 6;
         aFormat->mBlueCount     = 5;
         aFormat->mAlphaCount    = 0;
         aFormat->mRedShift      = 11;
         aFormat->mGreenShift    = 5;
         aFormat->mBlueShift     = 0;
         aFormat->mAlphaShift    = 0;
         break;

      case 24:
         aFormat->mRedZeroMask   = 0x0000FF;
         aFormat->mGreenZeroMask = 0x0000FF;
         aFormat->mBlueZeroMask  = 0x0000FF;
         aFormat->mAlphaZeroMask = 0;
         aFormat->mRedMask       = 0x0000FF;
         aFormat->mGreenMask     = 0x00FF00;
         aFormat->mBlueMask      = 0xFF0000;
         aFormat->mAlphaMask     = 0;
         aFormat->mRedCount      = 8;
         aFormat->mGreenCount    = 8;
         aFormat->mBlueCount     = 8;
         aFormat->mAlphaCount    = 0;
         aFormat->mRedShift      = 0;
         aFormat->mGreenShift    = 8;
         aFormat->mBlueShift     = 16;
         aFormat->mAlphaShift    = 0;
         break;

      case 32:
         aFormat->mRedZeroMask   = 0x000000FF;
         aFormat->mGreenZeroMask = 0x000000FF;
         aFormat->mBlueZeroMask  = 0x000000FF;
         aFormat->mAlphaZeroMask = 0x000000FF;
         aFormat->mRedMask       = 0x00FF0000;
         aFormat->mGreenMask     = 0x0000FF00;
         aFormat->mBlueMask      = 0x000000FF;
         aFormat->mAlphaMask     = 0xFF000000;
         aFormat->mRedCount      = 8;
         aFormat->mGreenCount    = 8;
         aFormat->mBlueCount     = 8;
         aFormat->mAlphaCount    = 8;
         aFormat->mRedShift      = 16;
         aFormat->mGreenShift    = 8;
         aFormat->mBlueShift     = 0;
         aFormat->mAlphaShift    = 24;
         break;

      default:
#ifdef DEBUG
         printf( "Bad bit-depth for GetPixelFormat (%d)\n", bih.cBitCount);
#endif
         break;
   }

   return NS_OK;
}


nsOnscreenSurface::nsOnscreenSurface() : mProxySurface(nsnull)
{
}

nsOnscreenSurface::~nsOnscreenSurface()
{
   NS_IF_RELEASE(mProxySurface);
}

void nsOnscreenSurface::EnsureProxy()
{
   if( !mProxySurface)
   {
      PRUint32 width, height;
      GetDimensions( &width, &height);

      mProxySurface = new nsOffscreenSurface;
      if( NS_SUCCEEDED(mProxySurface->Init( mPS, width, height, NS_CREATEDRAWINGSURFACE_FOR_PIXEL_ACCESS)))
      {
         NS_ADDREF(mProxySurface);
      }
      else
      {
         delete mProxySurface;
         mProxySurface = nsnull;
      }
   }
}

nsresult nsOnscreenSurface::Lock( PRInt32 aX, PRInt32 aY,
                                  PRUint32 aWidth, PRUint32 aHeight,
                                  void **aBits, PRInt32 *aStride,
                                  PRInt32 *aWidthBytes,
                                  PRUint32 aFlags)
{
   EnsureProxy();

#ifdef DEBUG
   printf( "Locking through a proxy\n");
#endif

   
   PRUint32 width, height;
   GetDimensions( &width, &height);
   POINTL pts[3] = { { 0, 0 }, { width, height }, { 0, 0 } };
   GFX (::GpiBitBlt (mProxySurface->GetPS (), mPS, 3, pts,
                     ROP_SRCCOPY, BBO_OR), GPI_ERROR);

   return mProxySurface->Lock( aX, aY, aWidth, aHeight,
                               aBits, aStride, aWidthBytes, aFlags);
}

nsresult nsOnscreenSurface::Unlock()
{
   nsresult rc = mProxySurface->Unlock();

   
   PRUint32 width, height;
   GetDimensions( &width, &height);
   POINTL pts[3] = { { 0, 0 }, { width, height }, { 0, 0 } };
   GFX (::GpiBitBlt (mPS, mProxySurface->GetPS (), 3, pts,
                     ROP_SRCCOPY, BBO_OR), GPI_ERROR);

   return rc;
}

nsresult nsOnscreenSurface::GetPixelFormat( nsPixelFormat *aFormat)
{
   EnsureProxy();
   return mProxySurface->GetPixelFormat( aFormat);
}

nsresult nsOnscreenSurface::IsOffscreen( PRBool *aOffScreen)
{
   if( !aOffScreen)
      return NS_ERROR_NULL_POINTER;

   *aOffScreen = PR_FALSE;

   return NS_OK;
}

nsresult nsOnscreenSurface::IsPixelAddressable( PRBool *aAddressable)
{
   if( !aAddressable)
      return NS_ERROR_NULL_POINTER;

   *aAddressable = PR_FALSE;

   return NS_OK;
}


nsWindowSurface::nsWindowSurface() : mWidget(nsnull)
{
}

nsWindowSurface::~nsWindowSurface()
{
   

   
   DisposeFonts();
   
   
   if (mOwnPS) {
     mWidget->FreeNativeData( (void*) mPS, NS_NATIVE_GRAPHIC);
   }
}

NS_IMETHODIMP nsWindowSurface::Init(HPS aPS, nsIWidget *aWidget)
{
  mPS = aPS;
  mWidget = aWidget;

  return NS_OK;
}

nsresult nsWindowSurface::Init( nsIWidget *aOwner)
{
   mWidget = aOwner;
   mPS = (HPS) mWidget->GetNativeData( NS_NATIVE_GRAPHIC);
   mOwnPS = PR_TRUE;

   return NS_OK;
}

nsresult nsWindowSurface::GetDimensions( PRUint32 *aWidth, PRUint32 *aHeight)
{
   
   
   nsRect rect;
   mWidget->GetClientBounds( rect);
   *aHeight = rect.height;
   *aWidth = rect.width;
   return NS_OK;
}

PRUint32 nsWindowSurface::GetHeight () 
{ 
   nsRect rect;

   mWidget->GetClientBounds (rect);

   return rect.height;
}


nsPrintSurface::nsPrintSurface()
{
}

nsresult nsPrintSurface::Init( HPS aPS, PRInt32 aWidth, PRInt32 aHeight, PRUint32 aFlags)
{
   mPS = aPS;
   mHeight = aHeight;
   mWidth = aWidth;

   return NS_OK;
}

nsPrintSurface::~nsPrintSurface()
{
   
}
