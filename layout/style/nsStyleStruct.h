









#ifndef nsStyleStruct_h___
#define nsStyleStruct_h___

#include "mozilla/Attributes.h"
#include "mozilla/CSSVariableValues.h"
#include "nsColor.h"
#include "nsCoord.h"
#include "nsMargin.h"
#include "nsFont.h"
#include "nsStyleCoord.h"
#include "nsStyleConsts.h"
#include "nsChangeHint.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsCSSValue.h"
#include "imgRequestProxy.h"
#include "Orientation.h"
#include "CounterStyleManager.h"

class nsIFrame;
class nsIURI;
class imgIContainer;


#include "nsStyleStructFwd.h"



#define NS_STYLE_INHERIT_MASK              0x000ffffff



#define NS_STYLE_HAS_TEXT_DECORATION_LINES 0x001000000

#define NS_STYLE_HAS_PSEUDO_ELEMENT_DATA   0x002000000

#define NS_STYLE_RELEVANT_LINK_VISITED     0x004000000

#define NS_STYLE_IS_STYLE_IF_VISITED       0x008000000

#define NS_STYLE_USES_GRANDANCESTOR_STYLE  0x010000000

#define NS_STYLE_IS_SHARED                 0x020000000


#define NS_STYLE_IS_GOING_AWAY             0x040000000

#define NS_STYLE_SUPPRESS_LINEBREAK        0x080000000

#define NS_STYLE_IN_DISPLAY_NONE_SUBTREE   0x100000000

#define NS_STYLE_INELIGIBLE_FOR_SHARING    0x200000000

#define NS_STYLE_CONTEXT_TYPE_SHIFT        34


#define NS_RULE_NODE_GC_MARK                0x02000000
#define NS_RULE_NODE_USED_DIRECTLY          0x04000000
#define NS_RULE_NODE_IS_IMPORTANT           0x08000000
#define NS_RULE_NODE_LEVEL_MASK             0xf0000000
#define NS_RULE_NODE_LEVEL_SHIFT            28



struct nsStyleFont {
  nsStyleFont(const nsFont& aFont, nsPresContext *aPresContext);
  nsStyleFont(const nsStyleFont& aStyleFont);
  explicit nsStyleFont(nsPresContext *aPresContext);
private:
  void Init(nsPresContext *aPresContext);
public:
  ~nsStyleFont(void) {
    MOZ_COUNT_DTOR(nsStyleFont);
  }

  nsChangeHint CalcDifference(const nsStyleFont& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(NS_STYLE_HINT_REFLOW,
                          nsChangeHint_NeutralChange);
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }
  static nsChangeHint CalcFontDifference(const nsFont& aFont1, const nsFont& aFont2);

  static nscoord ZoomText(nsPresContext* aPresContext, nscoord aSize);
  static nscoord UnZoomText(nsPresContext* aPresContext, nscoord aSize);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleFont_id, sz);
  }
  void Destroy(nsPresContext* aContext);

  void EnableZoom(nsPresContext* aContext, bool aEnable);

  nsFont  mFont;        
  nscoord mSize;        
                        
                        
                        
                        
                        
  uint8_t mGenericID;   
                        

  
  int8_t  mScriptLevel;          
  
  uint8_t mMathVariant;          
  
  uint8_t mMathDisplay;         

  
  bool mExplicitLanguage;        

  
  
  bool mAllowZoom;               

  
  nscoord mScriptUnconstrainedSize;
  nscoord mScriptMinSize;        
  float   mScriptSizeMultiplier; 
  nsCOMPtr<nsIAtom> mLanguage;   
};

struct nsStyleGradientStop {
  nsStyleCoord mLocation; 
  nscolor mColor;
  bool mIsInterpolationHint;

  
  bool operator==(const nsStyleGradientStop&) const = delete;
  bool operator!=(const nsStyleGradientStop&) const = delete;
};

class nsStyleGradient final {
public:
  nsStyleGradient();
  uint8_t mShape;  
  uint8_t mSize;   
                   
  bool mRepeating;
  bool mLegacySyntax;

  nsStyleCoord mBgPosX; 
  nsStyleCoord mBgPosY; 
  nsStyleCoord mAngle;  

  nsStyleCoord mRadiusX; 
  nsStyleCoord mRadiusY; 

  
  nsTArray<nsStyleGradientStop> mStops;

  bool operator==(const nsStyleGradient& aOther) const;
  bool operator!=(const nsStyleGradient& aOther) const {
    return !(*this == aOther);
  }

  bool IsOpaque();
  bool HasCalc();
  uint32_t Hash(PLDHashNumber aHash);

  NS_INLINE_DECL_REFCOUNTING(nsStyleGradient)

private:
  
  ~nsStyleGradient() {}

  nsStyleGradient(const nsStyleGradient& aOther) = delete;
  nsStyleGradient& operator=(const nsStyleGradient& aOther) = delete;
};

enum nsStyleImageType {
  eStyleImageType_Null,
  eStyleImageType_Image,
  eStyleImageType_Gradient,
  eStyleImageType_Element
};











struct nsStyleImage {
  nsStyleImage();
  ~nsStyleImage();
  nsStyleImage(const nsStyleImage& aOther);
  nsStyleImage& operator=(const nsStyleImage& aOther);

  void SetNull();
  void SetImageData(imgRequestProxy* aImage);
  void TrackImage(nsPresContext* aContext);
  void UntrackImage(nsPresContext* aContext);
  void SetGradientData(nsStyleGradient* aGradient);
  void SetElementId(const char16_t* aElementId);
  void SetCropRect(nsStyleSides* aCropRect);

  nsStyleImageType GetType() const {
    return mType;
  }
  imgRequestProxy* GetImageData() const {
    MOZ_ASSERT(mType == eStyleImageType_Image, "Data is not an image!");
    MOZ_ASSERT(mImageTracked,
               "Should be tracking any image we're going to use!");
    return mImage;
  }
  nsStyleGradient* GetGradientData() const {
    NS_ASSERTION(mType == eStyleImageType_Gradient, "Data is not a gradient!");
    return mGradient;
  }
  const char16_t* GetElementId() const {
    NS_ASSERTION(mType == eStyleImageType_Element, "Data is not an element!");
    return mElementId;
  }
  nsStyleSides* GetCropRect() const {
    NS_ASSERTION(mType == eStyleImageType_Image,
                 "Only image data can have a crop rect");
    return mCropRect;
  }

  








  bool ComputeActualCropRect(nsIntRect& aActualCropRect,
                               bool* aIsEntireImage = nullptr) const;

  


  nsresult StartDecoding() const;
  



  bool IsOpaque() const;
  




  bool IsComplete() const;
  




  bool IsLoaded() const;
  



  bool IsEmpty() const {
    
    
    
    
    
    
    
    return mType == eStyleImageType_Null;
  }

  bool operator==(const nsStyleImage& aOther) const;
  bool operator!=(const nsStyleImage& aOther) const {
    return !(*this == aOther);
  }

  bool ImageDataEquals(const nsStyleImage& aOther) const
  {
    return GetType() == eStyleImageType_Image &&
           aOther.GetType() == eStyleImageType_Image &&
           GetImageData() == aOther.GetImageData();
  }

  
  
  inline void SetSubImage(uint8_t aIndex, imgIContainer* aSubImage) const;
  inline imgIContainer* GetSubImage(uint8_t aIndex) const;

private:
  void DoCopy(const nsStyleImage& aOther);

  
  nsCOMArray<imgIContainer> mSubImages;

  nsStyleImageType mType;
  union {
    imgRequestProxy* mImage;
    nsStyleGradient* mGradient;
    char16_t* mElementId;
  };
  
  nsAutoPtr<nsStyleSides> mCropRect;
#ifdef DEBUG
  bool mImageTracked;
#endif
};

struct nsStyleColor {
  explicit nsStyleColor(nsPresContext* aPresContext);
  nsStyleColor(const nsStyleColor& aOther);
  ~nsStyleColor(void) {
    MOZ_COUNT_DTOR(nsStyleColor);
  }

  nsChangeHint CalcDifference(const nsStyleColor& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_VISUAL;
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return nsChangeHint(0);
  }

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleColor_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleColor();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleColor_id, this);
  }

  
  
  nscolor mColor;                 
};

