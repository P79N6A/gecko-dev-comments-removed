






#ifndef nsCSSRendering_h___
#define nsCSSRendering_h___

#include "gfxBlur.h"
#include "gfxContext.h"
#include "nsLayoutUtils.h"
#include "nsStyleStruct.h"
#include "nsIFrame.h"

class nsStyleContext;
class nsPresContext;
class nsRenderingContext;

namespace mozilla {

namespace layers {
class ImageContainer;
}





struct CSSSizeOrRatio
{
  CSSSizeOrRatio()
    : mRatio(0, 0)
    , mHasWidth(false)
    , mHasHeight(false) {}

  bool CanComputeConcreteSize() const
  {
    return mHasWidth + mHasHeight + HasRatio() >= 2;
  }
  bool IsConcrete() const { return mHasWidth && mHasHeight; }
  bool HasRatio() const { return mRatio.width > 0 && mRatio.height > 0; }
  bool IsEmpty() const
  {
    return (mHasWidth && mWidth <= 0) ||
           (mHasHeight && mHeight <= 0) ||
           mRatio.width <= 0 || mRatio.height <= 0; 
  }

  
  
  nsSize ComputeConcreteSize() const;

  void SetWidth(nscoord aWidth)
  {
    mWidth = aWidth;
    mHasWidth = true;
    if (mHasHeight) {
      mRatio = nsSize(mWidth, mHeight);
    }
  }
  void SetHeight(nscoord aHeight)
  {
    mHeight = aHeight;
    mHasHeight = true;
    if (mHasWidth) {
      mRatio = nsSize(mWidth, mHeight);
    }
  }
  void SetSize(const nsSize& aSize)
  {
    mWidth = aSize.width;
    mHeight = aSize.height;
    mHasWidth = true;
    mHasHeight = true;
    mRatio = aSize;    
  }
  void SetRatio(const nsSize& aRatio)
  {
    MOZ_ASSERT(!mHasWidth || !mHasHeight,
               "Probably shouldn't be setting a ratio if we have a concrete size");
    mRatio = aRatio;
  }

  nsSize mRatio;
  nscoord mWidth;
  nscoord mHeight;
  bool mHasWidth;
  bool mHasHeight;
};

}









class nsImageRenderer {
public:
  typedef mozilla::layers::LayerManager LayerManager;
  typedef mozilla::layers::ImageContainer ImageContainer;

  enum {
    FLAG_SYNC_DECODE_IMAGES = 0x01,
    FLAG_PAINTING_TO_WINDOW = 0x02
  };
  enum FitType
  {
    CONTAIN,
    COVER
  };

  nsImageRenderer(nsIFrame* aForFrame, const nsStyleImage* aImage, uint32_t aFlags);
  ~nsImageRenderer();
  




  bool PrepareImage();

  




   
  




  mozilla::CSSSizeOrRatio ComputeIntrinsicSize();

  





  static nsSize ComputeConstrainedSize(const nsSize& aConstrainingSize,
                                       const nsSize& aIntrinsicRatio,
                                       FitType aFitType);
  




  static nsSize ComputeConcreteSize(const mozilla::CSSSizeOrRatio& aSpecifiedSize,
                                    const mozilla::CSSSizeOrRatio& aIntrinsicSize,
                                    const nsSize& aDefaultSize);

  




  void SetPreferredSize(const mozilla::CSSSizeOrRatio& aIntrinsicSize,
                        const nsSize& aDefaultSize);

  




  void DrawBackground(nsPresContext*       aPresContext,
                      nsRenderingContext&  aRenderingContext,
                      const nsRect&        aDest,
                      const nsRect&        aFill,
                      const nsPoint&       aAnchor,
                      const nsRect&        aDirty);

  












  void
  DrawBorderImageComponent(nsPresContext*       aPresContext,
                           nsRenderingContext&  aRenderingContext,
                           const nsRect&        aDirtyRect,
                           const nsRect&        aFill,
                           const mozilla::CSSIntRect& aSrc,
                           uint8_t              aHFill,
                           uint8_t              aVFill,
                           const nsSize&        aUnitSize,
                           uint8_t              aIndex);

  bool IsRasterImage();
  bool IsAnimatedImage();
  already_AddRefed<ImageContainer> GetContainer(LayerManager* aManager);

  bool IsReady() { return mIsReady; }

private:
  






  void Draw(nsPresContext*       aPresContext,
            nsRenderingContext&  aRenderingContext,
            const nsRect&        aDirtyRect,
            const nsRect&        aDest,
            const nsRect&        aFill,
            const nsPoint&       aAnchor,
            const mozilla::CSSIntRect& aSrc);

  





