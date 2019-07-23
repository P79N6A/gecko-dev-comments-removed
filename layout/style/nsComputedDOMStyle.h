







































#ifndef nsComputedDOMStyle_h__
#define nsComputedDOMStyle_h__

#include "nsIComputedDOMStyle.h"

#include "nsROCSSPrimitiveValue.h"
#include "nsDOMCSSDeclaration.h"
#include "nsDOMCSSRGBColor.h"
#include "nsDOMCSSValueList.h"
#include "nsCSSProps.h"

#include "nsIPresShell.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"
#include "nsAutoPtr.h"

class nsComputedDOMStyle : public nsIComputedDOMStyle
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(nsIDOMElement *aElement,
                  const nsAString& aPseudoElt,
                  nsIPresShell *aPresShell);

  NS_DECL_NSICSSDECLARATION

  NS_DECL_NSIDOMCSSSTYLEDECLARATION

  nsComputedDOMStyle();
  virtual ~nsComputedDOMStyle();

  static void Shutdown();

private:
  void FlushPendingReflows();
  
#define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)                  \
  const nsStyle##name_ * GetStyle##name_() {                            \
    return mStyleContextHolder->GetStyle##name_();                      \
  }
#include "nsStyleStructList.h"
#undef STYLE_STRUCT

  nsresult GetOffsetWidthFor(PRUint8 aSide, nsIDOMCSSValue** aValue);

  nsresult GetAbsoluteOffset(PRUint8 aSide, nsIDOMCSSValue** aValue);

  nsresult GetRelativeOffset(PRUint8 aSide, nsIDOMCSSValue** aValue);

  nsresult GetStaticOffset(PRUint8 aSide, nsIDOMCSSValue** aValue);

  nsresult GetPaddingWidthFor(PRUint8 aSide, nsIDOMCSSValue** aValue);

  nsresult GetBorderColorsFor(PRUint8 aSide, nsIDOMCSSValue** aValue);

  nsresult GetBorderStyleFor(PRUint8 aSide, nsIDOMCSSValue** aValue);

  nsresult GetBorderRadiusFor(PRUint8 aSide, nsIDOMCSSValue** aValue);

  nsresult GetBorderWidthFor(PRUint8 aSide, nsIDOMCSSValue** aValue);

  nsresult GetBorderColorFor(PRUint8 aSide, nsIDOMCSSValue** aValue);

  nsresult GetOutlineRadiusFor(PRUint8 aSide, nsIDOMCSSValue** aValue);

  nsresult GetMarginWidthFor(PRUint8 aSide, nsIDOMCSSValue** aValue);

  PRBool GetLineHeightCoord(nscoord& aCoord);

  

  nsresult GetAppearance(nsIDOMCSSValue** aValue);

  
  nsresult GetBoxAlign(nsIDOMCSSValue** aValue);
  nsresult GetBoxDirection(nsIDOMCSSValue** aValue);
  nsresult GetBoxFlex(nsIDOMCSSValue** aValue);
  nsresult GetBoxOrdinalGroup(nsIDOMCSSValue** aValue);
  nsresult GetBoxOrient(nsIDOMCSSValue** aValue);
  nsresult GetBoxPack(nsIDOMCSSValue** aValue);
  nsresult GetBoxSizing(nsIDOMCSSValue** aValue);

  nsresult GetWidth(nsIDOMCSSValue** aValue);
  nsresult GetHeight(nsIDOMCSSValue** aValue);
  nsresult GetMaxHeight(nsIDOMCSSValue** aValue);
  nsresult GetMaxWidth(nsIDOMCSSValue** aValue);
  nsresult GetMinHeight(nsIDOMCSSValue** aValue);
  nsresult GetMinWidth(nsIDOMCSSValue** aValue);
  nsresult GetLeft(nsIDOMCSSValue** aValue);
  nsresult GetTop(nsIDOMCSSValue** aValue);
  nsresult GetRight(nsIDOMCSSValue** aValue);
  nsresult GetBottom(nsIDOMCSSValue** aValue);

  
  nsresult GetColor(nsIDOMCSSValue** aValue);
  nsresult GetFontFamily(nsIDOMCSSValue** aValue);
  nsresult GetFontStyle(nsIDOMCSSValue** aValue);
  nsresult GetFontSize(nsIDOMCSSValue** aValue);
  nsresult GetFontSizeAdjust(nsIDOMCSSValue** aValue);
  nsresult GetFontWeight(nsIDOMCSSValue** aValue);
  nsresult GetFontVariant(nsIDOMCSSValue** aValue);

  
  nsresult GetBackgroundAttachment(nsIDOMCSSValue** aValue);
  nsresult GetBackgroundColor(nsIDOMCSSValue** aValue);
  nsresult GetBackgroundImage(nsIDOMCSSValue** aValue);
  nsresult GetBackgroundPosition(nsIDOMCSSValue** aValue);
  nsresult GetBackgroundRepeat(nsIDOMCSSValue** aValue);
  nsresult GetBackgroundClip(nsIDOMCSSValue** aValue);
  nsresult GetBackgroundInlinePolicy(nsIDOMCSSValue** aValue);
  nsresult GetBackgroundOrigin(nsIDOMCSSValue** aValue);

  
  nsresult GetPadding(nsIDOMCSSValue** aValue);
  nsresult GetPaddingTop(nsIDOMCSSValue** aValue);
  nsresult GetPaddingBottom(nsIDOMCSSValue** aValue);
  nsresult GetPaddingLeft(nsIDOMCSSValue** aValue);
  nsresult GetPaddingRight(nsIDOMCSSValue** aValue);

  
  nsresult GetBorderCollapse(nsIDOMCSSValue** aValue);
  nsresult GetBorderSpacing(nsIDOMCSSValue** aValue);
  nsresult GetCaptionSide(nsIDOMCSSValue** aValue);
  nsresult GetEmptyCells(nsIDOMCSSValue** aValue);
  nsresult GetTableLayout(nsIDOMCSSValue** aValue);
  nsresult GetVerticalAlign(nsIDOMCSSValue** aValue);

  
  nsresult GetBorderStyle(nsIDOMCSSValue** aValue);
  nsresult GetBorderWidth(nsIDOMCSSValue** aValue);
  nsresult GetBorderTopStyle(nsIDOMCSSValue** aValue);
  nsresult GetBorderBottomStyle(nsIDOMCSSValue** aValue);
  nsresult GetBorderLeftStyle(nsIDOMCSSValue** aValue);
  nsresult GetBorderRightStyle(nsIDOMCSSValue** aValue);
  nsresult GetBorderTopWidth(nsIDOMCSSValue** aValue);
  nsresult GetBorderBottomWidth(nsIDOMCSSValue** aValue);
  nsresult GetBorderLeftWidth(nsIDOMCSSValue** aValue);
  nsresult GetBorderRightWidth(nsIDOMCSSValue** aValue);
  nsresult GetBorderTopColor(nsIDOMCSSValue** aValue);
  nsresult GetBorderBottomColor(nsIDOMCSSValue** aValue);
  nsresult GetBorderLeftColor(nsIDOMCSSValue** aValue);
  nsresult GetBorderRightColor(nsIDOMCSSValue** aValue);
  nsresult GetBorderBottomColors(nsIDOMCSSValue** aValue);
  nsresult GetBorderLeftColors(nsIDOMCSSValue** aValue);
  nsresult GetBorderRightColors(nsIDOMCSSValue** aValue);
  nsresult GetBorderTopColors(nsIDOMCSSValue** aValue);
  nsresult GetBorderRadiusBottomLeft(nsIDOMCSSValue** aValue);
  nsresult GetBorderRadiusBottomRight(nsIDOMCSSValue** aValue);
  nsresult GetBorderRadiusTopLeft(nsIDOMCSSValue** aValue);
  nsresult GetBorderRadiusTopRight(nsIDOMCSSValue** aValue);
  nsresult GetFloatEdge(nsIDOMCSSValue** aValue);

  
  nsresult GetMarginWidth(nsIDOMCSSValue** aValue);
  nsresult GetMarginTopWidth(nsIDOMCSSValue** aValue);
  nsresult GetMarginBottomWidth(nsIDOMCSSValue** aValue);
  nsresult GetMarginLeftWidth(nsIDOMCSSValue** aValue);
  nsresult GetMarginRightWidth(nsIDOMCSSValue** aValue);

  
  nsresult GetOutline(nsIDOMCSSValue** aValue);
  nsresult GetOutlineWidth(nsIDOMCSSValue** aValue);
  nsresult GetOutlineStyle(nsIDOMCSSValue** aValue);
  nsresult GetOutlineColor(nsIDOMCSSValue** aValue);
  nsresult GetOutlineOffset(nsIDOMCSSValue** aValue);
  nsresult GetOutlineRadiusBottomLeft(nsIDOMCSSValue** aValue);
  nsresult GetOutlineRadiusBottomRight(nsIDOMCSSValue** aValue);
  nsresult GetOutlineRadiusTopLeft(nsIDOMCSSValue** aValue);
  nsresult GetOutlineRadiusTopRight(nsIDOMCSSValue** aValue);

  
  nsresult GetContent(nsIDOMCSSValue** aValue);
  nsresult GetCounterIncrement(nsIDOMCSSValue** aValue);
  nsresult GetCounterReset(nsIDOMCSSValue** aValue);
  nsresult GetMarkerOffset(nsIDOMCSSValue** aValue);

  
  nsresult GetQuotes(nsIDOMCSSValue** aValue);

  
  nsresult GetZIndex(nsIDOMCSSValue** aValue);

  
  nsresult GetListStyleImage(nsIDOMCSSValue** aValue);
  nsresult GetListStylePosition(nsIDOMCSSValue** aValue);
  nsresult GetListStyleType(nsIDOMCSSValue** aValue);
  nsresult GetImageRegion(nsIDOMCSSValue** aValue);

  
  nsresult GetLineHeight(nsIDOMCSSValue** aValue);
  nsresult GetTextAlign(nsIDOMCSSValue** aValue);
  nsresult GetTextDecoration(nsIDOMCSSValue** aValue);
  nsresult GetTextIndent(nsIDOMCSSValue** aValue);
  nsresult GetTextTransform(nsIDOMCSSValue** aValue);
  nsresult GetLetterSpacing(nsIDOMCSSValue** aValue);
  nsresult GetWordSpacing(nsIDOMCSSValue** aValue);
  nsresult GetWhiteSpace(nsIDOMCSSValue** aValue);

  
  nsresult GetOpacity(nsIDOMCSSValue** aValue);
  nsresult GetVisibility(nsIDOMCSSValue** aValue);

  
  nsresult GetDirection(nsIDOMCSSValue** aValue);
  nsresult GetUnicodeBidi(nsIDOMCSSValue** aValue);

  
  nsresult GetBinding(nsIDOMCSSValue** aValue);
  nsresult GetClear(nsIDOMCSSValue** aValue);
  nsresult GetCssFloat(nsIDOMCSSValue** aValue);
  nsresult GetDisplay(nsIDOMCSSValue** aValue);
  nsresult GetPosition(nsIDOMCSSValue** aValue);
  nsresult GetClip(nsIDOMCSSValue** aValue);
  nsresult GetOverflow(nsIDOMCSSValue** aValue);
  nsresult GetOverflowX(nsIDOMCSSValue** aValue);
  nsresult GetOverflowY(nsIDOMCSSValue** aValue);
  nsresult GetPageBreakAfter(nsIDOMCSSValue** aValue);
  nsresult GetPageBreakBefore(nsIDOMCSSValue** aValue);

  
  nsresult GetCursor(nsIDOMCSSValue** aValue);
  nsresult GetForceBrokenImageIcon(nsIDOMCSSValue** aValue);
  nsresult GetIMEMode(nsIDOMCSSValue** aValue);
  nsresult GetUserFocus(nsIDOMCSSValue** aValue);
  nsresult GetUserInput(nsIDOMCSSValue** aValue);
  nsresult GetUserModify(nsIDOMCSSValue** aValue);
  nsresult GetUserSelect(nsIDOMCSSValue** aValue);

  
  nsresult GetColumnCount(nsIDOMCSSValue** aValue);
  nsresult GetColumnWidth(nsIDOMCSSValue** aValue);
  nsresult GetColumnGap(nsIDOMCSSValue** aValue);

