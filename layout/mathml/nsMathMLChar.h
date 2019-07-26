




#ifndef nsMathMLChar_h___
#define nsMathMLChar_h___

#include "nsMathMLOperators.h"
#include "nsMathMLFrame.h"

class nsGlyphTable;


enum {
  
  NS_STRETCH_NONE     = 0x00,
  
  NS_STRETCH_VARIABLE_MASK = 0x0F,
  NS_STRETCH_NORMAL   = 0x01, 
  NS_STRETCH_NEARER   = 0x02, 
  NS_STRETCH_SMALLER  = 0x04, 
  NS_STRETCH_LARGER   = 0x08, 
  
  NS_STRETCH_LARGEOP  = 0x10,
  NS_STRETCH_INTEGRAL  = 0x20,

  
  
  NS_STRETCH_MAXWIDTH = 0x40
};






struct nsGlyphCode {
  PRUnichar code[2]; 
  int32_t   font;

  int32_t Length() { return (code[1] == PRUnichar('\0') ? 1 : 2); }
  bool Exists() const
  {
    return (code[0] != 0);
  }
  bool operator==(const nsGlyphCode& other) const
  {
    return (other.code[0] == code[0] && other.code[1] == code[1] && 
            other.font == font);
  }
  bool operator!=(const nsGlyphCode& other) const
  {
    return ! operator==(other);
  }
};



class nsMathMLChar
{
public:
  
  nsMathMLChar() {
    MOZ_COUNT_CTOR(nsMathMLChar);
    mStyleContext = nullptr;
    mUnscaledAscent = 0;
    mScaleX = mScaleY = 1.0;
    mDrawNormal = true;
    mMirrored = false;
  }

  
  ~nsMathMLChar() {
    MOZ_COUNT_DTOR(nsMathMLChar);
    mStyleContext->Release();
  }

  void Display(nsDisplayListBuilder*   aBuilder,
               nsIFrame*               aForFrame,
               const nsDisplayListSet& aLists,
               uint32_t                aIndex,
               const nsRect*           aSelectedRect = nullptr);
          
  void PaintForeground(nsPresContext* aPresContext,
                       nsRenderingContext& aRenderingContext,
                       nsPoint aPt,
                       bool aIsSelected);

  
  
  
  nsresult
  Stretch(nsPresContext*           aPresContext,
          nsRenderingContext&     aRenderingContext,
          nsStretchDirection       aStretchDirection,
          const nsBoundingMetrics& aContainerSize,
          nsBoundingMetrics&       aDesiredStretchSize,
          uint32_t                 aStretchHint,
          bool                     aRTL);

  void
  SetData(nsPresContext* aPresContext,
          nsString&       aData);

  void
  GetData(nsString& aData) {
    aData = mData;
  }

  int32_t
  Length() {
    return mData.Length();
  }

  nsStretchDirection
  GetStretchDirection() {
    return mDirection;
  }

  
  
  const PRUnichar*
  get() {
    return mData.get();
  }

  void
  GetRect(nsRect& aRect) {
    aRect = mRect;
  }

  void
  SetRect(const nsRect& aRect) {
    mRect = aRect;
  }

  
  
  
  
  
  
  
  
  nscoord
  GetMaxWidth(nsPresContext* aPresContext,
              nsRenderingContext& aRenderingContext,
              uint32_t aStretchHint = NS_STRETCH_NORMAL,
              float aMaxSize = NS_MATHML_OPERATOR_SIZE_INFINITY,
              
              
              
              bool aMaxSizeIsAbsolute = false);

  
  
  
  
  
  void
  GetBoundingMetrics(nsBoundingMetrics& aBoundingMetrics) {
    aBoundingMetrics = mBoundingMetrics;
  }

  void
  SetBoundingMetrics(nsBoundingMetrics& aBoundingMetrics) {
    mBoundingMetrics = aBoundingMetrics;
  }

  
  
  
  
  nsStyleContext* GetStyleContext() const;

  void SetStyleContext(nsStyleContext* aStyleContext);

protected:
  friend class nsGlyphTable;
  nsString           mData;

private:
  nsRect             mRect;
  nsStretchDirection mDirection;
  nsBoundingMetrics  mBoundingMetrics;
  nsStyleContext*    mStyleContext;
  nsGlyphTable*      mGlyphTable;
  nsGlyphCode        mGlyph;
  
  
  nsString           mFamily;
  
  nscoord            mUnscaledAscent;
  
  float              mScaleX, mScaleY;
  
  bool               mDrawNormal;
  
  bool               mMirrored;

  class StretchEnumContext;
  friend class StretchEnumContext;

  
  nsresult
  StretchInternal(nsPresContext*           aPresContext,
                  nsRenderingContext&     aRenderingContext,
                  nsStretchDirection&      aStretchDirection,
                  const nsBoundingMetrics& aContainerSize,
                  nsBoundingMetrics&       aDesiredStretchSize,
                  uint32_t                 aStretchHint,
                  float           aMaxSize = NS_MATHML_OPERATOR_SIZE_INFINITY,
                  bool            aMaxSizeIsAbsolute = false);

  nsresult
  PaintVertically(nsPresContext*       aPresContext,
                  nsRenderingContext& aRenderingContext,
                  nsFont&              aFont,
                  nsStyleContext*      aStyleContext,
                  nsGlyphTable*        aGlyphTable,
                  nsRect&              aRect);

  nsresult
  PaintHorizontally(nsPresContext*       aPresContext,
                    nsRenderingContext& aRenderingContext,
                    nsFont&              aFont,
                    nsStyleContext*      aStyleContext,
                    nsGlyphTable*        aGlyphTable,
                    nsRect&              aRect);

  void
  ApplyTransforms(nsRenderingContext& aRenderingContext, nsRect &r);
};

#endif 
