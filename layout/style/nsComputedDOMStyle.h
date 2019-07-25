







































#ifndef nsComputedDOMStyle_h__
#define nsComputedDOMStyle_h__

#include "nsICSSDeclaration.h"

#include "nsROCSSPrimitiveValue.h"
#include "nsDOMCSSRGBColor.h"
#include "nsDOMCSSValueList.h"
#include "nsCSSProps.h"

#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"
#include "nsAutoPtr.h"
#include "nsStyleStruct.h"

class nsIPresShell;

class nsComputedDOMStyle : public nsICSSDeclaration,
                           public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsComputedDOMStyle)

  NS_IMETHOD Init(nsIDOMElement *aElement,
                  const nsAString& aPseudoElt,
                  nsIPresShell *aPresShell);

  NS_DECL_NSICSSDECLARATION

  NS_DECL_NSIDOMCSSSTYLEDECLARATION

  nsComputedDOMStyle();
  virtual ~nsComputedDOMStyle();

  static void Shutdown();

  virtual nsINode *GetParentObject()
  {
    return mContent;
  }

  static already_AddRefed<nsStyleContext>
  GetStyleContextForElement(mozilla::dom::Element* aElement, nsIAtom* aPseudo,
                            nsIPresShell* aPresShell);

  static already_AddRefed<nsStyleContext>
  GetStyleContextForElementNoFlush(mozilla::dom::Element* aElement,
                                   nsIAtom* aPseudo,
                                   nsIPresShell* aPresShell);

  static nsIPresShell*
  GetPresShellForContent(nsIContent* aContent);

  
  void SetExposeVisitedStyle(PRBool aExpose) {
    NS_ASSERTION(aExpose != mExposeVisitedStyle, "should always be changing");
    mExposeVisitedStyle = aExpose;
  }

private:
  void AssertFlushedPendingReflows() {
    NS_ASSERTION(mFlushedPendingReflows,
                 "property getter should have been marked layout-dependent");
  }

#define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)                  \
  const nsStyle##name_ * GetStyle##name_() {                            \
    return mStyleContextHolder->GetStyle##name_();                      \
  }