#ifdef MOZ_SVG
  
  nsresult GetSVGPaintFor(PRBool aFill, nsIDOMCSSValue** aValue);

  nsresult GetFill(nsIDOMCSSValue** aValue);
  nsresult GetStroke(nsIDOMCSSValue** aValue);
  nsresult GetMarkerEnd(nsIDOMCSSValue** aValue);
  nsresult GetMarkerMid(nsIDOMCSSValue** aValue);
  nsresult GetMarkerStart(nsIDOMCSSValue** aValue);
  nsresult GetStrokeDasharray(nsIDOMCSSValue** aValue);

  nsresult GetStrokeDashoffset(nsIDOMCSSValue** aValue);
  nsresult GetStrokeWidth(nsIDOMCSSValue** aValue);

  nsresult GetFillOpacity(nsIDOMCSSValue** aValue);
  nsresult GetFloodOpacity(nsIDOMCSSValue** aValue);
  nsresult GetStopOpacity(nsIDOMCSSValue** aValue);
  nsresult GetStrokeMiterlimit(nsIDOMCSSValue** aValue);
  nsresult GetStrokeOpacity(nsIDOMCSSValue** aValue);

  nsresult GetClipRule(nsIDOMCSSValue** aValue);
  nsresult GetFillRule(nsIDOMCSSValue** aValue);
  nsresult GetStrokeLinecap(nsIDOMCSSValue** aValue);
  nsresult GetStrokeLinejoin(nsIDOMCSSValue** aValue);
  nsresult GetTextAnchor(nsIDOMCSSValue** aValue);

  nsresult GetColorInterpolation(nsIDOMCSSValue** aValue);
  nsresult GetColorInterpolationFilters(nsIDOMCSSValue** aValue);
  nsresult GetDominantBaseline(nsIDOMCSSValue** aValue);
  nsresult GetPointerEvents(nsIDOMCSSValue** aValue);
  nsresult GetShapeRendering(nsIDOMCSSValue** aValue);
  nsresult GetTextRendering(nsIDOMCSSValue** aValue);

  nsresult GetStopColor(nsIDOMCSSValue** aValue);
  nsresult GetFloodColor(nsIDOMCSSValue** aValue);

  nsresult GetClipPath(nsIDOMCSSValue** aValue);
  nsresult GetFilter(nsIDOMCSSValue** aValue);
  nsresult GetMask(nsIDOMCSSValue** aValue);
