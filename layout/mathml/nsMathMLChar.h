




#ifndef nsMathMLChar_h___
#define nsMathMLChar_h___

#include "nsAutoPtr.h"
#include "nsColor.h"
#include "nsMathMLOperators.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsString.h"
#include "nsBoundingMetrics.h"
#include "gfxFont.h"

class nsGlyphTable;
class nsIFrame;
class nsDisplayListBuilder;
class nsDisplayListSet;
class nsPresContext;
class nsRenderingContext;
struct nsBoundingMetrics;
class nsStyleContext;
struct nsFont;


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
  union {
    char16_t code[2];
    uint32_t glyphID;
  };
  int8_t   font;

  bool IsGlyphID() const { return font == -1; }

  int32_t Length() const {
    return (IsGlyphID() || code[1] == PRUnichar('\0') ? 1 : 2);
  }
  bool Exists() const
  {
    return IsGlyphID() ? glyphID != 0 : code[0] != 0;
  }
  bool operator==(const nsGlyphCode& other) const
  {
    return (other.font == font &&
            ((IsGlyphID() && other.glyphID == glyphID) ||
             (!IsGlyphID() && other.code[0] == code[0] &&
              other.code[1] == code[1])));
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
    mDraw = DRAW_NORMAL;
    mMirrored = false;
  }

  
  ~nsMathMLChar();

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
          float                    aFontSizeInflation,
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

  
  
  const char16_t*
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
              float aFontSizeInflation,
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
  friend class nsPropertiesTable;
  friend class nsOpenTypeTable;
  nsString           mData;

private:
  nsRect             mRect;
  nsStretchDirection mDirection;
  nsBoundingMetrics  mBoundingMetrics;
  nsStyleContext*    mStyleContext;
  
  
  nsAutoPtr<gfxTextRun> mGlyphs[4];
  nsBoundingMetrics     mBmData[4];
  
  nscoord            mUnscaledAscent;
  
  float              mScaleX, mScaleY;

  
  
  
  
  
  enum DrawingMethod {
    DRAW_NORMAL, DRAW_VARIANT, DRAW_PARTS
  };
  DrawingMethod mDraw;

  
  bool               mMirrored;

  class StretchEnumContext;
  friend class StretchEnumContext;

  
  bool
  SetFontFamily(nsPresContext*          aPresContext,
                const nsGlyphTable*     aGlyphTable,
                const nsGlyphCode&      aGlyphCode,
                const mozilla::FontFamilyList& aDefaultFamily,
                nsFont&                 aFont,
                nsRefPtr<gfxFontGroup>* aFontGroup);

  nsresult
  StretchInternal(nsPresContext*           aPresContext,
                  gfxContext*              aThebesContext,
                  float                    aFontSizeInflation,
                  nsStretchDirection&      aStretchDirection,
                  const nsBoundingMetrics& aContainerSize,
                  nsBoundingMetrics&       aDesiredStretchSize,
                  uint32_t                 aStretchHint,
                  float           aMaxSize = NS_MATHML_OPERATOR_SIZE_INFINITY,
                  bool            aMaxSizeIsAbsolute = false);

  nsresult
  PaintVertically(nsPresContext* aPresContext,
                  gfxContext*    aThebesContext,
                  nsRect&        aRect,
                  nscolor        aColor);

  nsresult
  PaintHorizontally(nsPresContext* aPresContext,
                    gfxContext*    aThebesContext,
                    nsRect&        aRect,
                    nscolor        aColor);

  void
  ApplyTransforms(gfxContext* aThebesContext, int32_t aAppUnitsPerGfxUnit,
                  nsRect &r);
};

#endif 
