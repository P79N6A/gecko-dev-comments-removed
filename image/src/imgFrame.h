





































#ifndef imgFrame_h
#define imgFrame_h

#include "nsRect.h"
#include "nsPoint.h"
#include "nsSize.h"
#include "gfxTypes.h"
#include "nsID.h"
#include "gfxContext.h"
#include "gfxPattern.h"
#include "gfxDrawable.h"
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

  nsresult Init(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, gfxASurface::gfxImageFormat aFormat, PRUint8 aPaletteDepth = 0);
  nsresult Optimize();

  void Draw(gfxContext *aContext, gfxPattern::GraphicsFilter aFilter,
            const gfxMatrix &aUserSpaceToImageSpace, const gfxRect& aFill,
            const nsIntMargin &aPadding, const nsIntRect &aSubimage);

  nsresult Extract(const nsIntRect& aRegion, imgFrame** aResult);

  nsresult ImageUpdated(const nsIntRect &aUpdateRect);

  nsIntRect GetRect() const;
  gfxASurface::gfxImageFormat GetFormat() const;
  bool GetNeedsBackground() const;
  PRUint32 GetImageBytesPerRow() const;
  PRUint32 GetImageDataLength() const;
  bool GetIsPaletted() const;
  bool GetHasAlpha() const;
  void GetImageData(PRUint8 **aData, PRUint32 *length) const;
  void GetPaletteData(PRUint32 **aPalette, PRUint32 *length) const;

  PRInt32 GetTimeout() const;
  void SetTimeout(PRInt32 aTimeout);

  PRInt32 GetFrameDisposalMethod() const;
  void SetFrameDisposalMethod(PRInt32 aFrameDisposalMethod);
  PRInt32 GetBlendMethod() const;
  void SetBlendMethod(PRInt32 aBlendMethod);
  bool ImageComplete() const;

  void SetHasNoAlpha();

  bool GetCompositingFailed() const;
  void SetCompositingFailed(bool val);

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
#if defined(XP_WIN)
    if (mWinSurface)
      return mWinSurface;
#elif defined(XP_MACOSX)
    if (mQuartzSurface)
      return mQuartzSurface;
#endif
    return mImageSurface;
  }

  PRUint32 EstimateMemoryUsed(gfxASurface::MemoryLocation aLocation) const;

  PRUint8 GetPaletteDepth() const { return mPaletteDepth; }

private: 
  PRUint32 PaletteDataLength() const {
    return ((1 << mPaletteDepth) * sizeof(PRUint32));
  }

  struct SurfaceWithFormat {
    nsRefPtr<gfxDrawable> mDrawable;
    gfxImageSurface::gfxImageFormat mFormat;
    SurfaceWithFormat() {}
    SurfaceWithFormat(gfxDrawable* aDrawable, gfxImageSurface::gfxImageFormat aFormat)
     : mDrawable(aDrawable), mFormat(aFormat) {}
    bool IsValid() { return !!mDrawable; }
  };

  SurfaceWithFormat SurfaceForDrawing(bool               aDoPadding,
                                      bool               aDoPartialDecode,
                                      bool               aDoTile,
                                      const nsIntMargin& aPadding,
                                      gfxMatrix&         aUserSpaceToImageSpace,
                                      gfxRect&           aFill,
                                      gfxRect&           aSubimage,
                                      gfxRect&           aSourceRect,
                                      gfxRect&           aImageRect);

private: 
  nsRefPtr<gfxImageSurface> mImageSurface;
  nsRefPtr<gfxASurface> mOptSurface;
#if defined(XP_WIN)
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
  PRUint8      mPaletteDepth;
  PRInt8       mBlendMethod;
  bool mSinglePixel;
  bool mNeverUseDeviceSurface;
  bool mFormatChanged;
  bool mCompositingFailed;
  
  bool mLocked;

#ifdef XP_WIN
  bool mIsDDBSurface;
#endif

};

#endif 
