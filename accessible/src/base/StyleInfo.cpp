





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
                                                         nullptr,
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

  nscoord coordVal = 0;
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

    case eStyleUnit_Null:
    case eStyleUnit_Normal:
    case eStyleUnit_Auto:
    case eStyleUnit_None:
    case eStyleUnit_Factor:
    case eStyleUnit_Degree:
    case eStyleUnit_Grad:
    case eStyleUnit_Radian:
    case eStyleUnit_Turn:
    case eStyleUnit_Integer:
    case eStyleUnit_Enumerated:
    case eStyleUnit_Calc:
      break;
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
StyleInfo::FormatColor(const nscolor& aValue, nsString& aFormattedValue)
{
  
  aFormattedValue.AppendLiteral("rgb(");
  aFormattedValue.AppendInt(NS_GET_R(aValue));
  aFormattedValue.AppendLiteral(", ");
  aFormattedValue.AppendInt(NS_GET_G(aValue));
  aFormattedValue.AppendLiteral(", ");
  aFormattedValue.AppendInt(NS_GET_B(aValue));
  aFormattedValue.Append(')');
}

void
StyleInfo::FormatFontStyle(const nscoord& aValue, nsAString& aFormattedValue)
{
  nsCSSKeyword keyword =
    nsCSSProps::ValueToKeywordEnum(aValue, nsCSSProps::kFontStyleKTable);
  AppendUTF8toUTF16(nsCSSKeywords::GetStringValue(keyword), aFormattedValue);
}

void
StyleInfo::FormatTextDecorationStyle(PRUint8 aValue, nsAString& aFormattedValue)
{
  nsCSSKeyword keyword =
    nsCSSProps::ValueToKeywordEnum(aValue,
                                   nsCSSProps::kTextDecorationStyleKTable);
  AppendUTF8toUTF16(nsCSSKeywords::GetStringValue(keyword), aFormattedValue);
}
