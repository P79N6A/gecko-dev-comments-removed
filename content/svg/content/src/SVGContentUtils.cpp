






#include "SVGContentUtils.h"


#include "gfxMatrix.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "mozilla/RangedPtr.h"
#include "nsComputedDOMStyle.h"
#include "nsFontMetrics.h"
#include "nsIFrame.h"
#include "nsIScriptError.h"
#include "nsLayoutUtils.h"
#include "SVGAnimationElement.h"
#include "SVGAnimatedPreserveAspectRatio.h"
#include "nsContentUtils.h"
#include "mozilla/gfx/2D.h"

using namespace mozilla;
using namespace mozilla::dom;

SVGSVGElement*
SVGContentUtils::GetOuterSVGElement(nsSVGElement *aSVGElement)
{
  nsIContent *element = nullptr;
  nsIContent *ancestor = aSVGElement->GetFlattenedTreeParent();

  while (ancestor && ancestor->IsSVG() &&
                     ancestor->Tag() != nsGkAtoms::foreignObject) {
    element = ancestor;
    ancestor = element->GetFlattenedTreeParent();
  }

  if (element && element->Tag() == nsGkAtoms::svg) {
    return static_cast<SVGSVGElement*>(element);
  }
  return nullptr;
}

void
SVGContentUtils::ActivateByHyperlink(nsIContent *aContent)
{
  NS_ABORT_IF_FALSE(aContent->IsNodeOfType(nsINode::eANIMATION),
                    "Expecting an animation element");

  static_cast<SVGAnimationElement*>(aContent)->ActivateByHyperlink();
}

float
SVGContentUtils::GetFontSize(Element *aElement)
{
  if (!aElement)
    return 1.0f;

  nsRefPtr<nsStyleContext> styleContext = 
    nsComputedDOMStyle::GetStyleContextForElementNoFlush(aElement,
                                                         nullptr, nullptr);
  if (!styleContext) {
    
    NS_WARNING("Couldn't get style context for content in GetFontStyle");
    return 1.0f;
  }

  return GetFontSize(styleContext);
}

float
SVGContentUtils::GetFontSize(nsIFrame *aFrame)
{
  NS_ABORT_IF_FALSE(aFrame, "NULL frame in GetFontSize");
  return GetFontSize(aFrame->StyleContext());
}

float
SVGContentUtils::GetFontSize(nsStyleContext *aStyleContext)
{
  NS_ABORT_IF_FALSE(aStyleContext, "NULL style context in GetFontSize");

  nsPresContext *presContext = aStyleContext->PresContext();
  NS_ABORT_IF_FALSE(presContext, "NULL pres context in GetFontSize");

  nscoord fontSize = aStyleContext->StyleFont()->mSize;
  return nsPresContext::AppUnitsToFloatCSSPixels(fontSize) / 
         presContext->TextZoom();
}

float
SVGContentUtils::GetFontXHeight(Element *aElement)
{
  if (!aElement)
    return 1.0f;

  nsRefPtr<nsStyleContext> styleContext = 
    nsComputedDOMStyle::GetStyleContextForElementNoFlush(aElement,
                                                         nullptr, nullptr);
  if (!styleContext) {
    
    NS_WARNING("Couldn't get style context for content in GetFontStyle");
    return 1.0f;
  }

  return GetFontXHeight(styleContext);
}
  
float
SVGContentUtils::GetFontXHeight(nsIFrame *aFrame)
{
  NS_ABORT_IF_FALSE(aFrame, "NULL frame in GetFontXHeight");
  return GetFontXHeight(aFrame->StyleContext());
}

float
SVGContentUtils::GetFontXHeight(nsStyleContext *aStyleContext)
{
  NS_ABORT_IF_FALSE(aStyleContext, "NULL style context in GetFontXHeight");

  nsPresContext *presContext = aStyleContext->PresContext();
  NS_ABORT_IF_FALSE(presContext, "NULL pres context in GetFontXHeight");

  nsRefPtr<nsFontMetrics> fontMetrics;
  nsLayoutUtils::GetFontMetricsForStyleContext(aStyleContext,
                                               getter_AddRefs(fontMetrics));

  if (!fontMetrics) {
    
    NS_WARNING("no FontMetrics in GetFontXHeight()");
    return 1.0f;
  }

  nscoord xHeight = fontMetrics->XHeight();
  return nsPresContext::AppUnitsToFloatCSSPixels(xHeight) /
         presContext->TextZoom();
}
nsresult
SVGContentUtils::ReportToConsole(nsIDocument* doc,
                                 const char* aWarning,
                                 const PRUnichar **aParams,
                                 uint32_t aParamsLength)
{
  return nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                         NS_LITERAL_CSTRING("SVG"), doc,
                                         nsContentUtils::eSVG_PROPERTIES,
                                         aWarning,
                                         aParams, aParamsLength);
}

