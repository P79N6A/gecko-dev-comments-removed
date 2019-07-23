














































#ifndef nsStyleStruct_h___
#define nsStyleStruct_h___

#include "nsColor.h"
#include "nsCoord.h"
#include "nsMargin.h"
#include "nsRect.h"
#include "nsFont.h"
#include "nsStyleCoord.h"
#include "nsStyleConsts.h"
#include "nsChangeHint.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsIAtom.h"
#include "nsIURI.h"
#include "nsCSSValue.h"
#include "nsStyleTransformMatrix.h"

class nsIFrame;
class imgIRequest;


#include "nsStyleStructFwd.h"



#define NS_STYLE_INHERIT_MASK             0x00ffffff



#define NS_STYLE_HAS_TEXT_DECORATIONS     0x01000000

#define NS_STYLE_HAS_PSEUDO_ELEMENT_DATA  0x02000000


#define NS_RULE_NODE_GC_MARK              0x02000000
#define NS_RULE_NODE_IS_IMPORTANT         0x08000000
#define NS_RULE_NODE_LEVEL_MASK           0xf0000000
#define NS_RULE_NODE_LEVEL_SHIFT          28



struct nsStyleFont {
  nsStyleFont(const nsFont& aFont, nsPresContext *aPresContext);
  nsStyleFont(const nsStyleFont& aStyleFont);
  nsStyleFont(nsPresContext *aPresContext);
  ~nsStyleFont(void) {
    MOZ_COUNT_DTOR(nsStyleFont);
  }

  nsChangeHint CalcDifference(const nsStyleFont& aOther) const;
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif
  static nsChangeHint CalcFontDifference(const nsFont& aFont1, const nsFont& aFont2);

  static nscoord ZoomText(nsPresContext* aPresContext, nscoord aSize);
  static nscoord UnZoomText(nsPresContext* aPresContext, nscoord aSize);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW;
  void Destroy(nsPresContext* aContext);

  nsFont  mFont;        
  nscoord mSize;        
                        
                        
                        
                        
                        
  PRUint8 mGenericID;   
                        

#ifdef MOZ_MATHML
  
  PRInt8  mScriptLevel;          
  
  nscoord mScriptUnconstrainedSize;
  nscoord mScriptMinSize;        
  float   mScriptSizeMultiplier; 
#endif
};

struct nsStyleGradientStop {
  nscolor mColor;
  float mPosition; 
};

class nsStyleGradient {
public:
  nsStyleGradient();

  PRPackedBool mIsRadial;

  nsStyleCoord mStartX; 
  nsStyleCoord mStartY; 

  nsStyleCoord mEndX; 
  nsStyleCoord mEndY; 

  nscoord mStartRadius;
  nscoord mEndRadius;

  
  nsTArray<nsStyleGradientStop> mStops;

  nsrefcnt AddRef() {
    if (mRefCnt == PR_UINT32_MAX) {
      NS_WARNING("refcount overflow, leaking nsStyleGradient");
      return mRefCnt;
    }
    ++mRefCnt;
    NS_LOG_ADDREF(this, mRefCnt, "nsStyleGradient", sizeof(*this));
    return mRefCnt;
  }

  nsrefcnt Release() {
    if (mRefCnt == PR_UINT32_MAX) {
      NS_WARNING("refcount overflow, leaking nsStyleGradient");
      return mRefCnt;
    }
    --mRefCnt;
    NS_LOG_RELEASE(this, mRefCnt, "nsStyleGradient");
    if (mRefCnt == 0) {
      delete this;
      return 0;
    }
    return mRefCnt;
  }

  PRBool operator==(const nsStyleGradient& aOther) const;
  PRBool operator!=(const nsStyleGradient& aOther) const {
    return !(*this == aOther);
  };

private:
  nsrefcnt mRefCnt;

  ~nsStyleGradient() {}

  
  nsStyleGradient(const nsStyleGradient& aOther);
  nsStyleGradient& operator=(const nsStyleGradient& aOther);
};

enum nsStyleImageType {
  eStyleImageType_Null,
  eStyleImageType_Image,
  eStyleImageType_Gradient
};













struct nsStyleImage {
  nsStyleImage();
  ~nsStyleImage();
  nsStyleImage(const nsStyleImage& aOther);
  nsStyleImage& operator=(const nsStyleImage& aOther);

  void SetNull();
  void SetImageData(imgIRequest* aImage);
  void SetGradientData(nsStyleGradient* aGradient);
  void SetCropRect(nsStyleSides* aCropRect);

