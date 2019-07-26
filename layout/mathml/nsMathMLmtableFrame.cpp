




#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsBlockFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsTableRowFrame.h"
#include "nsINameSpaceManager.h"
#include "nsRenderingContext.h"

#include "nsTArray.h"
#include "nsCSSFrameConstructor.h"
#include "nsTableOuterFrame.h"
#include "nsTableFrame.h"
#include "nsTableCellFrame.h"
#include "celldata.h"

#include "nsMathMLmtableFrame.h"
#include <algorithm>

using namespace mozilla;









static void
SplitString(nsString&             aString, 
            nsTArray<PRUnichar*>& aOffset) 
{
  static const PRUnichar kNullCh = PRUnichar('\0');

  aString.Append(kNullCh);  

  PRUnichar* start = aString.BeginWriting();
  PRUnichar* end   = start;

  while (kNullCh != *start) {
    while ((kNullCh != *start) && nsCRT::IsAsciiSpace(*start)) {  
      start++;
    }
    end = start;

    while ((kNullCh != *end) && (false == nsCRT::IsAsciiSpace(*end))) { 
      end++;
    }
    *end = kNullCh; 

    if (start < end) {
      aOffset.AppendElement(start); 
    }

    start = ++end;
  }
}

struct nsValueList
{
  nsString             mData;
  nsTArray<PRUnichar*> mArray;

  nsValueList(nsString& aData) {
    mData.Assign(aData);
    SplitString(mData, mArray);
  }
};








static void
DestroyValueList(void* aPropertyValue)
{
  delete static_cast<nsValueList*>(aPropertyValue);
}

NS_DECLARE_FRAME_PROPERTY(RowAlignProperty, DestroyValueList)
NS_DECLARE_FRAME_PROPERTY(RowLinesProperty, DestroyValueList)
NS_DECLARE_FRAME_PROPERTY(ColumnAlignProperty, DestroyValueList)
NS_DECLARE_FRAME_PROPERTY(ColumnLinesProperty, DestroyValueList)

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

static PRUnichar*
GetValueAt(nsIFrame*                      aTableOrRowFrame,
           const FramePropertyDescriptor* aProperty,
           nsIAtom*                       aAttribute,
           int32_t                        aRowOrColIndex)
{
  FrameProperties props = aTableOrRowFrame->Properties();
  nsValueList* valueList = static_cast<nsValueList*>(props.Get(aProperty));
  if (!valueList) {
    
    nsAutoString values;
    aTableOrRowFrame->GetContent()->GetAttr(kNameSpaceID_None, aAttribute, values);
    if (!values.IsEmpty())
      valueList = new nsValueList(values);
    if (!valueList || !valueList->mArray.Length()) {
      delete valueList; 
      return nullptr;
    }
    props.Set(aProperty, valueList);
  }
  int32_t count = valueList->mArray.Length();
  return (aRowOrColIndex < count)
         ? valueList->mArray[aRowOrColIndex]
         : valueList->mArray[count-1];
}

#ifdef DEBUG
static bool
IsTable(uint8_t aDisplay)
{
  if ((aDisplay == NS_STYLE_DISPLAY_TABLE) ||
      (aDisplay == NS_STYLE_DISPLAY_INLINE_TABLE))
    return true;
  return false;
}