  already_AddRefed<gfxDrawable> DrawableForElement(const nsRect& aImageRect,
                                                   nsRenderingContext&  aRenderingContext);

  nsIFrame*                 mForFrame;
  const nsStyleImage*       mImage;
  nsStyleImageType          mType;
  nsCOMPtr<imgIContainer>   mImageContainer;
  nsRefPtr<nsStyleGradient> mGradientData;
  nsIFrame*                 mPaintServerFrame;
  nsLayoutUtils::SurfaceFromElementResult mImageElementSurface;
  bool                      mIsReady;
  nsSize                    mSize; 
  uint32_t                  mFlags;
};






struct nsBackgroundLayerState {
  


  nsBackgroundLayerState(nsIFrame* aForFrame, const nsStyleImage* aImage, uint32_t aFlags)
    : mImageRenderer(aForFrame, aImage, aFlags), mCompositingOp(gfxContext::OPERATOR_OVER) {}

  


  nsImageRenderer mImageRenderer;
  




  nsRect mDestArea;
  




  nsRect mFillArea;
  




  nsPoint mAnchor;
  


  gfxContext::GraphicsOperator mCompositingOp;
};

struct nsCSSRendering {
  typedef nsIFrame::Sides Sides;

  


  static void Init();
  
  


  static void Shutdown();
  
  static void PaintBoxShadowInner(nsPresContext* aPresContext,
                                  nsRenderingContext& aRenderingContext,
                                  nsIFrame* aForFrame,
                                  const nsRect& aFrameArea,
                                  const nsRect& aDirtyRect);

  static void PaintBoxShadowOuter(nsPresContext* aPresContext,
                                  nsRenderingContext& aRenderingContext,
                                  nsIFrame* aForFrame,
                                  const nsRect& aFrameArea,
                                  const nsRect& aDirtyRect,
                                  float aOpacity = 1.0);

  static void ComputePixelRadii(const nscoord *aAppUnitsRadii,
                                nscoord aAppUnitsPerPixel,
                                gfxCornerSizes *oBorderRadii);

  




  static void PaintBorder(nsPresContext* aPresContext,
                          nsRenderingContext& aRenderingContext,
                          nsIFrame* aForFrame,
                          const nsRect& aDirtyRect,
                          const nsRect& aBorderArea,
                          nsStyleContext* aStyleContext,
                          Sides aSkipSides = Sides());

  




  static void PaintBorderWithStyleBorder(nsPresContext* aPresContext,
                                         nsRenderingContext& aRenderingContext,
                                         nsIFrame* aForFrame,
                                         const nsRect& aDirtyRect,
                                         const nsRect& aBorderArea,
                                         const nsStyleBorder& aBorderStyle,
                                         nsStyleContext* aStyleContext,
                                         Sides aSkipSides = Sides());


  



  static void PaintOutline(nsPresContext* aPresContext,
                          nsRenderingContext& aRenderingContext,
                          nsIFrame* aForFrame,
                          const nsRect& aDirtyRect,
                          const nsRect& aBorderArea,
                          nsStyleContext* aStyleContext);

  





  static void PaintFocus(nsPresContext* aPresContext,
                         nsRenderingContext& aRenderingContext,
                         const nsRect& aFocusRect,
                         nscolor aColor);

  









  static void PaintGradient(nsPresContext* aPresContext,
                            nsRenderingContext& aRenderingContext,
                            nsStyleGradient* aGradient,
                            const nsRect& aDirtyRect,
                            const nsRect& aDest,
                            const nsRect& aFill,
                            const mozilla::CSSIntRect& aSrc,
                            const nsSize& aIntrinsiceSize);

  






  static nsIFrame* FindBackgroundStyleFrame(nsIFrame* aForFrame);

  


  static bool IsCanvasFrame(nsIFrame* aFrame);

  





  static bool FindBackground(nsIFrame* aForFrame,
                             nsStyleContext** aBackgroundSC);

  




  static nsStyleContext* FindRootFrameBackground(nsIFrame* aForFrame);

  










  static nsStyleContext*
  FindCanvasBackground(nsIFrame* aForFrame, nsIFrame* aRootElementFrame)
  {
    NS_ABORT_IF_FALSE(IsCanvasFrame(aForFrame), "not a canvas frame");
    if (aRootElementFrame)
      return FindRootFrameBackground(aRootElementFrame);

    
    
    
    return aForFrame->StyleContext();
  }

  








  static nsIFrame*
  FindNonTransparentBackgroundFrame(nsIFrame* aFrame,
                                    bool aStartAtParent = false);

  