struct nsStyleBackground {
  nsStyleBackground();
  nsStyleBackground(const nsStyleBackground& aOther);
  ~nsStyleBackground();

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleBackground_id, sz);
  }
  void Destroy(nsPresContext* aContext);

  nsChangeHint CalcDifference(const nsStyleBackground& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(nsChangeHint_UpdateEffects,
                          NS_CombineHint(NS_STYLE_HINT_VISUAL,
                                         nsChangeHint_NeutralChange));
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return nsChangeHint(0);
  }

  struct Position;
  friend struct Position;
  struct Position {
    typedef nsStyleCoord::CalcValue PositionCoord;
    PositionCoord mXPosition, mYPosition;

    
    Position() {}

    
    
    void SetInitialPercentValues(float aPercentVal);

    
    
    void SetInitialZeroValues();

    
    
    bool DependsOnPositioningAreaSize() const {
      return mXPosition.mPercent != 0.0f || mYPosition.mPercent != 0.0f;
    }

    bool operator==(const Position& aOther) const {
      return mXPosition == aOther.mXPosition &&
             mYPosition == aOther.mYPosition;
    }
    bool operator!=(const Position& aOther) const {
      return !(*this == aOther);
    }
  };

  struct Size;
  friend struct Size;
  struct Size {
    struct Dimension : public nsStyleCoord::CalcValue {
      nscoord ResolveLengthPercentage(nscoord aAvailable) const {
        double d = double(mPercent) * double(aAvailable) + double(mLength);
        if (d < 0.0)
          return 0;
        return NSToCoordRoundWithClamp(float(d));
      }
    };
    Dimension mWidth, mHeight;

    nscoord ResolveWidthLengthPercentage(const nsSize& aBgPositioningArea) const {
      MOZ_ASSERT(mWidthType == eLengthPercentage,
                 "resolving non-length/percent dimension!");
      return mWidth.ResolveLengthPercentage(aBgPositioningArea.width);
    }

    nscoord ResolveHeightLengthPercentage(const nsSize& aBgPositioningArea) const {
      MOZ_ASSERT(mHeightType == eLengthPercentage,
                 "resolving non-length/percent dimension!");
      return mHeight.ResolveLengthPercentage(aBgPositioningArea.height);
    }

    
    
    
    
    enum DimensionType {
      
      
      
      eContain, eCover,

      eAuto,
      eLengthPercentage,
      eDimensionType_COUNT
    };
    uint8_t mWidthType, mHeightType;

    
    
    
    bool DependsOnPositioningAreaSize(const nsStyleImage& aImage) const;

    
    Size() {}

    
    void SetInitialValues();

    bool operator==(const Size& aOther) const;
    bool operator!=(const Size& aOther) const {
      return !(*this == aOther);
    }
  };
  
  struct Repeat;
  friend struct Repeat;
  struct Repeat {
    uint8_t mXRepeat, mYRepeat;
    
    
    Repeat() {}

    
    void SetInitialValues();

    bool operator==(const Repeat& aOther) const {
      return mXRepeat == aOther.mXRepeat &&
             mYRepeat == aOther.mYRepeat;
    }
    bool operator!=(const Repeat& aOther) const {
      return !(*this == aOther);
    }
  };

  struct Layer;
  friend struct Layer;
  struct Layer {
    uint8_t mAttachment;                
    uint8_t mClip;                      
    uint8_t mOrigin;                    
    uint8_t mBlendMode;                 
    Repeat mRepeat;                     
    Position mPosition;                 
    nsStyleImage mImage;                
    Size mSize;                         

    
    Layer();
    ~Layer();

    
    
    void TrackImages(nsPresContext* aContext) {
      if (mImage.GetType() == eStyleImageType_Image)
        mImage.TrackImage(aContext);
    }
    void UntrackImages(nsPresContext* aContext) {
      if (mImage.GetType() == eStyleImageType_Image)
        mImage.UntrackImage(aContext);
    }

    void SetInitialValues();

    
    
    
    
    
    bool RenderingMightDependOnPositioningAreaSizeChange() const;

    
    
    bool operator==(const Layer& aOther) const;
    bool operator!=(const Layer& aOther) const {
      return !(*this == aOther);
    }
  };

  
  
  uint32_t mAttachmentCount,
           mClipCount,
           mOriginCount,
           mRepeatCount,
           mPositionCount,
           mImageCount,
           mSizeCount,
           mBlendModeCount;
  
  
  
  
  
  
  
  
  
  
  nsAutoTArray<Layer, 1> mLayers;

  const Layer& BottomLayer() const { return mLayers[mImageCount - 1]; }

  #define NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(var_, stylebg_) \
    for (uint32_t var_ = (stylebg_) ? (stylebg_)->mImageCount : 1; var_-- != 0; )
  #define NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT_WITH_RANGE(var_, stylebg_, start_, count_) \
    NS_ASSERTION((int32_t)(start_) >= 0 && (uint32_t)(start_) < ((stylebg_) ? (stylebg_)->mImageCount : 1), "Invalid layer start!"); \
    NS_ASSERTION((count_) > 0 && (count_) <= (start_) + 1, "Invalid layer range!"); \
    for (uint32_t var_ = (start_) + 1; var_-- != (uint32_t)((start_) + 1 - (count_)); )

  nscolor mBackgroundColor;       

  
  bool IsTransparent() const;

  
  
  
  
  bool HasFixedBackground() const;
};



#define BORDER_COLOR_FOREGROUND   0x20
#define OUTLINE_COLOR_INITIAL     0x80

#define BORDER_COLOR_SPECIAL      0xA0
#define BORDER_STYLE_MASK         0x1F

#define NS_SPACING_MARGIN   0
#define NS_SPACING_PADDING  1
#define NS_SPACING_BORDER   2


struct nsStyleMargin {
  nsStyleMargin(void);
  nsStyleMargin(const nsStyleMargin& aMargin);
  ~nsStyleMargin(void) {
    MOZ_COUNT_DTOR(nsStyleMargin);
  }

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleMargin_id, sz);
  }
  void Destroy(nsPresContext* aContext);

  void RecalcData();
  nsChangeHint CalcDifference(const nsStyleMargin& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return nsChangeHint(0);
  }

  nsStyleSides  mMargin;          

  bool IsWidthDependent() const { return !mHasCachedMargin; }
  bool GetMargin(nsMargin& aMargin) const
  {
    if (mHasCachedMargin) {
      aMargin = mCachedMargin;
      return true;
    }
    return false;
  }

protected:
  bool          mHasCachedMargin;
  nsMargin      mCachedMargin;
};


struct nsStylePadding {
  nsStylePadding(void);
  nsStylePadding(const nsStylePadding& aPadding);
  ~nsStylePadding(void) {
    MOZ_COUNT_DTOR(nsStylePadding);
  }

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStylePadding_id, sz);
  }
  void Destroy(nsPresContext* aContext);

  void RecalcData();
  nsChangeHint CalcDifference(const nsStylePadding& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_SubtractHint(NS_STYLE_HINT_REFLOW,
                           nsChangeHint_ClearDescendantIntrinsics);
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return nsChangeHint(0);
  }

  nsStyleSides  mPadding;         

  bool IsWidthDependent() const { return !mHasCachedPadding; }
  bool GetPadding(nsMargin& aPadding) const
  {
    if (mHasCachedPadding) {
      aPadding = mCachedPadding;
      return true;
    }
    return false;
  }

protected:
  bool          mHasCachedPadding;
  nsMargin      mCachedPadding;
};

struct nsBorderColors {
  nsBorderColors* mNext;
  nscolor mColor;

  nsBorderColors() : mNext(nullptr), mColor(NS_RGB(0,0,0)) {}
  explicit nsBorderColors(const nscolor& aColor) : mNext(nullptr), mColor(aColor) {}
  ~nsBorderColors();

  nsBorderColors* Clone() const { return Clone(true); }

  static bool Equal(const nsBorderColors* c1,
                      const nsBorderColors* c2) {
    if (c1 == c2)
      return true;
    while (c1 && c2) {
      if (c1->mColor != c2->mColor)
        return false;
      c1 = c1->mNext;
      c2 = c2->mNext;
    }
    
    
    return !c1 && !c2;
  }

private:
  nsBorderColors* Clone(bool aDeep) const;
};

struct nsCSSShadowItem {
  nscoord mXOffset;
  nscoord mYOffset;
  nscoord mRadius;
  nscoord mSpread;

  nscolor      mColor;
  bool mHasColor; 
  bool mInset;

  nsCSSShadowItem() : mHasColor(false) {
    MOZ_COUNT_CTOR(nsCSSShadowItem);
  }
  ~nsCSSShadowItem() {
    MOZ_COUNT_DTOR(nsCSSShadowItem);
  }

  bool operator==(const nsCSSShadowItem& aOther) const {
    return (mXOffset == aOther.mXOffset &&
            mYOffset == aOther.mYOffset &&
            mRadius == aOther.mRadius &&
            mHasColor == aOther.mHasColor &&
            mSpread == aOther.mSpread &&
            mInset == aOther.mInset &&
            (!mHasColor || mColor == aOther.mColor));
  }
  bool operator!=(const nsCSSShadowItem& aOther) const {
    return !(*this == aOther);
  }
};

class nsCSSShadowArray final {
  public:
    void* operator new(size_t aBaseSize, uint32_t aArrayLen) {
      
      
      
      
      
      return ::operator new(aBaseSize +
                            (aArrayLen - 1) * sizeof(nsCSSShadowItem));
    }

    explicit nsCSSShadowArray(uint32_t aArrayLen) :
      mLength(aArrayLen)
    {
      MOZ_COUNT_CTOR(nsCSSShadowArray);
      for (uint32_t i = 1; i < mLength; ++i) {
        
        
        new (&mArray[i]) nsCSSShadowItem();
      }
    }

private:
    
    ~nsCSSShadowArray() {
      MOZ_COUNT_DTOR(nsCSSShadowArray);
      for (uint32_t i = 1; i < mLength; ++i) {
        mArray[i].~nsCSSShadowItem();
      }
    }

public:
    uint32_t Length() const { return mLength; }
    nsCSSShadowItem* ShadowAt(uint32_t i) {
      MOZ_ASSERT(i < mLength, "Accessing too high an index in the text shadow array!");
      return &mArray[i];
    }
    const nsCSSShadowItem* ShadowAt(uint32_t i) const {
      MOZ_ASSERT(i < mLength, "Accessing too high an index in the text shadow array!");
      return &mArray[i];
    }

    bool HasShadowWithInset(bool aInset) {
      for (uint32_t i = 0; i < mLength; ++i) {
        if (mArray[i].mInset == aInset)
          return true;
      }
      return false;
    }

    bool operator==(const nsCSSShadowArray& aOther) const {
      if (mLength != aOther.Length())
        return false;

      for (uint32_t i = 0; i < mLength; ++i) {
        if (ShadowAt(i) != aOther.ShadowAt(i))
          return false;
      }

      return true;
    }

    NS_INLINE_DECL_REFCOUNTING(nsCSSShadowArray)

  private:
    uint32_t mLength;
    nsCSSShadowItem mArray[1]; 
};




#define NS_ROUND_BORDER_TO_PIXELS(l,tpp) \
  ((l) == 0) ? 0 : std::max((tpp), (l) / (tpp) * (tpp))



#define NS_ROUND_OFFSET_TO_PIXELS(l,tpp) \
  (((l) == 0) ? 0 : \
    ((l) > 0) ? std::max( (tpp), ((l) + ((tpp) / 2)) / (tpp) * (tpp)) : \
                std::min(-(tpp), ((l) - ((tpp) / 2)) / (tpp) * (tpp)))


static bool IsVisibleBorderStyle(uint8_t aStyle)
{
  return (aStyle != NS_STYLE_BORDER_STYLE_NONE &&
          aStyle != NS_STYLE_BORDER_STYLE_HIDDEN);
}

