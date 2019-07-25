







































#ifndef nsComputedDOMStyle_h__
#define nsComputedDOMStyle_h__

#include "nsDOMCSSDeclaration.h"

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

class nsComputedDOMStyle : public nsDOMCSSDeclaration,
                           public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsComputedDOMStyle,
                                           nsICSSDeclaration)

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

  
  
  
  virtual mozilla::css::Declaration* GetCSSDeclaration(PRBool);
  virtual nsresult SetCSSDeclaration(mozilla::css::Declaration*);
  virtual nsIDocument* DocToUpdate();
  virtual void GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv);

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

  
  
  

  nsIDOMCSSValue* GetEllipseRadii(const nsStyleCorners& aRadius,
                                  PRUint8 aFullCorner,
                                  PRBool aIsBorder); 

  nsIDOMCSSValue* GetOffsetWidthFor(mozilla::css::Side aSide);

  nsIDOMCSSValue* GetAbsoluteOffset(mozilla::css::Side aSide);

  nsIDOMCSSValue* GetRelativeOffset(mozilla::css::Side aSide);

  nsIDOMCSSValue* GetStaticOffset(mozilla::css::Side aSide);

  nsIDOMCSSValue* GetPaddingWidthFor(mozilla::css::Side aSide);

  nsIDOMCSSValue* GetBorderColorsFor(mozilla::css::Side aSide);

  nsIDOMCSSValue* GetBorderStyleFor(mozilla::css::Side aSide);

  nsIDOMCSSValue* GetBorderWidthFor(mozilla::css::Side aSide);

  nsIDOMCSSValue* GetBorderColorFor(mozilla::css::Side aSide);

  nsIDOMCSSValue* GetMarginWidthFor(mozilla::css::Side aSide);

  nsIDOMCSSValue* GetSVGPaintFor(PRBool aFill);

  PRBool GetLineHeightCoord(nscoord& aCoord);

  nsIDOMCSSValue* GetCSSShadowArray(nsCSSShadowArray* aArray,
                                    const nscolor& aDefaultColor,
                                    PRBool aIsBoxShadow);

  nsIDOMCSSValue* GetBackgroundList(PRUint8 nsStyleBackground::Layer::* aMember,
                                    PRUint32 nsStyleBackground::* aCount,
                                    const PRInt32 aTable[]);

  void GetCSSGradientString(const nsStyleGradient* aGradient,
                            nsAString& aString);
  void GetImageRectString(nsIURI* aURI,
                          const nsStyleSides& aCropRect,
                          nsString& aString);
  void AppendTimingFunction(nsDOMCSSValueList *aValueList,
                            const nsTimingFunction& aTimingFunction);

  




  nsIDOMCSSValue* DoGetAppearance();

  
  nsIDOMCSSValue* DoGetBoxAlign();
  nsIDOMCSSValue* DoGetBoxDirection();
  nsIDOMCSSValue* DoGetBoxFlex();
  nsIDOMCSSValue* DoGetBoxOrdinalGroup();
  nsIDOMCSSValue* DoGetBoxOrient();
  nsIDOMCSSValue* DoGetBoxPack();
  nsIDOMCSSValue* DoGetBoxSizing();

  nsIDOMCSSValue* DoGetWidth();
  nsIDOMCSSValue* DoGetHeight();
  nsIDOMCSSValue* DoGetMaxHeight();
  nsIDOMCSSValue* DoGetMaxWidth();
  nsIDOMCSSValue* DoGetMinHeight();
  nsIDOMCSSValue* DoGetMinWidth();
  nsIDOMCSSValue* DoGetLeft();
  nsIDOMCSSValue* DoGetTop();
  nsIDOMCSSValue* DoGetRight();
  nsIDOMCSSValue* DoGetBottom();
  nsIDOMCSSValue* DoGetStackSizing();

  
  nsIDOMCSSValue* DoGetColor();
  nsIDOMCSSValue* DoGetFontFamily();
  nsIDOMCSSValue* DoGetMozFontFeatureSettings();
  nsIDOMCSSValue* DoGetMozFontLanguageOverride();
  nsIDOMCSSValue* DoGetFontSize();
  nsIDOMCSSValue* DoGetFontSizeAdjust();
  nsIDOMCSSValue* DoGetFontStretch();
  nsIDOMCSSValue* DoGetFontStyle();
  nsIDOMCSSValue* DoGetFontWeight();
  nsIDOMCSSValue* DoGetFontVariant();

  
  nsIDOMCSSValue* DoGetBackgroundAttachment();
  nsIDOMCSSValue* DoGetBackgroundColor();
  nsIDOMCSSValue* DoGetBackgroundImage();
  nsIDOMCSSValue* DoGetBackgroundPosition();
  nsIDOMCSSValue* DoGetBackgroundRepeat();
  nsIDOMCSSValue* DoGetBackgroundClip();
  nsIDOMCSSValue* DoGetBackgroundInlinePolicy();
  nsIDOMCSSValue* DoGetBackgroundOrigin();
  nsIDOMCSSValue* DoGetMozBackgroundSize();

  
  nsIDOMCSSValue* DoGetPadding();
  nsIDOMCSSValue* DoGetPaddingTop();
  nsIDOMCSSValue* DoGetPaddingBottom();
  nsIDOMCSSValue* DoGetPaddingLeft();
  nsIDOMCSSValue* DoGetPaddingRight();

  
  nsIDOMCSSValue* DoGetBorderCollapse();
  nsIDOMCSSValue* DoGetBorderSpacing();
  nsIDOMCSSValue* DoGetCaptionSide();
  nsIDOMCSSValue* DoGetEmptyCells();
  nsIDOMCSSValue* DoGetTableLayout();
  nsIDOMCSSValue* DoGetVerticalAlign();

  
  nsIDOMCSSValue* DoGetBorderStyle();
  nsIDOMCSSValue* DoGetBorderWidth();
  nsIDOMCSSValue* DoGetBorderTopStyle();
  nsIDOMCSSValue* DoGetBorderBottomStyle();
  nsIDOMCSSValue* DoGetBorderLeftStyle();
  nsIDOMCSSValue* DoGetBorderRightStyle();
  nsIDOMCSSValue* DoGetBorderTopWidth();
  nsIDOMCSSValue* DoGetBorderBottomWidth();
  nsIDOMCSSValue* DoGetBorderLeftWidth();
  nsIDOMCSSValue* DoGetBorderRightWidth();
  nsIDOMCSSValue* DoGetBorderTopColor();
  nsIDOMCSSValue* DoGetBorderBottomColor();
  nsIDOMCSSValue* DoGetBorderLeftColor();
  nsIDOMCSSValue* DoGetBorderRightColor();
  nsIDOMCSSValue* DoGetBorderBottomColors();
  nsIDOMCSSValue* DoGetBorderLeftColors();
  nsIDOMCSSValue* DoGetBorderRightColors();
  nsIDOMCSSValue* DoGetBorderTopColors();
  nsIDOMCSSValue* DoGetBorderBottomLeftRadius();
  nsIDOMCSSValue* DoGetBorderBottomRightRadius();
  nsIDOMCSSValue* DoGetBorderTopLeftRadius();
  nsIDOMCSSValue* DoGetBorderTopRightRadius();
  nsIDOMCSSValue* DoGetFloatEdge();
  nsIDOMCSSValue* DoGetBorderImage();

  
  nsIDOMCSSValue* DoGetBoxShadow();

  
  nsIDOMCSSValue* DoGetWindowShadow();

  
  nsIDOMCSSValue* DoGetMarginWidth();
  nsIDOMCSSValue* DoGetMarginTopWidth();
  nsIDOMCSSValue* DoGetMarginBottomWidth();
  nsIDOMCSSValue* DoGetMarginLeftWidth();
  nsIDOMCSSValue* DoGetMarginRightWidth();

  
  nsIDOMCSSValue* DoGetOutline();
  nsIDOMCSSValue* DoGetOutlineWidth();
  nsIDOMCSSValue* DoGetOutlineStyle();
  nsIDOMCSSValue* DoGetOutlineColor();
  nsIDOMCSSValue* DoGetOutlineOffset();
  nsIDOMCSSValue* DoGetOutlineRadiusBottomLeft();
  nsIDOMCSSValue* DoGetOutlineRadiusBottomRight();
  nsIDOMCSSValue* DoGetOutlineRadiusTopLeft();
  nsIDOMCSSValue* DoGetOutlineRadiusTopRight();

  
  nsIDOMCSSValue* DoGetContent();
  nsIDOMCSSValue* DoGetCounterIncrement();
  nsIDOMCSSValue* DoGetCounterReset();
  nsIDOMCSSValue* DoGetMarkerOffset();

  
  nsIDOMCSSValue* DoGetQuotes();

  
  nsIDOMCSSValue* DoGetZIndex();

  
  nsIDOMCSSValue* DoGetListStyleImage();
  nsIDOMCSSValue* DoGetListStylePosition();
  nsIDOMCSSValue* DoGetListStyleType();
  nsIDOMCSSValue* DoGetImageRegion();

  
  nsIDOMCSSValue* DoGetLineHeight();
  nsIDOMCSSValue* DoGetTextAlign();
  nsIDOMCSSValue* DoGetMozTextBlink();
  nsIDOMCSSValue* DoGetTextDecoration();
  nsIDOMCSSValue* DoGetMozTextDecorationColor();
  nsIDOMCSSValue* DoGetMozTextDecorationLine();
  nsIDOMCSSValue* DoGetMozTextDecorationStyle();
  nsIDOMCSSValue* DoGetTextIndent();
  nsIDOMCSSValue* DoGetTextTransform();
  nsIDOMCSSValue* DoGetTextShadow();
  nsIDOMCSSValue* DoGetLetterSpacing();
  nsIDOMCSSValue* DoGetWordSpacing();
  nsIDOMCSSValue* DoGetWhiteSpace();
  nsIDOMCSSValue* DoGetWordWrap();
  nsIDOMCSSValue* DoGetHyphens();
  nsIDOMCSSValue* DoGetMozTabSize();

  
  nsIDOMCSSValue* DoGetOpacity();
  nsIDOMCSSValue* DoGetPointerEvents();
  nsIDOMCSSValue* DoGetVisibility();

  
  nsIDOMCSSValue* DoGetDirection();
  nsIDOMCSSValue* DoGetUnicodeBidi();

  
  nsIDOMCSSValue* DoGetBinding();
  nsIDOMCSSValue* DoGetClear();
  nsIDOMCSSValue* DoGetCssFloat();
  nsIDOMCSSValue* DoGetDisplay();
  nsIDOMCSSValue* DoGetPosition();
  nsIDOMCSSValue* DoGetClip();
  nsIDOMCSSValue* DoGetOverflow();
  nsIDOMCSSValue* DoGetOverflowX();
  nsIDOMCSSValue* DoGetOverflowY();
  nsIDOMCSSValue* DoGetResize();
  nsIDOMCSSValue* DoGetPageBreakAfter();
  nsIDOMCSSValue* DoGetPageBreakBefore();
  nsIDOMCSSValue* DoGetMozTransform();
  nsIDOMCSSValue* DoGetMozTransformOrigin();

  
  nsIDOMCSSValue* DoGetCursor();
  nsIDOMCSSValue* DoGetForceBrokenImageIcon();
  nsIDOMCSSValue* DoGetIMEMode();
  nsIDOMCSSValue* DoGetUserFocus();
  nsIDOMCSSValue* DoGetUserInput();
  nsIDOMCSSValue* DoGetUserModify();
  nsIDOMCSSValue* DoGetUserSelect();

  
  nsIDOMCSSValue* DoGetColumnCount();
  nsIDOMCSSValue* DoGetColumnWidth();
  nsIDOMCSSValue* DoGetColumnGap();
  nsIDOMCSSValue* DoGetColumnRuleWidth();
  nsIDOMCSSValue* DoGetColumnRuleStyle();
  nsIDOMCSSValue* DoGetColumnRuleColor();

  
  nsIDOMCSSValue* DoGetTransitionProperty();
  nsIDOMCSSValue* DoGetTransitionDuration();
  nsIDOMCSSValue* DoGetTransitionDelay();
  nsIDOMCSSValue* DoGetTransitionTimingFunction();