  static nscolor
  DetermineBackgroundColor(nsPresContext* aPresContext,
                           nsStyleContext* aStyleContext,
                           nsIFrame* aFrame,
                           bool& aDrawBackgroundImage,
                           bool& aDrawBackgroundColor);

  static nsRect
  ComputeBackgroundPositioningArea(nsPresContext* aPresContext,
                                   nsIFrame* aForFrame,
                                   const nsRect& aBorderArea,
                                   const nsStyleBackground::Layer& aLayer,
                                   nsIFrame** aAttachedToFrame);

  static nsBackgroundLayerState
  PrepareBackgroundLayer(nsPresContext* aPresContext,
                         nsIFrame* aForFrame,
                         uint32_t aFlags,
                         const nsRect& aBorderArea,
                         const nsRect& aBGClipRect,
                         const nsStyleBackground::Layer& aLayer);

  struct BackgroundClipState {
    nsRect mBGClipArea;  
    nsRect mAdditionalBGClipArea;  
    nsRect mDirtyRect;
    gfxRect mDirtyRectGfx;

    nscoord mRadii[8];
    gfxCornerSizes mClippedRadii;
    bool mHasRoundedCorners;
    bool mHasAdditionalBGClipArea;

    
    
    bool mCustomClip;
  };

  static void
  GetBackgroundClip(const nsStyleBackground::Layer& aLayer,
                    nsIFrame* aForFrame, const nsStyleBorder& aBorder, const nsRect& aBorderArea,
                    const nsRect& aCallerDirtyRect, bool aWillPaintBorder,
                    nscoord aAppUnitsPerPixel,
                     BackgroundClipState* aClipState);

  



  enum {
    



    PAINTBG_WILL_PAINT_BORDER = 0x01,
    


    PAINTBG_SYNC_DECODE_IMAGES = 0x02,
    



    PAINTBG_TO_WINDOW = 0x04
  };
  static void PaintBackground(nsPresContext* aPresContext,
                              nsRenderingContext& aRenderingContext,
                              nsIFrame* aForFrame,
                              const nsRect& aDirtyRect,
                              const nsRect& aBorderArea,
                              uint32_t aFlags,
                              nsRect* aBGClipRect = nullptr,
                              int32_t aLayer = -1);

  








  static void PaintBackgroundWithSC(nsPresContext* aPresContext,
                                    nsRenderingContext& aRenderingContext,
                                    nsIFrame* aForFrame,
                                    const nsRect& aDirtyRect,
                                    const nsRect& aBorderArea,
                                    nsStyleContext *aStyleContext,
                                    const nsStyleBorder& aBorder,
                                    uint32_t aFlags,
                                    nsRect* aBGClipRect = nullptr,
                                    int32_t aLayer = -1);

  




  static nsRect GetBackgroundLayerRect(nsPresContext* aPresContext,
                                       nsIFrame* aForFrame,
                                       const nsRect& aBorderArea,
                                       const nsRect& aClipRect,
                                       const nsStyleBackground::Layer& aLayer,
                                       uint32_t aFlags);

  


  static bool IsBackgroundImageDecodedForStyleContextAndLayer(
    const nsStyleBackground *aBackground, uint32_t aLayer);

  



  static bool AreAllBackgroundImagesDecodedForFrame(nsIFrame* aFrame);

  



  static void BeginFrameTreesLocked();
  




  static void EndFrameTreesLocked();

  
  
  static void DrawTableBorderSegment(nsRenderingContext& aContext,
                                     uint8_t              aBorderStyle,  
                                     nscolor              aBorderColor,
                                     const nsStyleBackground* aBGColor,
                                     const nsRect&        aBorderRect,
                                     int32_t              aAppUnitsPerCSSPixel,
                                     uint8_t              aStartBevelSide = 0,
                                     nscoord              aStartBevelOffset = 0,
                                     uint8_t              aEndBevelSide = 0,
                                     nscoord              aEndBevelOffset = 0);

  






































  static void PaintDecorationLine(nsIFrame* aFrame,
                                  gfxContext* aGfxContext,
                                  const gfxRect& aDirtyRect,
                                  const nscolor aColor,
                                  const gfxPoint& aPt,
                                  const gfxFloat aXInFrame,
                                  const gfxSize& aLineSize,
                                  const gfxFloat aAscent,
                                  const gfxFloat aOffset,
                                  const uint8_t aDecoration,
                                  const uint8_t aStyle,
                                  const gfxFloat aDescentLimit = -1.0);

  