bool
SVGContentUtils::EstablishesViewport(nsIContent *aContent)
{
  
  
  
  return aContent && aContent->IsSVG() &&
           (aContent->Tag() == nsGkAtoms::svg ||
            aContent->Tag() == nsGkAtoms::foreignObject ||
            aContent->Tag() == nsGkAtoms::symbol);
}

nsSVGElement*
SVGContentUtils::GetNearestViewportElement(nsIContent *aContent)
{
  nsIContent *element = aContent->GetFlattenedTreeParent();

  while (element && element->IsSVG()) {
    if (EstablishesViewport(element)) {
      if (element->Tag() == nsGkAtoms::foreignObject) {
        return nullptr;
      }
      return static_cast<nsSVGElement*>(element);
    }
    element = element->GetFlattenedTreeParent();
  }
  return nullptr;
}

static gfxMatrix
GetCTMInternal(nsSVGElement *aElement, bool aScreenCTM, bool aHaveRecursed)
{
  gfxMatrix matrix = aElement->PrependLocalTransformsTo(gfxMatrix(),
    aHaveRecursed ? nsSVGElement::eAllTransforms : nsSVGElement::eUserSpaceToParent);
  nsSVGElement *element = aElement;
  nsIContent *ancestor = aElement->GetFlattenedTreeParent();

  while (ancestor && ancestor->IsSVG() &&
                     ancestor->Tag() != nsGkAtoms::foreignObject) {
    element = static_cast<nsSVGElement*>(ancestor);
    matrix *= element->PrependLocalTransformsTo(gfxMatrix()); 
    if (!aScreenCTM && SVGContentUtils::EstablishesViewport(element)) {
      if (!element->NodeInfo()->Equals(nsGkAtoms::svg, kNameSpaceID_SVG) &&
          !element->NodeInfo()->Equals(nsGkAtoms::symbol, kNameSpaceID_SVG)) {
        NS_ERROR("New (SVG > 1.1) SVG viewport establishing element?");
        return gfxMatrix(0.0, 0.0, 0.0, 0.0, 0.0, 0.0); 
      }
      
      return matrix;
    }
    ancestor = ancestor->GetFlattenedTreeParent();
  }
  if (!aScreenCTM) {
    
    return gfxMatrix(0.0, 0.0, 0.0, 0.0, 0.0, 0.0); 
  }
  if (element->Tag() != nsGkAtoms::svg) {
    
    return gfxMatrix(0.0, 0.0, 0.0, 0.0, 0.0, 0.0); 
  }
  if (element == aElement && !aHaveRecursed) {
    
    
    
    
    
    
    matrix = aElement->PrependLocalTransformsTo(gfxMatrix());
  }
  if (!ancestor || !ancestor->IsElement()) {
    return matrix;
  }
  if (ancestor->IsSVG()) {
    return
      matrix * GetCTMInternal(static_cast<nsSVGElement*>(ancestor), true, true);
  }

  
  
  nsIDocument* currentDoc = aElement->GetCurrentDoc();
  float x = 0.0f, y = 0.0f;
  if (currentDoc && element->NodeInfo()->Equals(nsGkAtoms::svg, kNameSpaceID_SVG)) {
    nsIPresShell *presShell = currentDoc->GetShell();
    if (presShell) {
      nsIFrame* frame = element->GetPrimaryFrame();
      nsIFrame* ancestorFrame = presShell->GetRootFrame();
      if (frame && ancestorFrame) {
        nsPoint point = frame->GetOffsetTo(ancestorFrame);
        x = nsPresContext::AppUnitsToFloatCSSPixels(point.x);
        y = nsPresContext::AppUnitsToFloatCSSPixels(point.y);
      }
    }
  }
  return matrix * gfxMatrix().Translate(gfxPoint(x, y));
}

gfxMatrix
SVGContentUtils::GetCTM(nsSVGElement *aElement, bool aScreenCTM)
{
  return GetCTMInternal(aElement, aScreenCTM, false);
}

double
SVGContentUtils::ComputeNormalizedHypotenuse(double aWidth, double aHeight)
{
  return sqrt((aWidth*aWidth + aHeight*aHeight)/2);
}

float
SVGContentUtils::AngleBisect(float a1, float a2)
{
  float delta = fmod(a2 - a1, static_cast<float>(2*M_PI));
  if (delta < 0) {
    delta += static_cast<float>(2*M_PI);
  }
  
  float r = a1 + delta/2;
  if (delta >= M_PI) {
    
    r += static_cast<float>(M_PI);
  }
  return r;
}