#include "nsStyleStructList.h"
#undef STYLE_STRUCT

  nsresult GetEllipseRadii(const nsStyleCorners& aRadius,
                           PRUint8 aFullCorner,
                           nsIDOMCSSValue** aValue);

  nsresult GetOffsetWidthFor(mozilla::css::Side aSide, nsIDOMCSSValue** aValue);

  nsresult GetAbsoluteOffset(mozilla::css::Side aSide, nsIDOMCSSValue** aValue);

  nsresult GetRelativeOffset(mozilla::css::Side aSide, nsIDOMCSSValue** aValue);

  nsresult GetStaticOffset(mozilla::css::Side aSide, nsIDOMCSSValue** aValue);

  nsresult GetPaddingWidthFor(mozilla::css::Side aSide, nsIDOMCSSValue** aValue);

  nsresult GetBorderColorsFor(mozilla::css::Side aSide, nsIDOMCSSValue** aValue);

  nsresult GetBorderStyleFor(mozilla::css::Side aSide, nsIDOMCSSValue** aValue);

  nsresult GetBorderWidthFor(mozilla::css::Side aSide, nsIDOMCSSValue** aValue);

  nsresult GetBorderColorFor(mozilla::css::Side aSide, nsIDOMCSSValue** aValue);

  nsresult GetMarginWidthFor(mozilla::css::Side aSide, nsIDOMCSSValue** aValue);

  nsresult GetSVGPaintFor(PRBool aFill, nsIDOMCSSValue** aValue);

  PRBool GetLineHeightCoord(nscoord& aCoord);

  nsresult GetCSSShadowArray(nsCSSShadowArray* aArray,
                             const nscolor& aDefaultColor,
                             PRBool aIsBoxShadow,
                             nsIDOMCSSValue** aValue);

  nsresult GetBackgroundList(PRUint8 nsStyleBackground::Layer::* aMember,
                             PRUint32 nsStyleBackground::* aCount,
                             const PRInt32 aTable[],
                             nsIDOMCSSValue** aResult);

  nsresult GetCSSGradientString(const nsStyleGradient* aGradient,
                                nsAString& aString);
  nsresult GetImageRectString(nsIURI* aURI,
                              const nsStyleSides& aCropRect,
                              nsString& aString);

  




  nsresult DoGetAppearance(nsIDOMCSSValue** aValue);

  
  nsresult DoGetBoxAlign(nsIDOMCSSValue** aValue);
  nsresult DoGetBoxDirection(nsIDOMCSSValue** aValue);
  nsresult DoGetBoxFlex(nsIDOMCSSValue** aValue);
  nsresult DoGetBoxOrdinalGroup(nsIDOMCSSValue** aValue);
  nsresult DoGetBoxOrient(nsIDOMCSSValue** aValue);
  nsresult DoGetBoxPack(nsIDOMCSSValue** aValue);
  nsresult DoGetBoxSizing(nsIDOMCSSValue** aValue);

  nsresult DoGetWidth(nsIDOMCSSValue** aValue);
  nsresult DoGetHeight(nsIDOMCSSValue** aValue);
  nsresult DoGetMaxHeight(nsIDOMCSSValue** aValue);
  nsresult DoGetMaxWidth(nsIDOMCSSValue** aValue);
  nsresult DoGetMinHeight(nsIDOMCSSValue** aValue);
  nsresult DoGetMinWidth(nsIDOMCSSValue** aValue);
  nsresult DoGetLeft(nsIDOMCSSValue** aValue);
  nsresult DoGetTop(nsIDOMCSSValue** aValue);
  nsresult DoGetRight(nsIDOMCSSValue** aValue);
  nsresult DoGetBottom(nsIDOMCSSValue** aValue);
  nsresult DoGetStackSizing(nsIDOMCSSValue** aValue);

  
  nsresult DoGetColor(nsIDOMCSSValue** aValue);
  nsresult DoGetFontFamily(nsIDOMCSSValue** aValue);
  nsresult DoGetMozFontFeatureSettings(nsIDOMCSSValue** aValue);
  nsresult DoGetMozFontLanguageOverride(nsIDOMCSSValue** aValue);
  nsresult DoGetFontSize(nsIDOMCSSValue** aValue);
  nsresult DoGetFontSizeAdjust(nsIDOMCSSValue** aValue);
  nsresult DoGetFontStretch(nsIDOMCSSValue** aValue);
  nsresult DoGetFontStyle(nsIDOMCSSValue** aValue);
  nsresult DoGetFontWeight(nsIDOMCSSValue** aValue);
  nsresult DoGetFontVariant(nsIDOMCSSValue** aValue);

  
  nsresult DoGetBackgroundAttachment(nsIDOMCSSValue** aValue);
  nsresult DoGetBackgroundColor(nsIDOMCSSValue** aValue);
  nsresult DoGetBackgroundImage(nsIDOMCSSValue** aValue);
  nsresult DoGetBackgroundPosition(nsIDOMCSSValue** aValue);
  nsresult DoGetBackgroundRepeat(nsIDOMCSSValue** aValue);
  nsresult DoGetBackgroundClip(nsIDOMCSSValue** aValue);
  nsresult DoGetBackgroundInlinePolicy(nsIDOMCSSValue** aValue);
  nsresult DoGetBackgroundOrigin(nsIDOMCSSValue** aValue);
  nsresult DoGetMozBackgroundSize(nsIDOMCSSValue** aValue);

  
  nsresult DoGetPadding(nsIDOMCSSValue** aValue);
  nsresult DoGetPaddingTop(nsIDOMCSSValue** aValue);
  nsresult DoGetPaddingBottom(nsIDOMCSSValue** aValue);
  nsresult DoGetPaddingLeft(nsIDOMCSSValue** aValue);
  nsresult DoGetPaddingRight(nsIDOMCSSValue** aValue);

  
  nsresult DoGetBorderCollapse(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderSpacing(nsIDOMCSSValue** aValue);
  nsresult DoGetCaptionSide(nsIDOMCSSValue** aValue);
  nsresult DoGetEmptyCells(nsIDOMCSSValue** aValue);
  nsresult DoGetTableLayout(nsIDOMCSSValue** aValue);
  nsresult DoGetVerticalAlign(nsIDOMCSSValue** aValue);

  
  nsresult DoGetBorderStyle(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderWidth(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderTopStyle(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderBottomStyle(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderLeftStyle(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderRightStyle(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderTopWidth(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderBottomWidth(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderLeftWidth(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderRightWidth(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderTopColor(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderBottomColor(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderLeftColor(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderRightColor(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderBottomColors(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderLeftColors(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderRightColors(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderTopColors(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderRadiusBottomLeft(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderRadiusBottomRight(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderRadiusTopLeft(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderRadiusTopRight(nsIDOMCSSValue** aValue);
  nsresult DoGetFloatEdge(nsIDOMCSSValue** aValue);
  nsresult DoGetBorderImage(nsIDOMCSSValue** aValue);

  
  nsresult DoGetBoxShadow(nsIDOMCSSValue** aValue);

  
  nsresult DoGetWindowShadow(nsIDOMCSSValue** aValue);

  
  nsresult DoGetMarginWidth(nsIDOMCSSValue** aValue);
  nsresult DoGetMarginTopWidth(nsIDOMCSSValue** aValue);
  nsresult DoGetMarginBottomWidth(nsIDOMCSSValue** aValue);
  nsresult DoGetMarginLeftWidth(nsIDOMCSSValue** aValue);
  nsresult DoGetMarginRightWidth(nsIDOMCSSValue** aValue);

  
  nsresult DoGetOutline(nsIDOMCSSValue** aValue);
  nsresult DoGetOutlineWidth(nsIDOMCSSValue** aValue);
  nsresult DoGetOutlineStyle(nsIDOMCSSValue** aValue);
  nsresult DoGetOutlineColor(nsIDOMCSSValue** aValue);
  nsresult DoGetOutlineOffset(nsIDOMCSSValue** aValue);
  nsresult DoGetOutlineRadiusBottomLeft(nsIDOMCSSValue** aValue);
  nsresult DoGetOutlineRadiusBottomRight(nsIDOMCSSValue** aValue);
  nsresult DoGetOutlineRadiusTopLeft(nsIDOMCSSValue** aValue);
  nsresult DoGetOutlineRadiusTopRight(nsIDOMCSSValue** aValue);

  
  nsresult DoGetContent(nsIDOMCSSValue** aValue);
  nsresult DoGetCounterIncrement(nsIDOMCSSValue** aValue);
  nsresult DoGetCounterReset(nsIDOMCSSValue** aValue);
  nsresult DoGetMarkerOffset(nsIDOMCSSValue** aValue);

  
  nsresult DoGetQuotes(nsIDOMCSSValue** aValue);

  
  nsresult DoGetZIndex(nsIDOMCSSValue** aValue);

  
  nsresult DoGetListStyleImage(nsIDOMCSSValue** aValue);
  nsresult DoGetListStylePosition(nsIDOMCSSValue** aValue);
  nsresult DoGetListStyleType(nsIDOMCSSValue** aValue);
  nsresult DoGetImageRegion(nsIDOMCSSValue** aValue);

  
  nsresult DoGetLineHeight(nsIDOMCSSValue** aValue);
  nsresult DoGetTextAlign(nsIDOMCSSValue** aValue);
  nsresult DoGetTextDecoration(nsIDOMCSSValue** aValue);
  nsresult DoGetTextIndent(nsIDOMCSSValue** aValue);
  nsresult DoGetTextTransform(nsIDOMCSSValue** aValue);
  nsresult DoGetTextShadow(nsIDOMCSSValue** aValue);
  nsresult DoGetLetterSpacing(nsIDOMCSSValue** aValue);
  nsresult DoGetWordSpacing(nsIDOMCSSValue** aValue);
  nsresult DoGetWhiteSpace(nsIDOMCSSValue** aValue);
  nsresult DoGetWordWrap(nsIDOMCSSValue** aValue);
  nsresult DoGetMozTabSize(nsIDOMCSSValue** aValue);

  
  nsresult DoGetOpacity(nsIDOMCSSValue** aValue);
  nsresult DoGetPointerEvents(nsIDOMCSSValue** aValue);
  nsresult DoGetVisibility(nsIDOMCSSValue** aValue);

  
  nsresult DoGetDirection(nsIDOMCSSValue** aValue);
  nsresult DoGetUnicodeBidi(nsIDOMCSSValue** aValue);

  
  nsresult DoGetBinding(nsIDOMCSSValue** aValue);
  nsresult DoGetClear(nsIDOMCSSValue** aValue);
  nsresult DoGetCssFloat(nsIDOMCSSValue** aValue);
  nsresult DoGetDisplay(nsIDOMCSSValue** aValue);
  nsresult DoGetPosition(nsIDOMCSSValue** aValue);
  nsresult DoGetClip(nsIDOMCSSValue** aValue);
  nsresult DoGetOverflow(nsIDOMCSSValue** aValue);
  nsresult DoGetOverflowX(nsIDOMCSSValue** aValue);
  nsresult DoGetOverflowY(nsIDOMCSSValue** aValue);
  nsresult DoGetResize(nsIDOMCSSValue** aValue);
  nsresult DoGetPageBreakAfter(nsIDOMCSSValue** aValue);
  nsresult DoGetPageBreakBefore(nsIDOMCSSValue** aValue);
  nsresult DoGetMozTransform(nsIDOMCSSValue** aValue);
  nsresult DoGetMozTransformOrigin(nsIDOMCSSValue **aValue);

  
  nsresult DoGetCursor(nsIDOMCSSValue** aValue);
  nsresult DoGetForceBrokenImageIcon(nsIDOMCSSValue** aValue);
  nsresult DoGetIMEMode(nsIDOMCSSValue** aValue);
  nsresult DoGetUserFocus(nsIDOMCSSValue** aValue);
  nsresult DoGetUserInput(nsIDOMCSSValue** aValue);
  nsresult DoGetUserModify(nsIDOMCSSValue** aValue);
  nsresult DoGetUserSelect(nsIDOMCSSValue** aValue);

  
  nsresult DoGetColumnCount(nsIDOMCSSValue** aValue);
  nsresult DoGetColumnWidth(nsIDOMCSSValue** aValue);
  nsresult DoGetColumnGap(nsIDOMCSSValue** aValue);
  nsresult DoGetColumnRuleWidth(nsIDOMCSSValue** aValue);
  nsresult DoGetColumnRuleStyle(nsIDOMCSSValue** aValue);
  nsresult DoGetColumnRuleColor(nsIDOMCSSValue** aValue);

  
  nsresult DoGetTransitionProperty(nsIDOMCSSValue** aValue);
  nsresult DoGetTransitionDuration(nsIDOMCSSValue** aValue);
  nsresult DoGetTransitionDelay(nsIDOMCSSValue** aValue);
  nsresult DoGetTransitionTimingFunction(nsIDOMCSSValue** aValue);

  
  nsresult DoGetFill(nsIDOMCSSValue** aValue);
  nsresult DoGetStroke(nsIDOMCSSValue** aValue);
  nsresult DoGetMarkerEnd(nsIDOMCSSValue** aValue);
  nsresult DoGetMarkerMid(nsIDOMCSSValue** aValue);
  nsresult DoGetMarkerStart(nsIDOMCSSValue** aValue);
  nsresult DoGetStrokeDasharray(nsIDOMCSSValue** aValue);

  nsresult DoGetStrokeDashoffset(nsIDOMCSSValue** aValue);
  nsresult DoGetStrokeWidth(nsIDOMCSSValue** aValue);

  nsresult DoGetFillOpacity(nsIDOMCSSValue** aValue);
  nsresult DoGetFloodOpacity(nsIDOMCSSValue** aValue);
  nsresult DoGetStopOpacity(nsIDOMCSSValue** aValue);
  nsresult DoGetStrokeMiterlimit(nsIDOMCSSValue** aValue);
  nsresult DoGetStrokeOpacity(nsIDOMCSSValue** aValue);

  nsresult DoGetClipRule(nsIDOMCSSValue** aValue);
  nsresult DoGetFillRule(nsIDOMCSSValue** aValue);
  nsresult DoGetStrokeLinecap(nsIDOMCSSValue** aValue);
  nsresult DoGetStrokeLinejoin(nsIDOMCSSValue** aValue);
  nsresult DoGetTextAnchor(nsIDOMCSSValue** aValue);

  nsresult DoGetColorInterpolation(nsIDOMCSSValue** aValue);
  nsresult DoGetColorInterpolationFilters(nsIDOMCSSValue** aValue);
  nsresult DoGetDominantBaseline(nsIDOMCSSValue** aValue);
  nsresult DoGetImageRendering(nsIDOMCSSValue** aValue);
  nsresult DoGetShapeRendering(nsIDOMCSSValue** aValue);
  nsresult DoGetTextRendering(nsIDOMCSSValue** aValue);

  nsresult DoGetFloodColor(nsIDOMCSSValue** aValue);
  nsresult DoGetLightingColor(nsIDOMCSSValue** aValue);
  nsresult DoGetStopColor(nsIDOMCSSValue** aValue);

  nsresult DoGetClipPath(nsIDOMCSSValue** aValue);
  nsresult DoGetFilter(nsIDOMCSSValue** aValue);
  nsresult DoGetMask(nsIDOMCSSValue** aValue);

  nsROCSSPrimitiveValue* GetROCSSPrimitiveValue();
  nsDOMCSSValueList* GetROCSSValueList(PRBool aCommaDelimited);
  nsresult SetToRGBAColor(nsROCSSPrimitiveValue* aValue, nscolor aColor);
  nsresult SetValueToStyleImage(const nsStyleImage& aStyleImage,
                                nsROCSSPrimitiveValue* aValue);

  



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
  PRBool GetFrameBoundsWidthForTransform(nscoord &aWidth);
  PRBool GetFrameBoundsHeightForTransform(nscoord &aHeight);
  PRBool GetFrameBorderRectWidth(nscoord& aWidth);

  struct ComputedStyleMapEntry
  {
    
    typedef nsresult (nsComputedDOMStyle::*ComputeMethod)(nsIDOMCSSValue**);

    nsCSSProperty mProperty;
    ComputeMethod mGetter;
    PRBool mNeedsLayoutFlush;
  };

  static const ComputedStyleMapEntry* GetQueryablePropertyMap(PRUint32* aLength);

  
  
  
  nsWeakPtr mDocumentWeak;
  nsCOMPtr<nsIContent> mContent;

  




  nsRefPtr<nsStyleContext> mStyleContextHolder;
  nsCOMPtr<nsIAtom> mPseudo;

  




  nsIFrame* mOuterFrame;
  




  nsIFrame* mInnerFrame;
  



  nsIPresShell* mPresShell;

  PRInt32 mAppUnitsPerInch; 

  PRPackedBool mExposeVisitedStyle;

#ifdef DEBUG
  PRBool mFlushedPendingReflows;
#endif
};

nsresult
NS_NewComputedDOMStyle(nsIDOMElement *aElement, const nsAString &aPseudoElt,
                       nsIPresShell *aPresShell,
                       nsComputedDOMStyle **aComputedStyle);

#endif 

