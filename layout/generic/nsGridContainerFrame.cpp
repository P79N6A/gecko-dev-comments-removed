







#include "nsGridContainerFrame.h"

#include "nsCSSAnonBoxes.h"
#include "nsPresContext.h"
#include "nsReadableUtils.h"
#include "nsStyleContext.h"

using namespace mozilla;









static uint32_t
FindLine(const nsString& aName, uint32_t aNth,
         uint32_t aFromIndex, uint32_t aImplicitLine,
         const nsTArray<nsTArray<nsString>>& aNameList)
{
  MOZ_ASSERT(aNth != 0);
  const uint32_t len = aNameList.Length();
  uint32_t lastFound = 0;
  uint32_t line;
  uint32_t i = aFromIndex;
  for (; i < len; i = line) {
    line = i + 1;
    if (line == aImplicitLine || aNameList[i].Contains(aName)) {
      lastFound = line;
      if (--aNth == 0) {
        return lastFound;
      }
    }
  }
  if (aImplicitLine > i) {
    
    
    lastFound = aImplicitLine;
  }
  return lastFound;
}




static uint32_t
RFindLine(const nsString& aName, uint32_t aNth,
          uint32_t aFromIndex, uint32_t aImplicitLine,
          const nsTArray<nsTArray<nsString>>& aNameList)
{
  MOZ_ASSERT(aNth != 0);
  const uint32_t len = aNameList.Length();
  uint32_t lastFound = 0;
  
  
  if (aImplicitLine > len && aImplicitLine < aFromIndex) {
    lastFound = aImplicitLine;
    if (--aNth == 0) {
      return lastFound;
    }
  }
  uint32_t i = aFromIndex == 0 ? len : std::min(aFromIndex, len);
  for (; i; --i) {
    if (i == aImplicitLine || aNameList[i - 1].Contains(aName)) {
      lastFound = i;
      if (--aNth == 0) {
        break;
      }
    }
  }
  return lastFound;
}

static uint32_t
FindNamedLine(const nsString& aName, int32_t aNth,
              uint32_t aFromIndex, uint32_t aImplicitLine,
              const nsTArray<nsTArray<nsString>>& aNameList)
{
  MOZ_ASSERT(aNth != 0);
  if (aNth > 0) {
    return ::FindLine(aName, aNth, aFromIndex, aImplicitLine, aNameList);
  }
  return ::RFindLine(aName, -aNth, aFromIndex, aImplicitLine, aNameList);
}






static const css::GridNamedArea*
FindNamedArea(const nsSubstring& aName, const nsStylePosition* aStyle)
{
  if (!aStyle->mGridTemplateAreas) {
    return nullptr;
  }
  const nsTArray<css::GridNamedArea>& areas =
    aStyle->mGridTemplateAreas->mNamedAreas;
  size_t len = areas.Length();
  for (size_t i = 0; i < len; ++i) {
    const css::GridNamedArea& area = areas[i];
    if (area.mName == aName) {
      return &area;
    }
  }
  return nullptr;
}



static bool
IsNameWithSuffix(const nsString& aString, const nsString& aSuffix,
                 uint32_t* aIndex)
{
  if (StringEndsWith(aString, aSuffix)) {
    *aIndex = aString.Length() - aSuffix.Length();
    return *aIndex != 0;
  }
  return false;
}

static bool
IsNameWithEndSuffix(const nsString& aString, uint32_t* aIndex)
{
  return IsNameWithSuffix(aString, NS_LITERAL_STRING("-end"), aIndex);
}

static bool
IsNameWithStartSuffix(const nsString& aString, uint32_t* aIndex)
{
  return IsNameWithSuffix(aString, NS_LITERAL_STRING("-start"), aIndex);
}






