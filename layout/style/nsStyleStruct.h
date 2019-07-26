









#ifndef nsStyleStruct_h___
#define nsStyleStruct_h___

#include "mozilla/Attributes.h"

#include "nsColor.h"
#include "nsCoord.h"
#include "nsMargin.h"
#include "nsRect.h"
#include "nsFont.h"
#include "nsStyleCoord.h"
#include "nsStyleConsts.h"
#include "nsChangeHint.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsIAtom.h"
#include "nsCSSValue.h"
#include "imgRequestProxy.h"
#include <algorithm>

class nsIFrame;
class nsIURI;
class imgIContainer;


#include "nsStyleStructFwd.h"



#define NS_STYLE_INHERIT_MASK             0x007fffff



#define NS_STYLE_HAS_TEXT_DECORATION_LINES 0x00800000

#define NS_STYLE_HAS_PSEUDO_ELEMENT_DATA  0x01000000

#define NS_STYLE_RELEVANT_LINK_VISITED    0x02000000

#define NS_STYLE_IS_STYLE_IF_VISITED      0x04000000

#define NS_STYLE_CONTEXT_TYPE_MASK        0xf8000000
#define NS_STYLE_CONTEXT_TYPE_SHIFT       27


#define NS_RULE_NODE_GC_MARK              0x02000000
#define NS_RULE_NODE_USED_DIRECTLY        0x04000000
#define NS_RULE_NODE_IS_IMPORTANT         0x08000000
#define NS_RULE_NODE_LEVEL_MASK           0xf0000000
#define NS_RULE_NODE_LEVEL_SHIFT          28



struct nsStyleFont {
  nsStyleFont(const nsFont& aFont, nsPresContext *aPresContext);
  nsStyleFont(const nsStyleFont& aStyleFont);
  nsStyleFont(nsPresContext *aPresContext);
private:
  void Init(nsPresContext *aPresContext);
public:
  ~nsStyleFont(void) {
    MOZ_COUNT_DTOR(nsStyleFont);
  }

  nsChangeHint CalcDifference(const nsStyleFont& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_REFLOW;
  }
  static nsChangeHint CalcFontDifference(const nsFont& aFont1, const nsFont& aFont2);

  static nscoord ZoomText(nsPresContext* aPresContext, nscoord aSize);
  static nscoord UnZoomText(nsPresContext* aPresContext, nscoord aSize);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW;
  void Destroy(nsPresContext* aContext);

  nsFont  mFont;        
  nscoord mSize;        
                        
                        
                        
                        
                        
  uint8_t mGenericID;   
                        

  
  int8_t  mScriptLevel;          

  
  bool mExplicitLanguage;        

  
  nscoord mScriptUnconstrainedSize;
  nscoord mScriptMinSize;        
  float   mScriptSizeMultiplier; 
  nsCOMPtr<nsIAtom> mLanguage;   
};

struct nsStyleGradientStop {
  nsStyleCoord mLocation; 
  nscolor mColor;
};

class nsStyleGradient {
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

  nsStyleGradient(const nsStyleGradient& aOther) MOZ_DELETE;
  nsStyleGradient& operator=(const nsStyleGradient& aOther) MOZ_DELETE;
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
  void SetImageData(imgIRequest* aImage);
  void TrackImage(nsPresContext* aContext);
  void UntrackImage(nsPresContext* aContext);
  void SetGradientData(nsStyleGradient* aGradient);
  void SetElementId(const PRUnichar* aElementId);
  void SetCropRect(nsStyleSides* aCropRect);

