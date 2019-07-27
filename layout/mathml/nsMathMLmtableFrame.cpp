




#include "nsMathMLmtableFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsNameSpaceManager.h"
#include "nsRenderingContext.h"
#include "nsCSSRendering.h"
#include "nsMathMLElement.h"

#include "nsTArray.h"
#include "nsTableFrame.h"
#include "celldata.h"

#include "RestyleManager.h"
#include <algorithm>

#include "nsIScriptError.h"
#include "nsContentUtils.h"

using namespace mozilla;





static int8_t
ParseStyleValue(nsIAtom* aAttribute, const nsAString& aAttributeValue)
{
  if (aAttribute == nsGkAtoms::rowalign_) {
    if (aAttributeValue.EqualsLiteral("top"))
      return NS_STYLE_VERTICAL_ALIGN_TOP;
    else if (aAttributeValue.EqualsLiteral("bottom"))
      return NS_STYLE_VERTICAL_ALIGN_BOTTOM;
    else if (aAttributeValue.EqualsLiteral("center"))
      return NS_STYLE_VERTICAL_ALIGN_MIDDLE;
    else
      return NS_STYLE_VERTICAL_ALIGN_BASELINE;
  } else if (aAttribute == nsGkAtoms::columnalign_) {
    if (aAttributeValue.EqualsLiteral("left"))
      return NS_STYLE_TEXT_ALIGN_LEFT;
    else if (aAttributeValue.EqualsLiteral("right"))
      return NS_STYLE_TEXT_ALIGN_RIGHT;
    else
      return NS_STYLE_TEXT_ALIGN_CENTER;
  } else if (aAttribute == nsGkAtoms::rowlines_ ||
             aAttribute == nsGkAtoms::columnlines_) {
    if (aAttributeValue.EqualsLiteral("solid"))
      return NS_STYLE_BORDER_STYLE_SOLID;
    else if (aAttributeValue.EqualsLiteral("dashed"))
      return NS_STYLE_BORDER_STYLE_DASHED;
    else
      return NS_STYLE_BORDER_STYLE_NONE;
  } else {
    MOZ_CRASH("Unrecognized attribute.");
  }

  return -1;
}

static nsTArray<int8_t>*
ExtractStyleValues(const nsAString& aString, nsIAtom* aAttribute,
                   bool aAllowMultiValues)
{
  nsTArray<int8_t>* styleArray = nullptr;

  const char16_t* start = aString.BeginReading();
  const char16_t* end = aString.EndReading();

  int32_t startIndex = 0;
  int32_t count = 0;

  while (start < end) {
    
    while ((start < end) && nsCRT::IsAsciiSpace(*start)) {
      start++;
      startIndex++;
    }

    
    while ((start < end) && !nsCRT::IsAsciiSpace(*start)) {
      start++;
      count++;
    }

    
    if (count > 0) {
      if (!styleArray)
        styleArray = new nsTArray<int8_t>();

      
      
      if (styleArray->Length() > 1 && !aAllowMultiValues) {
        delete styleArray;
        return nullptr;
      }

      nsDependentSubstring valueString(aString, startIndex, count);
      int8_t styleValue = ParseStyleValue(aAttribute, valueString);
      styleArray->AppendElement(styleValue);

      startIndex += count;
      count = 0;
    }
  }
  return styleArray;
}

static nsresult ReportParseError(nsIFrame* aFrame, const char16_t* aAttribute,
                                 const char16_t* aValue)
{
  nsIContent* content = aFrame->GetContent();

  const char16_t* params[] =
    { aValue, aAttribute, content->NodeInfo()->NameAtom()->GetUTF16String() };

  return nsContentUtils::ReportToConsole(nsIScriptError::errorFlag,
                                         NS_LITERAL_CSTRING("Layout: MathML"),
                                         content->OwnerDoc(),
                                         nsContentUtils::eMATHML_PROPERTIES,
                                         "AttributeParsingError", params, 3);
}






NS_DECLARE_FRAME_PROPERTY(RowAlignProperty, DeleteValue<nsTArray<int8_t>>)
NS_DECLARE_FRAME_PROPERTY(RowLinesProperty, DeleteValue<nsTArray<int8_t>>)
NS_DECLARE_FRAME_PROPERTY(ColumnAlignProperty, DeleteValue<nsTArray<int8_t>>)
NS_DECLARE_FRAME_PROPERTY(ColumnLinesProperty, DeleteValue<nsTArray<int8_t>>)

static const FramePropertyDescriptor*
AttributeToProperty(nsIAtom* aAttribute)
{
  if (aAttribute == nsGkAtoms::rowalign_)
    return RowAlignProperty();
  if (aAttribute == nsGkAtoms::rowlines_)
    return RowLinesProperty();
  if (aAttribute == nsGkAtoms::columnalign_)
    return ColumnAlignProperty();
  NS_ASSERTION(aAttribute == nsGkAtoms::columnlines_, "Invalid attribute");
  return ColumnLinesProperty();
}







