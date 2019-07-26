



#ifndef nsViewportInfo_h___
#define nsViewportInfo_h___

#include "mozilla/StandardInteger.h"
#include "nscore.h"




static const double   kViewportMinScale = 0.0;
static const double   kViewportMaxScale = 10.0;
static const uint32_t kViewportMinWidth = 200;
static const uint32_t kViewportMaxWidth = 10000;
static const uint32_t kViewportMinHeight = 223;
static const uint32_t kViewportMaxHeight = 10000;
static const int32_t  kViewportDefaultScreenWidth = 980;





class NS_STACK_CLASS nsViewportInfo
{
  public:
    nsViewportInfo(uint32_t aDisplayWidth, uint32_t aDisplayHeight) :
      mDefaultZoom(1.0),
      mMinZoom(kViewportMinScale),
      mMaxZoom(kViewportMaxScale),
      mWidth(aDisplayWidth),
      mHeight(aDisplayHeight),
      mAutoSize(true),
      mAllowZoom(true)
    {
        ConstrainViewportValues();
    }

    nsViewportInfo(double aDefaultZoom,
                   double aMinZoom,
                   double aMaxZoom,
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

    double GetDefaultZoom() { return mDefaultZoom; }
    void SetDefaultZoom(const double aDefaultZoom);
    double GetMinZoom() { return mMinZoom; }
    double GetMaxZoom() { return mMaxZoom; }

    uint32_t GetWidth() { return mWidth; }
    uint32_t GetHeight() { return mHeight; }

    bool IsAutoSizeEnabled() { return mAutoSize; }
    bool IsZoomAllowed() { return mAllowZoom; }

  private:

    




    void ConstrainViewportValues();

    
    
    double mDefaultZoom;

    
    double mMinZoom;

    
    double mMaxZoom;

    
    
    uint32_t mWidth;

    
    
    uint32_t mHeight;

    
    
    
    
    bool mAutoSize;

    
    bool mAllowZoom;
};

#endif