  nsStyleImageType GetType() const {
    return mType;
  }
  imgIRequest* GetImageData() const {
    NS_ABORT_IF_FALSE(mType == eStyleImageType_Image, "Data is not an image!");
    NS_ABORT_IF_FALSE(mImageTracked,
                      "Should be tracking any image we're going to use!");
    return mImage;
  }
  nsStyleGradient* GetGradientData() const {
    NS_ASSERTION(mType == eStyleImageType_Gradient, "Data is not a gradient!");
    return mGradient;
  }
  const PRUnichar* GetElementId() const {
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
  



  bool IsEmpty() const {
    
    
    
    
    
    
    
    return mType == eStyleImageType_Null;
  }

  bool operator==(const nsStyleImage& aOther) const;
  bool operator!=(const nsStyleImage& aOther) const {
    return !(*this == aOther);
  }

private:
  void DoCopy(const nsStyleImage& aOther);

  nsStyleImageType mType;
  union {
    imgIRequest* mImage;
    nsStyleGradient* mGradient;
    PRUnichar* mElementId;
  };
  
  nsAutoPtr<nsStyleSides> mCropRect;
#ifdef DEBUG
  bool mImageTracked;
#endif
};

struct nsStyleColor {
  nsStyleColor(nsPresContext* aPresContext);
  nsStyleColor(const nsStyleColor& aOther);
  ~nsStyleColor(void) {
    MOZ_COUNT_DTOR(nsStyleColor);
  }

  nsChangeHint CalcDifference(const nsStyleColor& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_VISUAL;
  }

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleColor();
    aContext->FreeToShell(sizeof(nsStyleColor), this);
  }

  
  
  nscolor mColor;                 
};