static nsTArray<int8_t>*
FindCellProperty(const nsIFrame* aCellFrame,
                 const FramePropertyDescriptor* aFrameProperty)
{
  const nsIFrame* currentFrame = aCellFrame;
  nsTArray<int8_t>* propertyData = nullptr;

  while (currentFrame) {
    FrameProperties props = currentFrame->Properties();
    propertyData = static_cast<nsTArray<int8_t>*>(props.Get(aFrameProperty));
    bool frameIsTable = (currentFrame->GetType() == nsGkAtoms::tableFrame);

    if (propertyData || frameIsTable)
      currentFrame = nullptr; 
    else
      currentFrame = currentFrame->GetParent(); 
  }

  return propertyData;
}

static void
ApplyBorderToStyle(const nsMathMLmtdFrame* aFrame,
                   nsStyleBorder& aStyleBorder)
{
  int32_t rowIndex;
  int32_t columnIndex;
  aFrame->GetRowIndex(rowIndex);
  aFrame->GetColIndex(columnIndex);

  nscoord borderWidth =
    aFrame->PresContext()->GetBorderWidthTable()[NS_STYLE_BORDER_WIDTH_THIN];

  nsTArray<int8_t>* rowLinesList =
    FindCellProperty(aFrame, RowLinesProperty());

  nsTArray<int8_t>* columnLinesList =
    FindCellProperty(aFrame, ColumnLinesProperty());

  
  if (rowIndex > 0 && rowLinesList) {
    
    
    int32_t listLength = rowLinesList->Length();
    if (rowIndex < listLength) {
      aStyleBorder.SetBorderStyle(NS_SIDE_TOP,
                    rowLinesList->ElementAt(rowIndex - 1));
    } else {
      aStyleBorder.SetBorderStyle(NS_SIDE_TOP,
                    rowLinesList->ElementAt(listLength - 1));
    }
    aStyleBorder.SetBorderWidth(NS_SIDE_TOP, borderWidth);
  }

  
  if (columnIndex > 0 && columnLinesList) {
    
    
    int32_t listLength = columnLinesList->Length();
    if (columnIndex < listLength) {
      aStyleBorder.SetBorderStyle(NS_SIDE_LEFT,
                    columnLinesList->ElementAt(columnIndex - 1));
    } else {
      aStyleBorder.SetBorderStyle(NS_SIDE_LEFT,
                    columnLinesList->ElementAt(listLength - 1));
    }
    aStyleBorder.SetBorderWidth(NS_SIDE_LEFT, borderWidth);
  }
}

static nsMargin
ComputeBorderOverflow(nsMathMLmtdFrame* aFrame, nsStyleBorder aStyleBorder)
{
  nsMargin overflow;
  int32_t rowIndex;
  int32_t columnIndex;
  nsTableFrame* table = aFrame->GetTableFrame();
  aFrame->GetCellIndexes(rowIndex, columnIndex);
  if (!columnIndex) {
    overflow.left = table->GetColSpacing(-1);
    overflow.right = table->GetColSpacing(0) / 2;
  } else if (columnIndex == table->GetColCount() - 1) {
    overflow.left = table->GetColSpacing(columnIndex - 1) / 2;
    overflow.right =  table->GetColSpacing(columnIndex + 1);
  } else {
    overflow.left = table->GetColSpacing(columnIndex - 1) / 2;
    overflow.right = table->GetColSpacing(columnIndex) / 2;
  }
  if (!rowIndex) {
    overflow.top = table->GetRowSpacing(-1);
    overflow.bottom = table->GetRowSpacing(0) / 2;
  } else if (rowIndex == table->GetRowCount() - 1) {
    overflow.top = table->GetRowSpacing(rowIndex - 1) / 2;
    overflow.bottom = table->GetRowSpacing(rowIndex + 1);
  } else {
    overflow.top = table->GetRowSpacing(rowIndex - 1) / 2;
    overflow.bottom = table->GetRowSpacing(rowIndex) / 2;
  }
  return overflow;
}






class nsDisplaymtdBorder : public nsDisplayBorder {
public:
  nsDisplaymtdBorder(nsDisplayListBuilder* aBuilder, nsMathMLmtdFrame* aFrame)
    : nsDisplayBorder(aBuilder, aFrame)
  {
  }

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) override
  {
    *aSnap = true;
    nsStyleBorder styleBorder = *mFrame->StyleBorder();
    nsMathMLmtdFrame* frame = static_cast<nsMathMLmtdFrame*>(mFrame);
    ApplyBorderToStyle(frame, styleBorder);
    nsRect bounds = CalculateBounds(styleBorder);
    nsMargin overflow = ComputeBorderOverflow(frame, styleBorder);
    bounds.Inflate(overflow);
    return bounds;
  }

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) override
  {
    nsStyleBorder styleBorder = *mFrame->StyleBorder();
    nsMathMLmtdFrame* frame = static_cast<nsMathMLmtdFrame*>(mFrame);
    ApplyBorderToStyle(frame, styleBorder);

    nsRect bounds = nsRect(ToReferenceFrame(), mFrame->GetSize());
    nsMargin overflow = ComputeBorderOverflow(frame, styleBorder);
    bounds.Inflate(overflow);

    nsCSSRendering::PaintBorderWithStyleBorder(mFrame->PresContext(), *aCtx,
                                               mFrame, mVisibleRect,
                                               bounds,
                                               styleBorder,
                                               mFrame->StyleContext(),
                                               mFrame->GetSkipSides());
  }
};