  nsStyleImageType GetType() const {
    return mType;
  }
  imgIRequest* GetImageData() const {
    NS_ASSERTION(mType == eStyleImageType_Image, "Data is not an image!");
    return mImage;
  }
  nsStyleGradient* GetGradientData() const {
    NS_ASSERTION(mType == eStyleImageType_Gradient, "Data is not a gradient!");
    return mGradient;
  }
  nsStyleSides* GetCropRect() const {
    NS_ASSERTION(mType == eStyleImageType_Image,
                 "Only image data can have a crop rect");
    return mCropRect;
  }

  








  PRBool ComputeActualCropRect(nsIntRect& aActualCropRect,
                               PRBool* aIsEntireImage = nsnull) const;

  


  nsresult RequestDecode();
  



  PRBool IsOpaque() const;
  



  PRBool IsComplete() const;
  



  PRBool IsEmpty() const {
    
    
    
    
    
    
    
    return mType == eStyleImageType_Null;
  }

  PRBool operator==(const nsStyleImage& aOther) const;
  PRBool operator!=(const nsStyleImage& aOther) const {
    return !(*this == aOther);
  }

private:
  void DoCopy(const nsStyleImage& aOther);

  nsStyleImageType mType;
  union {
    imgIRequest* mImage;
    nsStyleGradient* mGradient;
  };
  
  nsAutoPtr<nsStyleSides> mCropRect;
};

struct nsStyleColor {
  nsStyleColor(nsPresContext* aPresContext);
  nsStyleColor(const nsStyleColor& aOther);
  ~nsStyleColor(void) {
    MOZ_COUNT_DTOR(nsStyleColor);
  }

  nsChangeHint CalcDifference(const nsStyleColor& aOther) const;
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif
  
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
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleBackground();
    aContext->FreeToShell(sizeof(nsStyleBackground), this);
  }

  nsChangeHint CalcDifference(const nsStyleBackground& aOther) const;
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif

  struct Position;
  friend struct Position;
  struct Position {
    typedef union {
      nscoord mCoord; 
      float   mFloat; 
    } PositionCoord;
    PositionCoord mXPosition, mYPosition;
    PRPackedBool mXIsPercent, mYIsPercent;

    
    Position() {}

    
    void SetInitialValues();

    PRBool operator==(const Position& aOther) const {
      return mXIsPercent == aOther.mXIsPercent &&
             (mXIsPercent ? (mXPosition.mFloat == aOther.mXPosition.mFloat)
                          : (mXPosition.mCoord == aOther.mXPosition.mCoord)) &&
             mYIsPercent == aOther.mYIsPercent &&
             (mYIsPercent ? (mYPosition.mFloat == aOther.mYPosition.mFloat)
                          : (mYPosition.mCoord == aOther.mYPosition.mCoord));
    }
    PRBool operator!=(const Position& aOther) const {
      return !(*this == aOther);
    }
  };

  struct Size;
  friend struct Size;
  struct Size {
    typedef union {
      nscoord mCoord; 
      float mFloat; 
    } Dimension;
    Dimension mWidth, mHeight;

    enum DimensionType {
      
      
      
      eContain, eCover,

      eAuto,
      ePercentage,
      eLength,
      eDimensionType_COUNT
    };
    PRUint8 mWidthType, mHeightType;

    
    Size() {}

    
    void SetInitialValues();

    PRBool operator==(const Size& aOther) const;
    PRBool operator!=(const Size& aOther) const {
      return !(*this == aOther);
    }
  };

  struct Layer;
  friend struct Layer;
  struct Layer {
    PRUint8 mAttachment;                
    PRUint8 mClip;                      
    PRUint8 mOrigin;                    
    PRUint8 mRepeat;                    
    Position mPosition;                 
    nsStyleImage mImage;                
    Size mSize;                         

    
    Layer();
    ~Layer();

    void SetInitialValues();

    
    
    PRBool operator==(const Layer& aOther) const;
    PRBool operator!=(const Layer& aOther) const {
      return !(*this == aOther);
    }
  };

  
  
  PRUint32 mAttachmentCount,
           mClipCount,
           mOriginCount,
           mRepeatCount,
           mPositionCount,
           mImageCount,
           mSizeCount;
  
  
  
  
  
  
  
  
  
  
  nsAutoTArray<Layer, 1> mLayers;

  const Layer& BottomLayer() const { return mLayers[mImageCount - 1]; }

  #define NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(var_, stylebg_) \
    for (PRUint32 var_ = (stylebg_)->mImageCount; var_-- != 0; )

  nscolor mBackgroundColor;       

  
  
  
  PRUint8 mBackgroundInlinePolicy; 

  
  PRBool IsTransparent() const;

  
  
  
  
  PRBool HasFixedBackground() const;
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif

  nsStyleSides  mMargin;          

  PRBool GetMargin(nsMargin& aMargin) const
  {
    if (mHasCachedMargin) {
      aMargin = mCachedMargin;
      return PR_TRUE;
    }
    return PR_FALSE;
  }

