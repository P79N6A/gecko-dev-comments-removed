






































#include "StyleInfo.h"

#include "mozilla/dom/Element.h"
#include "nsComputedDOMStyle.h"

using namespace mozilla;
using namespace mozilla::a11y;

StyleInfo::StyleInfo(dom::Element* aElement, nsIPresShell* aPresShell) :
  mElement(aElement)
{
  mStyleContext =
    nsComputedDOMStyle::GetStyleContextForElementNoFlush(aElement,
                                                         nsnull,
                                                         aPresShell);
}

void
StyleInfo::Display(nsAString& aValue)
{
  aValue.Truncate();
  AppendASCIItoUTF16(
    nsCSSProps::ValueToKeyword(mStyleContext->GetStyleDisplay()->mDisplay,
                               nsCSSProps::kDisplayKTable), aValue);
}

void
StyleInfo::TextAlign(nsAString& aValue)
{
  aValue.Truncate();
  AppendASCIItoUTF16(
    nsCSSProps::ValueToKeyword(mStyleContext->GetStyleText()->mTextAlign,
                               nsCSSProps::kTextAlignKTable), aValue);
}

void
StyleInfo::TextIndent(nsAString& aValue)
{
  aValue.Truncate();

  const nsStyleCoord& styleCoord =
    mStyleContext->GetStyleText()->mTextIndent;

  nscoord coordVal;
  switch (styleCoord.GetUnit()) {
    case eStyleUnit_Coord:
      coordVal = styleCoord.GetCoordValue();
      break;

    case eStyleUnit_Percent:
    {
      nsIFrame* frame = mElement->GetPrimaryFrame();
      nsIFrame* containerFrame = frame->GetContainingBlock();
      nscoord percentageBase = containerFrame->GetContentRect().width;
      coordVal = NSCoordSaturatingMultiply(percentageBase,
                                           styleCoord.GetPercentValue());
      break;
    }
  }

  aValue.AppendFloat(nsPresContext::AppUnitsToFloatCSSPixels(coordVal));
  aValue.AppendLiteral("px");
}

void
StyleInfo::Margin(css::Side aSide, nsAString& aValue)
{
  aValue.Truncate();

  nscoord coordVal = mElement->GetPrimaryFrame()->GetUsedMargin().Side(aSide);
  aValue.AppendFloat(nsPresContext::AppUnitsToFloatCSSPixels(coordVal));
  aValue.AppendLiteral("px");
}

void
StyleInfo::Format(const nscolor& aValue, nsString& aFormattedValue)
{
  
  aFormattedValue.AppendLiteral("rgb(");
  aFormattedValue.AppendInt(NS_GET_R(aValue));
  aFormattedValue.AppendLiteral(", ");
  aFormattedValue.AppendInt(NS_GET_G(aValue));
  aFormattedValue.AppendLiteral(", ");
  aFormattedValue.AppendInt(NS_GET_B(aValue));
  aFormattedValue.Append(')');
}
