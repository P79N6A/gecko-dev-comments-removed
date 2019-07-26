






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

  



  void Draw(nsPresContext*       aPresContext,
            nsRenderingContext&  aRenderingContext,
            const nsRect&        aDirtyRect,
            const nsRect&        aFill,
            const nsRect&        aDest);
  




  void DrawBackground(nsPresContext*       aPresContext,
                      nsRenderingContext&  aRenderingContext,
                      const nsRect&        aDest,
                      const nsRect&        aFill,
                      const nsPoint&       aAnchor,
                      const nsRect&        aDirty);

  bool IsRasterImage();
  bool IsAnimatedImage();
  already_AddRefed<ImageContainer> GetContainer(LayerManager* aManager);

  bool IsReady() { return mIsReady; }

private:
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
    : mImageRenderer(aForFrame, aImage, aFlags) {}

  


  nsImageRenderer mImageRenderer;
  




  nsRect mDestArea;
  




  nsRect mFillArea;
  




  nsPoint mAnchor;
};

struct nsCSSRendering {
  


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
                                  const nsRect& aDirtyRect);

  static void ComputePixelRadii(const nscoord *aAppUnitsRadii,
                                nscoord aAppUnitsPerPixel,
                                gfxCornerSizes *oBorderRadii);

  




  static void PaintBorder(nsPresContext* aPresContext,
                          nsRenderingContext& aRenderingContext,
                          nsIFrame* aForFrame,
                          const nsRect& aDirtyRect,
                          const nsRect& aBorderArea,
                          nsStyleContext* aStyleContext,
                          int aSkipSides = 0);

  



  static void PaintBorderWithStyleBorder(nsPresContext* aPresContext,
                                         nsRenderingContext& aRenderingContext,
                                         nsIFrame* aForFrame,
                                         const nsRect& aDirtyRect,
                                         const nsRect& aBorderArea,
                                         const nsStyleBorder& aBorderStyle,
                                         nsStyleContext* aStyleContext,
                                         int aSkipSides = 0);


  




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
                            const nsRect& aOneCellArea,
                            const nsRect& aFillArea);

  






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
                                   const nsStyleBackground& aBackground,
                                   const nsStyleBackground::Layer& aLayer,
                                   nsIFrame** aAttachedToFrame);

  static nsBackgroundLayerState
  PrepareBackgroundLayer(nsPresContext* aPresContext,
                         nsIFrame* aForFrame,
                         uint32_t aFlags,
                         const nsRect& aBorderArea,
                         const nsRect& aBGClipRect,
                         const nsStyleBackground& aBackground,
                         const nsStyleBackground::Layer& aLayer);

  



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
 
  static void PaintBackgroundColor(nsPresContext* aPresContext,
                                   nsRenderingContext& aRenderingContext,
                                   nsIFrame* aForFrame,
                                   const nsRect& aDirtyRect,
                                   const nsRect& aBorderArea,
                                   uint32_t aFlags);

  








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

  static void PaintBackgroundColorWithSC(nsPresContext* aPresContext,
                                         nsRenderingContext& aRenderingContext,
                                         nsIFrame* aForFrame,
                                         const nsRect& aDirtyRect,
                                         const nsRect& aBorderArea,
                                         nsStyleContext *aStyleContext,
                                         const nsStyleBorder& aBorder,
                                         uint32_t aFlags);
  




  static nsRect GetBackgroundLayerRect(nsPresContext* aPresContext,
                                       nsIFrame* aForFrame,
                                       const nsRect& aBorderArea,
                                       const nsRect& aClipRect,
                                       const nsStyleBackground& aBackground,
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

  




  void DoEffects();
  
  




  void DoPaint();

  




  gfxContext* GetContext();


  




  static nsMargin GetBlurRadiusMargin(nscoord aBlurRadius,
                                      int32_t aAppUnitsPerDevPixel);

protected:
  gfxAlphaBoxBlur blur;
  nsRefPtr<gfxContext> mContext;
  gfxContext* mDestinationCtx;

  

  bool mPreTransformed;

};

#endif