protected:
  PRPackedBool  mHasCachedMargin;
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif
  
  nsStyleSides  mPadding;         

  PRBool GetPadding(nsMargin& aPadding) const
  {
    if (mHasCachedPadding) {
      aPadding = mCachedPadding;
      return PR_TRUE;
    }
    return PR_FALSE;
  }

protected:
  PRPackedBool  mHasCachedPadding;
  nsMargin      mCachedPadding;
};

struct nsBorderColors {
  nsBorderColors* mNext;
  nscolor mColor;

  nsBorderColors() : mNext(nsnull), mColor(NS_RGB(0,0,0)) {}
  nsBorderColors(const nscolor& aColor) : mNext(nsnull), mColor(aColor) {}
  ~nsBorderColors();

  nsBorderColors* Clone() const { return Clone(PR_TRUE); }

  static PRBool Equal(const nsBorderColors* c1,
                      const nsBorderColors* c2) {
    if (c1 == c2)
      return PR_TRUE;
    while (c1 && c2) {
      if (c1->mColor != c2->mColor)
        return PR_FALSE;
      c1 = c1->mNext;
      c2 = c2->mNext;
    }
    
    
    return !c1 && !c2;
  }

private:
  nsBorderColors* Clone(PRBool aDeep) const;
};

struct nsCSSShadowItem {
  nscoord mXOffset;
  nscoord mYOffset;
  nscoord mRadius;
  nscoord mSpread;

  nscolor      mColor;
  PRPackedBool mHasColor; 
  PRPackedBool mInset;

  nsCSSShadowItem() : mHasColor(PR_FALSE) {
    MOZ_COUNT_CTOR(nsCSSShadowItem);
  }
  ~nsCSSShadowItem() {
    MOZ_COUNT_DTOR(nsCSSShadowItem);
  }

  PRBool operator==(const nsCSSShadowItem& aOther) {
    return (mXOffset == aOther.mXOffset &&
            mYOffset == aOther.mYOffset &&
            mRadius == aOther.mRadius &&
            mHasColor == aOther.mHasColor &&
            mSpread == aOther.mSpread &&
            mInset == aOther.mInset &&
            (!mHasColor || mColor == aOther.mColor));
  }
  PRBool operator!=(const nsCSSShadowItem& aOther) {
    return !(*this == aOther);
  }
};

class nsCSSShadowArray {
  public:
    void* operator new(size_t aBaseSize, PRUint32 aArrayLen) {
      
      
      
      
      
      return ::operator new(aBaseSize +
                            (aArrayLen - 1) * sizeof(nsCSSShadowItem));
    }

    nsCSSShadowArray(PRUint32 aArrayLen) :
      mLength(aArrayLen), mRefCnt(0)
    {
      MOZ_COUNT_CTOR(nsCSSShadowArray);
      for (PRUint32 i = 1; i < mLength; ++i) {
        
        
        new (&mArray[i]) nsCSSShadowItem();
      }
    }
    ~nsCSSShadowArray() {
      MOZ_COUNT_DTOR(nsCSSShadowArray);
      for (PRUint32 i = 1; i < mLength; ++i) {
        mArray[i].~nsCSSShadowItem();
      }
    }

    nsrefcnt AddRef() {
      if (mRefCnt == PR_UINT32_MAX) {
        NS_WARNING("refcount overflow, leaking object");
        return mRefCnt;
      }
      return ++mRefCnt;
    }
    nsrefcnt Release();

    PRUint32 Length() const { return mLength; }
    nsCSSShadowItem* ShadowAt(PRUint32 i) {
      NS_ABORT_IF_FALSE(i < mLength, "Accessing too high an index in the text shadow array!");
      return &mArray[i];
    }
    const nsCSSShadowItem* ShadowAt(PRUint32 i) const {
      NS_ABORT_IF_FALSE(i < mLength, "Accessing too high an index in the text shadow array!");
      return &mArray[i];
    }

  private:
    PRUint32 mLength;
    PRUint32 mRefCnt;
    nsCSSShadowItem mArray[1]; 
};