struct nsStyleBorder {
  explicit nsStyleBorder(nsPresContext* aContext);
  nsStyleBorder(const nsStyleBorder& aBorder);
  ~nsStyleBorder();

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleBorder_id, sz);
  }
  void Destroy(nsPresContext* aContext);

  nsChangeHint CalcDifference(const nsStyleBorder& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(NS_STYLE_HINT_REFLOW,
                          NS_CombineHint(nsChangeHint_BorderStyleNoneChange,
                                         nsChangeHint_NeutralChange));
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }

  void EnsureBorderColors() {
    if (!mBorderColors) {
      mBorderColors = new nsBorderColors*[4];
      if (mBorderColors)
        for (int32_t i = 0; i < 4; i++)
          mBorderColors[i] = nullptr;
    }
  }

  void ClearBorderColors(mozilla::css::Side aSide) {
    if (mBorderColors && mBorderColors[aSide]) {
      delete mBorderColors[aSide];
      mBorderColors[aSide] = nullptr;
    }
  }

  
  
  
  
  
  bool HasVisibleStyle(mozilla::css::Side aSide) const
  {
    return IsVisibleBorderStyle(GetBorderStyle(aSide));
  }

  
  void SetBorderWidth(mozilla::css::Side aSide, nscoord aBorderWidth)
  {
    nscoord roundedWidth =
      NS_ROUND_BORDER_TO_PIXELS(aBorderWidth, mTwipsPerPixel);
    mBorder.Side(aSide) = roundedWidth;
    if (HasVisibleStyle(aSide))
      mComputedBorder.Side(aSide) = roundedWidth;
  }

  
  
  
  const nsMargin& GetComputedBorder() const
  {
    return mComputedBorder;
  }

  bool HasBorder() const
  {
    return mComputedBorder != nsMargin(0,0,0,0) || !mBorderImageSource.IsEmpty();
  }

  
  
  
  
  nscoord GetComputedBorderWidth(mozilla::css::Side aSide) const
  {
    return GetComputedBorder().Side(aSide);
  }

  uint8_t GetBorderStyle(mozilla::css::Side aSide) const
  {
    NS_ASSERTION(aSide <= NS_SIDE_LEFT, "bad side");
    return (mBorderStyle[aSide] & BORDER_STYLE_MASK);
  }

  void SetBorderStyle(mozilla::css::Side aSide, uint8_t aStyle)
  {
    NS_ASSERTION(aSide <= NS_SIDE_LEFT, "bad side");
    mBorderStyle[aSide] &= ~BORDER_STYLE_MASK;
    mBorderStyle[aSide] |= (aStyle & BORDER_STYLE_MASK);
    mComputedBorder.Side(aSide) =
      (HasVisibleStyle(aSide) ? mBorder.Side(aSide) : 0);
  }

  inline bool IsBorderImageLoaded() const
  {
    return mBorderImageSource.IsLoaded();
  }

  
  inline nsresult RequestDecode();

  void GetBorderColor(mozilla::css::Side aSide, nscolor& aColor,
                      bool& aForeground) const
  {
    aForeground = false;
    NS_ASSERTION(aSide <= NS_SIDE_LEFT, "bad side");
    if ((mBorderStyle[aSide] & BORDER_COLOR_SPECIAL) == 0)
      aColor = mBorderColor[aSide];
    else if (mBorderStyle[aSide] & BORDER_COLOR_FOREGROUND)
      aForeground = true;
    else
      NS_NOTREACHED("OUTLINE_COLOR_INITIAL should not be set here");
  }

  void SetBorderColor(mozilla::css::Side aSide, nscolor aColor)
  {
    NS_ASSERTION(aSide <= NS_SIDE_LEFT, "bad side");
    mBorderColor[aSide] = aColor;
    mBorderStyle[aSide] &= ~BORDER_COLOR_SPECIAL;
  }

  void TrackImage(nsPresContext* aContext)
  {
    if (mBorderImageSource.GetType() == eStyleImageType_Image) {
      mBorderImageSource.TrackImage(aContext);
    }
  }
  void UntrackImage(nsPresContext* aContext)
  {
    if (mBorderImageSource.GetType() == eStyleImageType_Image) {
      mBorderImageSource.UntrackImage(aContext);
    }
  }

  nsMargin GetImageOutset() const;

  void GetCompositeColors(int32_t aIndex, nsBorderColors** aColors) const
  {
    if (!mBorderColors)
      *aColors = nullptr;
    else
      *aColors = mBorderColors[aIndex];
  }

  void AppendBorderColor(int32_t aIndex, nscolor aColor)
  {
    NS_ASSERTION(aIndex >= 0 && aIndex <= 3, "bad side for composite border color");
    nsBorderColors* colorEntry = new nsBorderColors(aColor);
    if (!mBorderColors[aIndex])
      mBorderColors[aIndex] = colorEntry;
    else {
      nsBorderColors* last = mBorderColors[aIndex];
      while (last->mNext)
        last = last->mNext;
      last->mNext = colorEntry;
    }
    mBorderStyle[aIndex] &= ~BORDER_COLOR_SPECIAL;
  }

  void SetBorderToForeground(mozilla::css::Side aSide)
  {
    NS_ASSERTION(aSide <= NS_SIDE_LEFT, "bad side");
    mBorderStyle[aSide] &= ~BORDER_COLOR_SPECIAL;
    mBorderStyle[aSide] |= BORDER_COLOR_FOREGROUND;
  }

  imgIRequest* GetBorderImageRequest() const
  {
    if (mBorderImageSource.GetType() == eStyleImageType_Image) {
      return mBorderImageSource.GetImageData();
    }
    return nullptr;
  }

public:
  nsBorderColors** mBorderColors;        
  nsRefPtr<nsCSSShadowArray> mBoxShadow; 

public:
  nsStyleCorners mBorderRadius;       
  nsStyleImage   mBorderImageSource;  
  nsStyleSides   mBorderImageSlice;   
  nsStyleSides   mBorderImageWidth;   
  nsStyleSides   mBorderImageOutset;  

  uint8_t        mBorderImageFill;    
  uint8_t        mBorderImageRepeatH; 
  uint8_t        mBorderImageRepeatV; 
  uint8_t        mFloatEdge;          
  uint8_t        mBoxDecorationBreak; 

protected:
  
  
  
  
  
  
  nsMargin      mComputedBorder;

  
  
  
  
  
  
  
  
  
  
  
  nsMargin      mBorder;

  uint8_t       mBorderStyle[4];  
  nscolor       mBorderColor[4];  
                                  
private:
  nscoord       mTwipsPerPixel;

  nsStyleBorder& operator=(const nsStyleBorder& aOther) = delete;
};


struct nsStyleOutline {
  explicit nsStyleOutline(nsPresContext* aPresContext);
  nsStyleOutline(const nsStyleOutline& aOutline);
  ~nsStyleOutline(void) {
    MOZ_COUNT_DTOR(nsStyleOutline);
  }

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleOutline_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleOutline();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleOutline_id, this);
  }

  void RecalcData(nsPresContext* aContext);
  nsChangeHint CalcDifference(const nsStyleOutline& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(NS_CombineHint(nsChangeHint_UpdateOverflow,
                                         nsChangeHint_SchedulePaint),
                          NS_CombineHint(nsChangeHint_RepaintFrame,
                                         nsChangeHint_NeutralChange));
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return nsChangeHint(0);
  }

  nsStyleCorners  mOutlineRadius; 

  
  
  nsStyleCoord  mOutlineWidth;    
  nscoord       mOutlineOffset;   

  bool GetOutlineWidth(nscoord& aWidth) const
  {
    if (mHasCachedOutline) {
      aWidth = mCachedOutlineWidth;
      return true;
    }
    return false;
  }

  uint8_t GetOutlineStyle(void) const
  {
    return (mOutlineStyle & BORDER_STYLE_MASK);
  }

  void SetOutlineStyle(uint8_t aStyle)
  {
    mOutlineStyle &= ~BORDER_STYLE_MASK;
    mOutlineStyle |= (aStyle & BORDER_STYLE_MASK);
  }

  
  bool GetOutlineColor(nscolor& aColor) const
  {
    if ((mOutlineStyle & BORDER_COLOR_SPECIAL) == 0) {
      aColor = mOutlineColor;
      return true;
    }
    return false;
  }

  void SetOutlineColor(nscolor aColor)
  {
    mOutlineColor = aColor;
    mOutlineStyle &= ~BORDER_COLOR_SPECIAL;
  }

  void SetOutlineInitialColor()
  {
    mOutlineStyle |= OUTLINE_COLOR_INITIAL;
  }

  bool GetOutlineInitialColor() const
  {
    return !!(mOutlineStyle & OUTLINE_COLOR_INITIAL);
  }

protected:
  
  
  nscoord       mCachedOutlineWidth;

  nscolor       mOutlineColor;    

  bool          mHasCachedOutline;
  uint8_t       mOutlineStyle;    

  nscoord       mTwipsPerPixel;
};


struct nsStyleList {
  explicit nsStyleList(nsPresContext* aPresContext);
  nsStyleList(const nsStyleList& aStyleList);
  ~nsStyleList(void);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleList_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleList();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleList_id, this);
  }

  nsChangeHint CalcDifference(const nsStyleList& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(NS_STYLE_HINT_FRAMECHANGE,
                          nsChangeHint_NeutralChange);
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }

  imgRequestProxy* GetListStyleImage() const { return mListStyleImage; }
  void SetListStyleImage(imgRequestProxy* aReq)
  {
    if (mListStyleImage)
      mListStyleImage->UnlockImage();
    mListStyleImage = aReq;
    if (mListStyleImage)
      mListStyleImage->LockImage();
  }

  void GetListStyleType(nsSubstring& aType) const { aType = mListStyleType; }
  mozilla::CounterStyle* GetCounterStyle() const
  {
    return mCounterStyle.get();
  }
  void SetListStyleType(const nsSubstring& aType,
                        mozilla::CounterStyle* aStyle)
  {
    mListStyleType = aType;
    mCounterStyle = aStyle;
  }
  void SetListStyleType(const nsSubstring& aType,
                        nsPresContext* aPresContext)
  {
    SetListStyleType(aType, aPresContext->
                     CounterStyleManager()->BuildCounterStyle(aType));
  }

  uint8_t   mListStylePosition;         
private:
  nsString  mListStyleType;             
  nsRefPtr<mozilla::CounterStyle> mCounterStyle; 
  nsRefPtr<imgRequestProxy> mListStyleImage; 
  nsStyleList& operator=(const nsStyleList& aOther) = delete;
public:
  nsRect        mImageRegion;           
};































struct nsStyleGridTemplate {
  bool mIsSubgrid;
  nsTArray<nsTArray<nsString>> mLineNameLists;
  nsTArray<nsStyleCoord> mMinTrackSizingFunctions;
  nsTArray<nsStyleCoord> mMaxTrackSizingFunctions;

  nsStyleGridTemplate()
    : mIsSubgrid(false)
  {
  }

  inline bool operator!=(const nsStyleGridTemplate& aOther) const {
    return mLineNameLists != aOther.mLineNameLists ||
           mMinTrackSizingFunctions != aOther.mMinTrackSizingFunctions ||
           mMaxTrackSizingFunctions != aOther.mMaxTrackSizingFunctions;
  }
};

struct nsStyleGridLine {
  
  bool mHasSpan;
  int32_t mInteger;  
  nsString mLineName;  

  nsStyleGridLine()
    : mHasSpan(false)
    , mInteger(0)
    
  {
  }

  nsStyleGridLine(const nsStyleGridLine& aOther)
  {
    (*this) = aOther;
  }

  void operator=(const nsStyleGridLine& aOther)
  {
    mHasSpan = aOther.mHasSpan;
    mInteger = aOther.mInteger;
    mLineName = aOther.mLineName;
  }

  bool operator!=(const nsStyleGridLine& aOther) const
  {
    return mHasSpan != aOther.mHasSpan ||
           mInteger != aOther.mInteger ||
           mLineName != aOther.mLineName;
  }