#ifdef MOZ_CSS_ANIMATIONS
  
  nsIDOMCSSValue* DoGetAnimationName();
  nsIDOMCSSValue* DoGetAnimationDuration();
  nsIDOMCSSValue* DoGetAnimationDelay();
  nsIDOMCSSValue* DoGetAnimationTimingFunction();
  nsIDOMCSSValue* DoGetAnimationDirection();
  nsIDOMCSSValue* DoGetAnimationFillMode();
  nsIDOMCSSValue* DoGetAnimationIterationCount();
  nsIDOMCSSValue* DoGetAnimationPlayState();
#endif

  
  nsIDOMCSSValue* DoGetFill();
  nsIDOMCSSValue* DoGetStroke();
  nsIDOMCSSValue* DoGetMarkerEnd();
  nsIDOMCSSValue* DoGetMarkerMid();
  nsIDOMCSSValue* DoGetMarkerStart();
  nsIDOMCSSValue* DoGetStrokeDasharray();

  nsIDOMCSSValue* DoGetStrokeDashoffset();
  nsIDOMCSSValue* DoGetStrokeWidth();

  nsIDOMCSSValue* DoGetFillOpacity();
  nsIDOMCSSValue* DoGetFloodOpacity();
  nsIDOMCSSValue* DoGetStopOpacity();
  nsIDOMCSSValue* DoGetStrokeMiterlimit();
  nsIDOMCSSValue* DoGetStrokeOpacity();

  nsIDOMCSSValue* DoGetClipRule();
  nsIDOMCSSValue* DoGetFillRule();
  nsIDOMCSSValue* DoGetStrokeLinecap();
  nsIDOMCSSValue* DoGetStrokeLinejoin();
  nsIDOMCSSValue* DoGetTextAnchor();

  nsIDOMCSSValue* DoGetColorInterpolation();
  nsIDOMCSSValue* DoGetColorInterpolationFilters();
  nsIDOMCSSValue* DoGetDominantBaseline();
  nsIDOMCSSValue* DoGetImageRendering();
  nsIDOMCSSValue* DoGetShapeRendering();
  nsIDOMCSSValue* DoGetTextRendering();

  nsIDOMCSSValue* DoGetFloodColor();
  nsIDOMCSSValue* DoGetLightingColor();
  nsIDOMCSSValue* DoGetStopColor();

  nsIDOMCSSValue* DoGetClipPath();
  nsIDOMCSSValue* DoGetFilter();
  nsIDOMCSSValue* DoGetMask();

  nsROCSSPrimitiveValue* GetROCSSPrimitiveValue();
  nsDOMCSSValueList* GetROCSSValueList(PRBool aCommaDelimited);
  void SetToRGBAColor(nsROCSSPrimitiveValue* aValue, nscolor aColor);
  void SetValueToStyleImage(const nsStyleImage& aStyleImage,
                            nsROCSSPrimitiveValue* aValue);

  



  typedef PRBool (nsComputedDOMStyle::*PercentageBaseGetter)(nscoord&);

  













  void SetValueToCoord(nsROCSSPrimitiveValue* aValue,
                       const nsStyleCoord& aCoord,
                       PRBool aClampNegativeCalc,
                       PercentageBaseGetter aPercentageBaseGetter = nsnull,
                       const PRInt32 aTable[] = nsnull,
                       nscoord aMinAppUnits = nscoord_MIN,
                       nscoord aMaxAppUnits = nscoord_MAX);

  





  nscoord StyleCoordToNSCoord(const nsStyleCoord& aCoord,
                              PercentageBaseGetter aPercentageBaseGetter,
                              nscoord aDefaultValue,
                              PRBool aClampNegativeCalc);

  PRBool GetCBContentWidth(nscoord& aWidth);
  PRBool GetCBContentHeight(nscoord& aWidth);
  PRBool GetFrameBoundsWidthForTransform(nscoord &aWidth);
  PRBool GetFrameBoundsHeightForTransform(nscoord &aHeight);
  PRBool GetFrameBorderRectWidth(nscoord& aWidth);
  PRBool GetFrameBorderRectHeight(nscoord& aHeight);

  struct ComputedStyleMapEntry
  {
    
    typedef nsIDOMCSSValue* (nsComputedDOMStyle::*ComputeMethod)();

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