#define NS_ROUND_BORDER_TO_PIXELS(l,tpp) \
  ((l) == 0) ? 0 : PR_MAX((tpp), (l) / (tpp) * (tpp))



#define NS_ROUND_OFFSET_TO_PIXELS(l,tpp) \
  (((l) == 0) ? 0 : \
    ((l) > 0) ? PR_MAX( (tpp), ((l) + ((tpp) / 2)) / (tpp) * (tpp)) : \
                PR_MIN(-(tpp), ((l) - ((tpp) / 2)) / (tpp) * (tpp)))


static PRBool IsVisibleBorderStyle(PRUint8 aStyle)
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif
  PRBool ImageBorderDiffers() const;
 
  nsStyleCorners mBorderRadius;    
  nsStyleSides  mBorderImageSplit; 
  PRUint8       mFloatEdge;       
  PRUint8       mBorderImageHFill; 
  PRUint8       mBorderImageVFill; 
  nsBorderColors** mBorderColors; 
  nsRefPtr<nsCSSShadowArray> mBoxShadow; 
  PRBool        mHaveBorderImageWidth; 
  nsMargin      mBorderImageWidth; 
  
  void EnsureBorderColors() {
    if (!mBorderColors) {
      mBorderColors = new nsBorderColors*[4];
      if (mBorderColors)
        for (PRInt32 i = 0; i < 4; i++)
          mBorderColors[i] = nsnull;
    }
  }

  void ClearBorderColors(PRUint8 aSide) {
    if (mBorderColors && mBorderColors[aSide]) {
      delete mBorderColors[aSide];
      mBorderColors[aSide] = nsnull;
    }
  }

  
  
  
  
  
  PRBool HasVisibleStyle(PRUint8 aSide)
  {
    return IsVisibleBorderStyle(GetBorderStyle(aSide));
  }

  
  void SetBorderWidth(PRUint8 aSide, nscoord aBorderWidth)
  {
    nscoord roundedWidth =
      NS_ROUND_BORDER_TO_PIXELS(aBorderWidth, mTwipsPerPixel);
    mBorder.side(aSide) = roundedWidth;
    if (HasVisibleStyle(aSide))
      mComputedBorder.side(aSide) = roundedWidth;
  }

  void SetBorderImageWidthOverride(PRUint8 aSide, nscoord aBorderWidth)
  {
    mBorderImageWidth.side(aSide) =
      NS_ROUND_BORDER_TO_PIXELS(aBorderWidth, mTwipsPerPixel);
  }

  
  
  
  
  
  const nsMargin& GetActualBorder() const;
  
  
  
  
  const nsMargin& GetComputedBorder() const
  {
    return mComputedBorder;
  }

  
  
  
  
  nscoord GetActualBorderWidth(PRUint8 aSide) const
  {
    return GetActualBorder().side(aSide);
  }

  PRUint8 GetBorderStyle(PRUint8 aSide) const
  {
    NS_ASSERTION(aSide <= NS_SIDE_LEFT, "bad side"); 
    return (mBorderStyle[aSide] & BORDER_STYLE_MASK); 
  }

  void SetBorderStyle(PRUint8 aSide, PRUint8 aStyle)
  {
    NS_ASSERTION(aSide <= NS_SIDE_LEFT, "bad side"); 
    mBorderStyle[aSide] &= ~BORDER_STYLE_MASK; 
    mBorderStyle[aSide] |= (aStyle & BORDER_STYLE_MASK);
    mComputedBorder.side(aSide) =
      (HasVisibleStyle(aSide) ? mBorder.side(aSide) : 0);
  }

  
  inline PRBool IsBorderImageLoaded() const;
  inline nsresult RequestDecode();

  void GetBorderColor(PRUint8 aSide, nscolor& aColor,
                      PRBool& aForeground) const
  {
    aForeground = PR_FALSE;
    NS_ASSERTION(aSide <= NS_SIDE_LEFT, "bad side"); 
    if ((mBorderStyle[aSide] & BORDER_COLOR_SPECIAL) == 0)
      aColor = mBorderColor[aSide]; 
    else if (mBorderStyle[aSide] & BORDER_COLOR_FOREGROUND)
      aForeground = PR_TRUE;
    else
      NS_NOTREACHED("OUTLINE_COLOR_INITIAL should not be set here");
  }

  void SetBorderColor(PRUint8 aSide, nscolor aColor) 
  {
    NS_ASSERTION(aSide <= NS_SIDE_LEFT, "bad side"); 
    mBorderColor[aSide] = aColor; 
    mBorderStyle[aSide] &= ~BORDER_COLOR_SPECIAL;
  }

  
  inline void SetBorderImage(imgIRequest* aImage);
  inline imgIRequest* GetBorderImage() const;

  void GetCompositeColors(PRInt32 aIndex, nsBorderColors** aColors) const
  {
    if (!mBorderColors)
      *aColors = nsnull;
    else
      *aColors = mBorderColors[aIndex];
  }

  void AppendBorderColor(PRInt32 aIndex, nscolor aColor)
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

  void SetBorderToForeground(PRUint8 aSide)
  {
    NS_ASSERTION(aSide <= NS_SIDE_LEFT, "bad side"); 
    mBorderStyle[aSide] &= ~BORDER_COLOR_SPECIAL;
    mBorderStyle[aSide] |= BORDER_COLOR_FOREGROUND; 
  }