#ifdef DEBUG
#define DEBUG_VERIFY_THAT_FRAME_IS(_frame, _expected) \
  NS_ASSERTION(NS_STYLE_DISPLAY_##_expected == _frame->StyleDisplay()->mDisplay, "internal error");
#else
#define DEBUG_VERIFY_THAT_FRAME_IS(_frame, _expected)
#endif

static void
ParseFrameAttribute(nsIFrame* aFrame, nsIAtom* aAttribute,
                    bool aAllowMultiValues)
{
  nsAutoString attrValue;

  nsIContent* frameContent = aFrame->GetContent();
  frameContent->GetAttr(kNameSpaceID_None, aAttribute, attrValue);

  if (!attrValue.IsEmpty()) {
    nsTArray<int8_t>* valueList =
      ExtractStyleValues(attrValue, aAttribute, aAllowMultiValues);

    
    
    if (valueList) {
      
      NS_ASSERTION(valueList->Length() >= 1, "valueList should not be empty!");
      FrameProperties props = aFrame->Properties();
      props.Set(AttributeToProperty(aAttribute), valueList);
    } else {
      ReportParseError(aFrame, aAttribute->GetUTF16String(), attrValue.get());
    }
  }
}



























































static const float kDefaultRowspacingEx = 1.0f;
static const float kDefaultColumnspacingEm = 0.8f;
static const float kDefaultFramespacingArg0Em = 0.4f;
static const float kDefaultFramespacingArg1Ex = 0.5f;

static void
ExtractSpacingValues(const nsAString&   aString,
                     nsIAtom*           aAttribute,
                     nsTArray<nscoord>& aSpacingArray,
                     nsIFrame*          aFrame,
                     nscoord            aDefaultValue0,
                     nscoord            aDefaultValue1,
                     float              aFontSizeInflation)
{
  nsPresContext* presContext = aFrame->PresContext();
  nsStyleContext* styleContext = aFrame->StyleContext();

  const char16_t* start = aString.BeginReading();
  const char16_t* end = aString.EndReading();

  int32_t startIndex = 0;
  int32_t count = 0;
  int32_t elementNum = 0;

  while (start < end) {
    
    while ((start < end) && nsCRT::IsAsciiSpace(*start)) {
      start++;
      startIndex++;
    }

    
    while ((start < end) && !nsCRT::IsAsciiSpace(*start)) {
      start++;
      count++;
    }

    
    if (count > 0) {
      const nsAString& str = Substring(aString, startIndex, count);
      nsAutoString valueString;
      valueString.Assign(str);
      nscoord newValue;
      if (aAttribute == nsGkAtoms::framespacing_ && elementNum) {
        newValue = aDefaultValue1;
      } else {
        newValue = aDefaultValue0;
      }
      nsMathMLFrame::ParseNumericValue(valueString, &newValue,
                                       nsMathMLElement::PARSE_ALLOW_UNITLESS,
                                       presContext, styleContext,
                                       aFontSizeInflation);
      aSpacingArray.AppendElement(newValue);

      startIndex += count;
      count = 0;
      elementNum++;
    }
  }
}

static void
ParseSpacingAttribute(nsMathMLmtableFrame* aFrame, nsIAtom* aAttribute)
{
  NS_ASSERTION(aAttribute == nsGkAtoms::rowspacing_ ||
               aAttribute == nsGkAtoms::columnspacing_ ||
               aAttribute == nsGkAtoms::framespacing_,
               "Non spacing attribute passed");

  nsAutoString attrValue;
  nsIContent* frameContent = aFrame->GetContent();
  frameContent->GetAttr(kNameSpaceID_None, aAttribute, attrValue);

  if (nsGkAtoms::framespacing_ == aAttribute) {
    nsAutoString frame;
    frameContent->GetAttr(kNameSpaceID_None, nsGkAtoms::frame, frame);
    if (frame.IsEmpty() || frame.EqualsLiteral("none")) {
      aFrame->SetFrameSpacing(0, 0);
      return;
    }
  }

  nscoord value;
  nscoord value2;
  
  float fontSizeInflation = nsLayoutUtils::FontSizeInflationFor(aFrame);
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(aFrame, getter_AddRefs(fm),
                                        fontSizeInflation);
  if (nsGkAtoms::rowspacing_ == aAttribute) {
    value = kDefaultRowspacingEx * fm->XHeight();
    value2 = 0;
  } else if (nsGkAtoms::columnspacing_ == aAttribute) {
    value = kDefaultColumnspacingEm * fm->EmHeight();
    value2 = 0;
  } else {
    value = kDefaultFramespacingArg0Em * fm->EmHeight();
    value2 = kDefaultFramespacingArg1Ex * fm->XHeight();
  }

  nsTArray<nscoord> valueList;
  ExtractSpacingValues(attrValue, aAttribute, valueList, aFrame, value, value2,
                       fontSizeInflation);
  if (valueList.Length() == 0) {
    if (frameContent->HasAttr(kNameSpaceID_None, aAttribute)) {
      ReportParseError(aFrame, aAttribute->GetUTF16String(),
                       attrValue.get());
    }
    valueList.AppendElement(value);
  }
  if (aAttribute == nsGkAtoms::framespacing_) {
    if (valueList.Length() == 1) {
      if(frameContent->HasAttr(kNameSpaceID_None, aAttribute)) {
        ReportParseError(aFrame, aAttribute->GetUTF16String(),
                         attrValue.get());
      }
      valueList.AppendElement(value2);
    } else if (valueList.Length() != 2) {
      ReportParseError(aFrame, aAttribute->GetUTF16String(),
                       attrValue.get());
    }
  }

  if (aAttribute == nsGkAtoms::rowspacing_) {
    aFrame->SetRowSpacingArray(valueList);
  } else if (aAttribute == nsGkAtoms::columnspacing_) {
    aFrame->SetColSpacingArray(valueList);
  } else {
      aFrame->SetFrameSpacing(valueList.ElementAt(0),
                              valueList.ElementAt(1));
  }
}

static void ParseSpacingAttributes(nsMathMLmtableFrame* aTableFrame)
{
  ParseSpacingAttribute(aTableFrame, nsGkAtoms::rowspacing_);
  ParseSpacingAttribute(aTableFrame, nsGkAtoms::columnspacing_);
  ParseSpacingAttribute(aTableFrame, nsGkAtoms::framespacing_);
  aTableFrame->SetUseCSSSpacing();
}



static void
MapAllAttributesIntoCSS(nsMathMLmtableFrame* aTableFrame)
{
  
  ParseFrameAttribute(aTableFrame, nsGkAtoms::rowalign_, true);
  ParseFrameAttribute(aTableFrame, nsGkAtoms::rowlines_, true);

  
  ParseFrameAttribute(aTableFrame, nsGkAtoms::columnalign_, true);
  ParseFrameAttribute(aTableFrame, nsGkAtoms::columnlines_, true);

  
  ParseSpacingAttributes(aTableFrame);

  
  nsIFrame* rgFrame = aTableFrame->GetFirstPrincipalChild();
  if (!rgFrame || rgFrame->GetType() != nsGkAtoms::tableRowGroupFrame)
    return;

  nsIFrame* rowFrame = rgFrame->GetFirstPrincipalChild();
  for ( ; rowFrame; rowFrame = rowFrame->GetNextSibling()) {
    DEBUG_VERIFY_THAT_FRAME_IS(rowFrame, TABLE_ROW);
    if (rowFrame->GetType() == nsGkAtoms::tableRowFrame) {
      
      ParseFrameAttribute(rowFrame, nsGkAtoms::rowalign_, false);
      
      ParseFrameAttribute(rowFrame, nsGkAtoms::columnalign_, true);

      nsIFrame* cellFrame = rowFrame->GetFirstPrincipalChild();
      for ( ; cellFrame; cellFrame = cellFrame->GetNextSibling()) {
        DEBUG_VERIFY_THAT_FRAME_IS(cellFrame, TABLE_CELL);
        if (IS_TABLE_CELL(cellFrame->GetType())) {
          
          ParseFrameAttribute(cellFrame, nsGkAtoms::rowalign_, false);
          
          ParseFrameAttribute(cellFrame, nsGkAtoms::columnalign_, false);
        }
      }
    }
  }
}












enum eAlign {
  eAlign_top,
  eAlign_bottom,
  eAlign_center,
  eAlign_baseline,
  eAlign_axis
};

static void
ParseAlignAttribute(nsString& aValue, eAlign& aAlign, int32_t& aRowIndex)
{
  
  aRowIndex = 0;
  aAlign = eAlign_axis;
  int32_t len = 0;

  
  
  aValue.CompressWhitespace(true, false);

  if (0 == aValue.Find("top")) {
    len = 3; 
    aAlign = eAlign_top;
  }
  else if (0 == aValue.Find("bottom")) {
    len = 6; 
    aAlign = eAlign_bottom;
  }
  else if (0 == aValue.Find("center")) {
    len = 6; 
    aAlign = eAlign_center;
  }
  else if (0 == aValue.Find("baseline")) {
    len = 8; 
    aAlign = eAlign_baseline;
  }
  else if (0 == aValue.Find("axis")) {
    len = 4; 
    aAlign = eAlign_axis;
  }
  if (len) {
    nsresult error;
    aValue.Cut(0, len); 
    aRowIndex = aValue.ToInteger(&error);
    if (NS_FAILED(error))
      aRowIndex = 0;
  }
}

#ifdef DEBUG_rbs_off

static void
ListMathMLTree(nsIFrame* atLeast)
{
  
  nsIFrame* f = atLeast;
  for ( ; f; f = f->GetParent()) {
    nsIContent* c = f->GetContent();
    if (!c || c->IsMathMLElement(nsGkAtoms::math) ||
        c->NodeInfo()->NameAtom(nsGkAtoms::body))  
      break;
  }
  if (!f) f = atLeast;
  f->List(stdout, 0);
}
#endif




NS_QUERYFRAME_HEAD(nsMathMLmtableOuterFrame)
  NS_QUERYFRAME_ENTRY(nsIMathMLFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsTableOuterFrame)

nsContainerFrame*
NS_NewMathMLmtableOuterFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmtableOuterFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmtableOuterFrame)
 