gfxMatrix
SVGContentUtils::GetViewBoxTransform(float aViewportWidth, float aViewportHeight,
                                     float aViewboxX, float aViewboxY,
                                     float aViewboxWidth, float aViewboxHeight,
                                     const SVGAnimatedPreserveAspectRatio &aPreserveAspectRatio)
{
  return GetViewBoxTransform(aViewportWidth, aViewportHeight,
                             aViewboxX, aViewboxY,
                             aViewboxWidth, aViewboxHeight,
                             aPreserveAspectRatio.GetAnimValue());
}

gfxMatrix
SVGContentUtils::GetViewBoxTransform(float aViewportWidth, float aViewportHeight,
                                     float aViewboxX, float aViewboxY,
                                     float aViewboxWidth, float aViewboxHeight,
                                     const SVGPreserveAspectRatio &aPreserveAspectRatio)
{
  NS_ASSERTION(aViewportWidth  >= 0, "viewport width must be nonnegative!");
  NS_ASSERTION(aViewportHeight >= 0, "viewport height must be nonnegative!");
  NS_ASSERTION(aViewboxWidth  > 0, "viewBox width must be greater than zero!");
  NS_ASSERTION(aViewboxHeight > 0, "viewBox height must be greater than zero!");

  SVGAlign align = aPreserveAspectRatio.GetAlign();
  SVGMeetOrSlice meetOrSlice = aPreserveAspectRatio.GetMeetOrSlice();

  
  if (align == SVG_PRESERVEASPECTRATIO_UNKNOWN)
    align = SVG_PRESERVEASPECTRATIO_XMIDYMID;
  if (meetOrSlice == SVG_MEETORSLICE_UNKNOWN)
    meetOrSlice = SVG_MEETORSLICE_MEET;

  float a, d, e, f;
  a = aViewportWidth / aViewboxWidth;
  d = aViewportHeight / aViewboxHeight;
  e = 0.0f;
  f = 0.0f;

  if (align != SVG_PRESERVEASPECTRATIO_NONE &&
      a != d) {
    if ((meetOrSlice == SVG_MEETORSLICE_MEET && a < d) ||
        (meetOrSlice == SVG_MEETORSLICE_SLICE && d < a)) {
      d = a;
      switch (align) {
      case SVG_PRESERVEASPECTRATIO_XMINYMIN:
      case SVG_PRESERVEASPECTRATIO_XMIDYMIN:
      case SVG_PRESERVEASPECTRATIO_XMAXYMIN:
        break;
      case SVG_PRESERVEASPECTRATIO_XMINYMID:
      case SVG_PRESERVEASPECTRATIO_XMIDYMID:
      case SVG_PRESERVEASPECTRATIO_XMAXYMID:
        f = (aViewportHeight - a * aViewboxHeight) / 2.0f;
        break;
      case SVG_PRESERVEASPECTRATIO_XMINYMAX:
      case SVG_PRESERVEASPECTRATIO_XMIDYMAX:
      case SVG_PRESERVEASPECTRATIO_XMAXYMAX:
        f = aViewportHeight - a * aViewboxHeight;
        break;
      default:
        NS_NOTREACHED("Unknown value for align");
      }
    }
    else if (
      (meetOrSlice == SVG_MEETORSLICE_MEET &&
      d < a) ||
      (meetOrSlice == SVG_MEETORSLICE_SLICE &&
      a < d)) {
      a = d;
      switch (align) {
      case SVG_PRESERVEASPECTRATIO_XMINYMIN:
      case SVG_PRESERVEASPECTRATIO_XMINYMID:
      case SVG_PRESERVEASPECTRATIO_XMINYMAX:
        break;
      case SVG_PRESERVEASPECTRATIO_XMIDYMIN:
      case SVG_PRESERVEASPECTRATIO_XMIDYMID:
      case SVG_PRESERVEASPECTRATIO_XMIDYMAX:
        e = (aViewportWidth - a * aViewboxWidth) / 2.0f;
        break;
      case SVG_PRESERVEASPECTRATIO_XMAXYMIN:
      case SVG_PRESERVEASPECTRATIO_XMAXYMID:
      case SVG_PRESERVEASPECTRATIO_XMAXYMAX:
        e = aViewportWidth - a * aViewboxWidth;
        break;
      default:
        NS_NOTREACHED("Unknown value for align");
      }
    }
    else NS_NOTREACHED("Unknown value for meetOrSlice");
  }
  
  if (aViewboxX) e += -a * aViewboxX;
  if (aViewboxY) f += -d * aViewboxY;
  
  return gfxMatrix(a, 0.0f, 0.0f, d, e, f);
}