protected:
  
  
  
  
  
  
  
  nsMargin      mComputedBorder;

  
  
  
  
  
  
  
  
  
  
  
  nsMargin      mBorder;

  PRUint8       mBorderStyle[4];  
  nscolor       mBorderColor[4];  
                                  

  nsCOMPtr<imgIRequest> mBorderImage; 

  nscoord       mTwipsPerPixel;
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif
 
  nsStyleCorners  mOutlineRadius; 

  
  
  nsStyleCoord  mOutlineWidth;    
  nscoord       mOutlineOffset;   

  PRBool GetOutlineWidth(nscoord& aWidth) const
  {
    if (mHasCachedOutline) {
      aWidth = mCachedOutlineWidth;
      return PR_TRUE;
    }
    return PR_FALSE;
  }

  PRUint8 GetOutlineStyle(void) const
  {
    return (mOutlineStyle & BORDER_STYLE_MASK);
  }

  void SetOutlineStyle(PRUint8 aStyle)
  {
    mOutlineStyle &= ~BORDER_STYLE_MASK;
    mOutlineStyle |= (aStyle & BORDER_STYLE_MASK);
  }

  
  PRBool GetOutlineColor(nscolor& aColor) const
  {
    if ((mOutlineStyle & BORDER_COLOR_SPECIAL) == 0) {
      aColor = mOutlineColor;
      return PR_TRUE;
    }
    return PR_FALSE;
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

  PRBool GetOutlineInitialColor() const
  {
    return !!(mOutlineStyle & OUTLINE_COLOR_INITIAL);
  }

protected:
  
  
  nscoord       mCachedOutlineWidth;

  nscolor       mOutlineColor;    

  PRPackedBool  mHasCachedOutline;
  PRUint8       mOutlineStyle;    

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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif
  
  PRUint8   mListStyleType;             
  PRUint8   mListStylePosition;         
  nsCOMPtr<imgIRequest> mListStyleImage; 
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif
  
  nsStyleSides  mOffset;                
  nsStyleCoord  mWidth;                 
  nsStyleCoord  mMinWidth;              
  nsStyleCoord  mMaxWidth;              
  nsStyleCoord  mHeight;                
  nsStyleCoord  mMinHeight;             
  nsStyleCoord  mMaxHeight;             
  PRUint8       mBoxSizing;             
  nsStyleCoord  mZIndex;                
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

  nsChangeHint CalcDifference(const nsStyleTextReset& aOther) const;
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif
  
  PRUint8 mTextDecoration;              
  PRUint8 mUnicodeBidi;                 

  nsStyleCoord  mVerticalAlign;         
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif

  PRUint8 mTextAlign;                   
  PRUint8 mTextTransform;               
  PRUint8 mWhiteSpace;                  
  PRUint8 mWordWrap;                    

  nsStyleCoord  mLetterSpacing;         
  nsStyleCoord  mLineHeight;            
  nsStyleCoord  mTextIndent;            
  nscoord mWordSpacing;                 

  nsRefPtr<nsCSSShadowArray> mTextShadow; 
  
  PRBool WhiteSpaceIsSignificant() const {
    return mWhiteSpace == NS_STYLE_WHITESPACE_PRE ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_WRAP;
  }

  PRBool NewlineIsSignificant() const {
    return mWhiteSpace == NS_STYLE_WHITESPACE_PRE ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_WRAP ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_LINE;
  }

  PRBool WhiteSpaceCanWrap() const {
    return mWhiteSpace == NS_STYLE_WHITESPACE_NORMAL ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_WRAP ||
           mWhiteSpace == NS_STYLE_WHITESPACE_PRE_LINE;
  }

  PRBool WordCanWrap() const {
    return WhiteSpaceCanWrap() && mWordWrap == NS_STYLE_WORDWRAP_BREAK_WORD;
  }
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif
  
  PRUint8 mDirection;                  
  PRUint8   mVisible;                  
  nsCOMPtr<nsIAtom> mLangGroup;        
  PRUint8 mPointerEvents;              

  PRBool IsVisible() const {
    return (mVisible == NS_STYLE_VISIBILITY_VISIBLE);
  }

  PRBool IsVisibleOrCollapsed() const {
    return ((mVisible == NS_STYLE_VISIBILITY_VISIBLE) ||
            (mVisible == NS_STYLE_VISIBILITY_COLLAPSE));
  }
};

