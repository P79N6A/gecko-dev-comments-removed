







































#ifndef nsMathMLChar_h___
#define nsMathMLChar_h___

#include "nsMathMLOperators.h"
#include "nsMathMLFrame.h"

class nsGlyphTable;


#define NS_STRETCH_NORMAL  0x00000001 // try to stretch to requested size - DEFAULT
#define NS_STRETCH_NEARER  0x00000002 // stretch very close to requested size
#define NS_STRETCH_SMALLER 0x00000004 // don't stretch more than requested size
#define NS_STRETCH_LARGER  0x00000008 // don't stretch less than requested size
#define NS_STRETCH_LARGEOP 0x00000010 // for a largeop in displaystyle





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
                       nsIRenderingContext& aRenderingContext,
                       nsPoint aPt,
                       PRBool aIsSelected);

  
  
  
  nsresult
  Stretch(nsPresContext*      aPresContext,
          nsIRenderingContext& aRenderingContext,
          nsStretchDirection   aStretchDirection,
          nsBoundingMetrics&   aContainerSize,
          nsBoundingMetrics&   aDesiredStretchSize,
          PRUint32             aStretchHint = NS_STRETCH_NORMAL);

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
  PRInt32            mOperator;
  nsStretchDirection mDirection;
  nsBoundingMetrics  mBoundingMetrics;
  nsStyleContext*    mStyleContext;
  nsGlyphTable*      mGlyphTable;
  nsGlyphCode        mGlyph;
  
  
  nsString           mFamily;

  
  PRBool
  TryVariants(nsPresContext*       aPresContext,
              nsIRenderingContext& aRenderingContext,
              nsGlyphTable*        aGlyphTable,
              nscoord              aTargetSize,
              PRUint32             aStretchHint,
              const nsAString&     aFamilies);

  PRBool
  TryParts(nsPresContext*       aPresContext,
           nsIRenderingContext& aRenderingContext,
           nsGlyphTable*        aGlyphTable,
           nscoord              aTargetSize,
           PRUint32             aStretchHint,
           const nsAString&     aFamilies);

  static PRBool
  StretchResolverCallback (const nsAString& aFamily, void *aData);

  static PRBool
  StretchEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData);

  nsresult
  ComposeChildren(nsPresContext*       aPresContext,
                  nsIRenderingContext& aRenderingContext,
                  nsGlyphTable*        aGlyphTable,
                  nscoord              aTargetSize,
                  nsBoundingMetrics&   aCompositeSize,
                  PRUint32             aStretchHint);

  nsresult
  PaintVertically(nsPresContext*       aPresContext,
                  nsIRenderingContext& aRenderingContext,
                  nsFont&              aFont,
                  nsStyleContext*      aStyleContext,
                  nsGlyphTable*        aGlyphTable,
                  nsRect&              aRect);

  nsresult
  PaintHorizontally(nsPresContext*       aPresContext,
                    nsIRenderingContext& aRenderingContext,
                    nsFont&              aFont,
                    nsStyleContext*      aStyleContext,
                    nsGlyphTable*        aGlyphTable,
                    nsRect&              aRect);
};

#endif 