nsMathMLmtableOuterFrame::~nsMathMLmtableOuterFrame()
{
}

nsresult
nsMathMLmtableOuterFrame::AttributeChanged(int32_t  aNameSpaceID,
                                           nsIAtom* aAttribute,
                                           int32_t  aModType)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  nsIFrame* tableFrame = mFrames.FirstChild();
  NS_ASSERTION(tableFrame && tableFrame->GetType() == nsGkAtoms::tableFrame,
               "should always have an inner table frame");
  nsIFrame* rgFrame = tableFrame->GetFirstPrincipalChild();
  if (!rgFrame || rgFrame->GetType() != nsGkAtoms::tableRowGroupFrame)
    return NS_OK;

  
  if (aAttribute == nsGkAtoms::align) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eResize, NS_FRAME_IS_DIRTY);
    return NS_OK;
  }

  
  
  
  if (aAttribute == nsGkAtoms::displaystyle_) {
    nsMathMLContainerFrame::RebuildAutomaticDataForChildren(GetParent());
    
    
    PresContext()->PresShell()->
      FrameNeedsReflow(GetParent(), nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
    return NS_OK;
  }

  

  nsPresContext* presContext = tableFrame->PresContext();
  if (aAttribute == nsGkAtoms::rowspacing_ ||
      aAttribute == nsGkAtoms::columnspacing_ ||
      aAttribute == nsGkAtoms::framespacing_ ) {
    nsMathMLmtableFrame* mathMLmtableFrame = do_QueryFrame(tableFrame);
    if (mathMLmtableFrame) {
      ParseSpacingAttribute(mathMLmtableFrame, aAttribute);
      mathMLmtableFrame->SetUseCSSSpacing();
    }
  } else if (aAttribute == nsGkAtoms::rowalign_ ||
             aAttribute == nsGkAtoms::rowlines_ ||
             aAttribute == nsGkAtoms::columnalign_ ||
             aAttribute == nsGkAtoms::columnlines_) {
    
    presContext->PropertyTable()->
      Delete(tableFrame, AttributeToProperty(aAttribute));
    
    ParseFrameAttribute(tableFrame, aAttribute, true);
  } else {
    
    return NS_OK;
  }

  
  presContext->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);

  return NS_OK;
}

