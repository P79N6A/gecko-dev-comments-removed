





































#ifndef imgFrame_h
#define imgFrame_h

#include "nsRect.h"
#include "nsPoint.h"
#include "nsSize.h"
#include "gfxTypes.h"
#include "nsID.h"
#include "gfxIFormats.h"
#include "gfxContext.h"
#include "gfxPattern.h"
#include "gfxImageSurface.h"
#if defined(XP_WIN)
#include "gfxWindowsSurface.h"
#elif defined(XP_MACOSX)
#include "gfxQuartzImageSurface.h"
#endif
#include "nsAutoPtr.h"

class imgFrame
{
public:
  imgFrame();
  ~imgFrame();

  nsresult Init(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, gfxASurface::gfxImageFormat aFormat, PRInt8 aPaletteDepth = 0);
  nsresult Optimize();

  void Draw(gfxContext *aContext, gfxPattern::GraphicsFilter aFilter,
            const gfxMatrix &aUserSpaceToImageSpace, const gfxRect& aFill,
            const nsIntMargin &aPadding, const nsIntRect &aSubimage);

  nsresult Extract(const nsIntRect& aRegion, imgFrame** aResult);

  nsresult ImageUpdated(const nsIntRect &aUpdateRect);

  PRInt32 GetX() const;
  PRInt32 GetY() const;
  PRInt32 GetWidth() const;
  PRInt32 GetHeight() const;
  nsIntRect GetRect() const;
  gfxASurface::gfxImageFormat GetFormat() const;
  PRBool GetNeedsBackground() const;
  PRUint32 GetImageBytesPerRow() const;
  PRUint32 GetImageDataLength() const;
  PRBool GetIsPaletted() const;
  PRBool GetHasAlpha() const;
  void GetImageData(PRUint8 **aData, PRUint32 *length) const;
  void GetPaletteData(PRUint32 **aPalette, PRUint32 *length) const;

  PRInt32 GetTimeout() const;
  void SetTimeout(PRInt32 aTimeout);

  PRInt32 GetFrameDisposalMethod() const;
  void SetFrameDisposalMethod(PRInt32 aFrameDisposalMethod);
  PRInt32 GetBlendMethod() const;
  void SetBlendMethod(PRInt32 aBlendMethod);
  PRBool ImageComplete() const;

  void SetHasNoAlpha();

  PRBool GetCompositingFailed() const;
  void SetCompositingFailed(PRBool val);

  nsresult LockImageData();
  nsresult UnlockImageData();

  nsresult GetSurface(gfxASurface **aSurface) const
  {
    *aSurface = ThebesSurface();
    NS_IF_ADDREF(*aSurface);
    return NS_OK;
  }

  nsresult GetPattern(gfxPattern **aPattern) const
  {
    if (mSinglePixel)
      *aPattern = new gfxPattern(mSinglePixelColor);
    else
      *aPattern = new gfxPattern(ThebesSurface());
    NS_ADDREF(*aPattern);
    return NS_OK;
  }

  gfxASurface* ThebesSurface() const
  {
    if (mOptSurface)
      return mOptSurface;
#if defined(XP_WIN) && !defined(WINCE)
    if (mWinSurface)
      return mWinSurface;
#elif defined(XP_MACOSX)
    if (mQuartzSurface)
      return mQuartzSurface;
#endif
    return mImageSurface;
  }


private: 
  PRUint32 PaletteDataLength() const {
    return ((1 << mPaletteDepth) * sizeof(PRUint32));
  }

private: 
  nsRefPtr<gfxImageSurface> mImageSurface;
  nsRefPtr<gfxASurface> mOptSurface;
#if defined(XP_WIN) && !defined(WINCE)
  nsRefPtr<gfxWindowsSurface> mWinSurface;
#elif defined(XP_MACOSX)
  nsRefPtr<gfxQuartzImageSurface> mQuartzSurface;
#endif

  nsIntSize    mSize;
  nsIntPoint   mOffset;

  nsIntRect    mDecoded;

  
  
  
  
  PRUint8*     mPalettedImageData;

  gfxRGBA      mSinglePixelColor;

  PRInt32      mTimeout; 
  PRInt32      mDisposalMethod;

  gfxASurface::gfxImageFormat mFormat;
  PRInt8       mPaletteDepth;
  PRInt8       mBlendMethod;
  PRPackedBool mSinglePixel;
  PRPackedBool mNeverUseDeviceSurface;
  PRPackedBool mFormatChanged;
  PRPackedBool mCompositingFailed;

#ifdef XP_WIN
  PRPackedBool mIsDDBSurface;
#endif

};

#endif 