#define DEBUG_VERIFY_THAT_FRAME_IS(_frame, _expected) \
  NS_ASSERTION(NS_STYLE_DISPLAY_##_expected == _frame->GetStyleDisplay()->mDisplay, "internal error");
#define DEBUG_VERIFY_THAT_FRAME_IS_TABLE(_frame) \
  NS_ASSERTION(IsTable(_frame->GetStyleDisplay()->mDisplay), "internal error");
#else
#define DEBUG_VERIFY_THAT_FRAME_IS(_frame, _expected)
#define DEBUG_VERIFY_THAT_FRAME_IS_TABLE(_frame)
#endif



static void
MapRowAttributesIntoCSS(nsIFrame* aTableFrame,
                        nsIFrame* aRowFrame)
{
  DEBUG_VERIFY_THAT_FRAME_IS_TABLE(aTableFrame);
  DEBUG_VERIFY_THAT_FRAME_IS(aRowFrame, TABLE_ROW);
  int32_t rowIndex = ((nsTableRowFrame*)aRowFrame)->GetRowIndex();
  nsIContent* rowContent = aRowFrame->GetContent();
  PRUnichar* attr;

  
  if (!rowContent->HasAttr(kNameSpaceID_None, nsGkAtoms::rowalign_) &&
      !rowContent->HasAttr(kNameSpaceID_None, nsGkAtoms::_moz_math_rowalign_)) {
    
    attr = GetValueAt(aTableFrame, RowAlignProperty(),
                      nsGkAtoms::rowalign_, rowIndex);
    if (attr) {
      
      rowContent->SetAttr(kNameSpaceID_None, nsGkAtoms::_moz_math_rowalign_,
                          nsDependentString(attr), false);
    }
  }

  
  
  
  
  
  if (rowIndex > 0 &&
      !rowContent->HasAttr(kNameSpaceID_None, nsGkAtoms::_moz_math_rowline_)) {
    attr = GetValueAt(aTableFrame, RowLinesProperty(),
                      nsGkAtoms::rowlines_, rowIndex-1);
    if (attr) {
      
      rowContent->SetAttr(kNameSpaceID_None, nsGkAtoms::_moz_math_rowline_,
                          nsDependentString(attr), false);
    }
  }
}



static void
MapColAttributesIntoCSS(nsIFrame* aTableFrame,
                        nsIFrame* aRowFrame,
                        nsIFrame* aCellFrame)
{
  DEBUG_VERIFY_THAT_FRAME_IS_TABLE(aTableFrame);
  DEBUG_VERIFY_THAT_FRAME_IS(aRowFrame, TABLE_ROW);
  DEBUG_VERIFY_THAT_FRAME_IS(aCellFrame, TABLE_CELL);
  int32_t rowIndex, colIndex;
  ((nsTableCellFrame*)aCellFrame)->GetCellIndexes(rowIndex, colIndex);
  nsIContent* cellContent = aCellFrame->GetContent();
  PRUnichar* attr;

  
  if (!cellContent->HasAttr(kNameSpaceID_None, nsGkAtoms::columnalign_) &&
      !cellContent->HasAttr(kNameSpaceID_None,
                            nsGkAtoms::_moz_math_columnalign_)) {
    
    attr = GetValueAt(aRowFrame, ColumnAlignProperty(),
                      nsGkAtoms::columnalign_, colIndex);
    if (!attr) {
      
      attr = GetValueAt(aTableFrame, ColumnAlignProperty(),
                        nsGkAtoms::columnalign_, colIndex);
    }
    if (attr) {
      
      cellContent->SetAttr(kNameSpaceID_None, nsGkAtoms::_moz_math_columnalign_,
                           nsDependentString(attr), false);
    }
  }

  
  
  
  
  
  if (colIndex > 0 &&
      !cellContent->HasAttr(kNameSpaceID_None,
                            nsGkAtoms::_moz_math_columnline_)) {
    attr = GetValueAt(aTableFrame, ColumnLinesProperty(),
                      nsGkAtoms::columnlines_, colIndex-1);
    if (attr) {
      
      cellContent->SetAttr(kNameSpaceID_None, nsGkAtoms::_moz_math_columnline_,
                           nsDependentString(attr), false);
    }
  }
}



static void
MapAllAttributesIntoCSS(nsIFrame* aTableFrame)
{
  
  nsIFrame* rgFrame = aTableFrame->GetFirstPrincipalChild();
  if (!rgFrame || rgFrame->GetType() != nsGkAtoms::tableRowGroupFrame)
    return;

  nsIFrame* rowFrame = rgFrame->GetFirstPrincipalChild();
  for ( ; rowFrame; rowFrame = rowFrame->GetNextSibling()) {
    DEBUG_VERIFY_THAT_FRAME_IS(rowFrame, TABLE_ROW);
    if (rowFrame->GetType() == nsGkAtoms::tableRowFrame) {
      MapRowAttributesIntoCSS(aTableFrame, rowFrame);
      nsIFrame* cellFrame = rowFrame->GetFirstPrincipalChild();
      for ( ; cellFrame; cellFrame = cellFrame->GetNextSibling()) {
        DEBUG_VERIFY_THAT_FRAME_IS(cellFrame, TABLE_CELL);
        if (IS_TABLE_CELL(cellFrame->GetType())) {
          MapColAttributesIntoCSS(aTableFrame, rowFrame, cellFrame);
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

NS_IMETHODIMP
nsMathMLmtableOuterFrame::InheritAutomaticData(nsIFrame* aParent)
{
  

  
  nsMathMLFrame::InheritAutomaticData(aParent);

  
  if (mContent->Tag() == nsGkAtoms::mtable_)
    nsMathMLFrame::FindAttrDisplaystyle(mContent, mPresentationData);

  return NS_OK;
}




NS_IMETHODIMP
nsMathMLmtableOuterFrame::UpdatePresentationData(uint32_t aFlagsValues,
                                                 uint32_t aWhichFlags)
{
  if (NS_MATHML_HAS_EXPLICIT_DISPLAYSTYLE(mPresentationData.flags)) {
    
    aWhichFlags &= ~NS_MATHML_DISPLAYSTYLE;
    aFlagsValues &= ~NS_MATHML_DISPLAYSTYLE;
  }

  return nsMathMLFrame::UpdatePresentationData(aFlagsValues, aWhichFlags);
}

NS_IMETHODIMP
nsMathMLmtableOuterFrame::UpdatePresentationDataFromChildAt(int32_t  aFirstIndex,
                                                            int32_t  aLastIndex,
                                                            uint32_t aFlagsValues,
                                                            uint32_t aWhichFlags)
{
  if (NS_MATHML_HAS_EXPLICIT_DISPLAYSTYLE(mPresentationData.flags)) {
    
    aWhichFlags &= ~NS_MATHML_DISPLAYSTYLE;
    aFlagsValues &= ~NS_MATHML_DISPLAYSTYLE;
  }

  nsMathMLContainerFrame::PropagatePresentationDataFromChildAt(this,
    aFirstIndex, aLastIndex, aFlagsValues, aWhichFlags);

  return NS_OK; 
}

NS_IMETHODIMP
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

  
  nsIAtom* MOZrowAtom = nullptr;
  nsIAtom* MOZcolAtom = nullptr;
  if (aAttribute == nsGkAtoms::rowalign_)
    MOZrowAtom = nsGkAtoms::_moz_math_rowalign_;
  else if (aAttribute == nsGkAtoms::rowlines_)
    MOZrowAtom = nsGkAtoms::_moz_math_rowline_;
  else if (aAttribute == nsGkAtoms::columnalign_)
    MOZcolAtom = nsGkAtoms::_moz_math_columnalign_;
  else if (aAttribute == nsGkAtoms::columnlines_)
    MOZcolAtom = nsGkAtoms::_moz_math_columnline_;

  if (!MOZrowAtom && !MOZcolAtom)
    return NS_OK;

  nsPresContext* presContext = tableFrame->PresContext();
  
  presContext->PropertyTable()->
    Delete(tableFrame, AttributeToProperty(aAttribute));

  
  nsIFrame* rowFrame = rgFrame->GetFirstPrincipalChild();
  for ( ; rowFrame; rowFrame = rowFrame->GetNextSibling()) {
    if (rowFrame->GetType() == nsGkAtoms::tableRowFrame) {
      if (MOZrowAtom) { 
        rowFrame->GetContent()->UnsetAttr(kNameSpaceID_None, MOZrowAtom, false);
        MapRowAttributesIntoCSS(tableFrame, rowFrame);    
      } else { 
        nsIFrame* cellFrame = rowFrame->GetFirstPrincipalChild();
        for ( ; cellFrame; cellFrame = cellFrame->GetNextSibling()) {
          if (IS_TABLE_CELL(cellFrame->GetType())) {
            cellFrame->GetContent()->UnsetAttr(kNameSpaceID_None, MOZcolAtom, false);
            MapColAttributesIntoCSS(tableFrame, rowFrame, cellFrame);
          }
        }
      }
    }
  }

  
  presContext->PresShell()->FrameConstructor()->
    PostRestyleEvent(mContent->AsElement(), eRestyle_Subtree,
                     nsChangeHint_AllReflowHints);

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

NS_IMETHODIMP
nsMathMLmtableOuterFrame::Reflow(nsPresContext*          aPresContext,
                                 nsHTMLReflowMetrics&     aDesiredSize,
                                 const nsHTMLReflowState& aReflowState,
                                 nsReflowStatus&          aStatus)
{
  nsresult rv;
  nsAutoString value;
  

  rv = nsTableOuterFrame::Reflow(aPresContext, aDesiredSize, aReflowState,
                                 aStatus);
  NS_ASSERTION(aDesiredSize.height >= 0, "illegal height for mtable");
  NS_ASSERTION(aDesiredSize.width >= 0, "illegal width for mtable");

  
  
  int32_t rowIndex = 0;
  eAlign tableAlign = eAlign_axis;
  GetAttribute(mContent, nullptr, nsGkAtoms::align, value);
  if (!value.IsEmpty()) {
    ParseAlignAttribute(value, tableAlign, rowIndex);
  }

  
  
  
  
  nscoord dy = 0;
  nscoord height = aDesiredSize.height;
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
      aDesiredSize.ascent = dy;
      break;
    case eAlign_bottom:
      aDesiredSize.ascent = dy + height;
      break;
    case eAlign_center:
      aDesiredSize.ascent = dy + height/2;
      break;
    case eAlign_baseline:
      if (rowFrame) {
        
        nscoord rowAscent = ((nsTableRowFrame*)rowFrame)->GetMaxCellAscent();
        if (rowAscent) { 
          aDesiredSize.ascent = dy + rowAscent;
          break;
        }
      }
      
      aDesiredSize.ascent = dy + height/2;
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
          aDesiredSize.ascent = dy + rowAscent;
          break;
        }
      }
      
      aDesiredSize.ascent = dy + height/2 + axisHeight;
    }
  }

  mReference.x = 0;
  mReference.y = aDesiredSize.ascent;

  
  mBoundingMetrics = nsBoundingMetrics();
  mBoundingMetrics.ascent = aDesiredSize.ascent;
  mBoundingMetrics.descent = aDesiredSize.height - aDesiredSize.ascent;
  mBoundingMetrics.width = aDesiredSize.width;
  mBoundingMetrics.leftBearing = 0;
  mBoundingMetrics.rightBearing = aDesiredSize.width;

  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);

  return rv;
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

NS_IMETHODIMP
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

  
  PresContext()->PresShell()->FrameConstructor()->
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

NS_IMETHODIMP
nsMathMLmtrFrame::AttributeChanged(int32_t  aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   int32_t  aModType)
{
  
  
  
  
  

  if (aAttribute == nsGkAtoms::rowalign_) {
    
    mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::_moz_math_rowalign_,
                        false);
    MapRowAttributesIntoCSS(nsTableFrame::GetTableFrame(this), this);
    
    return NS_OK;
  }

  if (aAttribute != nsGkAtoms::columnalign_)
    return NS_OK;

  nsPresContext* presContext = PresContext();
  
  presContext->PropertyTable()->Delete(this, AttributeToProperty(aAttribute));

  
  
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  nsIFrame* cellFrame = GetFirstPrincipalChild();
  for ( ; cellFrame; cellFrame = cellFrame->GetNextSibling()) {
    if (IS_TABLE_CELL(cellFrame->GetType())) {
      cellFrame->GetContent()->
        UnsetAttr(kNameSpaceID_None, nsGkAtoms::_moz_math_columnalign_,
                  false);
      MapColAttributesIntoCSS(tableFrame, this, cellFrame);
    }
  }

  
  presContext->PresShell()->FrameConstructor()->
    PostRestyleEvent(mContent->AsElement(), eRestyle_Subtree,
                     nsChangeHint_AllReflowHints);

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

NS_IMETHODIMP
nsMathMLmtdFrame::AttributeChanged(int32_t  aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   int32_t  aModType)
{
  
  
  
  
  
  

  if (aAttribute == nsGkAtoms::columnalign_) {
    
    mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::_moz_math_columnalign_,
                        false);
    MapColAttributesIntoCSS(nsTableFrame::GetTableFrame(this), mParent, this);
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




NS_QUERYFRAME_HEAD(nsMathMLmtdInnerFrame)
  NS_QUERYFRAME_ENTRY(nsIMathMLFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBlockFrame)

nsIFrame*
NS_NewMathMLmtdInnerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmtdInnerFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmtdInnerFrame)

nsMathMLmtdInnerFrame::~nsMathMLmtdInnerFrame()
{
}

NS_IMETHODIMP
nsMathMLmtdInnerFrame::Reflow(nsPresContext*          aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState,
                              nsReflowStatus&          aStatus)
{
  
  nsresult rv = nsBlockFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);

  
  
  return rv;
}