  static void DecorationLineToPath(nsIFrame* aFrame,
                                   gfxContext* aGfxContext,
                                   const gfxRect& aDirtyRect,
                                   const nscolor aColor,
                                   const gfxPoint& aPt,
                                   const gfxFloat aXInFrame,
                                   const gfxSize& aLineSize,
                                   const gfxFloat aAscent,
                                   const gfxFloat aOffset,
                                   const uint8_t aDecoration,
                                   const uint8_t aStyle,
                                   const gfxFloat aDescentLimit = -1.0);

  
































  static nsRect GetTextDecorationRect(nsPresContext* aPresContext,
                                      const gfxSize& aLineSize,
                                      const gfxFloat aAscent,
                                      const gfxFloat aOffset,
                                      const uint8_t aDecoration,
                                      const uint8_t aStyle,
                                      const gfxFloat aDescentLimit = -1.0);

  static gfxContext::GraphicsOperator GetGFXBlendMode(uint8_t mBlendMode) {
    switch (mBlendMode) {
      case NS_STYLE_BLEND_NORMAL:      return gfxContext::OPERATOR_OVER;
      case NS_STYLE_BLEND_MULTIPLY:    return gfxContext::OPERATOR_MULTIPLY;
      case NS_STYLE_BLEND_SCREEN:      return gfxContext::OPERATOR_SCREEN;
      case NS_STYLE_BLEND_OVERLAY:     return gfxContext::OPERATOR_OVERLAY;
      case NS_STYLE_BLEND_DARKEN:      return gfxContext::OPERATOR_DARKEN;
      case NS_STYLE_BLEND_LIGHTEN:     return gfxContext::OPERATOR_LIGHTEN;
      case NS_STYLE_BLEND_COLOR_DODGE: return gfxContext::OPERATOR_COLOR_DODGE;
      case NS_STYLE_BLEND_COLOR_BURN:  return gfxContext::OPERATOR_COLOR_BURN;
      case NS_STYLE_BLEND_HARD_LIGHT:  return gfxContext::OPERATOR_HARD_LIGHT;
      case NS_STYLE_BLEND_SOFT_LIGHT:  return gfxContext::OPERATOR_SOFT_LIGHT;
      case NS_STYLE_BLEND_DIFFERENCE:  return gfxContext::OPERATOR_DIFFERENCE;
      case NS_STYLE_BLEND_EXCLUSION:   return gfxContext::OPERATOR_EXCLUSION;
      case NS_STYLE_BLEND_HUE:         return gfxContext::OPERATOR_HUE;
      case NS_STYLE_BLEND_SATURATION:  return gfxContext::OPERATOR_SATURATION;
      case NS_STYLE_BLEND_COLOR:       return gfxContext::OPERATOR_COLOR;
      case NS_STYLE_BLEND_LUMINOSITY:  return gfxContext::OPERATOR_LUMINOSITY;
      default:                         MOZ_ASSERT(false); return gfxContext::OPERATOR_OVER;
    }

    return gfxContext::OPERATOR_OVER;
  }

protected:
  static gfxRect GetTextDecorationRectInternal(const gfxPoint& aPt,
                                               const gfxSize& aLineSize,
                                               const gfxFloat aAscent,
                                               const gfxFloat aOffset,
                                               const uint8_t aDecoration,
                                               const uint8_t aStyle,
                                               const gfxFloat aDscentLimit);

  


















  static gfxRect ExpandPaintingRectForDecorationLine(
                   nsIFrame* aFrame,
                   const uint8_t aStyle,
                   const gfxRect &aClippedRect,
                   const gfxFloat aXInFrame,
                   const gfxFloat aCycleLength);
};














class nsContextBoxBlur {
public:
  enum {
    FORCE_MASK = 0x01
  };
  















































  gfxContext* Init(const nsRect& aRect, nscoord aSpreadRadius,
                   nscoord aBlurRadius,
                   int32_t aAppUnitsPerDevPixel, gfxContext* aDestinationCtx,
                   const nsRect& aDirtyRect, const gfxRect* aSkipRect,
                   uint32_t aFlags = 0);

  




  void DoPaint();

  




  gfxContext* GetContext();


  




  static nsMargin GetBlurRadiusMargin(nscoord aBlurRadius,
                                      int32_t aAppUnitsPerDevPixel);

  




















  static void BlurRectangle(gfxContext* aDestinationCtx,
                            const nsRect& aRect,
                            int32_t aAppUnitsPerDevPixel,
                            gfxCornerSizes* aCornerRadii,
                            nscoord aBlurRadius,
                            const gfxRGBA& aShadowColor,
                            const nsRect& aDirtyRect,
                            const gfxRect& aSkipRect);

protected:
  gfxAlphaBoxBlur mAlphaBoxBlur;
  nsRefPtr<gfxContext> mContext;
  gfxContext* mDestinationCtx;

  

  bool mPreTransformed;

};

#endif
