








































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
  PRUnichar code; 
  PRInt32   font;

  PRBool Exists() const
  {
    return (code != 0);
  }
  PRBool operator==(const nsGlyphCode& other) const
  {
    return other.code == code && other.font == font;
  }
  PRBool operator!=(const nsGlyphCode& other) const
  {
    return ! operator==(other);
  }
};











class nsMathMLChar
{
public:
  
  nsMathMLChar(nsMathMLChar* aParent = nsnull) {
    MOZ_COUNT_CTOR(nsMathMLChar);
    mStyleContext = nsnull;
    mSibling = nsnull;
    mParent = aParent;
    mUnscaledAscent = 0;
    mScaleX = mScaleY = 1.0;
    mDrawNormal = PR_TRUE;
  }

  ~nsMathMLChar() { 
    MOZ_COUNT_DTOR(nsMathMLChar);
    
    
    if (!mParent && mStyleContext) { 
      mStyleContext->Release();
    }
    if (mSibling) {
      delete mSibling;
    }
  }

  nsresult
  Display(nsDisplayListBuilder*   aBuilder,
          nsIFrame*               aForFrame,
          const nsDisplayListSet& aLists,
          const nsRect*           aSelectedRect = nsnull);
          
  void PaintForeground(nsPresContext* aPresContext,
                       nsRenderingContext& aRenderingContext,
                       nsPoint aPt,
                       PRBool aIsSelected);

  
  
  
  nsresult
  Stretch(nsPresContext*           aPresContext,
          nsRenderingContext&     aRenderingContext,
          nsStretchDirection       aStretchDirection,
          const nsBoundingMetrics& aContainerSize,
          nsBoundingMetrics&       aDesiredStretchSize,
          PRUint32                 aStretchHint = NS_STRETCH_NORMAL);

  void
  SetData(nsPresContext* aPresContext,
          nsString&       aData);

  void
  GetData(nsString& aData) {
    aData = mData;
  }

  PRInt32
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
    
    if (!mParent && mSibling) { 
      for (nsMathMLChar* child = mSibling; child; child = child->mSibling) {
        nsRect rect; 
        child->GetRect(rect);
        rect.MoveBy(mRect.x, mRect.y);
        child->SetRect(rect);
      }
    }
  }

  
  
  
  
  
  
  
  
  nscoord
  GetMaxWidth(nsPresContext* aPresContext,
              nsRenderingContext& aRenderingContext,
              PRUint32 aStretchHint = NS_STRETCH_NORMAL,
              float aMaxSize = NS_MATHML_OPERATOR_SIZE_INFINITY,
              
              
              
              PRBool aMaxSizeIsAbsolute = PR_FALSE);

  
  
  
  
  
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

  
  nsMathMLChar*      mSibling;
  nsMathMLChar*      mParent;

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
  
  PRPackedBool       mDrawNormal;

  class StretchEnumContext;
  friend class StretchEnumContext;

  
  nsresult
  StretchInternal(nsPresContext*           aPresContext,
                  nsRenderingContext&     aRenderingContext,
                  nsStretchDirection&      aStretchDirection,
                  const nsBoundingMetrics& aContainerSize,
                  nsBoundingMetrics&       aDesiredStretchSize,
                  PRUint32                 aStretchHint,
                  float           aMaxSize = NS_MATHML_OPERATOR_SIZE_INFINITY,
                  PRBool          aMaxSizeIsAbsolute = PR_FALSE);

  nsresult
  ComposeChildren(nsPresContext*       aPresContext,
                  nsRenderingContext& aRenderingContext,
                  nsGlyphTable*        aGlyphTable,
                  nscoord              aTargetSize,
                  nsBoundingMetrics&   aCompositeSize,
                  PRUint32             aStretchHint);

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