struct nsTimingFunction {
  explicit nsTimingFunction(PRInt32 aTimingFunctionType
                              = NS_STYLE_TRANSITION_TIMING_FUNCTION_EASE)
  {
    AssignFromKeyword(aTimingFunctionType);
  }

  nsTimingFunction(float x1, float y1, float x2, float y2)
    : mX1(x1)
    , mY1(y1)
    , mX2(x2)
    , mY2(y2)
  {}

  float mX1;
  float mY1;
  float mX2;
  float mY2;

  PRBool operator==(const nsTimingFunction& aOther) const
  {
    return !(*this != aOther);
  }

  PRBool operator!=(const nsTimingFunction& aOther) const
  {
    return mX1 != aOther.mX1 || mY1 != aOther.mY1 ||
           mX2 != aOther.mX2 || mY2 != aOther.mY2;
  }

private:
  void AssignFromKeyword(PRInt32 aTimingFunctionType);
};

struct nsTransition {
  nsTransition() {  }
  explicit nsTransition(const nsTransition& aCopy);

  void SetInitialValues();

  

  nsTimingFunction& GetTimingFunction() { return mTimingFunction; }
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

private:
  nsTimingFunction mTimingFunction;
  float mDuration;
  float mDelay;
  nsCSSProperty mProperty;
  nsCOMPtr<nsIAtom> mUnknownProperty; 
                                      
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif

  
  
  nsRefPtr<nsCSSValue::URL> mBinding;    
#if 0
  
  
  
  nsMargin  mClip;              
#else
  nsRect    mClip;              
#endif
  float   mOpacity;             
  PRUint8 mDisplay;             
  PRUint8 mOriginalDisplay;     
  PRUint8 mAppearance;          
  PRUint8 mPosition;            
  PRUint8 mFloats;              
  PRUint8 mBreakType;           
  PRPackedBool mBreakBefore;    
  PRPackedBool mBreakAfter;     
  PRUint8 mOverflowX;           
  PRUint8 mOverflowY;           
  PRUint8   mClipFlags;         
  PRPackedBool mTransformPresent;  
  nsStyleTransformMatrix mTransform; 
  nsStyleCoord mTransformOrigin[2]; 
  nsAutoTArray<nsTransition, 1> mTransitions; 
  
  
  PRUint32 mTransitionTimingFunctionCount,
           mTransitionDurationCount,
           mTransitionDelayCount,
           mTransitionPropertyCount;

  PRBool IsBlockInside() const {
    return NS_STYLE_DISPLAY_BLOCK == mDisplay ||
           NS_STYLE_DISPLAY_LIST_ITEM == mDisplay ||
           NS_STYLE_DISPLAY_INLINE_BLOCK == mDisplay;
    
    
    
  }

  PRBool IsBlockOutside() const {
    return NS_STYLE_DISPLAY_BLOCK == mDisplay ||
           NS_STYLE_DISPLAY_LIST_ITEM == mDisplay ||
           NS_STYLE_DISPLAY_TABLE == mDisplay;
  }

  PRBool IsInlineOutside() const {
    return NS_STYLE_DISPLAY_INLINE == mDisplay ||
           NS_STYLE_DISPLAY_INLINE_BLOCK == mDisplay ||
           NS_STYLE_DISPLAY_INLINE_TABLE == mDisplay ||
           NS_STYLE_DISPLAY_INLINE_BOX == mDisplay ||
           NS_STYLE_DISPLAY_INLINE_GRID == mDisplay ||
           NS_STYLE_DISPLAY_INLINE_STACK == mDisplay;
  }

  PRBool IsFloating() const {
    return NS_STYLE_FLOAT_NONE != mFloats;
  }