  void SetToInteger(uint32_t value)
  {
    mHasSpan = false;
    mInteger = value;
    mLineName.Truncate();
  }

  void SetAuto()
  {
    mHasSpan = false;
    mInteger = 0;
    mLineName.Truncate();
  }

  bool IsAuto() const
  {
    bool haveInitialValues =  mInteger == 0 && mLineName.IsEmpty();
    MOZ_ASSERT(!(haveInitialValues && mHasSpan),
               "should not have 'span' when other components are "
               "at their initial values");
    return haveInitialValues;
  }
};

struct nsStylePosition {
  nsStylePosition(void);
  nsStylePosition(const nsStylePosition& aOther);
  ~nsStylePosition(void);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStylePosition_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStylePosition();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStylePosition_id, this);
  }

  nsChangeHint CalcDifference(const nsStylePosition& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(NS_STYLE_HINT_REFLOW,
                          nsChangeHint(nsChangeHint_RecomputePosition |
                                       nsChangeHint_UpdateParentOverflow));
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return nsChangeHint(0);
  }

  
  
  typedef nsStyleBackground::Position Position;

  Position      mObjectPosition;        
  nsStyleSides  mOffset;                
  nsStyleCoord  mWidth;                 
  nsStyleCoord  mMinWidth;              
  nsStyleCoord  mMaxWidth;              
  nsStyleCoord  mHeight;                
  nsStyleCoord  mMinHeight;             
  nsStyleCoord  mMaxHeight;             
  nsStyleCoord  mFlexBasis;             
  nsStyleCoord  mGridAutoColumnsMin;    
  nsStyleCoord  mGridAutoColumnsMax;    
  nsStyleCoord  mGridAutoRowsMin;       
  nsStyleCoord  mGridAutoRowsMax;       
  uint8_t       mGridAutoFlow;          
  uint8_t       mBoxSizing;             
  uint8_t       mAlignContent;          
  uint8_t       mAlignItems;            
  uint8_t       mAlignSelf;             
  uint8_t       mFlexDirection;         
  uint8_t       mFlexWrap;              
  uint8_t       mJustifyContent;        
  uint8_t       mObjectFit;             
  int32_t       mOrder;                 
  float         mFlexGrow;              
  float         mFlexShrink;            
  nsStyleCoord  mZIndex;                
  nsStyleGridTemplate mGridTemplateColumns;
  nsStyleGridTemplate mGridTemplateRows;

  
  nsRefPtr<mozilla::css::GridTemplateAreasValue> mGridTemplateAreas;

  nsStyleGridLine mGridColumnStart;
  nsStyleGridLine mGridColumnEnd;
  nsStyleGridLine mGridRowStart;
  nsStyleGridLine mGridRowEnd;

  bool WidthDependsOnContainer() const
    {
      return mWidth.GetUnit() == eStyleUnit_Auto ||
        WidthCoordDependsOnContainer(mWidth);
    }

  
  
  
  
  
  
  
  
  bool MinWidthDependsOnContainer() const
    { return WidthCoordDependsOnContainer(mMinWidth); }
  bool MaxWidthDependsOnContainer() const
    { return WidthCoordDependsOnContainer(mMaxWidth); }

  
  
  
  
  
  
  bool HeightDependsOnContainer() const
    {
      return mHeight.GetUnit() == eStyleUnit_Auto || 
        HeightCoordDependsOnContainer(mHeight);
    }

  
  
  bool MinHeightDependsOnContainer() const
    { return HeightCoordDependsOnContainer(mMinHeight); }
  bool MaxHeightDependsOnContainer() const
    { return HeightCoordDependsOnContainer(mMaxHeight); }

  bool OffsetHasPercent(mozilla::css::Side aSide) const
  {
    return mOffset.Get(aSide).HasPercent();
  }

private:
  static bool WidthCoordDependsOnContainer(const nsStyleCoord &aCoord);
  static bool HeightCoordDependsOnContainer(const nsStyleCoord &aCoord)
    { return aCoord.HasPercent(); }
};

struct nsStyleTextOverflowSide {
  nsStyleTextOverflowSide() : mType(NS_STYLE_TEXT_OVERFLOW_CLIP) {}

  bool operator==(const nsStyleTextOverflowSide& aOther) const {
    return mType == aOther.mType &&
           (mType != NS_STYLE_TEXT_OVERFLOW_STRING ||
            mString == aOther.mString);
  }
  bool operator!=(const nsStyleTextOverflowSide& aOther) const {
    return !(*this == aOther);
  }

  nsString mString;
  uint8_t  mType;
};

struct nsStyleTextOverflow {
  nsStyleTextOverflow() : mLogicalDirections(true) {}
  bool operator==(const nsStyleTextOverflow& aOther) const {
    return mLeft == aOther.mLeft && mRight == aOther.mRight;
  }
  bool operator!=(const nsStyleTextOverflow& aOther) const {
    return !(*this == aOther);
  }

  
  const nsStyleTextOverflowSide& GetLeft(uint8_t aDirection) const {
    NS_ASSERTION(aDirection == NS_STYLE_DIRECTION_LTR ||
                 aDirection == NS_STYLE_DIRECTION_RTL, "bad direction");
    return !mLogicalDirections || aDirection == NS_STYLE_DIRECTION_LTR ?
             mLeft : mRight;
  }

  
  const nsStyleTextOverflowSide& GetRight(uint8_t aDirection) const {
    NS_ASSERTION(aDirection == NS_STYLE_DIRECTION_LTR ||
                 aDirection == NS_STYLE_DIRECTION_RTL, "bad direction");
    return !mLogicalDirections || aDirection == NS_STYLE_DIRECTION_LTR ?
             mRight : mLeft;
  }

  
  const nsStyleTextOverflowSide* GetFirstValue() const {
    return mLogicalDirections ? &mRight : &mLeft;
  }

  
  const nsStyleTextOverflowSide* GetSecondValue() const {
    return mLogicalDirections ? nullptr : &mRight;
  }

  nsStyleTextOverflowSide mLeft;  
  nsStyleTextOverflowSide mRight; 
  bool mLogicalDirections;  
};

struct nsStyleTextReset {
  nsStyleTextReset(void);
  nsStyleTextReset(const nsStyleTextReset& aOther);
  ~nsStyleTextReset(void);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleTextReset_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleTextReset();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleTextReset_id, this);
  }

  uint8_t GetDecorationStyle() const
  {
    return (mTextDecorationStyle & BORDER_STYLE_MASK);
  }

  void SetDecorationStyle(uint8_t aStyle)
  {
    MOZ_ASSERT((aStyle & BORDER_STYLE_MASK) == aStyle,
               "style doesn't fit");
    mTextDecorationStyle &= ~BORDER_STYLE_MASK;
    mTextDecorationStyle |= (aStyle & BORDER_STYLE_MASK);
  }

  void GetDecorationColor(nscolor& aColor, bool& aForeground) const
  {
    aForeground = false;
    if ((mTextDecorationStyle & BORDER_COLOR_SPECIAL) == 0) {
      aColor = mTextDecorationColor;
    } else if (mTextDecorationStyle & BORDER_COLOR_FOREGROUND) {
      aForeground = true;
    } else {
      NS_NOTREACHED("OUTLINE_COLOR_INITIAL should not be set here");
    }
  }

  void SetDecorationColor(nscolor aColor)
  {
    mTextDecorationColor = aColor;
    mTextDecorationStyle &= ~BORDER_COLOR_SPECIAL;
  }

  void SetDecorationColorToForeground()
  {
    mTextDecorationStyle &= ~BORDER_COLOR_SPECIAL;
    mTextDecorationStyle |= BORDER_COLOR_FOREGROUND;
  }

  nsChangeHint CalcDifference(const nsStyleTextReset& aOther) const;
  static nsChangeHint MaxDifference() {
    return nsChangeHint(
        NS_STYLE_HINT_REFLOW | 
        nsChangeHint_UpdateSubtreeOverflow);
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }

  nsStyleCoord  mVerticalAlign;         
  nsStyleTextOverflow mTextOverflow;    

  uint8_t mTextDecorationLine;          
  uint8_t mUnicodeBidi;                 
protected:
  uint8_t mTextDecorationStyle;         

  nscolor mTextDecorationColor;         
};

struct nsStyleText {
  nsStyleText(void);
  nsStyleText(const nsStyleText& aOther);
  ~nsStyleText(void);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleText_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleText();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleText_id, this);
  }

  nsChangeHint CalcDifference(const nsStyleText& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }

  uint8_t mTextAlign;                   
  uint8_t mTextAlignLast;               
  bool mTextAlignTrue : 1;              
  bool mTextAlignLastTrue : 1;          
  uint8_t mTextTransform;               
  uint8_t mWhiteSpace;                  
  uint8_t mWordBreak;                   
  uint8_t mWordWrap;                    
  uint8_t mHyphens;                     
  uint8_t mRubyAlign;                   
  uint8_t mRubyPosition;                
  uint8_t mTextSizeAdjust;              
  uint8_t mTextCombineUpright;          
  uint8_t mControlCharacterVisibility;  
  int32_t mTabSize;                     

  nscoord mWordSpacing;                 
  nsStyleCoord  mLetterSpacing;         
  nsStyleCoord  mLineHeight;            
  nsStyleCoord  mTextIndent;            

  nsRefPtr<nsCSSShadowArray> mTextShadow; 

  bool WhiteSpaceIsSignificant() const {
    return mWhiteSpace == NS_STYLE_WHITESPACE_PRE ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_WRAP ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_SPACE;
  }

  bool NewlineIsSignificantStyle() const {
    return mWhiteSpace == NS_STYLE_WHITESPACE_PRE ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_WRAP ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_LINE;
  }

  bool WhiteSpaceOrNewlineIsSignificant() const {
    return mWhiteSpace == NS_STYLE_WHITESPACE_PRE ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_WRAP ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_LINE ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_SPACE;
  }

  bool TabIsSignificant() const {
    return mWhiteSpace == NS_STYLE_WHITESPACE_PRE ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_WRAP;
  }

  bool WhiteSpaceCanWrapStyle() const {
    return mWhiteSpace == NS_STYLE_WHITESPACE_NORMAL ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_WRAP ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_LINE;
  }

  bool WordCanWrapStyle() const {
    return WhiteSpaceCanWrapStyle() &&
           mWordWrap == NS_STYLE_WORDWRAP_BREAK_WORD;
  }

  
  inline bool HasTextShadow() const;
  inline nsCSSShadowArray* GetTextShadow() const;

  
  
  
  
  inline bool NewlineIsSignificant(const nsIFrame* aContextFrame) const;
  inline bool WhiteSpaceCanWrap(const nsIFrame* aContextFrame) const;
  inline bool WordCanWrap(const nsIFrame* aContextFrame) const;
};