#endif 

  nsROCSSPrimitiveValue* GetROCSSPrimitiveValue();
  nsDOMCSSValueList* GetROCSSValueList(PRBool aCommaDelimited);
  nsresult SetToRGBAColor(nsROCSSPrimitiveValue* aValue, nscolor aColor);
  
  



  typedef PRBool (nsComputedDOMStyle::*PercentageBaseGetter)(nscoord&);

  













  void SetValueToCoord(nsROCSSPrimitiveValue* aValue,
                       const nsStyleCoord& aCoord,
                       PercentageBaseGetter aPercentageBaseGetter = nsnull,
                       const PRInt32 aTable[] = nsnull,
                       nscoord aMinAppUnits = nscoord_MIN,
                       nscoord aMaxAppUnits = nscoord_MAX);

  





  nscoord StyleCoordToNSCoord(const nsStyleCoord& aCoord,
                              PercentageBaseGetter aPercentageBaseGetter,
                              nscoord aDefaultValue);

  PRBool GetCBContentWidth(nscoord& aWidth);
  PRBool GetCBContentHeight(nscoord& aWidth);
  PRBool GetFrameBorderRectWidth(nscoord& aWidth);

  struct ComputedStyleMapEntry
  {
    
    typedef nsresult (nsComputedDOMStyle::*ComputeMethod)(nsIDOMCSSValue**);

    nsCSSProperty mProperty;
    ComputeMethod mGetter;
  };

  static const ComputedStyleMapEntry* GetQueryablePropertyMap(PRUint32* aLength);

  CSS2PropertiesTearoff mInner;

  
  
  
  nsWeakPtr mDocumentWeak;
  nsCOMPtr<nsIContent> mContent;

  




  nsRefPtr<nsStyleContext> mStyleContextHolder;
  nsCOMPtr<nsIAtom> mPseudo;

  



  nsIFrame* mFrame;
  



  nsIPresShell* mPresShell;

  PRInt32 mAppUnitsPerInch; 
};

#endif 