  PRBool IsAbsolutelyPositioned() const {return (NS_STYLE_POSITION_ABSOLUTE == mPosition) ||
                                                (NS_STYLE_POSITION_FIXED == mPosition);}

  
  PRBool IsPositioned() const {
    return IsAbsolutelyPositioned() ||
      NS_STYLE_POSITION_RELATIVE == mPosition || mTransformPresent;
  }

  PRBool IsScrollableOverflow() const {
    
    
    return mOverflowX != NS_STYLE_OVERFLOW_VISIBLE &&
           mOverflowX != NS_STYLE_OVERFLOW_CLIP;
  }

  
  
  PRBool IsTableClip() const {
    return mOverflowX == NS_STYLE_OVERFLOW_CLIP ||
           (mOverflowX == NS_STYLE_OVERFLOW_HIDDEN &&
            mOverflowY == NS_STYLE_OVERFLOW_HIDDEN);
  }

  
  PRBool HasTransform() const {
    return mTransformPresent;
  }
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif
  
  PRUint8       mLayoutStrategy;
  PRUint8       mFrame;         
  PRUint8       mRules;         
  PRInt32       mCols;          
  PRInt32       mSpan;          
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif
  
  nscoord       mBorderSpacingX;
  nscoord       mBorderSpacingY;
  PRUint8       mBorderCollapse;
  PRUint8       mCaptionSide;   
  PRUint8       mEmptyCells;    
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
    imgIRequest *mImage;
    nsCSSValue::Array* mCounters;
  } mContent;

  nsStyleContentData() : mType(nsStyleContentType(0)) { mContent.mString = nsnull; }
  ~nsStyleContentData();
  nsStyleContentData& operator=(const nsStyleContentData& aOther);
  PRBool operator==(const nsStyleContentData& aOther) const;

  PRBool operator!=(const nsStyleContentData& aOther) const {
    return !(*this == aOther);
  }
private:
  nsStyleContentData(const nsStyleContentData&); 
};

struct nsStyleCounterData {
  nsString  mCounter;
  PRInt32   mValue;
};


#define DELETE_ARRAY_IF(array)  if (array) { delete[] array; array = nsnull; }

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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif
  
  PRUint32  QuotesCount(void) const { return mQuotesCount; } 

  const nsString* OpenQuoteAt(PRUint32 aIndex) const
  {
    NS_ASSERTION(aIndex < mQuotesCount, "out of range");
    return mQuotes + (aIndex * 2);
  }
  const nsString* CloseQuoteAt(PRUint32 aIndex) const
  {
    NS_ASSERTION(aIndex < mQuotesCount, "out of range");
    return mQuotes + (aIndex * 2 + 1);
  }
  nsresult  GetQuotesAt(PRUint32 aIndex, nsString& aOpen, nsString& aClose) const {
    if (aIndex < mQuotesCount) {
      aIndex *= 2;
      aOpen = mQuotes[aIndex];
      aClose = mQuotes[++aIndex];
      return NS_OK;
    }
    return NS_ERROR_ILLEGAL_VALUE;
  }

  nsresult  AllocateQuotes(PRUint32 aCount) {
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

  nsresult  SetQuotesAt(PRUint32 aIndex, const nsString& aOpen, const nsString& aClose) {
    if (aIndex < mQuotesCount) {
      aIndex *= 2;
      mQuotes[aIndex] = aOpen;
      mQuotes[++aIndex] = aClose;
      return NS_OK;
    }
    return NS_ERROR_ILLEGAL_VALUE;
  }

protected:
  PRUint32            mQuotesCount;
  nsString*           mQuotes;
};

struct nsStyleContent {
  nsStyleContent(void);
  nsStyleContent(const nsStyleContent& aContent);
  ~nsStyleContent(void);

  void* operator new(size_t sz, nsPresContext* aContext) CPP_THROW_NEW {
    return aContext->AllocateFromShell(sz);
  }
  void Destroy(nsPresContext* aContext) {
    this->~nsStyleContent();
    aContext->FreeToShell(sizeof(nsStyleContent), this);
  }

  nsChangeHint CalcDifference(const nsStyleContent& aOther) const;
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif

  PRUint32  ContentCount(void) const  { return mContentCount; } 

  const nsStyleContentData& ContentAt(PRUint32 aIndex) const {
    NS_ASSERTION(aIndex < mContentCount, "out of range");
    return mContents[aIndex];
  }

  nsStyleContentData& ContentAt(PRUint32 aIndex) {
    NS_ASSERTION(aIndex < mContentCount, "out of range");
    return mContents[aIndex];
  }