struct nsStyleImageOrientation {
  static nsStyleImageOrientation CreateAsAngleAndFlip(double aRadians,
                                                      bool aFlip) {
    uint8_t orientation(0);

    
    double roundedAngle = fmod(aRadians, 2 * M_PI);
    if      (roundedAngle < 0.25 * M_PI) orientation = ANGLE_0;
    else if (roundedAngle < 0.75 * M_PI) orientation = ANGLE_90;
    else if (roundedAngle < 1.25 * M_PI) orientation = ANGLE_180;
    else if (roundedAngle < 1.75 * M_PI) orientation = ANGLE_270;
    else                                 orientation = ANGLE_0;

    
    if (aFlip)
      orientation |= FLIP_MASK;

    return nsStyleImageOrientation(orientation);
  }

  static nsStyleImageOrientation CreateAsFlip() {
    return nsStyleImageOrientation(FLIP_MASK);
  }

  static nsStyleImageOrientation CreateAsFromImage() {
    return nsStyleImageOrientation(FROM_IMAGE_MASK);
  }

  
  nsStyleImageOrientation() : mOrientation(0) { }

  bool IsDefault()   const { return mOrientation == 0; }
  bool IsFlipped()   const { return mOrientation & FLIP_MASK; }
  bool IsFromImage() const { return mOrientation & FROM_IMAGE_MASK; }
  bool SwapsWidthAndHeight() const {
    uint8_t angle = mOrientation & ORIENTATION_MASK;
    return (angle == ANGLE_90) || (angle == ANGLE_270);
  }

  mozilla::image::Angle Angle() const {
    switch (mOrientation & ORIENTATION_MASK) {
      case ANGLE_0:   return mozilla::image::Angle::D0;
      case ANGLE_90:  return mozilla::image::Angle::D90;
      case ANGLE_180: return mozilla::image::Angle::D180;
      case ANGLE_270: return mozilla::image::Angle::D270;
      default:
        NS_NOTREACHED("Unexpected angle");
        return mozilla::image::Angle::D0;
    }
  }

  nsStyleCoord AngleAsCoord() const {
    switch (mOrientation & ORIENTATION_MASK) {
      case ANGLE_0:   return nsStyleCoord(0.0f,   eStyleUnit_Degree);
      case ANGLE_90:  return nsStyleCoord(90.0f,  eStyleUnit_Degree);
      case ANGLE_180: return nsStyleCoord(180.0f, eStyleUnit_Degree);
      case ANGLE_270: return nsStyleCoord(270.0f, eStyleUnit_Degree);
      default:
        NS_NOTREACHED("Unexpected angle");
        return nsStyleCoord();
    }
  }

  bool operator==(const nsStyleImageOrientation& aOther) const {
    return aOther.mOrientation == mOrientation;
  }

  bool operator!=(const nsStyleImageOrientation& aOther) const {
    return !(*this == aOther);
  }

protected:
  enum Bits {
    ORIENTATION_MASK = 0x1 | 0x2,  
    FLIP_MASK        = 0x4,        
    FROM_IMAGE_MASK  = 0x8,        
  };                               

  enum Angles {
    ANGLE_0   = 0,
    ANGLE_90  = 1,
    ANGLE_180 = 2,
    ANGLE_270 = 3,
  };

  explicit nsStyleImageOrientation(uint8_t aOrientation)
    : mOrientation(aOrientation)
  { }

  uint8_t mOrientation;
};

struct nsStyleVisibility {
  explicit nsStyleVisibility(nsPresContext* aPresContext);
  nsStyleVisibility(const nsStyleVisibility& aVisibility);
  ~nsStyleVisibility() {
    MOZ_COUNT_DTOR(nsStyleVisibility);
  }

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleVisibility_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleVisibility();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleVisibility_id, this);
  }

  nsChangeHint CalcDifference(const nsStyleVisibility& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }

  nsStyleImageOrientation mImageOrientation;  
  uint8_t mDirection;                  
  uint8_t mVisible;                    
  uint8_t mPointerEvents;              
  uint8_t mWritingMode;                
  uint8_t mTextOrientation;            

  bool IsVisible() const {
    return (mVisible == NS_STYLE_VISIBILITY_VISIBLE);
  }

  bool IsVisibleOrCollapsed() const {
    return ((mVisible == NS_STYLE_VISIBILITY_VISIBLE) ||
            (mVisible == NS_STYLE_VISIBILITY_COLLAPSE));
  }

  inline uint8_t GetEffectivePointerEvents(nsIFrame* aFrame) const;
};

struct nsTimingFunction {
  enum Type { Function, StepStart, StepEnd };

  explicit nsTimingFunction(int32_t aTimingFunctionType
                              = NS_STYLE_TRANSITION_TIMING_FUNCTION_EASE)
  {
    AssignFromKeyword(aTimingFunctionType);
  }

  nsTimingFunction(float x1, float y1, float x2, float y2)
    : mType(Function)
  {
    mFunc.mX1 = x1;
    mFunc.mY1 = y1;
    mFunc.mX2 = x2;
    mFunc.mY2 = y2;
  }

  nsTimingFunction(Type aType, uint32_t aSteps)
    : mType(aType)
  {
    MOZ_ASSERT(mType == StepStart || mType == StepEnd, "wrong type");
    mSteps = aSteps;
  }

  nsTimingFunction(const nsTimingFunction& aOther)
  {
    *this = aOther;
  }

  Type mType;
  union {
    struct {
      float mX1;
      float mY1;
      float mX2;
      float mY2;
    } mFunc;
    uint32_t mSteps;
  };

  nsTimingFunction&
  operator=(const nsTimingFunction& aOther)
  {
    if (&aOther == this)
      return *this;

    mType = aOther.mType;

    if (mType == Function) {
      mFunc.mX1 = aOther.mFunc.mX1;
      mFunc.mY1 = aOther.mFunc.mY1;
      mFunc.mX2 = aOther.mFunc.mX2;
      mFunc.mY2 = aOther.mFunc.mY2;
    } else {
      mSteps = aOther.mSteps;
    }

    return *this;
  }

  bool operator==(const nsTimingFunction& aOther) const
  {
    if (mType != aOther.mType) {
      return false;
    }
    if (mType == Function) {
      return mFunc.mX1 == aOther.mFunc.mX1 && mFunc.mY1 == aOther.mFunc.mY1 &&
             mFunc.mX2 == aOther.mFunc.mX2 && mFunc.mY2 == aOther.mFunc.mY2;
    }
    return mSteps == aOther.mSteps;
  }

  bool operator!=(const nsTimingFunction& aOther) const
  {
    return !(*this == aOther);
  }

private:
  void AssignFromKeyword(int32_t aTimingFunctionType);
};

namespace mozilla {

struct StyleTransition {
  StyleTransition() {  }
  explicit StyleTransition(const StyleTransition& aCopy);

  void SetInitialValues();

  

  const nsTimingFunction& GetTimingFunction() const { return mTimingFunction; }
  float GetDelay() const { return mDelay; }
  float GetDuration() const { return mDuration; }
  nsCSSProperty GetProperty() const { return mProperty; }
  nsIAtom* GetUnknownProperty() const { return mUnknownProperty; }

  float GetCombinedDuration() const {
    
    return std::max(mDuration, 0.0f) + mDelay;
  }

  void SetTimingFunction(const nsTimingFunction& aTimingFunction)
    { mTimingFunction = aTimingFunction; }
  void SetDelay(float aDelay) { mDelay = aDelay; }
  void SetDuration(float aDuration) { mDuration = aDuration; }
  void SetProperty(nsCSSProperty aProperty)
    {
      NS_ASSERTION(aProperty != eCSSProperty_UNKNOWN, "invalid property");
      mProperty = aProperty;
    }
  void SetUnknownProperty(const nsAString& aUnknownProperty);
  void CopyPropertyFrom(const StyleTransition& aOther)
    {
      mProperty = aOther.mProperty;
      mUnknownProperty = aOther.mUnknownProperty;
    }

  nsTimingFunction& TimingFunctionSlot() { return mTimingFunction; }

  bool operator==(const StyleTransition& aOther) const;
  bool operator!=(const StyleTransition& aOther) const
    { return !(*this == aOther); }

private:
  nsTimingFunction mTimingFunction;
  float mDuration;
  float mDelay;
  nsCSSProperty mProperty;
  nsCOMPtr<nsIAtom> mUnknownProperty; 
                                      
};

struct StyleAnimation {
  StyleAnimation() {  }
  explicit StyleAnimation(const StyleAnimation& aCopy);

  void SetInitialValues();

  

  const nsTimingFunction& GetTimingFunction() const { return mTimingFunction; }
  float GetDelay() const { return mDelay; }
  float GetDuration() const { return mDuration; }
  const nsString& GetName() const { return mName; }
  uint8_t GetDirection() const { return mDirection; }
  uint8_t GetFillMode() const { return mFillMode; }
  uint8_t GetPlayState() const { return mPlayState; }
  float GetIterationCount() const { return mIterationCount; }

  void SetTimingFunction(const nsTimingFunction& aTimingFunction)
    { mTimingFunction = aTimingFunction; }
  void SetDelay(float aDelay) { mDelay = aDelay; }
  void SetDuration(float aDuration) { mDuration = aDuration; }
  void SetName(const nsSubstring& aName) { mName = aName; }
  void SetDirection(uint8_t aDirection) { mDirection = aDirection; }
  void SetFillMode(uint8_t aFillMode) { mFillMode = aFillMode; }
  void SetPlayState(uint8_t aPlayState) { mPlayState = aPlayState; }
  void SetIterationCount(float aIterationCount)
    { mIterationCount = aIterationCount; }

  nsTimingFunction& TimingFunctionSlot() { return mTimingFunction; }

  bool operator==(const StyleAnimation& aOther) const;
  bool operator!=(const StyleAnimation& aOther) const
    { return !(*this == aOther); }

private:
  nsTimingFunction mTimingFunction;
  float mDuration;
  float mDelay;
  nsString mName; 
  uint8_t mDirection;
  uint8_t mFillMode;
  uint8_t mPlayState;
  float mIterationCount; 
};

} 