nsIFrame*
nsMathMLmtableOuterFrame::GetRowFrameAt(nsPresContext* aPresContext,
                                        int32_t         aRowIndex)
{
  int32_t rowCount = GetRowCount();

  
  if (aRowIndex < 0) {
    aRowIndex = rowCount + aRowIndex;
  } else {
    
    --aRowIndex;
  }

  
  if (0 <= aRowIndex && aRowIndex <= rowCount) {
    nsIFrame* tableFrame = mFrames.FirstChild();
    NS_ASSERTION(tableFrame && tableFrame->GetType() == nsGkAtoms::tableFrame,
                 "should always have an inner table frame");
    nsIFrame* rgFrame = tableFrame->GetFirstPrincipalChild();
    if (!rgFrame || rgFrame->GetType() != nsGkAtoms::tableRowGroupFrame)
      return nullptr;
    nsTableIterator rowIter(*rgFrame);
    nsIFrame* rowFrame = rowIter.First();
    for ( ; rowFrame; rowFrame = rowIter.Next()) {
      if (aRowIndex == 0) {
        DEBUG_VERIFY_THAT_FRAME_IS(rowFrame, TABLE_ROW);
        if (rowFrame->GetType() != nsGkAtoms::tableRowFrame)
          return nullptr;

        return rowFrame;
      }
      --aRowIndex;
    }
  }
  return nullptr;
}