  nsresult AllocateContents(PRUint32 aCount);

  PRUint32  CounterIncrementCount(void) const { return mIncrementCount; }  
  const nsStyleCounterData* GetCounterIncrementAt(PRUint32 aIndex) const {
    NS_ASSERTION(aIndex < mIncrementCount, "out of range");
    return &mIncrements[aIndex];
  }

  nsresult  AllocateCounterIncrements(PRUint32 aCount) {
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

  nsresult  SetCounterIncrementAt(PRUint32 aIndex, const nsString& aCounter, PRInt32 aIncrement) {
    if (aIndex < mIncrementCount) {
      mIncrements[aIndex].mCounter = aCounter;
      mIncrements[aIndex].mValue = aIncrement;
      return NS_OK;
    }
    return NS_ERROR_ILLEGAL_VALUE;
  }

  PRUint32  CounterResetCount(void) const { return mResetCount; }  
  const nsStyleCounterData* GetCounterResetAt(PRUint32 aIndex) const {
    NS_ASSERTION(aIndex < mResetCount, "out of range");
    return &mResets[aIndex];
  }

  nsresult  AllocateCounterResets(PRUint32 aCount) {
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

  nsresult  SetCounterResetAt(PRUint32 aIndex, const nsString& aCounter, PRInt32 aValue) {
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

  PRUint32            mContentCount;
  PRUint32            mIncrementCount;
  PRUint32            mResetCount;
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif

  PRUint8   mUserSelect;      
  PRUint8   mForceBrokenImageIcon; 
  PRUint8   mIMEMode;         
  PRUint8   mWindowShadow;    
};

struct nsCursorImage {
  nsCOMPtr<imgIRequest> mImage;
  PRBool mHaveHotspot;
  float mHotspotX, mHotspotY;

  nsCursorImage();
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif

  PRUint8   mUserInput;       
  PRUint8   mUserModify;      
  PRUint8   mUserFocus;       
  
  PRUint8   mCursor;          

  PRUint32 mCursorArrayLength;
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif
  
  float         mBoxFlex;               
  PRUint32      mBoxOrdinal;            
  PRUint8       mBoxAlign;              
  PRUint8       mBoxDirection;          
  PRUint8       mBoxOrient;             
  PRUint8       mBoxPack;               
  PRPackedBool  mStretchStack;          
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif

  PRUint32     mColumnCount; 
  nsStyleCoord mColumnWidth; 
  nsStyleCoord mColumnGap;   

  nscolor      mColumnRuleColor;  
  PRUint8      mColumnRuleStyle;  
  
  
  PRPackedBool mColumnRuleColorIsForeground;

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

#ifdef MOZ_SVG
enum nsStyleSVGPaintType {
  eStyleSVGPaintType_None = 1,
  eStyleSVGPaintType_Color,
  eStyleSVGPaintType_Server
};

struct nsStyleSVGPaint
{
  union {
    nscolor mColor;
    nsIURI *mPaintServer;
  } mPaint;
  nsStyleSVGPaintType mType;
  nscolor mFallbackColor;

  nsStyleSVGPaint() : mType(nsStyleSVGPaintType(0)) { mPaint.mPaintServer = nsnull; }
  ~nsStyleSVGPaint();
  void SetType(nsStyleSVGPaintType aType);
  nsStyleSVGPaint& operator=(const nsStyleSVGPaint& aOther);
  PRBool operator==(const nsStyleSVGPaint& aOther) const; 

  PRBool operator!=(const nsStyleSVGPaint& aOther) const {
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif

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

  PRUint32         mStrokeDasharrayLength;
  PRUint8          mClipRule;         
  PRUint8          mColorInterpolation; 
  PRUint8          mColorInterpolationFilters; 
  PRUint8          mFillRule;         
  PRUint8          mImageRendering;   
  PRUint8          mShapeRendering;   
  PRUint8          mStrokeLinecap;    
  PRUint8          mStrokeLinejoin;   
  PRUint8          mTextAnchor;       
  PRUint8          mTextRendering;    
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
#ifdef DEBUG
  static nsChangeHint MaxDifference();
#endif

  nsCOMPtr<nsIURI> mClipPath;         
  nsCOMPtr<nsIURI> mFilter;           
  nsCOMPtr<nsIURI> mMask;             
  nscolor          mStopColor;        
  nscolor          mFloodColor;       
  nscolor          mLightingColor;    

  float            mStopOpacity;      
  float            mFloodOpacity;     

  PRUint8          mDominantBaseline; 
};
#endif

#endif 
