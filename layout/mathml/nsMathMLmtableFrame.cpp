




#include "nsMathMLmtableFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsNameSpaceManager.h"
#include "nsRenderingContext.h"
#include "nsCSSRendering.h"

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
    { aValue, aAttribute, content->Tag()->GetUTF16String() };

  return nsContentUtils::ReportToConsole(nsIScriptError::errorFlag,
                                         NS_LITERAL_CSTRING("MathML"),
                                         content->OwnerDoc(),
                                         nsContentUtils::eMATHML_PROPERTIES,
                                         "AttributeParsingError", params, 3);
}






static void
DestroyStylePropertyList(void* aPropertyValue)
{
  delete static_cast<nsTArray<int8_t>*>(aPropertyValue);
}

NS_DECLARE_FRAME_PROPERTY(RowAlignProperty, DestroyStylePropertyList)
NS_DECLARE_FRAME_PROPERTY(RowLinesProperty, DestroyStylePropertyList)
NS_DECLARE_FRAME_PROPERTY(ColumnAlignProperty, DestroyStylePropertyList)
NS_DECLARE_FRAME_PROPERTY(ColumnLinesProperty, DestroyStylePropertyList)

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






class nsDisplaymtdBorder : public nsDisplayBorder {
public:
  nsDisplaymtdBorder(nsDisplayListBuilder* aBuilder, nsMathMLmtdFrame* aFrame)
    : nsDisplayBorder(aBuilder, aFrame)
  {
  }

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE
  {
    nsStyleBorder styleBorder = *mFrame->StyleBorder();
    ApplyBorderToStyle(static_cast<nsMathMLmtdFrame*>(mFrame), styleBorder);
    return CalculateBounds(styleBorder);
  }

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE
  {
    nsStyleBorder styleBorder = *mFrame->StyleBorder();
    ApplyBorderToStyle(static_cast<nsMathMLmtdFrame*>(mFrame), styleBorder);

    nsPoint offset = ToReferenceFrame();
    nsCSSRendering::PaintBorderWithStyleBorder(mFrame->PresContext(), *aCtx,
                                               mFrame, mVisibleRect,
                                               nsRect(offset,
                                                      mFrame->GetSize()),
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



static void
MapAllAttributesIntoCSS(nsIFrame* aTableFrame)
{
  
  ParseFrameAttribute(aTableFrame, nsGkAtoms::rowalign_, true);
  ParseFrameAttribute(aTableFrame, nsGkAtoms::rowlines_, true);

  
  ParseFrameAttribute(aTableFrame, nsGkAtoms::columnalign_, true);
  ParseFrameAttribute(aTableFrame, nsGkAtoms::columnlines_, true);

  
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
    if (!c || c->Tag() == nsGkAtoms::math || c->Tag() == nsGkAtoms::body)
      break;
  }
  if (!f) f = atLeast;
  f->List(stdout, 0);
}
#endif




NS_QUERYFRAME_HEAD(nsMathMLmtableOuterFrame)
  NS_QUERYFRAME_ENTRY(nsIMathMLFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsTableOuterFrame)

nsIFrame*
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
    nsMathMLContainerFrame::RebuildAutomaticDataForChildren(mParent);
    
    
    PresContext()->PresShell()->
      FrameNeedsReflow(mParent, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
    return NS_OK;
  }

  

  
  if (aAttribute != nsGkAtoms::rowalign_ &&
      aAttribute != nsGkAtoms::rowlines_ &&
      aAttribute != nsGkAtoms::columnalign_ &&
      aAttribute != nsGkAtoms::columnlines_) {
    return NS_OK;
  }

  nsPresContext* presContext = tableFrame->PresContext();

  
  presContext->PropertyTable()->
    Delete(tableFrame, AttributeToProperty(aAttribute));

  
  ParseFrameAttribute(tableFrame, aAttribute, true);

  
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
  nscoord height = aDesiredSize.Height();
  nsIFrame* rowFrame = nullptr;
  if (rowIndex) {
    rowFrame = GetRowFrameAt(aPresContext, rowIndex);
    if (rowFrame) {
      
      nsIFrame* frame = rowFrame;
      height = frame->GetSize().height;
      do {
        dy += frame->GetPosition().y;
        frame = frame->GetParent();
      } while (frame != this);
    }
  }
  switch (tableAlign) {
    case eAlign_top:
      aDesiredSize.SetTopAscent(dy);
      break;
    case eAlign_bottom:
      aDesiredSize.SetTopAscent(dy + height);
      break;
    case eAlign_center:
      aDesiredSize.SetTopAscent(dy + height / 2);
      break;
    case eAlign_baseline:
      if (rowFrame) {
        
        nscoord rowAscent = ((nsTableRowFrame*)rowFrame)->GetMaxCellAscent();
        if (rowAscent) { 
          aDesiredSize.SetTopAscent(dy + rowAscent);
          break;
        }
      }
      
      aDesiredSize.SetTopAscent(dy + height / 2);
      break;
    case eAlign_axis:
    default: {
      
      nsRefPtr<nsFontMetrics> fm;
      nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
      aReflowState.rendContext->SetFont(fm);
      nscoord axisHeight;
      GetAxisHeight(*aReflowState.rendContext,
                    aReflowState.rendContext->FontMetrics(),
                    axisHeight);
      if (rowFrame) {
        
        
        
        nscoord rowAscent = ((nsTableRowFrame*)rowFrame)->GetMaxCellAscent();
        if (rowAscent) { 
          aDesiredSize.SetTopAscent(dy + rowAscent);
          break;
        }
      }
      
      aDesiredSize.SetTopAscent(dy + height / 2 + axisHeight);
    }
  }

  mReference.x = 0;
  mReference.y = aDesiredSize.TopAscent();

  
  mBoundingMetrics = nsBoundingMetrics();
  mBoundingMetrics.ascent = aDesiredSize.TopAscent();
  mBoundingMetrics.descent = aDesiredSize.Height() - aDesiredSize.TopAscent();
  mBoundingMetrics.width = aDesiredSize.Width();
  mBoundingMetrics.leftBearing = 0;
  mBoundingMetrics.rightBearing = aDesiredSize.Width();

  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

nsIFrame*
NS_NewMathMLmtableFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmtableFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmtableFrame)

nsMathMLmtableFrame::~nsMathMLmtableFrame()
{
}

nsresult
nsMathMLmtableFrame::SetInitialChildList(ChildListID  aListID,
                                         nsFrameList& aChildList)
{
  nsresult rv = nsTableFrame::SetInitialChildList(aListID, aChildList);
  if (NS_FAILED(rv)) return rv;
  MapAllAttributesIntoCSS(this);
  return rv;
}

void
nsMathMLmtableFrame::RestyleTable()
{
  
  MapAllAttributesIntoCSS(this);

  
  PresContext()->RestyleManager()->
    PostRestyleEvent(mContent->AsElement(), eRestyle_Subtree,
                     nsChangeHint_AllReflowHints);
}




nsIFrame*
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




nsIFrame*
NS_NewMathMLmtdFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmtdFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmtdFrame)

nsMathMLmtdFrame::~nsMathMLmtdFrame()
{
}

int32_t
nsMathMLmtdFrame::GetRowSpan()
{
  int32_t rowspan = 1;

  
  if ((mContent->Tag() == nsGkAtoms::mtd_) && !StyleContext()->GetPseudo()) {
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

  
  if ((mContent->Tag() == nsGkAtoms::mtd_) && !StyleContext()->GetPseudo()) {
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




NS_QUERYFRAME_HEAD(nsMathMLmtdInnerFrame)
  NS_QUERYFRAME_ENTRY(nsIMathMLFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBlockFrame)

nsIFrame*
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
