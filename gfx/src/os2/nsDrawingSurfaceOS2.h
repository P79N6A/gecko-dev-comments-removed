














































#ifndef _nsDrawingSurfaceOS2_h
#define _nsDrawingSurfaceOS2_h

#include "nsIDrawingSurface.h"

class nsHashtable;
class nsIWidget;
class nsFontOS2;












class nsDrawingSurfaceOS2 : public nsIDrawingSurface
{
   nsHashtable   *mHTFonts; 
   long           mNextID;  
   long           mTopID;   

protected:
   HPS            mPS;      
   PRBool         mOwnPS;   
   PRInt32        mWidth;   
   PRInt32        mHeight;


   nsDrawingSurfaceOS2();
   virtual ~nsDrawingSurfaceOS2();

   NS_DECL_ISUPPORTS

   void DisposeFonts();     


 public:
   NS_IMETHOD GetDimensions( PRUint32 *aWidth, PRUint32 *aHeight);

   
   HPS  GetPS() { return mPS; }
   void SelectFont(nsFontOS2* aFont);
   void FlushFontCache();
   virtual PRUint32 GetHeight() { return mHeight; }

   
   void NS2PM_ININ (const nsRect &in, RECTL &rcl);
   void NS2PM_INEX (const nsRect &in, RECTL &rcl);
   void PM2NS_ININ (const RECTL &in, nsRect &out);
   void NS2PM      (PPOINTL aPointl, ULONG cPointls);
};


class nsOffscreenSurface : public nsDrawingSurfaceOS2
{
   HDC     mDC;
   HBITMAP mBitmap;

   PBITMAPINFOHEADER2  mInfoHeader;
   PRUint8            *mBits;

   PRInt32  mYPels;
   PRUint32 mScans;

 public:
   nsOffscreenSurface();
   virtual ~nsOffscreenSurface();

   
   NS_IMETHOD Init( HPS aCompatiblePS, PRInt32 aWidth, PRInt32 aHeight, PRUint32 aFlags);
   NS_IMETHOD Init(HPS aPS);

   HDC GetDC() { return mDC; }

   
   NS_IMETHOD Lock( PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                    void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                    PRUint32 aFlags);
   NS_IMETHOD Unlock();
   NS_IMETHOD IsOffscreen( PRBool *aOffScreen);
   NS_IMETHOD IsPixelAddressable( PRBool *aAddressable);
   NS_IMETHOD GetPixelFormat( nsPixelFormat *aFormat);
};


class nsOnscreenSurface : public nsDrawingSurfaceOS2
{
   nsOffscreenSurface *mProxySurface;
   void                EnsureProxy();

 protected:
   nsOnscreenSurface();
   virtual ~nsOnscreenSurface();

 public:
   
   NS_IMETHOD Lock( PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                    void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                    PRUint32 aFlags);
   NS_IMETHOD Unlock();
   NS_IMETHOD IsOffscreen( PRBool *aOffScreen);
   NS_IMETHOD IsPixelAddressable( PRBool *aAddressable);
   NS_IMETHOD GetPixelFormat( nsPixelFormat *aFormat);
};


class nsWindowSurface : public nsOnscreenSurface
{
   nsIWidget *mWidget; 

 public:
   nsWindowSurface();
   virtual ~nsWindowSurface();

   NS_IMETHOD Init(HPS aPS, nsIWidget *aWidget);
   NS_IMETHOD Init( nsIWidget *aOwner);
   NS_IMETHOD GetDimensions( PRUint32 *aWidth, PRUint32 *aHeight);

   virtual PRUint32 GetHeight ();
};


class nsPrintSurface : public nsOnscreenSurface
{
 public:
   nsPrintSurface();
   virtual ~nsPrintSurface();

   NS_IMETHOD Init( HPS aPS, PRInt32 aWidth, PRInt32 aHeight, PRUint32 aFlags);
};

#endif