void
nsMathMLmtableOuterFrame::Reflow(nsPresContext*          aPresContext,
                                 nsHTMLReflowMetrics&     aDesiredSize,
                                 const nsHTMLReflowState& aReflowState,
                                 nsReflowStatus&          aStatus)
{
  nsAutoString value;
  

  nsTableOuterFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
  NS_ASSERTION(aDesiredSize.Height() >= 0, "illegal height for mtable");
  NS_ASSERTION(aDesiredSize.Width() >= 0, "illegal width for mtable");

  
  int32_t rowIndex = 0;
  eAlign tableAlign = eAlign_axis;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::align, value);
  if (!value.IsEmpty()) {
    ParseAlignAttribute(value, tableAlign, rowIndex);
  }

  
  
  
  
  nscoord dy = 0;
  WritingMode wm = aDesiredSize.GetWritingMode();
  nscoord blockSize = aDesiredSize.BSize(wm);
  nsIFrame* rowFrame = nullptr;
  if (rowIndex) {
    rowFrame = GetRowFrameAt(aPresContext, rowIndex);
    if (rowFrame) {
      
      nsIFrame* frame = rowFrame;
      LogicalRect frameRect(wm, frame->GetRect(), aReflowState.ComputedWidth());
      blockSize = frameRect.BSize(wm);
      do {
        dy += frameRect.BStart(wm);
        frame = frame->GetParent();
      } while (frame != this);
    }
  }
  switch (tableAlign) {
    case eAlign_top:
      aDesiredSize.SetBlockStartAscent(dy);
      break;
    case eAlign_bottom:
      aDesiredSize.SetBlockStartAscent(dy + blockSize);
      break;
    case eAlign_center:
      aDesiredSize.SetBlockStartAscent(dy + blockSize / 2);
      break;
    case eAlign_baseline:
      if (rowFrame) {
        
        nscoord rowAscent = ((nsTableRowFrame*)rowFrame)->GetMaxCellAscent();
        if (rowAscent) { 
          aDesiredSize.SetBlockStartAscent(dy + rowAscent);
          break;
        }
      }
      
      aDesiredSize.SetBlockStartAscent(dy + blockSize / 2);
      break;
    case eAlign_axis:
    default: {
      
      nsRefPtr<nsFontMetrics> fm;
      nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
                                            nsLayoutUtils::
                                            FontSizeInflationFor(this));
      nscoord axisHeight;
      GetAxisHeight(*aReflowState.rendContext, fm, axisHeight);
      if (rowFrame) {
        
        
        
        nscoord rowAscent = ((nsTableRowFrame*)rowFrame)->GetMaxCellAscent();
        if (rowAscent) { 
          aDesiredSize.SetBlockStartAscent(dy + rowAscent);
          break;
        }
      }
      
      aDesiredSize.SetBlockStartAscent(dy + blockSize / 2 + axisHeight);
    }
  }

  mReference.x = 0;
  mReference.y = aDesiredSize.BlockStartAscent();

  
  mBoundingMetrics = nsBoundingMetrics();
  mBoundingMetrics.ascent = aDesiredSize.BlockStartAscent();
  mBoundingMetrics.descent = aDesiredSize.Height() -
                             aDesiredSize.BlockStartAscent();
  mBoundingMetrics.width = aDesiredSize.Width();
  mBoundingMetrics.leftBearing = 0;
  mBoundingMetrics.rightBearing = aDesiredSize.Width();

  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

nsContainerFrame*
NS_NewMathMLmtableFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmtableFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmtableFrame)

nsMathMLmtableFrame::~nsMathMLmtableFrame()
{
}

void
nsMathMLmtableFrame::SetInitialChildList(ChildListID  aListID,
                                         nsFrameList& aChildList)
{
  nsTableFrame::SetInitialChildList(aListID, aChildList);
  MapAllAttributesIntoCSS(this);
}

void
nsMathMLmtableFrame::RestyleTable()
{
  
  MapAllAttributesIntoCSS(this);

  
  PresContext()->RestyleManager()->
    PostRestyleEvent(mContent->AsElement(), eRestyle_Subtree,
                     nsChangeHint_AllReflowHints);
}

nscoord
nsMathMLmtableFrame::GetColSpacing(int32_t aColIndex)
{
  if (mUseCSSSpacing) {
    return nsTableFrame::GetColSpacing(aColIndex);
  }
  if (!mColSpacing.Length()) {
    NS_ERROR("mColSpacing should not be empty");
    return 0;
  }
  if (aColIndex < 0 || aColIndex >= GetColCount()) {
    NS_ASSERTION(aColIndex == -1 || aColIndex == GetColCount(),
                 "Desired column beyond bounds of table and border");
    return mFrameSpacingX;
  }
  if ((uint32_t) aColIndex >= mColSpacing.Length()) {
    return mColSpacing.LastElement();
  }
  return mColSpacing.ElementAt(aColIndex);
}

nscoord
nsMathMLmtableFrame::GetColSpacing(int32_t aStartColIndex,
                                     int32_t aEndColIndex)
{
  if (mUseCSSSpacing) {
    return nsTableFrame::GetColSpacing(aStartColIndex, aEndColIndex);
  }
  if (aStartColIndex == aEndColIndex) {
    return 0;
  }
  if (!mColSpacing.Length()) {
    NS_ERROR("mColSpacing should not be empty");
    return 0;
  }
  nscoord space = 0;
  if (aStartColIndex < 0) {
    NS_ASSERTION(aStartColIndex == -1,
                 "Desired column beyond bounds of table and border");
    space += mFrameSpacingX;
    aStartColIndex = 0;
  }
  if (aEndColIndex >= GetColCount()) {
    NS_ASSERTION(aEndColIndex == GetColCount(),
                 "Desired column beyond bounds of table and border");
    space += mFrameSpacingX;
    aEndColIndex = GetColCount();
  }
  
  int32_t min = std::min(aEndColIndex, (int32_t) mColSpacing.Length());
  for (int32_t i = aStartColIndex; i < min; i++) {
    space += mColSpacing.ElementAt(i);
  }
  
  
  
  space += (aEndColIndex - min) * mColSpacing.LastElement();
  return space;
}