struct nsStyleBackground {
  nsStyleBackground();
  nsStyleBackground(const nsStyleBackground& aOther);
  ~nsStyleBackground();

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext);

  nsChangeHint CalcDifference(const nsStyleBackground& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(nsChangeHint_UpdateEffects, NS_STYLE_HINT_VISUAL);
  }

  struct Position;
  friend struct Position;
  struct Position {
    typedef nsStyleCoord::Calc PositionCoord;
    PositionCoord mXPosition, mYPosition;

    
    Position() {}

    
    void SetInitialValues();

    
    
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
    struct Dimension : public nsStyleCoord::Calc {
      nscoord ResolveLengthPercentage(nscoord aAvailable) const {
        double d = double(mPercent) * double(aAvailable) + double(mLength);
        if (d < 0.0)
          return 0;
        return NSToCoordRoundWithClamp(float(d));
      }
    };
    Dimension mWidth, mHeight;

    nscoord ResolveWidthLengthPercentage(const nsSize& aBgPositioningArea) const {
      NS_ABORT_IF_FALSE(mWidthType == eLengthPercentage,
                        "resolving non-length/percent dimension!");
      return mWidth.ResolveLengthPercentage(aBgPositioningArea.width);
    }

    nscoord ResolveHeightLengthPercentage(const nsSize& aBgPositioningArea) const {
      NS_ABORT_IF_FALSE(mHeightType == eLengthPercentage,
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
           mSizeCount;
  
  
  
  
  
  
  
  
  
  
  nsAutoTArray<Layer, 1> mLayers;

  const Layer& BottomLayer() const { return mLayers[mImageCount - 1]; }

  #define NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(var_, stylebg_) \
    for (uint32_t var_ = (stylebg_) ? (stylebg_)->mImageCount : 1; var_-- != 0; )
  #define NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT_WITH_RANGE(var_, stylebg_, start_, count_) \
    NS_ASSERTION((int32_t)(start_) >= 0 && (uint32_t)(start_) < ((stylebg_) ? (stylebg_)->mImageCount : 1), "Invalid layer start!"); \
    NS_ASSERTION((count_) > 0 && (count_) <= (start_) + 1, "Invalid layer range!"); \
    for (uint32_t var_ = (start_) + 1; var_-- != (uint32_t)((start_) + 1 - (count_)); )

  nscolor mBackgroundColor;       

  
  
  
  uint8_t mBackgroundInlinePolicy; 

  
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

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW;
  void Destroy(nsPresContext* aContext);

  void RecalcData();
  nsChangeHint CalcDifference(const nsStyleMargin& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_SubtractHint(NS_STYLE_HINT_REFLOW,
                           NS_CombineHint(nsChangeHint_ClearDescendantIntrinsics,
                                          nsChangeHint_NeedDirtyReflow));
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

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW;
  void Destroy(nsPresContext* aContext);

  void RecalcData();
  nsChangeHint CalcDifference(const nsStylePadding& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_SubtractHint(NS_STYLE_HINT_REFLOW,
                           nsChangeHint_ClearDescendantIntrinsics);
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
  nsBorderColors(const nscolor& aColor) : mNext(nullptr), mColor(aColor) {}
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

  bool operator==(const nsCSSShadowItem& aOther) {
    return (mXOffset == aOther.mXOffset &&
            mYOffset == aOther.mYOffset &&
            mRadius == aOther.mRadius &&
            mHasColor == aOther.mHasColor &&
            mSpread == aOther.mSpread &&
            mInset == aOther.mInset &&
            (!mHasColor || mColor == aOther.mColor));
  }
  bool operator!=(const nsCSSShadowItem& aOther) {
    return !(*this == aOther);
  }
};

class nsCSSShadowArray {
  public:
    void* operator new(size_t aBaseSize, uint32_t aArrayLen) {
      
      
      
      
      
      return ::operator new(aBaseSize +
                            (aArrayLen - 1) * sizeof(nsCSSShadowItem));
    }

    nsCSSShadowArray(uint32_t aArrayLen) :
      mLength(aArrayLen)
    {
      MOZ_COUNT_CTOR(nsCSSShadowArray);
      for (uint32_t i = 1; i < mLength; ++i) {
        
        
        new (&mArray[i]) nsCSSShadowItem();
      }
    }
    ~nsCSSShadowArray() {
      MOZ_COUNT_DTOR(nsCSSShadowArray);
      for (uint32_t i = 1; i < mLength; ++i) {
        mArray[i].~nsCSSShadowItem();
      }
    }

    uint32_t Length() const { return mLength; }
    nsCSSShadowItem* ShadowAt(uint32_t i) {
      NS_ABORT_IF_FALSE(i < mLength, "Accessing too high an index in the text shadow array!");
      return &mArray[i];
    }
    const nsCSSShadowItem* ShadowAt(uint32_t i) const {
      NS_ABORT_IF_FALSE(i < mLength, "Accessing too high an index in the text shadow array!");
      return &mArray[i];
    }

    bool HasShadowWithInset(bool aInset) {
      for (uint32_t i = 0; i < mLength; ++i) {
        if (mArray[i].mInset == aInset)
          return true;
      }
      return false;
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
  nsStyleBorder(nsPresContext* aContext);
  nsStyleBorder(const nsStyleBorder& aBorder);
  ~nsStyleBorder();

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW;
  void Destroy(nsPresContext* aContext);

  nsChangeHint CalcDifference(const nsStyleBorder& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(NS_STYLE_HINT_REFLOW,
                          nsChangeHint_BorderStyleNoneChange);
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
    return mComputedBorder != nsMargin(0,0,0,0) || mBorderImageSource;
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

  
  inline bool IsBorderImageLoaded() const;
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

  
  inline void SetBorderImage(imgRequestProxy* aImage);
  inline imgRequestProxy* GetBorderImage() const;

  bool HasBorderImage() {return !!mBorderImageSource;}

  void TrackImage(nsPresContext* aContext);
  void UntrackImage(nsPresContext* aContext);

  nsMargin GetImageOutset() const;

  
  
  inline void SetSubImage(uint8_t aIndex, imgIContainer* aSubImage) const;
  inline imgIContainer* GetSubImage(uint8_t aIndex) const;

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

public:
  nsBorderColors** mBorderColors;        
  nsRefPtr<nsCSSShadowArray> mBoxShadow; 

#ifdef DEBUG
  bool mImageTracked;
#endif

protected:
  nsRefPtr<imgRequestProxy> mBorderImageSource; 

public:
  nsStyleCorners mBorderRadius;       
  nsStyleSides   mBorderImageSlice;   
  nsStyleSides   mBorderImageWidth;   
  nsStyleSides   mBorderImageOutset;  

  uint8_t        mBorderImageFill;    
  uint8_t        mBorderImageRepeatH; 
  uint8_t        mBorderImageRepeatV; 
  uint8_t        mFloatEdge;          

protected:
  
  
  
  
  
  
  nsMargin      mComputedBorder;

  
  
  
  
  
  
  
  
  
  
  
  nsMargin      mBorder;

  uint8_t       mBorderStyle[4];  
  nscolor       mBorderColor[4];  
                                  
private:
  
  nsCOMArray<imgIContainer> mSubImages;

  nscoord       mTwipsPerPixel;

  nsStyleBorder& operator=(const nsStyleBorder& aOther) MOZ_DELETE;
};


struct nsStyleOutline {
  nsStyleOutline(nsPresContext* aPresContext);
  nsStyleOutline(const nsStyleOutline& aOutline);
  ~nsStyleOutline(void) {
    MOZ_COUNT_DTOR(nsStyleOutline);
  }

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleOutline();
    aContext->FreeToShell(sizeof(nsStyleOutline), this);
  }

  void RecalcData(nsPresContext* aContext);
  nsChangeHint CalcDifference(const nsStyleOutline& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(nsChangeHint_AllReflowHints,
                          nsChangeHint_RepaintFrame);
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
  nsStyleList(void);
  nsStyleList(const nsStyleList& aStyleList);
  ~nsStyleList(void);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleList();
    aContext->FreeToShell(sizeof(nsStyleList), this);
  }

  nsChangeHint CalcDifference(const nsStyleList& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
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

  uint8_t   mListStyleType;             
  uint8_t   mListStylePosition;         
private:
  nsRefPtr<imgRequestProxy> mListStyleImage; 
  nsStyleList& operator=(const nsStyleList& aOther) MOZ_DELETE;
public:
  nsRect        mImageRegion;           
};

struct nsStylePosition {
  nsStylePosition(void);
  nsStylePosition(const nsStylePosition& aOther);
  ~nsStylePosition(void);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStylePosition();
    aContext->FreeToShell(sizeof(nsStylePosition), this);
  }

  nsChangeHint CalcDifference(const nsStylePosition& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(NS_STYLE_HINT_REFLOW,
                          nsChangeHint(nsChangeHint_RecomputePosition |
                                       nsChangeHint_UpdateOverflow));
  }

  nsStyleSides  mOffset;                
  nsStyleCoord  mWidth;                 
  nsStyleCoord  mMinWidth;              
  nsStyleCoord  mMaxWidth;              
  nsStyleCoord  mHeight;                
  nsStyleCoord  mMinHeight;             
  nsStyleCoord  mMaxHeight;             
#ifdef MOZ_FLEXBOX
  nsStyleCoord  mFlexBasis;             
#endif 
  uint8_t       mBoxSizing;             
#ifdef MOZ_FLEXBOX
  uint8_t       mAlignItems;            
  uint8_t       mAlignSelf;             
  uint8_t       mFlexDirection;         
  uint8_t       mJustifyContent;        
  int32_t       mOrder;                 
  float         mFlexGrow;              
  float         mFlexShrink;            
#endif 
  nsStyleCoord  mZIndex;                

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
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleTextReset();
    aContext->FreeToShell(sizeof(nsStyleTextReset), this);
  }

  uint8_t GetDecorationStyle() const
  {
    return (mTextDecorationStyle & BORDER_STYLE_MASK);
  }

  void SetDecorationStyle(uint8_t aStyle)
  {
    NS_ABORT_IF_FALSE((aStyle & BORDER_STYLE_MASK) == aStyle,
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
    return NS_STYLE_HINT_REFLOW;
  }

  nsStyleCoord  mVerticalAlign;         
  nsStyleTextOverflow mTextOverflow;    

  uint8_t mTextBlink;                   
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
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleText();
    aContext->FreeToShell(sizeof(nsStyleText), this);
  }

  nsChangeHint CalcDifference(const nsStyleText& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
  }

  uint8_t mTextAlign;                   
  uint8_t mTextAlignLast;               
  uint8_t mTextTransform;               
  uint8_t mWhiteSpace;                  
  uint8_t mWordBreak;                   
  uint8_t mWordWrap;                    
  uint8_t mHyphens;                     
  uint8_t mTextSizeAdjust;              
  int32_t mTabSize;                     

  nscoord mWordSpacing;                 
  nsStyleCoord  mLetterSpacing;         
  nsStyleCoord  mLineHeight;            
  nsStyleCoord  mTextIndent;            

  nsRefPtr<nsCSSShadowArray> mTextShadow; 

  bool WhiteSpaceIsSignificant() const {
    return mWhiteSpace == NS_STYLE_WHITESPACE_PRE ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_WRAP ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_DISCARD_NEWLINES;
  }

  bool NewlineIsSignificant() const {
    return mWhiteSpace == NS_STYLE_WHITESPACE_PRE ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_WRAP ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_LINE;
  }

  bool NewlineIsDiscarded() const {
    return mWhiteSpace == NS_STYLE_WHITESPACE_PRE_DISCARD_NEWLINES;
  }

  bool WhiteSpaceOrNewlineIsSignificant() const {
    return mWhiteSpace == NS_STYLE_WHITESPACE_PRE ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_WRAP ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_LINE ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_DISCARD_NEWLINES;
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

  

  
  
  
  
  inline bool HasTextShadow(const nsIFrame* aContextFrame) const;
  inline nsCSSShadowArray* GetTextShadow(const nsIFrame* aContextFrame) const;
  inline bool WhiteSpaceCanWrap(const nsIFrame* aContextFrame) const;
  inline bool WordCanWrap(const nsIFrame* aContextFrame) const;
};

struct nsStyleVisibility {
  nsStyleVisibility(nsPresContext* aPresContext);
  nsStyleVisibility(const nsStyleVisibility& aVisibility);
  ~nsStyleVisibility() {
    MOZ_COUNT_DTOR(nsStyleVisibility);
  }

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleVisibility();
    aContext->FreeToShell(sizeof(nsStyleVisibility), this);
  }

  nsChangeHint CalcDifference(const nsStyleVisibility& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
  }

  uint8_t mDirection;                  
  uint8_t mVisible;                    
  uint8_t mPointerEvents;              

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
    NS_ABORT_IF_FALSE(mType == StepStart || mType == StepEnd, "wrong type");
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

struct nsTransition {
  nsTransition() {  }
  explicit nsTransition(const nsTransition& aCopy);

  void SetInitialValues();

  

  const nsTimingFunction& GetTimingFunction() const { return mTimingFunction; }
  float GetDelay() const { return mDelay; }
  float GetDuration() const { return mDuration; }
  nsCSSProperty GetProperty() const { return mProperty; }
  nsIAtom* GetUnknownProperty() const { return mUnknownProperty; }

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
  void CopyPropertyFrom(const nsTransition& aOther)
    {
      mProperty = aOther.mProperty;
      mUnknownProperty = aOther.mUnknownProperty;
    }

  nsTimingFunction& TimingFunctionSlot() { return mTimingFunction; }

private:
  nsTimingFunction mTimingFunction;
  float mDuration;
  float mDelay;
  nsCSSProperty mProperty;
  nsCOMPtr<nsIAtom> mUnknownProperty; 
                                      
};

struct nsAnimation {
  nsAnimation() {  }
  explicit nsAnimation(const nsAnimation& aCopy);

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

struct nsStyleDisplay {
  nsStyleDisplay();
  nsStyleDisplay(const nsStyleDisplay& aOther);
  ~nsStyleDisplay() {
    MOZ_COUNT_DTOR(nsStyleDisplay);
  }

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleDisplay();
    aContext->FreeToShell(sizeof(nsStyleDisplay), this);
  }

  nsChangeHint CalcDifference(const nsStyleDisplay& aOther) const;
  static nsChangeHint MaxDifference() {
    
    return nsChangeHint(NS_STYLE_HINT_FRAMECHANGE |
                        nsChangeHint_UpdateOpacityLayer |
                        nsChangeHint_UpdateTransformLayer |
                        nsChangeHint_UpdateOverflow |
                        nsChangeHint_AddOrRemoveTransform);
  }

  
  
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
  uint8_t mResize;              
  uint8_t mClipFlags;           
  uint8_t mOrient;              

  
  
  
  
  uint8_t mBackfaceVisibility;
  uint8_t mTransformStyle;
  const nsCSSValueList *mSpecifiedTransform; 
  nsStyleCoord mTransformOrigin[3]; 
  nsStyleCoord mChildPerspective; 
  nsStyleCoord mPerspectiveOrigin[2]; 

  nsAutoTArray<nsTransition, 1> mTransitions; 
  
  
  uint32_t mTransitionTimingFunctionCount,
           mTransitionDurationCount,
           mTransitionDelayCount,
           mTransitionPropertyCount;

  nsAutoTArray<nsAnimation, 1> mAnimations; 
  
  
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
           NS_STYLE_DISPLAY_INLINE_BLOCK == mDisplay;
    
    
    
  }

  bool IsBlockOutsideStyle() const {
    return NS_STYLE_DISPLAY_BLOCK == mDisplay ||
#ifdef MOZ_FLEXBOX
           NS_STYLE_DISPLAY_FLEX == mDisplay ||
#endif 
           NS_STYLE_DISPLAY_LIST_ITEM == mDisplay ||
           NS_STYLE_DISPLAY_TABLE == mDisplay;
  }

  static bool IsDisplayTypeInlineOutside(uint8_t aDisplay) {
    return NS_STYLE_DISPLAY_INLINE == aDisplay ||
           NS_STYLE_DISPLAY_INLINE_BLOCK == aDisplay ||
           NS_STYLE_DISPLAY_INLINE_TABLE == aDisplay ||
           NS_STYLE_DISPLAY_INLINE_BOX == aDisplay ||
#ifdef MOZ_FLEXBOX
           NS_STYLE_DISPLAY_INLINE_FLEX == aDisplay ||
#endif 
           NS_STYLE_DISPLAY_INLINE_GRID == aDisplay ||
           NS_STYLE_DISPLAY_INLINE_STACK == aDisplay;
  }

  bool IsInlineOutsideStyle() const {
    return IsDisplayTypeInlineOutside(mDisplay);
  }

  bool IsOriginalDisplayInlineOutsideStyle() const {
    return IsDisplayTypeInlineOutside(mOriginalDisplay);
  }

  bool IsFloatingStyle() const {
    return NS_STYLE_FLOAT_NONE != mFloats;
  }

  bool IsAbsolutelyPositionedStyle() const {
    return NS_STYLE_POSITION_ABSOLUTE == mPosition ||
           NS_STYLE_POSITION_FIXED == mPosition;
  }

  bool IsRelativelyPositionedStyle() const {
    return mPosition == NS_STYLE_POSITION_RELATIVE;
  }

  bool IsPositionedStyle() const {
    return IsAbsolutelyPositionedStyle() || IsRelativelyPositionedStyle();
  }

  bool IsScrollableOverflow() const {
    
    
    return mOverflowX != NS_STYLE_OVERFLOW_VISIBLE &&
           mOverflowX != NS_STYLE_OVERFLOW_CLIP;
  }

  

  bool HasTransformStyle() const {
    return mSpecifiedTransform != nullptr || 
           mTransformStyle == NS_STYLE_TRANSFORM_STYLE_PRESERVE_3D ||
           mBackfaceVisibility == NS_STYLE_BACKFACE_VISIBILITY_HIDDEN;
  }

  

  
  
  
  
  inline bool IsBlockInside(const nsIFrame* aContextFrame) const;
  inline bool IsBlockOutside(const nsIFrame* aContextFrame) const;
  inline bool IsInlineOutside(const nsIFrame* aContextFrame) const;
  inline bool IsOriginalDisplayInlineOutside(const nsIFrame* aContextFrame) const;
  inline uint8_t GetDisplay(const nsIFrame* aContextFrame) const;
  inline bool IsFloating(const nsIFrame* aContextFrame) const;
  inline bool IsPositioned(const nsIFrame* aContextFrame) const;
  inline bool IsRelativelyPositioned(const nsIFrame* aContextFrame) const;
  inline bool IsAbsolutelyPositioned(const nsIFrame* aContextFrame) const;

  

  inline bool HasTransform(const nsIFrame* aContextFrame) const;
};

struct nsStyleTable {
  nsStyleTable(void);
  nsStyleTable(const nsStyleTable& aOther);
  ~nsStyleTable(void);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleTable();
    aContext->FreeToShell(sizeof(nsStyleTable), this);
  }

  nsChangeHint CalcDifference(const nsStyleTable& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
  }

  uint8_t       mLayoutStrategy;
  uint8_t       mFrame;         
  uint8_t       mRules;         
  int32_t       mSpan;          
};

struct nsStyleTableBorder {
  nsStyleTableBorder(nsPresContext* aContext);
  nsStyleTableBorder(const nsStyleTableBorder& aOther);
  ~nsStyleTableBorder(void);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleTableBorder();
    aContext->FreeToShell(sizeof(nsStyleTableBorder), this);
  }

  nsChangeHint CalcDifference(const nsStyleTableBorder& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
  }

  nscoord       mBorderSpacingX;
  nscoord       mBorderSpacingY;
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
  eStyleContentType_AltContent    = 50
};

struct nsStyleContentData {
  nsStyleContentType  mType;
  union {
    PRUnichar *mString;
    imgRequestProxy *mImage;
    nsCSSValue::Array* mCounters;
  } mContent;
#ifdef DEBUG
  bool mImageTracked;
#endif

  nsStyleContentData()
    : mType(nsStyleContentType(0))
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
    NS_ABORT_IF_FALSE(!mImageTracked,
                      "Setting a new image without untracking the old one!");
    NS_ABORT_IF_FALSE(mType == eStyleContentType_Image, "Wrong type!");
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
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleQuotes();
    aContext->FreeToShell(sizeof(nsStyleQuotes), this);
  }

  void SetInitial();
  void CopyFrom(const nsStyleQuotes& aSource);

  nsChangeHint CalcDifference(const nsStyleQuotes& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
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
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext);

  nsChangeHint CalcDifference(const nsStyleContent& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
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
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleUIReset();
    aContext->FreeToShell(sizeof(nsStyleUIReset), this);
  }

  nsChangeHint CalcDifference(const nsStyleUIReset& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
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
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleUserInterface();
    aContext->FreeToShell(sizeof(nsStyleUserInterface), this);
  }

  nsChangeHint CalcDifference(const nsStyleUserInterface& aOther) const;
  static nsChangeHint MaxDifference() {
    return nsChangeHint(nsChangeHint_UpdateCursor | NS_STYLE_HINT_FRAMECHANGE);
  }

  uint8_t   mUserInput;       
  uint8_t   mUserModify;      
  uint8_t   mUserFocus;       

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
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleXUL();
    aContext->FreeToShell(sizeof(nsStyleXUL), this);
  }

  nsChangeHint CalcDifference(const nsStyleXUL& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
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
  nsStyleColumn(nsPresContext* aPresContext);
  nsStyleColumn(const nsStyleColumn& aSource);
  ~nsStyleColumn();

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleColumn();
    aContext->FreeToShell(sizeof(nsStyleColumn), this);
  }

  nsChangeHint CalcDifference(const nsStyleColumn& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_STYLE_HINT_FRAMECHANGE;
  }

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
  eStyleSVGPaintType_ObjectFill,
  eStyleSVGPaintType_ObjectStroke
};

enum nsStyleSVGOpacitySource {
  eStyleSVGOpacitySource_Normal,
  eStyleSVGOpacitySource_ObjectFillOpacity,
  eStyleSVGOpacitySource_ObjectStrokeOpacity
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
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleSVG();
    aContext->FreeToShell(sizeof(nsStyleSVG), this);
  }

  nsChangeHint CalcDifference(const nsStyleSVG& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(NS_CombineHint(nsChangeHint_UpdateEffects,
                                         nsChangeHint_AllReflowHints),
                                         nsChangeHint_RepaintFrame);
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
};

struct nsStyleSVGReset {
  nsStyleSVGReset();
  nsStyleSVGReset(const nsStyleSVGReset& aSource);
  ~nsStyleSVGReset();

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleSVGReset();
    aContext->FreeToShell(sizeof(nsStyleSVGReset), this);
  }

  nsChangeHint CalcDifference(const nsStyleSVGReset& aOther) const;
  static nsChangeHint MaxDifference() {
    return NS_CombineHint(nsChangeHint_UpdateEffects, NS_STYLE_HINT_REFLOW);
  }

  nsCOMPtr<nsIURI> mClipPath;         
  nsCOMPtr<nsIURI> mFilter;           
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

#endif 
