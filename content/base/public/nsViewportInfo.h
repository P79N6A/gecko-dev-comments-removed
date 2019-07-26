



#ifndef nsViewportInfo_h___
#define nsViewportInfo_h___

#include <stdint.h>
#include "mozilla/Attributes.h"
#include "Units.h"




static const mozilla::LayoutDeviceToScreenScale kViewportMinScale(0.0f);
static const mozilla::LayoutDeviceToScreenScale kViewportMaxScale(10.0f);
static const mozilla::CSSIntSize kViewportMinSize(200, 223);
static const mozilla::CSSIntSize kViewportMaxSize(10000, 10000);
static const int32_t  kViewportDefaultScreenWidth = 980;





class MOZ_STACK_CLASS nsViewportInfo
{
  public:
    nsViewportInfo(const mozilla::ScreenIntSize& aDisplaySize) :
      mDefaultZoom(1.0),
      mAutoSize(true),
      mAllowZoom(true)
    {
        mSize = mozilla::gfx::RoundedToInt(mozilla::ScreenSize(aDisplaySize) / mDefaultZoom);
        mozilla::CSSToLayoutDeviceScale pixelRatio(1.0f);
        mMinZoom = pixelRatio * kViewportMinScale;
        mMaxZoom = pixelRatio * kViewportMaxScale;
        ConstrainViewportValues();
    }

    nsViewportInfo(const mozilla::CSSToScreenScale& aDefaultZoom,
                   const mozilla::CSSToScreenScale& aMinZoom,
                   const mozilla::CSSToScreenScale& aMaxZoom,
                   const mozilla::CSSIntSize& aSize,
                   bool aAutoSize,
                   bool aAllowZoom) :
                     mDefaultZoom(aDefaultZoom),
                     mMinZoom(aMinZoom),
                     mMaxZoom(aMaxZoom),
                     mSize(aSize),
                     mAutoSize(aAutoSize),
                     mAllowZoom(aAllowZoom)
    {
      ConstrainViewportValues();
    }

    mozilla::CSSToScreenScale GetDefaultZoom() { return mDefaultZoom; }
    void SetDefaultZoom(const mozilla::CSSToScreenScale& aDefaultZoom);
    mozilla::CSSToScreenScale GetMinZoom() { return mMinZoom; }
    mozilla::CSSToScreenScale GetMaxZoom() { return mMaxZoom; }

    mozilla::CSSIntSize GetSize() { return mSize; }

    bool IsAutoSizeEnabled() { return mAutoSize; }
    bool IsZoomAllowed() { return mAllowZoom; }

  private:

    




    void ConstrainViewportValues();

    
    
    mozilla::CSSToScreenScale mDefaultZoom;

    
    mozilla::CSSToScreenScale mMinZoom;

    
    mozilla::CSSToScreenScale mMaxZoom;

    
    mozilla::CSSIntSize mSize;

    
    
    
    
    bool mAutoSize;

    
    bool mAllowZoom;
};

#endif