struct nsStyleDisplay {
  nsStyleDisplay();
  nsStyleDisplay(const nsStyleDisplay& aOther);
  ~nsStyleDisplay() {
    MOZ_COUNT_DTOR(nsStyleDisplay);
  }

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleDisplay_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleDisplay();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleDisplay_id, this);
  }

  nsChangeHint CalcDifference(const nsStyleDisplay& aOther) const;
  static nsChangeHint MaxDifference() {
    
    return nsChangeHint(NS_STYLE_HINT_FRAMECHANGE |
                        nsChangeHint_UpdateOpacityLayer |
                        nsChangeHint_UpdateTransformLayer |
                        nsChangeHint_UpdateOverflow |
                        nsChangeHint_UpdatePostTransformOverflow |
                        nsChangeHint_AddOrRemoveTransform |
                        nsChangeHint_NeutralChange);
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return nsChangeHint(0);
  }

  
  
  
  typedef nsStyleBackground::Position Position;

  
  
  nsRefPtr<mozilla::css::URLValue> mBinding;    
  nsRect  mClip;                
  float   mOpacity;             
  uint8_t mDisplay;             
  uint8_t mOriginalDisplay;     
                                
                                
  uint8_t mAppearance;          
  uint8_t mPosition;            
  uint8_t mFloats;              
  uint8_t mOriginalFloats;      
                                
  uint8_t mBreakType;           
  uint8_t mBreakInside;         
  bool mBreakBefore;    
  bool mBreakAfter;     
  uint8_t mOverflowX;           
  uint8_t mOverflowY;           
  uint8_t mOverflowClipBox;     
  uint8_t mResize;              
  uint8_t mClipFlags;           
  uint8_t mOrient;              
  uint8_t mMixBlendMode;        
  uint8_t mIsolation;           
  uint8_t mWillChangeBitField;  
                                
                                
                                
                                
                                
  nsAutoTArray<nsString, 1> mWillChange;

  uint8_t mTouchAction;         
  uint8_t mScrollBehavior;      
  uint8_t mScrollSnapTypeX;     
  uint8_t mScrollSnapTypeY;     
  nsStyleCoord mScrollSnapPointsX; 
  nsStyleCoord mScrollSnapPointsY; 
  Position mScrollSnapDestination; 
  nsTArray<Position> mScrollSnapCoordinate; 

  
  
  
  
  uint8_t mBackfaceVisibility;
  uint8_t mTransformStyle;
  nsRefPtr<nsCSSValueSharedList> mSpecifiedTransform; 
  nsStyleCoord mTransformOrigin[3]; 
  nsStyleCoord mChildPerspective; 
  nsStyleCoord mPerspectiveOrigin[2]; 

  nsAutoTArray<mozilla::StyleTransition, 1> mTransitions; 
  
  
  uint32_t mTransitionTimingFunctionCount,
           mTransitionDurationCount,
           mTransitionDelayCount,
           mTransitionPropertyCount;

  nsAutoTArray<mozilla::StyleAnimation, 1> mAnimations; 
  
  
  uint32_t mAnimationTimingFunctionCount,
           mAnimationDurationCount,
           mAnimationDelayCount,
           mAnimationNameCount,
           mAnimationDirectionCount,
           mAnimationFillModeCount,
           mAnimationPlayStateCount,
           mAnimationIterationCountCount;

  bool IsBlockInsideStyle() const {
    return NS_STYLE_DISPLAY_BLOCK == mDisplay ||
           NS_STYLE_DISPLAY_LIST_ITEM == mDisplay ||
           NS_STYLE_DISPLAY_INLINE_BLOCK == mDisplay ||
           NS_STYLE_DISPLAY_TABLE_CAPTION == mDisplay;
    
    
    
  }

  bool IsBlockOutsideStyle() const {
    return NS_STYLE_DISPLAY_BLOCK == mDisplay ||
           NS_STYLE_DISPLAY_FLEX == mDisplay ||
           NS_STYLE_DISPLAY_GRID == mDisplay ||
           NS_STYLE_DISPLAY_LIST_ITEM == mDisplay ||
           NS_STYLE_DISPLAY_TABLE == mDisplay;
  }

  static bool IsDisplayTypeInlineOutside(uint8_t aDisplay) {
    return NS_STYLE_DISPLAY_INLINE == aDisplay ||
           NS_STYLE_DISPLAY_INLINE_BLOCK == aDisplay ||
           NS_STYLE_DISPLAY_INLINE_TABLE == aDisplay ||
           NS_STYLE_DISPLAY_INLINE_BOX == aDisplay ||
           NS_STYLE_DISPLAY_INLINE_FLEX == aDisplay ||
           NS_STYLE_DISPLAY_INLINE_GRID == aDisplay ||
           NS_STYLE_DISPLAY_INLINE_XUL_GRID == aDisplay ||
           NS_STYLE_DISPLAY_INLINE_STACK == aDisplay ||
           NS_STYLE_DISPLAY_RUBY == aDisplay ||
           NS_STYLE_DISPLAY_RUBY_BASE == aDisplay ||
           NS_STYLE_DISPLAY_RUBY_BASE_CONTAINER == aDisplay ||
           NS_STYLE_DISPLAY_RUBY_TEXT == aDisplay ||
           NS_STYLE_DISPLAY_RUBY_TEXT_CONTAINER == aDisplay ||
           NS_STYLE_DISPLAY_CONTENTS == aDisplay;
  }

  bool IsInlineOutsideStyle() const {
    return IsDisplayTypeInlineOutside(mDisplay);
  }

  bool IsOriginalDisplayInlineOutsideStyle() const {
    return IsDisplayTypeInlineOutside(mOriginalDisplay);
  }

  bool IsInnerTableStyle() const {
    return NS_STYLE_DISPLAY_TABLE_CAPTION == mDisplay ||
           NS_STYLE_DISPLAY_TABLE_CELL == mDisplay ||
           NS_STYLE_DISPLAY_TABLE_ROW == mDisplay ||
           NS_STYLE_DISPLAY_TABLE_ROW_GROUP == mDisplay ||
           NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == mDisplay ||
           NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == mDisplay ||
           NS_STYLE_DISPLAY_TABLE_COLUMN == mDisplay ||
           NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == mDisplay;
  }

  bool IsFloatingStyle() const {
    return NS_STYLE_FLOAT_NONE != mFloats;
  }

  bool IsAbsolutelyPositionedStyle() const {
    return NS_STYLE_POSITION_ABSOLUTE == mPosition ||
           NS_STYLE_POSITION_FIXED == mPosition;
  }

  bool IsRelativelyPositionedStyle() const {
    return NS_STYLE_POSITION_RELATIVE == mPosition ||
           NS_STYLE_POSITION_STICKY == mPosition;
  }

  static bool IsRubyDisplayType(uint8_t aDisplay) {
    return NS_STYLE_DISPLAY_RUBY == aDisplay ||
           NS_STYLE_DISPLAY_RUBY_BASE == aDisplay ||
           NS_STYLE_DISPLAY_RUBY_BASE_CONTAINER == aDisplay ||
           NS_STYLE_DISPLAY_RUBY_TEXT == aDisplay ||
           NS_STYLE_DISPLAY_RUBY_TEXT_CONTAINER == aDisplay;
  }

  bool IsRubyDisplayType() const {
    return IsRubyDisplayType(mDisplay);
  }

  bool IsFlexOrGridDisplayType() const {
    return NS_STYLE_DISPLAY_FLEX == mDisplay ||
           NS_STYLE_DISPLAY_INLINE_FLEX == mDisplay ||
           NS_STYLE_DISPLAY_GRID == mDisplay ||
           NS_STYLE_DISPLAY_INLINE_GRID == mDisplay;
  }

  bool IsOutOfFlowStyle() const {
    return (IsAbsolutelyPositionedStyle() || IsFloatingStyle());
  }

  bool IsScrollableOverflow() const {
    
    
    return mOverflowX != NS_STYLE_OVERFLOW_VISIBLE &&
           mOverflowX != NS_STYLE_OVERFLOW_CLIP;
  }

  

  bool HasTransformStyle() const {
    return mSpecifiedTransform != nullptr ||
           mTransformStyle == NS_STYLE_TRANSFORM_STYLE_PRESERVE_3D ||
           (mWillChangeBitField & NS_STYLE_WILL_CHANGE_TRANSFORM);
  }

  bool HasPerspectiveStyle() const {
    return mChildPerspective.GetUnit() == eStyleUnit_Coord;
  }

  bool BackfaceIsHidden() const {
    return mBackfaceVisibility == NS_STYLE_BACKFACE_VISIBILITY_HIDDEN;
  }

  

  
  
  
  
  inline bool IsBlockInside(const nsIFrame* aContextFrame) const;
  inline bool IsBlockOutside(const nsIFrame* aContextFrame) const;
  inline bool IsInlineOutside(const nsIFrame* aContextFrame) const;
  inline bool IsOriginalDisplayInlineOutside(const nsIFrame* aContextFrame) const;
  inline uint8_t GetDisplay(const nsIFrame* aContextFrame) const;
  inline bool IsFloating(const nsIFrame* aContextFrame) const;
  inline bool IsAbsPosContainingBlock(const nsIFrame* aContextFrame) const;
  inline bool IsRelativelyPositioned(const nsIFrame* aContextFrame) const;
  inline bool IsAbsolutelyPositioned(const nsIFrame* aContextFrame) const;

  

  




  inline bool HasTransform(const nsIFrame* aContextFrame) const;

  




  inline bool IsFixedPosContainingBlock(const nsIFrame* aContextFrame) const;
};

struct nsStyleTable {
  nsStyleTable(void);
  nsStyleTable(const nsStyleTable& aOther);
  ~nsStyleTable(void);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleTable_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleTable();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleTable_id, this);
  }

  nsChangeHint CalcDifference(const nsStyleTable& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }

  uint8_t       mLayoutStrategy;
  int32_t       mSpan;          
};

struct nsStyleTableBorder {
  nsStyleTableBorder();
  nsStyleTableBorder(const nsStyleTableBorder& aOther);
  ~nsStyleTableBorder(void);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleTableBorder_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleTableBorder();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleTableBorder_id, this);
  }

  nsChangeHint CalcDifference(const nsStyleTableBorder& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }

  nscoord       mBorderSpacingCol;
  nscoord       mBorderSpacingRow;
  uint8_t       mBorderCollapse;
  uint8_t       mCaptionSide;   
  uint8_t       mEmptyCells;    
};

enum nsStyleContentType {
  eStyleContentType_String        = 1,
  eStyleContentType_Image         = 10,
  eStyleContentType_Attr          = 20,
  eStyleContentType_Counter       = 30,
  eStyleContentType_Counters      = 31,
  eStyleContentType_OpenQuote     = 40,
  eStyleContentType_CloseQuote    = 41,
  eStyleContentType_NoOpenQuote   = 42,
  eStyleContentType_NoCloseQuote  = 43,
  eStyleContentType_AltContent    = 50,
  eStyleContentType_Uninitialized
};

struct nsStyleContentData {
  nsStyleContentType  mType;
  union {
    char16_t *mString;
    imgRequestProxy *mImage;
    nsCSSValue::Array* mCounters;
  } mContent;
#ifdef DEBUG
  bool mImageTracked;
#endif

  nsStyleContentData()
    : mType(eStyleContentType_Uninitialized)
#ifdef DEBUG
    , mImageTracked(false)
#endif
  { mContent.mString = nullptr; }