NS_QUERYFRAME_HEAD(nsGridContainerFrame)
  NS_QUERYFRAME_ENTRY(nsGridContainerFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

NS_IMPL_FRAMEARENA_HELPERS(nsGridContainerFrame)

nsContainerFrame*
NS_NewGridContainerFrame(nsIPresShell* aPresShell,
                         nsStyleContext* aContext)
{
  return new (aPresShell) nsGridContainerFrame(aContext);
}







void
nsGridContainerFrame::AddImplicitNamedAreas(
  const nsTArray<nsTArray<nsString>>& aLineNameLists)
{
  
  
  
  
  
  
  
  const uint32_t len = aLineNameLists.Length();
  nsTHashtable<nsStringHashKey> currentStarts;
  ImplicitNamedAreas* areas = GetImplicitNamedAreas();
  for (uint32_t i = 0; i < len; ++i) {
    const nsTArray<nsString>& names(aLineNameLists[i]);
    const uint32_t jLen = names.Length();
    for (uint32_t j = 0; j < jLen; ++j) {
      const nsString& name = names[j];
      uint32_t index;
      if (::IsNameWithStartSuffix(name, &index)) {
        currentStarts.PutEntry(nsDependentSubstring(name, 0, index));
      } else if (::IsNameWithEndSuffix(name, &index)) {
        nsDependentSubstring area(name, 0, index);
        if (currentStarts.Contains(area)) {
          if (!areas) {
            areas = new ImplicitNamedAreas;
            Properties().Set(ImplicitNamedAreasProperty(), areas);
          }
          areas->PutEntry(area);
        }
      }
    }
  }
}

void
nsGridContainerFrame::InitImplicitNamedAreas(const nsStylePosition* aStyle)
{
  ImplicitNamedAreas* areas = GetImplicitNamedAreas();
  if (areas) {
    
    
    areas->Clear();
  }
  AddImplicitNamedAreas(aStyle->mGridTemplateColumns.mLineNameLists);
  AddImplicitNamedAreas(aStyle->mGridTemplateRows.mLineNameLists);
  if (areas && areas->Count() == 0) {
    Properties().Delete(ImplicitNamedAreasProperty());
  }
}

uint32_t
nsGridContainerFrame::ResolveLine(
  const nsStyleGridLine& aLine,
  int32_t aNth,
  uint32_t aFromIndex,
  const nsTArray<nsTArray<nsString>>& aLineNameList,
  uint32_t GridNamedArea::* aAreaStart,
  uint32_t GridNamedArea::* aAreaEnd,
  uint32_t aExplicitGridEnd,
  LineRangeSide aSide,
  const nsStylePosition* aStyle)
{
  MOZ_ASSERT(!aLine.IsAuto());
  uint32_t line = 0;
  if (aLine.mLineName.IsEmpty()) {
    MOZ_ASSERT(aNth != 0, "css-grid 9.2: <integer> must not be zero.");
    line = std::max(int32_t(aFromIndex) + aNth, 1);
  } else {
    if (aNth == 0) {
      
      aNth = 1;
    }
    bool isNameOnly = !aLine.mHasSpan && aLine.mInteger == 0;
    if (isNameOnly) {
      const GridNamedArea* area = ::FindNamedArea(aLine.mLineName, aStyle);
      if (area || HasImplicitNamedArea(aLine.mLineName)) {
        
        
        
        uint32_t implicitLine = 0;
        nsAutoString lineName(aLine.mLineName);
        if (aSide == eLineRangeSideStart) {
          lineName.AppendLiteral("-start");
          implicitLine = area ? area->*aAreaStart : 0;
        } else {
          lineName.AppendLiteral("-end");
          implicitLine = area ? area->*aAreaEnd : 0;
        }
        
        
        line = ::FindNamedLine(lineName, aNth, aFromIndex, implicitLine,
                               aLineNameList);
      }
    }

    if (line == 0) {
      
      uint32_t implicitLine = 0;
      uint32_t index;
      auto GridNamedArea::* areaEdge = aAreaStart;
      bool found = ::IsNameWithStartSuffix(aLine.mLineName, &index);
      if (!found) {
        found = ::IsNameWithEndSuffix(aLine.mLineName, &index);
        areaEdge = aAreaEnd;
      }
      if (found) {
        const GridNamedArea* area =
          ::FindNamedArea(nsDependentSubstring(aLine.mLineName, 0, index),
                          aStyle);
        if (area) {
          implicitLine = area->*areaEdge;
        }
      }
      line = ::FindNamedLine(aLine.mLineName, aNth, aFromIndex, implicitLine,
                             aLineNameList);
    }

    if (line == 0) {
      
      if (aLine.mHasSpan) {
        
        
        line = std::max(int32_t(aFromIndex) + aNth, 1);
      } else {
        
        
        
        
        line = aNth >= 0 ? 1 : aExplicitGridEnd;
      }
    }
  }
  
  
  MOZ_ASSERT(line != 0 || (!aLine.mHasSpan && aLine.mInteger == 0),
             "Given a <integer> or 'span' the result should never be auto");
  return line;
}

nsGridContainerFrame::LinePair
nsGridContainerFrame::ResolveLineRangeHelper(
  const nsStyleGridLine& aStart,
  const nsStyleGridLine& aEnd,
  const nsTArray<nsTArray<nsString>>& aLineNameList,
  uint32_t GridNamedArea::* aAreaStart,
  uint32_t GridNamedArea::* aAreaEnd,
  uint32_t aExplicitGridEnd,
  const nsStylePosition* aStyle)
{
  if (aStart.mHasSpan) {
    if (aEnd.mHasSpan || aEnd.IsAuto()) {
      
      if (aStart.mLineName.IsEmpty()) {
        
        
        return LinePair(0, aStart.mInteger);
      }
      
      
      return LinePair(0, 1); 
    }

    auto end = ResolveLine(aEnd, aEnd.mInteger, 0, aLineNameList, aAreaStart,
                           aAreaEnd, aExplicitGridEnd, eLineRangeSideEnd,
                           aStyle);
    if (end == 0) {
      
      return LinePair(0, aStart.mInteger);
    }
    int32_t span = aStart.mInteger == 0 ? 1 : aStart.mInteger;
    auto start = ResolveLine(aStart, -span, end, aLineNameList, aAreaStart,
                             aAreaEnd, aExplicitGridEnd, eLineRangeSideStart,
                             aStyle);
    MOZ_ASSERT(start > 0, "A start span can never resolve to 'auto'");
    return LinePair(start, end);
  }

  uint32_t start = 0;
  if (!aStart.IsAuto()) {
    start = ResolveLine(aStart, aStart.mInteger, 0, aLineNameList, aAreaStart,
                        aAreaEnd, aExplicitGridEnd, eLineRangeSideStart,
                        aStyle);
  }
  if (aEnd.IsAuto()) {
    
    return LinePair(start, 1); 
  }
  if (start == 0 && aEnd.mHasSpan) {
    if (aEnd.mLineName.IsEmpty()) {
      
      MOZ_ASSERT(aEnd.mInteger != 0);
      return LinePair(0, aEnd.mInteger);
    }
    
    
    return LinePair(0, 1); 
  }

  uint32_t from = aEnd.mHasSpan ? start : 0;
  auto end = ResolveLine(aEnd, aEnd.mInteger, from, aLineNameList, aAreaStart,
                         aAreaEnd, aExplicitGridEnd, eLineRangeSideEnd, aStyle);
  if (end == 0) {
    
    end = 1; 
  } else if (start == 0) {
    
    start = std::max(1U, end - 1);
  }
  return LinePair(start, end);
}

nsGridContainerFrame::LineRange
nsGridContainerFrame::ResolveLineRange(
  const nsStyleGridLine& aStart,
  const nsStyleGridLine& aEnd,
  const nsTArray<nsTArray<nsString>>& aLineNameList,
  uint32_t GridNamedArea::* aAreaStart,
  uint32_t GridNamedArea::* aAreaEnd,
  uint32_t aExplicitGridEnd,
  const nsStylePosition* aStyle)
{
  LinePair r = ResolveLineRangeHelper(aStart, aEnd, aLineNameList, aAreaStart,
                                      aAreaEnd, aExplicitGridEnd, aStyle);
  MOZ_ASSERT(r.second != 0);

  
  if (r.second <= r.first) {
    r.second = r.first + 1;
  }
  return LineRange(r.first, r.second);
}

nsGridContainerFrame::GridArea
nsGridContainerFrame::PlaceDefinite(nsIFrame* aChild,
                                    const nsStylePosition* aStyle)
{
  const nsStylePosition* itemStyle = aChild->StylePosition();
  return GridArea(
    ResolveLineRange(itemStyle->mGridColumnStart, itemStyle->mGridColumnEnd,
                     aStyle->mGridTemplateColumns.mLineNameLists,
                     &GridNamedArea::mColumnStart, &GridNamedArea::mColumnEnd,
                     mExplicitGridColEnd, aStyle),
    ResolveLineRange(itemStyle->mGridRowStart, itemStyle->mGridRowEnd,
                     aStyle->mGridTemplateRows.mLineNameLists,
                     &GridNamedArea::mRowStart, &GridNamedArea::mRowEnd,
                     mExplicitGridRowEnd, aStyle));
}

void
nsGridContainerFrame::InitializeGridBounds(const nsStylePosition* aStyle)
{
  
  uint32_t colEnd = aStyle->mGridTemplateColumns.mLineNameLists.Length();
  uint32_t rowEnd = aStyle->mGridTemplateRows.mLineNameLists.Length();
  auto areas = aStyle->mGridTemplateAreas.get();
  mExplicitGridColEnd = std::max(colEnd, areas ? areas->mNColumns + 1 : 1);
  mExplicitGridRowEnd = std::max(rowEnd, areas ? areas->NRows() + 1 : 1);
  mGridColEnd = mExplicitGridColEnd;
  mGridRowEnd = mExplicitGridRowEnd;
}

void
nsGridContainerFrame::PlaceGridItems(const nsStylePosition* aStyle)
{
  InitializeGridBounds(aStyle);

  
  
  for (nsFrameList::Enumerator e(PrincipalChildList()); !e.AtEnd(); e.Next()) {
    nsIFrame* child = e.get();
    PlaceDefinite(child, aStyle);
  }
}

void
nsGridContainerFrame::Reflow(nsPresContext*           aPresContext,
                             nsHTMLReflowMetrics&     aDesiredSize,
                             const nsHTMLReflowState& aReflowState,
                             nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsGridContainerFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  if (IsFrameTreeTooDeep(aReflowState, aDesiredSize, aStatus)) {
    return;
  }

#ifdef DEBUG
  SanityCheckAnonymousGridItems();
#endif 

  LogicalMargin bp = aReflowState.ComputedLogicalBorderPadding();
  bp.ApplySkipSides(GetLogicalSkipSides());
  nscoord contentBSize = GetEffectiveComputedBSize(aReflowState);
  if (contentBSize == NS_AUTOHEIGHT) {
    contentBSize = 0;
  }
  WritingMode wm = aReflowState.GetWritingMode();
  LogicalSize finalSize(wm,
                        aReflowState.ComputedISize() + bp.IStartEnd(wm),
                        contentBSize + bp.BStartEnd(wm));
  aDesiredSize.SetSize(wm, finalSize);
  aDesiredSize.SetOverflowAreasToDesiredBounds();

  const nsStylePosition* stylePos = aReflowState.mStylePosition;
  InitImplicitNamedAreas(stylePos);
  PlaceGridItems(stylePos);

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

nsIAtom*
nsGridContainerFrame::GetType() const
{
  return nsGkAtoms::gridContainerFrame;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsGridContainerFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("GridContainer"), aResult);
}
#endif

#ifdef DEBUG
static bool
FrameWantsToBeInAnonymousGridItem(nsIFrame* aFrame)
{
  
  
  return (aFrame->IsFrameOfType(nsIFrame::eLineParticipant) ||
          nsGkAtoms::placeholderFrame == aFrame->GetType());
}







void
nsGridContainerFrame::SanityCheckAnonymousGridItems() const
{
  
  
  ChildListIDs noCheckLists = kAbsoluteList | kFixedList;
  ChildListIDs checkLists = kPrincipalList | kOverflowList;
  for (nsIFrame::ChildListIterator childLists(this);
       !childLists.IsDone(); childLists.Next()) {
    if (!checkLists.Contains(childLists.CurrentID())) {
      MOZ_ASSERT(noCheckLists.Contains(childLists.CurrentID()),
                 "unexpected non-empty child list");
      continue;
    }

    bool prevChildWasAnonGridItem = false;
    nsFrameList children = childLists.CurrentList();
    for (nsFrameList::Enumerator e(children); !e.AtEnd(); e.Next()) {
      nsIFrame* child = e.get();
      MOZ_ASSERT(!FrameWantsToBeInAnonymousGridItem(child),
                 "frame wants to be inside an anonymous grid item, "
                 "but it isn't");
      if (child->StyleContext()->GetPseudo() ==
            nsCSSAnonBoxes::anonymousGridItem) {








        MOZ_ASSERT(!prevChildWasAnonGridItem, "two anon grid items in a row");
        nsIFrame* firstWrappedChild = child->GetFirstPrincipalChild();
        MOZ_ASSERT(firstWrappedChild,
                   "anonymous grid item is empty (shouldn't happen)");
        prevChildWasAnonGridItem = true;
      } else {
        prevChildWasAnonGridItem = false;
      }
    }
  }
}
#endif 
