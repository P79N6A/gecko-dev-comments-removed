



#ifndef nsViewportInfo_h___
#define nsViewportInfo_h___

#include <stdint.h>
#include "nscore.h"
#include "Units.h"




static const mozilla::LayoutDeviceToScreenScale kViewportMinScale(0.0f);
static const mozilla::LayoutDeviceToScreenScale kViewportMaxScale(10.0f);
static const uint32_t kViewportMinWidth = 200;
static const uint32_t kViewportMaxWidth = 10000;
static const uint32_t kViewportMinHeight = 223;
static const uint32_t kViewportMaxHeight = 10000;
static const int32_t  kViewportDefaultScreenWidth = 980;





class MOZ_STACK_CLASS nsViewportInfo
{
  public:
    nsViewportInfo(uint32_t aDisplayWidth, uint32_t aDisplayHeight) :
      mDefaultZoom(1.0),
      mWidth(aDisplayWidth),
      mHeight(aDisplayHeight),
      mAutoSize(true),
      mAllowZoom(true)
    {
        mozilla::CSSToLayoutDeviceScale pixelRatio(1.0f);
        mMinZoom = pixelRatio * kViewportMinScale;
        mMaxZoom = pixelRatio * kViewportMaxScale;
        ConstrainViewportValues();
    }

    nsViewportInfo(const mozilla::CSSToScreenScale& aDefaultZoom,
                   const mozilla::CSSToScreenScale& aMinZoom,
                   const mozilla::CSSToScreenScale& aMaxZoom,
                   uint32_t aWidth,
                   uint32_t aHeight,
                   bool aAutoSize,
                   bool aAllowZoom) :
                     mDefaultZoom(aDefaultZoom),
                     mMinZoom(aMinZoom),
                     mMaxZoom(aMaxZoom),
                     mWidth(aWidth),
                     mHeight(aHeight),
                     mAutoSize(aAutoSize),
                     mAllowZoom(aAllowZoom)
    {
      ConstrainViewportValues();
    }

    mozilla::CSSToScreenScale GetDefaultZoom() { return mDefaultZoom; }
    void SetDefaultZoom(const mozilla::CSSToScreenScale& aDefaultZoom);
    mozilla::CSSToScreenScale GetMinZoom() { return mMinZoom; }
    mozilla::CSSToScreenScale GetMaxZoom() { return mMaxZoom; }

    uint32_t GetWidth() { return mWidth; }
    uint32_t GetHeight() { return mHeight; }

    bool IsAutoSizeEnabled() { return mAutoSize; }
    bool IsZoomAllowed() { return mAllowZoom; }

  private:

    




    void ConstrainViewportValues();

    
    
    mozilla::CSSToScreenScale mDefaultZoom;

    
    mozilla::CSSToScreenScale mMinZoom;

    
    mozilla::CSSToScreenScale mMaxZoom;

    
    
    uint32_t mWidth;

    
    
    uint32_t mHeight;

    
    
    
    
    bool mAutoSize;

    
    bool mAllowZoom;
};

#endif