  ~nsStyleContentData();
  nsStyleContentData& operator=(const nsStyleContentData& aOther);
  bool operator==(const nsStyleContentData& aOther) const;

  bool operator!=(const nsStyleContentData& aOther) const {
    return !(*this == aOther);
  }

  void TrackImage(nsPresContext* aContext);
  void UntrackImage(nsPresContext* aContext);

  void SetImage(imgRequestProxy* aRequest)
  {
    MOZ_ASSERT(!mImageTracked,
               "Setting a new image without untracking the old one!");
    MOZ_ASSERT(mType == eStyleContentType_Image, "Wrong type!");
    NS_IF_ADDREF(mContent.mImage = aRequest);
  }
private:
  nsStyleContentData(const nsStyleContentData&); 
};

struct nsStyleCounterData {
  nsString  mCounter;
  int32_t   mValue;
};


#define DELETE_ARRAY_IF(array)  if (array) { delete[] array; array = nullptr; }

struct nsStyleQuotes {
  nsStyleQuotes();
  nsStyleQuotes(const nsStyleQuotes& aQuotes);
  ~nsStyleQuotes();

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleQuotes_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleQuotes();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleQuotes_id, this);
  }

  void SetInitial();
  void CopyFrom(const nsStyleQuotes& aSource);

  nsChangeHint CalcDifference(const nsStyleQuotes& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }

  uint32_t  QuotesCount(void) const { return mQuotesCount; } 

  const nsString* OpenQuoteAt(uint32_t aIndex) const
  {
    NS_ASSERTION(aIndex < mQuotesCount, "out of range");
    return mQuotes + (aIndex * 2);
  }
  const nsString* CloseQuoteAt(uint32_t aIndex) const
  {
    NS_ASSERTION(aIndex < mQuotesCount, "out of range");
    return mQuotes + (aIndex * 2 + 1);
  }
  nsresult  GetQuotesAt(uint32_t aIndex, nsString& aOpen, nsString& aClose) const {
    if (aIndex < mQuotesCount) {
      aIndex *= 2;
      aOpen = mQuotes[aIndex];
      aClose = mQuotes[++aIndex];
      return NS_OK;
    }
    return NS_ERROR_ILLEGAL_VALUE;
  }

  nsresult  AllocateQuotes(uint32_t aCount) {
    if (aCount != mQuotesCount) {
      DELETE_ARRAY_IF(mQuotes);
      if (aCount) {
        mQuotes = new nsString[aCount * 2];
        if (! mQuotes) {
          mQuotesCount = 0;
          return NS_ERROR_OUT_OF_MEMORY;
        }
      }
      mQuotesCount = aCount;
    }
    return NS_OK;
  }

  nsresult  SetQuotesAt(uint32_t aIndex, const nsString& aOpen, const nsString& aClose) {
    if (aIndex < mQuotesCount) {
      aIndex *= 2;
      mQuotes[aIndex] = aOpen;
      mQuotes[++aIndex] = aClose;
      return NS_OK;
    }
    return NS_ERROR_ILLEGAL_VALUE;
  }

protected:
  uint32_t            mQuotesCount;
  nsString*           mQuotes;
};

struct nsStyleContent {
  nsStyleContent(void);
  nsStyleContent(const nsStyleContent& aContent);
  ~nsStyleContent(void);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleContent_id, sz);
  }
  void Destroy(nsPresContext* aContext);

  nsChangeHint CalcDifference(const nsStyleContent& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }

  uint32_t  ContentCount(void) const  { return mContentCount; } 

  const nsStyleContentData& ContentAt(uint32_t aIndex) const {
    NS_ASSERTION(aIndex < mContentCount, "out of range");
    return mContents[aIndex];
  }

  nsStyleContentData& ContentAt(uint32_t aIndex) {
    NS_ASSERTION(aIndex < mContentCount, "out of range");
    return mContents[aIndex];
  }

  nsresult AllocateContents(uint32_t aCount);

  uint32_t  CounterIncrementCount(void) const { return mIncrementCount; }  
  const nsStyleCounterData* GetCounterIncrementAt(uint32_t aIndex) const {
    NS_ASSERTION(aIndex < mIncrementCount, "out of range");
    return &mIncrements[aIndex];
  }

  nsresult  AllocateCounterIncrements(uint32_t aCount) {
    if (aCount != mIncrementCount) {
      DELETE_ARRAY_IF(mIncrements);
      if (aCount) {
        mIncrements = new nsStyleCounterData[aCount];
        if (! mIncrements) {
          mIncrementCount = 0;
          return NS_ERROR_OUT_OF_MEMORY;
        }
      }
      mIncrementCount = aCount;
    }
    return NS_OK;
  }

  nsresult  SetCounterIncrementAt(uint32_t aIndex, const nsString& aCounter, int32_t aIncrement) {
    if (aIndex < mIncrementCount) {
      mIncrements[aIndex].mCounter = aCounter;
      mIncrements[aIndex].mValue = aIncrement;
      return NS_OK;
    }
    return NS_ERROR_ILLEGAL_VALUE;
  }

  uint32_t  CounterResetCount(void) const { return mResetCount; }  
  const nsStyleCounterData* GetCounterResetAt(uint32_t aIndex) const {
    NS_ASSERTION(aIndex < mResetCount, "out of range");
    return &mResets[aIndex];
  }

  nsresult  AllocateCounterResets(uint32_t aCount) {
    if (aCount != mResetCount) {
      DELETE_ARRAY_IF(mResets);
      if (aCount) {
        mResets = new nsStyleCounterData[aCount];
        if (! mResets) {
          mResetCount = 0;
          return NS_ERROR_OUT_OF_MEMORY;
        }
      }
      mResetCount = aCount;
    }
    return NS_OK;
  }

  nsresult  SetCounterResetAt(uint32_t aIndex, const nsString& aCounter, int32_t aValue) {
    if (aIndex < mResetCount) {
      mResets[aIndex].mCounter = aCounter;
      mResets[aIndex].mValue = aValue;
      return NS_OK;
    }
    return NS_ERROR_ILLEGAL_VALUE;
  }

  nsStyleCoord  mMarkerOffset;  

protected:
  nsStyleContentData* mContents;
  nsStyleCounterData* mIncrements;
  nsStyleCounterData* mResets;

  uint32_t            mContentCount;
  uint32_t            mIncrementCount;
  uint32_t            mResetCount;
};

struct nsStyleUIReset {
  nsStyleUIReset(void);
  nsStyleUIReset(const nsStyleUIReset& aOther);
  ~nsStyleUIReset(void);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleUIReset_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleUIReset();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleUIReset_id, this);
  }

  nsChangeHint CalcDifference(const nsStyleUIReset& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }

  uint8_t   mUserSelect;      
  uint8_t   mForceBrokenImageIcon; 
  uint8_t   mIMEMode;         
  uint8_t   mWindowShadow;    
};

struct nsCursorImage {
  bool mHaveHotspot;
  float mHotspotX, mHotspotY;

  nsCursorImage();
  nsCursorImage(const nsCursorImage& aOther);
  ~nsCursorImage();

  nsCursorImage& operator=(const nsCursorImage& aOther);
  




  void SetImage(imgIRequest *aImage) {
    if (mImage)
      mImage->UnlockImage();
    mImage = aImage;
    if (mImage)
      mImage->LockImage();
  }
  imgIRequest* GetImage() const {
    return mImage;
  }

private:
  nsCOMPtr<imgIRequest> mImage;
};

struct nsStyleUserInterface {
  nsStyleUserInterface(void);
  nsStyleUserInterface(const nsStyleUserInterface& aOther);
  ~nsStyleUserInterface(void);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleUserInterface_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleUserInterface();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleUserInterface_id, this);
  }

  nsChangeHint CalcDifference(const nsStyleUserInterface& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(NS_STYLE_HINT_FRAMECHANGE,
                          NS_CombineHint(nsChangeHint_UpdateCursor,
                                         nsChangeHint_NeutralChange));
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }

  uint8_t   mUserInput;       
  uint8_t   mUserModify;      
  uint8_t   mUserFocus;       
  uint8_t   mWindowDragging;  

  uint8_t   mCursor;          

  uint32_t mCursorArrayLength;
  nsCursorImage *mCursorArray;
                              
                              
                              

  
  
  void CopyCursorArrayFrom(const nsStyleUserInterface& aSource);
};

struct nsStyleXUL {
  nsStyleXUL();
  nsStyleXUL(const nsStyleXUL& aSource);
  ~nsStyleXUL();

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleXUL_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleXUL();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleXUL_id, this);
  }

  nsChangeHint CalcDifference(const nsStyleXUL& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }

  float         mBoxFlex;               
  uint32_t      mBoxOrdinal;            
  uint8_t       mBoxAlign;              
  uint8_t       mBoxDirection;          
  uint8_t       mBoxOrient;             
  uint8_t       mBoxPack;               
  bool          mStretchStack;          
};

struct nsStyleColumn {
  explicit nsStyleColumn(nsPresContext* aPresContext);
  nsStyleColumn(const nsStyleColumn& aSource);
  ~nsStyleColumn();

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleColumn_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleColumn();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleColumn_id, this);
  }

  nsChangeHint CalcDifference(const nsStyleColumn& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(NS_STYLE_HINT_FRAMECHANGE,
                          nsChangeHint_NeutralChange);
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }

  



  static const uint32_t kMaxColumnCount;

  uint32_t     mColumnCount; 
  nsStyleCoord mColumnWidth; 
  nsStyleCoord mColumnGap;   

  nscolor      mColumnRuleColor;  
  uint8_t      mColumnRuleStyle;  
  uint8_t      mColumnFill;  

  
  
  bool mColumnRuleColorIsForeground;

  void SetColumnRuleWidth(nscoord aWidth) {
    mColumnRuleWidth = NS_ROUND_BORDER_TO_PIXELS(aWidth, mTwipsPerPixel);
  }

  nscoord GetComputedColumnRuleWidth() const {
    return (IsVisibleBorderStyle(mColumnRuleStyle) ? mColumnRuleWidth : 0);
  }

protected:
  nscoord mColumnRuleWidth;  
  nscoord mTwipsPerPixel;
};

enum nsStyleSVGPaintType {
  eStyleSVGPaintType_None = 1,
  eStyleSVGPaintType_Color,
  eStyleSVGPaintType_Server,
  eStyleSVGPaintType_ContextFill,
  eStyleSVGPaintType_ContextStroke
};

enum nsStyleSVGOpacitySource {
  eStyleSVGOpacitySource_Normal,
  eStyleSVGOpacitySource_ContextFillOpacity,
  eStyleSVGOpacitySource_ContextStrokeOpacity
};