nscoord
nsMathMLmtableFrame::GetRowSpacing(int32_t aRowIndex)
{
  if (mUseCSSSpacing) {
    return nsTableFrame::GetRowSpacing(aRowIndex);
  }
  if (!mRowSpacing.Length()) {
    NS_ERROR("mRowSpacing should not be empty");
    return 0;
  }
  if (aRowIndex < 0 || aRowIndex >= GetRowCount()) {
    NS_ASSERTION(aRowIndex == -1 || aRowIndex == GetRowCount(),
                 "Desired row beyond bounds of table and border");
    return mFrameSpacingY;
  }
  if ((uint32_t) aRowIndex >= mRowSpacing.Length()) {
    return mRowSpacing.LastElement();
  }
  return mRowSpacing.ElementAt(aRowIndex);
}

nscoord
nsMathMLmtableFrame::GetRowSpacing(int32_t aStartRowIndex,
                                     int32_t aEndRowIndex)
{
  if (mUseCSSSpacing) {
    return nsTableFrame::GetRowSpacing(aStartRowIndex, aEndRowIndex);
  }
  if (aStartRowIndex == aEndRowIndex) {
    return 0;
  }
  if (!mRowSpacing.Length()) {
    NS_ERROR("mRowSpacing should not be empty");
    return 0;
  }
  nscoord space = 0;
  if (aStartRowIndex < 0) {
    NS_ASSERTION(aStartRowIndex == -1,
                 "Desired row beyond bounds of table and border");
    space += mFrameSpacingY;
    aStartRowIndex = 0;
  }
  if (aEndRowIndex >= GetRowCount()) {
    NS_ASSERTION(aEndRowIndex == GetRowCount(),
                 "Desired row beyond bounds of table and border");
    space += mFrameSpacingY;
    aEndRowIndex = GetRowCount();
  }
  
  int32_t min = std::min(aEndRowIndex, (int32_t) mRowSpacing.Length());
  for (int32_t i = aStartRowIndex; i < min; i++) {
    space += mRowSpacing.ElementAt(i);
  }
  
  
  
  space += (aEndRowIndex - min) * mRowSpacing.LastElement();
  return space;
}

void
nsMathMLmtableFrame::SetUseCSSSpacing()
{
  mUseCSSSpacing =
    !(mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::rowspacing_) ||
      mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::columnspacing_) ||
      mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::framespacing_));
}

NS_QUERYFRAME_HEAD(nsMathMLmtableFrame)
  NS_QUERYFRAME_ENTRY(nsMathMLmtableFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsTableFrame)




nsContainerFrame*
NS_NewMathMLmtrFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmtrFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmtrFrame)

nsMathMLmtrFrame::~nsMathMLmtrFrame()
{
}

nsresult
nsMathMLmtrFrame::AttributeChanged(int32_t  aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   int32_t  aModType)
{
  
  
  
  

  nsPresContext* presContext = PresContext();

  if (aAttribute != nsGkAtoms::rowalign_ &&
      aAttribute != nsGkAtoms::columnalign_) {
    return NS_OK;
  }

  presContext->PropertyTable()->Delete(this, AttributeToProperty(aAttribute));

  bool allowMultiValues = (aAttribute == nsGkAtoms::columnalign_);

  
  ParseFrameAttribute(this, aAttribute, allowMultiValues);

  
  presContext->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);

  return NS_OK;
}




nsContainerFrame*
NS_NewMathMLmtdFrame(nsIPresShell* aPresShell, nsStyleContext* aContext,
                     nsTableFrame* aTableFrame)
{
  return new (aPresShell) nsMathMLmtdFrame(aContext, aTableFrame);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmtdFrame)

nsMathMLmtdFrame::~nsMathMLmtdFrame()
{
}

void
nsMathMLmtdFrame::Init(nsIContent*       aContent,
                       nsContainerFrame* aParent,
                       nsIFrame*         aPrevInFlow)
{
  nsTableCellFrame::Init(aContent, aParent, aPrevInFlow);

  
  
  RemoveStateBits(NS_FRAME_FONT_INFLATION_FLOW_ROOT);
}

int32_t
nsMathMLmtdFrame::GetRowSpan()
{
  int32_t rowspan = 1;

  
  if (mContent->IsMathMLElement(nsGkAtoms::mtd_) &&
      !StyleContext()->GetPseudo()) {
    nsAutoString value;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::rowspan, value);
    if (!value.IsEmpty()) {
      nsresult error;
      rowspan = value.ToInteger(&error);
      if (NS_FAILED(error) || rowspan < 0)
        rowspan = 1;
      rowspan = std::min(rowspan, MAX_ROWSPAN);
    }
  }
  return rowspan;
}