static inline bool
IsDigit(PRUnichar aCh)
{
  return aCh >= '0' && aCh <= '9';
}




static inline uint32_t
DecimalDigitValue(PRUnichar aCh)
{
  MOZ_ASSERT(IsDigit(aCh), "Digit expected");
  return aCh - '0';
}

template<class floatType>
bool
SVGContentUtils::ParseNumber(const nsAString& aString, 
                             floatType& aValue,
                             nsAString& aLeftOver)
{
  mozilla::RangedPtr<const PRUnichar> iter(aString.Data(), aString.Length());
  const mozilla::RangedPtr<const PRUnichar> end(aString.Data() + aString.Length(),
                                                aString.Data(), aString.Length());

  if (iter == end) {
    return false;
  }

  
  int32_t sign = *iter == '-' ? -1 : 1;

  if (*iter == '-' || *iter == '+') {
    ++iter;
    if (iter == end) {
      return false;
    }
  }

  
  floatType intPart = floatType(0);

  bool gotDot = *iter == '.';

  if (!gotDot) {
    if (!IsDigit(*iter)) {
      return false;
    }
    do {
      intPart = floatType(10) * intPart + DecimalDigitValue(*iter);
      ++iter;
    } while (iter != end && IsDigit(*iter));

    if (iter != end) {
      gotDot = *iter == '.';
    }
  }

  
  floatType fracPart = floatType(0);

  if (gotDot) {
    ++iter;
    if (iter == end || !IsDigit(*iter)) {
      return false;
    }
    
    floatType divisor = floatType(10);
    do {
      fracPart += DecimalDigitValue(*iter) / divisor;
      divisor *= 10;
      ++iter;
    } while (iter != end && IsDigit(*iter));
  }

  bool gotE = false;
  int32_t exponent = 0;
  int32_t expSign;

  if (iter != end && (*iter == 'e' || *iter == 'E')) {

    mozilla::RangedPtr<const PRUnichar> expIter(iter);

    ++expIter;
    if (expIter != end) {
      expSign = *expIter == '-' ? -1 : 1;
      if (*expIter == '-' || *expIter == '+') {
        ++expIter;
        if (expIter != end && IsDigit(*expIter)) {
          
          
          gotE = true;
        }
      }
    }

    if (gotE) {
      iter = expIter;
      do {
        exponent = 10 * exponent + DecimalDigitValue(*iter);
        ++iter;
      } while (iter != end && IsDigit(*iter));
    }
  }

  
  aValue = sign * (intPart + fracPart);
  if (gotE) {
    aValue *= pow(floatType(10), floatType(expSign * exponent));
  }
  
  aLeftOver = Substring(iter.get(), end.get());
  return NS_finite(aValue);
}

template bool
SVGContentUtils::ParseNumber<float>(const nsAString& aString, 
                                    float& aValue,
                                    nsAString& aLeftOver);
template bool
SVGContentUtils::ParseNumber<double>(const nsAString& aString, 
                                     double& aValue,
                                     nsAString& aLeftOver);

template<class floatType>
bool
SVGContentUtils::ParseNumber(const nsAString& aString, 
                             floatType& aValue)
{
  nsAutoString leftOver;

  if (!ParseNumber(aString, aValue, leftOver)) {
    return false;
  }

  return leftOver.IsEmpty();
}

template bool
SVGContentUtils::ParseNumber<float>(const nsAString& aString, 
                                    float& aValue);
template bool
SVGContentUtils::ParseNumber<double>(const nsAString& aString, 
                                     double& aValue);

bool
SVGContentUtils::ParseInteger(const nsAString& aString,
                              int32_t& aValue)
{
  mozilla::RangedPtr<const PRUnichar> iter(aString.Data(), aString.Length());
  const mozilla::RangedPtr<const PRUnichar> end(aString.Data() + aString.Length(),
                                                aString.Data(), aString.Length());

  if (iter == end) {
    return false;
  }

  int32_t sign = *iter == '-' ? -1 : 1;

  if (*iter == '-' || *iter == '+') {
    ++iter;
    if (iter == end) {
      return false;
    }
  }

  int64_t value = 0;

  do {
    if (!IsDigit(*iter)) {
      return false;
    }
    if (value <= std::numeric_limits<int32_t>::max()) {
      value = 10 * value + DecimalDigitValue(*iter);
    }
    ++iter;
  } while (iter != end);

  aValue = int32_t(clamped(sign * value,
                           int64_t(std::numeric_limits<int32_t>::min()),
                           int64_t(std::numeric_limits<int32_t>::max())));
  return true;
}