struct nsStyleSVGPaint
{
  union {
    nscolor mColor;
    nsIURI *mPaintServer;
  } mPaint;
  nsStyleSVGPaintType mType;
  nscolor mFallbackColor;

  nsStyleSVGPaint() : mType(nsStyleSVGPaintType(0)) { mPaint.mPaintServer = nullptr; }
  ~nsStyleSVGPaint();
  void SetType(nsStyleSVGPaintType aType);
  nsStyleSVGPaint& operator=(const nsStyleSVGPaint& aOther);
  bool operator==(const nsStyleSVGPaint& aOther) const;

  bool operator!=(const nsStyleSVGPaint& aOther) const {
    return !(*this == aOther);
  }
};

struct nsStyleSVG {
  nsStyleSVG();
  nsStyleSVG(const nsStyleSVG& aSource);
  ~nsStyleSVG();

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleSVG_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleSVG();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleSVG_id, this);
  }

  nsChangeHint CalcDifference(const nsStyleSVG& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(NS_CombineHint(nsChangeHint_UpdateEffects,
             NS_CombineHint(nsChangeHint_NeedReflow, nsChangeHint_NeedDirtyReflow)), 
                                         nsChangeHint_RepaintFrame);
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return nsChangeHint_NeedReflow;
  }

  nsStyleSVGPaint  mFill;             
  nsStyleSVGPaint  mStroke;           
  nsCOMPtr<nsIURI> mMarkerEnd;        
  nsCOMPtr<nsIURI> mMarkerMid;        
  nsCOMPtr<nsIURI> mMarkerStart;      
  nsStyleCoord    *mStrokeDasharray;  

  nsStyleCoord     mStrokeDashoffset; 
  nsStyleCoord     mStrokeWidth;      

  float            mFillOpacity;      
  float            mStrokeMiterlimit; 
  float            mStrokeOpacity;    

  uint32_t         mStrokeDasharrayLength;
  uint8_t          mClipRule;         
  uint8_t          mColorInterpolation; 
  uint8_t          mColorInterpolationFilters; 
  uint8_t          mFillRule;         
  uint8_t          mImageRendering;   
  uint8_t          mPaintOrder;       
  uint8_t          mShapeRendering;   
  uint8_t          mStrokeLinecap;    
  uint8_t          mStrokeLinejoin;   
  uint8_t          mTextAnchor;       
  uint8_t          mTextRendering;    

  
  
  
  nsStyleSVGOpacitySource mFillOpacitySource    : 3;
  nsStyleSVGOpacitySource mStrokeOpacitySource  : 3;

  
  bool mStrokeDasharrayFromObject   : 1;
  bool mStrokeDashoffsetFromObject  : 1;
  bool mStrokeWidthFromObject       : 1;

  bool HasMarker() const {
    return mMarkerStart || mMarkerMid || mMarkerEnd;
  }

  



  bool HasStroke() const {
    return mStroke.mType != eStyleSVGPaintType_None && mStrokeOpacity > 0;
  }

  



  bool HasFill() const {
    return mFill.mType != eStyleSVGPaintType_None && mFillOpacity > 0;
  }
};

class nsStyleBasicShape final {
public:
  enum Type {
    eInset,
    eCircle,
    eEllipse,
    ePolygon
  };

  explicit nsStyleBasicShape(Type type)
    : mType(type),
      mFillRule(NS_STYLE_FILL_RULE_NONZERO)
  {
    mPosition.SetInitialPercentValues(0.5f);
  }

  Type GetShapeType() const { return mType; }

  int32_t GetFillRule() const { return mFillRule; }
  void SetFillRule(int32_t aFillRule)
  {
    NS_ASSERTION(mType == ePolygon, "expected polygon");
    mFillRule = aFillRule;
  }

  typedef nsStyleBackground::Position Position;
  Position& GetPosition() {
    NS_ASSERTION(mType == eCircle || mType == eEllipse,
                 "expected circle or ellipse");
    return mPosition;
  }
  const Position& GetPosition() const {
    NS_ASSERTION(mType == eCircle || mType == eEllipse,
                 "expected circle or ellipse");
    return mPosition;
  }

  bool HasRadius() const {
    NS_ASSERTION(mType == eInset, "expected inset");
    nsStyleCoord zero;
    zero.SetCoordValue(0);
    NS_FOR_CSS_HALF_CORNERS(corner) {
      if (mRadius.Get(corner) != zero) {
        return true;
      }
    }
    return false;
  }
  nsStyleCorners& GetRadius() {
    NS_ASSERTION(mType == eInset, "expected inset");
    return mRadius;
  }
  const nsStyleCorners& GetRadius() const {
    NS_ASSERTION(mType == eInset, "expected inset");
    return mRadius;
  }

  
  
  nsTArray<nsStyleCoord>& Coordinates()
  {
    return mCoordinates;
  }

  const nsTArray<nsStyleCoord>& Coordinates() const
  {
    return mCoordinates;
  }

  bool operator==(const nsStyleBasicShape& aOther) const
  {
    return mType == aOther.mType &&
           mFillRule == aOther.mFillRule &&
           mCoordinates == aOther.mCoordinates &&
           mPosition == aOther.mPosition &&
           mRadius == aOther.mRadius;
  }
  bool operator!=(const nsStyleBasicShape& aOther) const {
    return !(*this == aOther);
  }

  NS_INLINE_DECL_REFCOUNTING(nsStyleBasicShape);

private:
  ~nsStyleBasicShape() {}

  Type mType;
  int32_t mFillRule;

  
  
  nsTArray<nsStyleCoord> mCoordinates;
  Position mPosition;
  nsStyleCorners mRadius;
};

struct nsStyleClipPath
{
  nsStyleClipPath();
  nsStyleClipPath(const nsStyleClipPath& aSource);
  ~nsStyleClipPath();

  nsStyleClipPath& operator=(const nsStyleClipPath& aOther);

  bool operator==(const nsStyleClipPath& aOther) const;
  bool operator!=(const nsStyleClipPath& aOther) const {
    return !(*this == aOther);
  }

  int32_t GetType() const {
    return mType;
  }

  nsIURI* GetURL() const {
    NS_ASSERTION(mType == NS_STYLE_CLIP_PATH_URL, "wrong clip-path type");
    return mURL;
  }
  void SetURL(nsIURI* aURL);

  nsStyleBasicShape* GetBasicShape() const {
    NS_ASSERTION(mType == NS_STYLE_CLIP_PATH_SHAPE, "wrong clip-path type");
    return mBasicShape;
  }

  void SetBasicShape(nsStyleBasicShape* mBasicShape,
                     uint8_t aSizingBox = NS_STYLE_CLIP_SHAPE_SIZING_NOBOX);

  uint8_t GetSizingBox() const { return mSizingBox; }
  void SetSizingBox(uint8_t aSizingBox);

private:
  void ReleaseRef();
  void* operator new(size_t) = delete;

  int32_t mType; 
  union {
    nsStyleBasicShape* mBasicShape;
    nsIURI* mURL;
  };
  uint8_t mSizingBox; 
};

struct nsStyleFilter {
  nsStyleFilter();
  nsStyleFilter(const nsStyleFilter& aSource);
  ~nsStyleFilter();

  nsStyleFilter& operator=(const nsStyleFilter& aOther);

  bool operator==(const nsStyleFilter& aOther) const;
  bool operator!=(const nsStyleFilter& aOther) const {
    return !(*this == aOther);
  }

  int32_t GetType() const {
    return mType;
  }

  const nsStyleCoord& GetFilterParameter() const {
    NS_ASSERTION(mType != NS_STYLE_FILTER_DROP_SHADOW &&
                 mType != NS_STYLE_FILTER_URL &&
                 mType != NS_STYLE_FILTER_NONE, "wrong filter type");
    return mFilterParameter;
  }
  void SetFilterParameter(const nsStyleCoord& aFilterParameter,
                          int32_t aType);

  nsIURI* GetURL() const {
    NS_ASSERTION(mType == NS_STYLE_FILTER_URL, "wrong filter type");
    return mURL;
  }
  void SetURL(nsIURI* aURL);

  nsCSSShadowArray* GetDropShadow() const {
    NS_ASSERTION(mType == NS_STYLE_FILTER_DROP_SHADOW, "wrong filter type");
    return mDropShadow;
  }
  void SetDropShadow(nsCSSShadowArray* aDropShadow);

private:
  void ReleaseRef();

  int32_t mType; 
  nsStyleCoord mFilterParameter; 
  union {
    nsIURI* mURL;
    nsCSSShadowArray* mDropShadow;
  };
};

template<>
struct nsTArray_CopyChooser<nsStyleFilter> {
  typedef nsTArray_CopyWithConstructors<nsStyleFilter> Type;
};

struct nsStyleSVGReset {
  nsStyleSVGReset();
  nsStyleSVGReset(const nsStyleSVGReset& aSource);
  ~nsStyleSVGReset();

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleSVGReset_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleSVGReset();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleSVGReset_id, this);
  }

  nsChangeHint CalcDifference(const nsStyleSVGReset& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(nsChangeHint_UpdateEffects,
            NS_CombineHint(nsChangeHint_UpdateOverflow, NS_STYLE_HINT_REFLOW));
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return NS_CombineHint(nsChangeHint_NeedReflow,
                          nsChangeHint_ClearAncestorIntrinsics);
  }

  bool HasFilters() const {
    return mFilters.Length() > 0;
  }

  bool HasNonScalingStroke() const {
    return mVectorEffect == NS_STYLE_VECTOR_EFFECT_NON_SCALING_STROKE;
  }

  nsStyleClipPath mClipPath;          
  nsTArray<nsStyleFilter> mFilters;   
  nsCOMPtr<nsIURI> mMask;             
  nscolor          mStopColor;        
  nscolor          mFloodColor;       
  nscolor          mLightingColor;    

  float            mStopOpacity;      
  float            mFloodOpacity;     

  uint8_t          mDominantBaseline; 
  uint8_t          mVectorEffect;     
  uint8_t          mMaskType;         
};

struct nsStyleVariables {
  nsStyleVariables();
  nsStyleVariables(const nsStyleVariables& aSource);
  ~nsStyleVariables();

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->PresShell()->
      AllocateByObjectID(nsPresArena::nsStyleVariables_id, sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleVariables();
    aContext->PresShell()->
      FreeByObjectID(nsPresArena::nsStyleVariables_id, this);
  }

  nsChangeHint CalcDifference(const nsStyleVariables& aOther) const;
  static nsChangeHint MaxDifference() {
    return nsChangeHint(0);
  }
  static nsChangeHint MaxDifferenceNeverInherited() {
    
    
    return nsChangeHint(0);
  }

  mozilla::CSSVariableValues mVariables;
};

#endif 