int32_t
nsMathMLmtdFrame::GetColSpan()
{
  int32_t colspan = 1;

  
  if (mContent->IsMathMLElement(nsGkAtoms::mtd_) &&
      !StyleContext()->GetPseudo()) {
    nsAutoString value;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::columnspan_, value);
    if (!value.IsEmpty()) {
      nsresult error;
      colspan = value.ToInteger(&error);
      if (NS_FAILED(error) || colspan < 0 || colspan > MAX_COLSPAN)
        colspan = 1;
    }
  }
  return colspan;
}

nsresult
nsMathMLmtdFrame::AttributeChanged(int32_t  aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   int32_t  aModType)
{
  
  
  
  
  
  

  if (aAttribute == nsGkAtoms::rowalign_ ||
      aAttribute == nsGkAtoms::columnalign_) {

    nsPresContext* presContext = PresContext();
    presContext->PropertyTable()->Delete(this, AttributeToProperty(aAttribute));

    
    ParseFrameAttribute(this, aAttribute, false);
    return NS_OK;
  }

  if (aAttribute == nsGkAtoms::rowspan ||
      aAttribute == nsGkAtoms::columnspan_) {
    
    if (aAttribute == nsGkAtoms::columnspan_)
      aAttribute = nsGkAtoms::colspan;
    return nsTableCellFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
  }

  return NS_OK;
}

uint8_t
nsMathMLmtdFrame::GetVerticalAlign() const
{
  
  uint8_t alignment = nsTableCellFrame::GetVerticalAlign();

  nsTArray<int8_t>* alignmentList = FindCellProperty(this, RowAlignProperty());

  if (alignmentList) {
    int32_t rowIndex;
    GetRowIndex(rowIndex);

    
    
    if (rowIndex < (int32_t)alignmentList->Length())
      alignment = alignmentList->ElementAt(rowIndex);
    else
      alignment = alignmentList->ElementAt(alignmentList->Length() - 1);
  }

  return alignment;
}

nsresult
nsMathMLmtdFrame::ProcessBorders(nsTableFrame* aFrame,
                                 nsDisplayListBuilder* aBuilder,
                                 const nsDisplayListSet& aLists)
{
  aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
                                            nsDisplaymtdBorder(aBuilder, this));
  return NS_OK;
}

nsMargin*
nsMathMLmtdFrame::GetBorderWidth(nsMargin& aBorder) const
{
  nsStyleBorder styleBorder = *StyleBorder();
  ApplyBorderToStyle(this, styleBorder);
  aBorder = styleBorder.GetComputedBorder();
  return &aBorder;
}

nsMargin
nsMathMLmtdFrame::GetBorderOverflow()
{
  nsStyleBorder styleBorder = *StyleBorder();
  ApplyBorderToStyle(this, styleBorder);
  nsMargin overflow = ComputeBorderOverflow(this, styleBorder);
  return overflow;
}




NS_QUERYFRAME_HEAD(nsMathMLmtdInnerFrame)
  NS_QUERYFRAME_ENTRY(nsIMathMLFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBlockFrame)

nsContainerFrame*
NS_NewMathMLmtdInnerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmtdInnerFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmtdInnerFrame)

nsMathMLmtdInnerFrame::nsMathMLmtdInnerFrame(nsStyleContext* aContext)
  : nsBlockFrame(aContext)
{
  
  mUniqueStyleText = new (PresContext()) nsStyleText(*StyleText());
}

nsMathMLmtdInnerFrame::~nsMathMLmtdInnerFrame()
{
  mUniqueStyleText->Destroy(PresContext());
}

void
nsMathMLmtdInnerFrame::Reflow(nsPresContext*          aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState,
                              nsReflowStatus&          aStatus)
{
  
  nsBlockFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);

  
  
}

const
nsStyleText* nsMathMLmtdInnerFrame::StyleTextForLineLayout()
{
  
  uint8_t alignment = StyleText()->mTextAlign;

  nsTArray<int8_t>* alignmentList =
    FindCellProperty(this, ColumnAlignProperty());

  if (alignmentList) {
    nsMathMLmtdFrame* cellFrame = (nsMathMLmtdFrame*)GetParent();
    int32_t columnIndex;
    cellFrame->GetColIndex(columnIndex);

    
    
    if (columnIndex < (int32_t)alignmentList->Length())
      alignment = alignmentList->ElementAt(columnIndex);
    else
      alignment = alignmentList->ElementAt(alignmentList->Length() - 1);
  }

  mUniqueStyleText->mTextAlign = alignment;
  return mUniqueStyleText;
}

 void
nsMathMLmtdInnerFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsBlockFrame::DidSetStyleContext(aOldStyleContext);
  mUniqueStyleText->Destroy(PresContext());
  mUniqueStyleText = new (PresContext()) nsStyleText(*StyleText());
}
