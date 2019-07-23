





































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsBlockFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsINameSpaceManager.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsTArray.h"
#include "nsCSSFrameConstructor.h"
#include "nsTableOuterFrame.h"
#include "nsTableFrame.h"
#include "nsTableCellFrame.h"
#include "celldata.h"

#include "nsMathMLmtableFrame.h"









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

    while ((kNullCh != *end) && (PR_FALSE == nsCRT::IsAsciiSpace(*end))) { 
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
DestroyValueListFunc(void*    aFrame,
                     nsIAtom* aPropertyName,
                     void*    aPropertyValue,
                     void*    aDtorData)
{
  delete static_cast<nsValueList*>(aPropertyValue);
}

static PRUnichar*
GetValueAt(nsIFrame* aTableOrRowFrame,
           nsIAtom*  aAttribute,
           PRInt32   aRowOrColIndex)
{
  nsValueList* valueList = static_cast<nsValueList*>
                                      (aTableOrRowFrame->GetProperty(aAttribute));
  if (!valueList) {
    
    nsAutoString values;
    aTableOrRowFrame->GetContent()->GetAttr(kNameSpaceID_None, aAttribute, values);
    if (!values.IsEmpty())
      valueList = new nsValueList(values);
    if (!valueList || !valueList->mArray.Length()) {
      delete valueList; 
      return nsnull;
    }
    aTableOrRowFrame->SetProperty(aAttribute, valueList, DestroyValueListFunc);
  }
  PRInt32 count = valueList->mArray.Length();
  return (aRowOrColIndex < count)
         ? valueList->mArray[aRowOrColIndex]
         : valueList->mArray[count-1];
}

#ifdef NS_DEBUG
static PRBool
IsTable(PRUint8 aDisplay)
{
  if ((aDisplay == NS_STYLE_DISPLAY_TABLE) ||
      (aDisplay == NS_STYLE_DISPLAY_INLINE_TABLE))
    return PR_TRUE;
  return PR_FALSE;
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
  PRInt32 rowIndex = ((nsTableRowFrame*)aRowFrame)->GetRowIndex();
  nsIContent* rowContent = aRowFrame->GetContent();
  PRUnichar* attr;

  
  if (!rowContent->HasAttr(kNameSpaceID_None, nsGkAtoms::rowalign_) &&
      !rowContent->HasAttr(kNameSpaceID_None, nsGkAtoms::MOZrowalign)) {
    
    attr = GetValueAt(aTableFrame, nsGkAtoms::rowalign_, rowIndex);
    if (attr) {
      
      rowContent->SetAttr(kNameSpaceID_None, nsGkAtoms::MOZrowalign,
                          nsDependentString(attr), PR_FALSE);
    }
  }

  
  
  
  
  
  if (rowIndex > 0 &&
      !rowContent->HasAttr(kNameSpaceID_None, nsGkAtoms::MOZrowline)) {
    attr = GetValueAt(aTableFrame, nsGkAtoms::rowlines_, rowIndex-1);
    if (attr) {
      
      rowContent->SetAttr(kNameSpaceID_None, nsGkAtoms::MOZrowline,
                          nsDependentString(attr), PR_FALSE);
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
  PRInt32 rowIndex, colIndex;
  ((nsTableCellFrame*)aCellFrame)->GetCellIndexes(rowIndex, colIndex);
  nsIContent* cellContent = aCellFrame->GetContent();
  PRUnichar* attr;

  
  if (!cellContent->HasAttr(kNameSpaceID_None, nsGkAtoms::columnalign_) &&
      !cellContent->HasAttr(kNameSpaceID_None, nsGkAtoms::MOZcolumnalign)) {
    
    attr = GetValueAt(aRowFrame, nsGkAtoms::columnalign_, colIndex);
    if (!attr) {
      
      attr = GetValueAt(aTableFrame, nsGkAtoms::columnalign_, colIndex);
    }
    if (attr) {
      
      cellContent->SetAttr(kNameSpaceID_None, nsGkAtoms::MOZcolumnalign,
                           nsDependentString(attr), PR_FALSE);
    }
  }

  
  
  
  
  
  if (colIndex > 0 &&
      !cellContent->HasAttr(kNameSpaceID_None, nsGkAtoms::MOZcolumnline)) {
    attr = GetValueAt(aTableFrame, nsGkAtoms::columnlines_, colIndex-1);
    if (attr) {
      
      cellContent->SetAttr(kNameSpaceID_None, nsGkAtoms::MOZcolumnline,
                           nsDependentString(attr), PR_FALSE);
    }
  }
}



static void
MapAllAttributesIntoCSS(nsIFrame* aTableFrame)
{
  
  nsIFrame* rgFrame = aTableFrame->GetFirstChild(nsnull);
  if (!rgFrame || rgFrame->GetType() != nsGkAtoms::tableRowGroupFrame)
    return;

  nsIFrame* rowFrame = rgFrame->GetFirstChild(nsnull);
  for ( ; rowFrame; rowFrame = rowFrame->GetNextSibling()) {
    DEBUG_VERIFY_THAT_FRAME_IS(rowFrame, TABLE_ROW);
    if (rowFrame->GetType() == nsGkAtoms::tableRowFrame) {
      MapRowAttributesIntoCSS(aTableFrame, rowFrame);
      nsIFrame* cellFrame = rowFrame->GetFirstChild(nsnull);
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
ParseAlignAttribute(nsString& aValue, eAlign& aAlign, PRInt32& aRowIndex)
{
  
  aRowIndex = 0;
  aAlign = eAlign_axis;
  PRInt32 len = 0;
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
    PRInt32 error;
    aValue.Cut(0, len); 
    aRowIndex = aValue.ToInteger(&error);
    if (error)
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
nsMathMLmtableOuterFrame::UpdatePresentationData(PRUint32 aFlagsValues,
                                                 PRUint32 aWhichFlags)
{
  if (NS_MATHML_HAS_EXPLICIT_DISPLAYSTYLE(mPresentationData.flags)) {
    
    aWhichFlags &= ~NS_MATHML_DISPLAYSTYLE;
    aFlagsValues &= ~NS_MATHML_DISPLAYSTYLE;
  }

  return nsMathMLFrame::UpdatePresentationData(aFlagsValues, aWhichFlags);
}

NS_IMETHODIMP
nsMathMLmtableOuterFrame::UpdatePresentationDataFromChildAt(PRInt32  aFirstIndex,
                                                            PRInt32  aLastIndex,
                                                            PRUint32 aFlagsValues,
                                                            PRUint32 aWhichFlags)
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
nsMathMLmtableOuterFrame::AttributeChanged(PRInt32  aNameSpaceID,
                                           nsIAtom* aAttribute,
                                           PRInt32  aModType)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  nsIFrame* tableFrame = mFrames.FirstChild();
  if (!tableFrame || tableFrame->GetType() != nsGkAtoms::tableFrame)
    return NS_OK;
  nsIFrame* rgFrame = tableFrame->GetFirstChild(nsnull);
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

  
  nsIAtom* MOZrowAtom = nsnull;
  nsIAtom* MOZcolAtom = nsnull;
  if (aAttribute == nsGkAtoms::rowalign_)
    MOZrowAtom = nsGkAtoms::MOZrowalign;
  else if (aAttribute == nsGkAtoms::rowlines_)
    MOZrowAtom = nsGkAtoms::MOZrowline;
  else if (aAttribute == nsGkAtoms::columnalign_)
    MOZcolAtom = nsGkAtoms::MOZcolumnalign;
  else if (aAttribute == nsGkAtoms::columnlines_)
    MOZcolAtom = nsGkAtoms::MOZcolumnline;

  if (!MOZrowAtom && !MOZcolAtom)
    return NS_OK;

  
  tableFrame->DeleteProperty(aAttribute);

  
  nsIFrame* rowFrame = rgFrame->GetFirstChild(nsnull);
  for ( ; rowFrame; rowFrame = rowFrame->GetNextSibling()) {
    if (rowFrame->GetType() == nsGkAtoms::tableRowFrame) {
      if (MOZrowAtom) { 
        rowFrame->GetContent()->UnsetAttr(kNameSpaceID_None, MOZrowAtom, PR_FALSE);
        MapRowAttributesIntoCSS(tableFrame, rowFrame);    
      } else { 
        nsIFrame* cellFrame = rowFrame->GetFirstChild(nsnull);
        for ( ; cellFrame; cellFrame = cellFrame->GetNextSibling()) {
          if (IS_TABLE_CELL(cellFrame->GetType())) {
            cellFrame->GetContent()->UnsetAttr(kNameSpaceID_None, MOZcolAtom, PR_FALSE);
            MapColAttributesIntoCSS(tableFrame, rowFrame, cellFrame);
          }
        }
      }
    }
  }

  
  PresContext()->PresShell()->FrameConstructor()->
    PostRestyleEvent(mContent, eReStyle_Self, nsChangeHint_ReflowFrame);

  return NS_OK;
}

nsIFrame*
nsMathMLmtableOuterFrame::GetRowFrameAt(nsPresContext* aPresContext,
                                        PRInt32         aRowIndex)
{
  PRInt32 rowCount, colCount;
  GetTableSize(rowCount, colCount);

  
  if (aRowIndex < 0) {
    aRowIndex = rowCount + aRowIndex;
  }
  
  --aRowIndex;

  
  if (0 <= aRowIndex && aRowIndex <= rowCount) {
    nsIFrame* tableFrame = mFrames.FirstChild();
    if (!tableFrame || tableFrame->GetType() != nsGkAtoms::tableFrame)
      return nsnull;
    nsIFrame* rgFrame = tableFrame->GetFirstChild(nsnull);
    if (!rgFrame || rgFrame->GetType() != nsGkAtoms::tableRowGroupFrame)
      return nsnull;
    nsTableIterator rowIter(*rgFrame);
    nsIFrame* rowFrame = rowIter.First();
    for ( ; rowFrame; rowFrame = rowIter.Next()) {
      if (aRowIndex == 0) {
        DEBUG_VERIFY_THAT_FRAME_IS(rowFrame, TABLE_ROW);
        if (rowFrame->GetType() != nsGkAtoms::tableRowFrame)
          return nsnull;

        return rowFrame;
      }
      --aRowIndex;
    }
  }
  return nsnull;
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

  
  
  PRInt32 rowIndex = 0;
  eAlign tableAlign = eAlign_axis;
  GetAttribute(mContent, nsnull, nsGkAtoms::align, value);
  if (!value.IsEmpty()) {
    ParseAlignAttribute(value, tableAlign, rowIndex);
  }

  
  
  
  
  nscoord dy = 0;
  nscoord height = aDesiredSize.height;
  nsIFrame* rowFrame = nsnull;
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
      
      aReflowState.rendContext->SetFont(GetStyleFont()->mFont, nsnull,
                                        aPresContext->GetUserFontSet());
      nsCOMPtr<nsIFontMetrics> fm;
      aReflowState.rendContext->GetFontMetrics(*getter_AddRefs(fm));
      nscoord axisHeight;
      GetAxisHeight(*aReflowState.rendContext, fm, axisHeight);
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

  
  mBoundingMetrics.Clear();
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
nsMathMLmtableFrame::SetInitialChildList(nsIAtom*  aListName,
                                         nsFrameList& aChildList)
{
  nsresult rv = nsTableFrame::SetInitialChildList(aListName, aChildList);
  if (NS_FAILED(rv)) return rv;
  MapAllAttributesIntoCSS(this);
  return rv;
}

void
nsMathMLmtableFrame::RestyleTable()
{
  
  MapAllAttributesIntoCSS(this);

  
  PresContext()->PresShell()->FrameConstructor()->
    PostRestyleEvent(mContent, eReStyle_Self, nsChangeHint_ReflowFrame);
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
nsMathMLmtrFrame::AttributeChanged(PRInt32  aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   PRInt32  aModType)
{
  
  
  
  
  

  if (aAttribute == nsGkAtoms::rowalign_) {
    
    mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::MOZrowalign, PR_FALSE);
    MapRowAttributesIntoCSS(nsTableFrame::GetTableFrame(this), this);
    
    return NS_OK;
  }

  if (aAttribute != nsGkAtoms::columnalign_)
    return NS_OK;

  
  DeleteProperty(aAttribute);

  
  
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  nsIFrame* cellFrame = GetFirstChild(nsnull);
  for ( ; cellFrame; cellFrame = cellFrame->GetNextSibling()) {
    if (IS_TABLE_CELL(cellFrame->GetType())) {
      cellFrame->GetContent()->
        UnsetAttr(kNameSpaceID_None, nsGkAtoms::MOZcolumnalign, PR_FALSE);
      MapColAttributesIntoCSS(tableFrame, this, cellFrame);
    }
  }

  
  PresContext()->PresShell()->FrameConstructor()->
    PostRestyleEvent(mContent, eReStyle_Self, nsChangeHint_ReflowFrame);

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

PRInt32
nsMathMLmtdFrame::GetRowSpan()
{
  PRInt32 rowspan = 1;

  
  if ((mContent->Tag() == nsGkAtoms::mtd_) && !GetStyleContext()->GetPseudo()) {
    nsAutoString value;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::rowspan, value);
    if (!value.IsEmpty()) {
      PRInt32 error;
      rowspan = value.ToInteger(&error);
      if (error || rowspan < 0)
        rowspan = 1;
      rowspan = NS_MIN(rowspan, MAX_ROWSPAN);
    }
  }
  return rowspan;
}

PRInt32
nsMathMLmtdFrame::GetColSpan()
{
  PRInt32 colspan = 1;

  
  if ((mContent->Tag() == nsGkAtoms::mtd_) && !GetStyleContext()->GetPseudo()) {
    nsAutoString value;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::columnspan_, value);
    if (!value.IsEmpty()) {
      PRInt32 error;
      colspan = value.ToInteger(&error);
      if (error || colspan < 0 || colspan > MAX_COLSPAN)
        colspan = 1;
    }
  }
  return colspan;
}

NS_IMETHODIMP
nsMathMLmtdFrame::AttributeChanged(PRInt32  aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   PRInt32  aModType)
{
  
  
  
  
  
  

  if (aAttribute == nsGkAtoms::columnalign_) {
    
    mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::MOZcolumnalign, PR_FALSE);
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
