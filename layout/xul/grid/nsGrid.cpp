











#include "nsGrid.h"
#include "nsGridRowGroupLayout.h"
#include "nsBox.h"
#include "nsIScrollableFrame.h"
#include "nsSprocketLayout.h"
#include "nsGridLayout2.h"
#include "nsGridRow.h"
#include "nsGridCell.h"
#include "nsHTMLReflowState.h"









































































nsGrid::nsGrid():mBox(nullptr),
                 mRows(nullptr),
                 mColumns(nullptr), 
                 mRowsBox(nullptr),
                 mColumnsBox(nullptr),
                 mNeedsRebuild(true),
                 mRowCount(0),
                 mColumnCount(0),
                 mExtraRowCount(0),
                 mExtraColumnCount(0),
                 mCellMap(nullptr),
                 mMarkingDirty(false)
{
    MOZ_COUNT_CTOR(nsGrid);
}

nsGrid::~nsGrid()
{
    FreeMap();
    MOZ_COUNT_DTOR(nsGrid);
}






void
nsGrid::NeedsRebuild(nsBoxLayoutState& aState)
{
  if (mNeedsRebuild)
    return;

  
  mNeedsRebuild = true;

  
  
  mRowsBox = nullptr;
  mColumnsBox = nullptr;
  FindRowsAndColumns(&mRowsBox, &mColumnsBox);

  
  DirtyRows(mRowsBox, aState);
  DirtyRows(mColumnsBox, aState);
}






void
nsGrid::RebuildIfNeeded()
{
  if (!mNeedsRebuild)
    return;

  mNeedsRebuild = false;

  
  FindRowsAndColumns(&mRowsBox, &mColumnsBox);

  
  int32_t computedRowCount = 0;
  int32_t computedColumnCount = 0;
  int32_t rowCount = 0;
  int32_t columnCount = 0;

  CountRowsColumns(mRowsBox, rowCount, computedColumnCount);
  CountRowsColumns(mColumnsBox, columnCount, computedRowCount);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  mExtraColumnCount = computedColumnCount - columnCount;
  if (computedColumnCount > columnCount) {
     columnCount = computedColumnCount;
  }

  
  mExtraRowCount = computedRowCount - rowCount;
  if (computedRowCount > rowCount) {
     rowCount = computedRowCount;
  }

  
  BuildRows(mRowsBox, rowCount, &mRows, true);
  BuildRows(mColumnsBox, columnCount, &mColumns, false);

  
  mCellMap = BuildCellMap(rowCount, columnCount);

  mRowCount = rowCount;
  mColumnCount = columnCount;

  
  PopulateCellMap(mRows, mColumns, mRowCount, mColumnCount, true);
  PopulateCellMap(mColumns, mRows, mColumnCount, mRowCount, false);
}

void
nsGrid::FreeMap()
{
  if (mRows) 
    delete[] mRows;

  if (mColumns)
    delete[] mColumns;

  if (mCellMap)
    delete[] mCellMap;

  mRows = nullptr;
  mColumns = nullptr;
  mCellMap = nullptr;
  mColumnCount = 0;
  mRowCount = 0;
  mExtraColumnCount = 0;
  mExtraRowCount = 0;
  mRowsBox = nullptr;
  mColumnsBox = nullptr;
}




void
nsGrid::FindRowsAndColumns(nsIFrame** aRows, nsIFrame** aColumns)
{
  *aRows = nullptr;
  *aColumns = nullptr;

  
  nsIFrame* child = nullptr;
  
  if (mBox)
    child = nsBox::GetChildBox(mBox);

  while(child)
  {
    nsIFrame* oldBox = child;
    nsIScrollableFrame *scrollFrame = do_QueryFrame(child);
    if (scrollFrame) {
       nsIFrame* scrolledFrame = scrollFrame->GetScrolledFrame();
       NS_ASSERTION(scrolledFrame,"Error no scroll frame!!");
       child = do_QueryFrame(scrolledFrame);
    }

    nsCOMPtr<nsIGridPart> monument = GetPartFromBox(child);
    if (monument)
    {
      nsGridRowGroupLayout* rowGroup = monument->CastToRowGroupLayout();
      if (rowGroup) {
         bool isHorizontal = !nsSprocketLayout::IsHorizontal(child);
         if (isHorizontal)
           *aRows = child;
         else
           *aColumns = child;

         if (*aRows && *aColumns)
           return;
      }
    }

    if (scrollFrame) {
      child = oldBox;
    }

    child = nsBox::GetNextBox(child);
  }
}






void
nsGrid::CountRowsColumns(nsIFrame* aRowBox, int32_t& aRowCount, int32_t& aComputedColumnCount)
{
  aRowCount = 0;
  aComputedColumnCount = 0;
  
  if (aRowBox) {
    nsCOMPtr<nsIGridPart> monument = GetPartFromBox(aRowBox);
    if (monument) 
       monument->CountRowsColumns(aRowBox, aRowCount, aComputedColumnCount);
  }
}





void
nsGrid::BuildRows(nsIFrame* aBox, int32_t aRowCount, nsGridRow** aRows, bool aIsHorizontal)
{
  
  if (aRowCount == 0) {

    
    if (*aRows)
      delete[] (*aRows);

    *aRows = nullptr;
    return;
  }

  
  nsGridRow* row;
  
  
  if (aIsHorizontal)
  { 
    if (aRowCount > mRowCount) {
       delete[] mRows;
       row = new nsGridRow[aRowCount];
    } else {
      for (int32_t i=0; i < mRowCount; i++)
        mRows[i].Init(nullptr, false);

      row = mRows;
    }
  } else {
    if (aRowCount > mColumnCount) {
       delete[] mColumns;
       row = new nsGridRow[aRowCount];
    } else {
       for (int32_t i=0; i < mColumnCount; i++)
         mColumns[i].Init(nullptr, false);

       row = mColumns;
    }
  }

  
  if (aBox)
  {
    nsCOMPtr<nsIGridPart> monument = GetPartFromBox(aBox);
    if (monument) {
       monument->BuildRows(aBox, row);
    }
  }

  *aRows = row;
}





nsGridCell*
nsGrid::BuildCellMap(int32_t aRows, int32_t aColumns)
{
  int32_t size = aRows*aColumns;
  int32_t oldsize = mRowCount*mColumnCount;
  if (size == 0) {
    delete[] mCellMap;
  }
  else {
    if (size > oldsize) {
      delete[] mCellMap;
      return new nsGridCell[size];
    } else {
      
      for (int32_t i=0; i < oldsize; i++)
      {
        mCellMap[i].SetBoxInRow(nullptr);
        mCellMap[i].SetBoxInColumn(nullptr);
      }
      return mCellMap;
    }
  }
  return nullptr;
}





void
nsGrid::PopulateCellMap(nsGridRow* aRows, nsGridRow* aColumns, int32_t aRowCount, int32_t aColumnCount, bool aIsHorizontal)
{
  if (!aRows)
    return;

   
  int32_t j = 0;

  for(int32_t i=0; i < aRowCount; i++) 
  {
     nsIFrame* child = nullptr;
     nsGridRow* row = &aRows[i];

     
     if (row->mIsBogus) 
       continue;

     child = row->mBox;
     if (child) {
       child = nsBox::GetChildBox(child);

       j = 0;

       while(child && j < aColumnCount)
       {
         
         nsGridRow* column = &aColumns[j];
         if (column->mIsBogus) 
         {
           j++;
           continue;
         }

         if (aIsHorizontal)
           GetCellAt(j,i)->SetBoxInRow(child);
         else
           GetCellAt(i,j)->SetBoxInColumn(child);

         child = nsBox::GetNextBox(child);

         j++;
       }
     }
  }
}





void 
nsGrid::DirtyRows(nsIFrame* aRowBox, nsBoxLayoutState& aState)
{
  
  mMarkingDirty = true;

  
  if (aRowBox) {
    nsCOMPtr<nsIGridPart> part = GetPartFromBox(aRowBox);
    if (part) 
       part->DirtyRows(aRowBox, aState);
  }

  mMarkingDirty = false;
}

nsGridRow*
nsGrid::GetColumnAt(int32_t aIndex, bool aIsHorizontal)
{
  return GetRowAt(aIndex, !aIsHorizontal);
}

nsGridRow*
nsGrid::GetRowAt(int32_t aIndex, bool aIsHorizontal)
{
  RebuildIfNeeded();

  if (aIsHorizontal) {
    NS_ASSERTION(aIndex < mRowCount && aIndex >= 0, "Index out of range");
    return &mRows[aIndex];
  } else {
    NS_ASSERTION(aIndex < mColumnCount && aIndex >= 0, "Index out of range");
    return &mColumns[aIndex];
  }
}

nsGridCell*
nsGrid::GetCellAt(int32_t aX, int32_t aY)
{
  RebuildIfNeeded();

  NS_ASSERTION(aY < mRowCount && aY >= 0, "Index out of range");
  NS_ASSERTION(aX < mColumnCount && aX >= 0, "Index out of range");
  return &mCellMap[aY*mColumnCount+aX];
}

int32_t
nsGrid::GetExtraColumnCount(bool aIsHorizontal)
{
  return GetExtraRowCount(!aIsHorizontal);
}

int32_t
nsGrid::GetExtraRowCount(bool aIsHorizontal)
{
  RebuildIfNeeded();

  if (aIsHorizontal)
    return mExtraRowCount;
  else
    return mExtraColumnCount;
}







nsSize
nsGrid::GetPrefRowSize(nsBoxLayoutState& aState, int32_t aRowIndex, bool aIsHorizontal)
{ 
  nsSize size(0,0);
  if (!(aRowIndex >=0 && aRowIndex < GetRowCount(aIsHorizontal)))
    return size;

  nscoord height = GetPrefRowHeight(aState, aRowIndex, aIsHorizontal);
  SetLargestSize(size, height, aIsHorizontal);

  return size;
}

nsSize
nsGrid::GetMinRowSize(nsBoxLayoutState& aState, int32_t aRowIndex, bool aIsHorizontal)
{ 
  nsSize size(0,0);
  if (!(aRowIndex >=0 && aRowIndex < GetRowCount(aIsHorizontal)))
    return size;

  nscoord height = GetMinRowHeight(aState, aRowIndex, aIsHorizontal);
  SetLargestSize(size, height, aIsHorizontal);

  return size;
}

nsSize
nsGrid::GetMaxRowSize(nsBoxLayoutState& aState, int32_t aRowIndex, bool aIsHorizontal)
{ 
  nsSize size(NS_INTRINSICSIZE,NS_INTRINSICSIZE);
  if (!(aRowIndex >=0 && aRowIndex < GetRowCount(aIsHorizontal)))
    return size;

  nscoord height = GetMaxRowHeight(aState, aRowIndex, aIsHorizontal);
  SetSmallestSize(size, height, aIsHorizontal);

  return size;
}


nsIGridPart*
nsGrid::GetPartFromBox(nsIFrame* aBox)
{
  if (!aBox)
    return nullptr;

  nsBoxLayout* layout = aBox->GetLayoutManager();
  return layout ? layout->AsGridPart() : nullptr;
}

nsMargin
nsGrid::GetBoxTotalMargin(nsIFrame* aBox, bool aIsHorizontal)
{
  nsMargin margin(0,0,0,0);
  
  
  
  nsIGridPart* part = GetPartFromBox(aBox);
  if (part)
    margin = part->GetTotalMargin(aBox, aIsHorizontal);

  return margin;
}










void
nsGrid::GetFirstAndLastRow(nsBoxLayoutState& aState, 
                          int32_t& aFirstIndex, 
                          int32_t& aLastIndex, 
                          nsGridRow*& aFirstRow,
                          nsGridRow*& aLastRow,
                          bool aIsHorizontal)
{
  aFirstRow = nullptr;
  aLastRow = nullptr;
  aFirstIndex = -1;
  aLastIndex = -1;

  int32_t count = GetRowCount(aIsHorizontal);

  if (count == 0)
    return;


  
  
  
  

  
  int32_t i;
  for (i=0; i < count; i++)
  {
     nsGridRow* row = GetRowAt(i,aIsHorizontal);
     if (!row->IsCollapsed()) {
       aFirstIndex = i;
       aFirstRow = row;
       break;
     }
  }

  
  for (i=count-1; i >= 0; i--)
  {
     nsGridRow* row = GetRowAt(i,aIsHorizontal);
     if (!row->IsCollapsed()) {
       aLastIndex = i;
       aLastRow = row;
       break;
     }

  }
}






void
nsGrid::GetRowOffsets(nsBoxLayoutState& aState, int32_t aIndex, nscoord& aTop, nscoord& aBottom, bool aIsHorizontal)
{

  RebuildIfNeeded();

  nsGridRow* row = GetRowAt(aIndex, aIsHorizontal);

  if (row->IsOffsetSet()) 
  {
    aTop    = row->mTop;
    aBottom = row->mBottom;
    return;
  }

  
  nsIFrame* box = row->GetBox();

  
  nsMargin margin(0,0,0,0);
  nsMargin border(0,0,0,0);
  nsMargin padding(0,0,0,0);
  nsMargin totalBorderPadding(0,0,0,0);
  nsMargin totalMargin(0,0,0,0);

  
  
  if (box && !row->mIsBogus)
  {
    if (!box->IsCollapsed())
    {
       
       
       
       box->GetBorder(border);
       box->GetPadding(padding);

       totalBorderPadding += border;
       totalBorderPadding += padding;
     }

     
     
     
     
     

     totalMargin = GetBoxTotalMargin(box, aIsHorizontal);
  }

  if (aIsHorizontal) {
    row->mTop = totalBorderPadding.top;
    row->mBottom = totalBorderPadding.bottom;
    row->mTopMargin = totalMargin.top;
    row->mBottomMargin = totalMargin.bottom;
  } else {
    row->mTop = totalBorderPadding.left;
    row->mBottom = totalBorderPadding.right;
    row->mTopMargin = totalMargin.left;
    row->mBottomMargin = totalMargin.right;
  }

  
  

  
  

  
  
  int32_t firstIndex = 0;
  int32_t lastIndex = 0;
  nsGridRow* firstRow = nullptr;
  nsGridRow* lastRow = nullptr;
  GetFirstAndLastRow(aState, firstIndex, lastIndex, firstRow, lastRow, aIsHorizontal);

  if (aIndex == firstIndex || aIndex == lastIndex) {
    nscoord maxTop = 0;
    nscoord maxBottom = 0;

    
    
    int32_t count = GetColumnCount(aIsHorizontal); 

    for (int32_t i=0; i < count; i++)
    {  
      nsMargin totalChildBorderPadding(0,0,0,0);

      nsGridRow* column = GetColumnAt(i,aIsHorizontal);
      nsIFrame* box = column->GetBox();

      if (box) 
      {
        
        if (!box->IsCollapsed())
        {
           
           
           
           margin = GetBoxTotalMargin(box, !aIsHorizontal);
           
           
           
           box->GetBorder(border);
           box->GetPadding(padding);
           totalChildBorderPadding += border;
           totalChildBorderPadding += padding;
           totalChildBorderPadding += margin;
        }

        nscoord top;
        nscoord bottom;

        
        if (aIndex == firstIndex) {
          if (aIsHorizontal) {
            top = totalChildBorderPadding.top;
          } else {
            top = totalChildBorderPadding.left;
          }
          if (top > maxTop)
            maxTop = top;
        } 

        
        if (aIndex == lastIndex) {
          if (aIsHorizontal) {
            bottom = totalChildBorderPadding.bottom;
          } else {
            bottom = totalChildBorderPadding.right;
          }
          if (bottom > maxBottom)
             maxBottom = bottom;
        }

      }
    
      
      
      if (aIndex == firstIndex) {
        if (maxTop > (row->mTop + row->mTopMargin))
          row->mTop = maxTop - row->mTopMargin;
      }

      
      
      if (aIndex == lastIndex) {
        if (maxBottom > (row->mBottom + row->mBottomMargin))
          row->mBottom = maxBottom - row->mBottomMargin;
      }
    }
  }
  
  aTop    = row->mTop;
  aBottom = row->mBottom;
}






nscoord
nsGrid::GetPrefRowHeight(nsBoxLayoutState& aState, int32_t aIndex, bool aIsHorizontal)
{
  RebuildIfNeeded();

  nsGridRow* row = GetRowAt(aIndex, aIsHorizontal);

  if (row->IsCollapsed())
    return 0;

  if (row->IsPrefSet()) 
    return row->mPref;

  nsIFrame* box = row->mBox;

  
  if (box) 
  {
    bool widthSet, heightSet;
    nsSize cssSize(-1, -1);
    nsIFrame::AddCSSPrefSize(box, cssSize, widthSet, heightSet);

    row->mPref = GET_HEIGHT(cssSize, aIsHorizontal);

    
    if (row->mPref != -1)
      return row->mPref;
  }

  
  nscoord top;
  nscoord bottom;
  GetRowOffsets(aState, aIndex, top, bottom, aIsHorizontal);

  
  
  if (row->mIsBogus)
  {
     nsSize size(0,0);
     if (box) 
     {
       size = box->GetPrefSize(aState);
       nsBox::AddMargin(box, size);
       nsGridLayout2::AddOffset(aState, box, size);
     }

     row->mPref = GET_HEIGHT(size, aIsHorizontal);
     return row->mPref;
  }

  nsSize size(0,0);

  nsGridCell* child;

  int32_t count = GetColumnCount(aIsHorizontal); 

  for (int32_t i=0; i < count; i++)
  {  
    if (aIsHorizontal)
     child = GetCellAt(i,aIndex);
    else
     child = GetCellAt(aIndex,i);

    
    if (!child->IsCollapsed())
    {
      nsSize childSize = child->GetPrefSize(aState);

      nsSprocketLayout::AddLargestSize(size, childSize, aIsHorizontal);
    }
  }

  row->mPref = GET_HEIGHT(size, aIsHorizontal) + top + bottom;

  return row->mPref;
}

nscoord
nsGrid::GetMinRowHeight(nsBoxLayoutState& aState, int32_t aIndex, bool aIsHorizontal)
{
  RebuildIfNeeded();

  nsGridRow* row = GetRowAt(aIndex, aIsHorizontal);

  if (row->IsCollapsed())
    return 0;

  if (row->IsMinSet()) 
    return row->mMin;

  nsIFrame* box = row->mBox;

  
  if (box) {
    bool widthSet, heightSet;
    nsSize cssSize(-1, -1);
    nsIFrame::AddCSSMinSize(aState, box, cssSize, widthSet, heightSet);

    row->mMin = GET_HEIGHT(cssSize, aIsHorizontal);

    
    if (row->mMin != -1)
      return row->mMin;
  }

  
  nscoord top;
  nscoord bottom;
  GetRowOffsets(aState, aIndex, top, bottom, aIsHorizontal);

  
  
  if (row->mIsBogus)
  {
     nsSize size(0,0);
     if (box) {
       size = box->GetPrefSize(aState);
       nsBox::AddMargin(box, size);
       nsGridLayout2::AddOffset(aState, box, size);
     }

     row->mMin = GET_HEIGHT(size, aIsHorizontal) + top + bottom;
     return row->mMin;
  }

  nsSize size(0,0);

  nsGridCell* child;

  int32_t count = GetColumnCount(aIsHorizontal); 

  for (int32_t i=0; i < count; i++)
  {  
    if (aIsHorizontal)
     child = GetCellAt(i,aIndex);
    else
     child = GetCellAt(aIndex,i);

    
    if (!child->IsCollapsed())
    {
      nsSize childSize = child->GetMinSize(aState);

      nsSprocketLayout::AddLargestSize(size, childSize, aIsHorizontal);
    }
  }

  row->mMin = GET_HEIGHT(size, aIsHorizontal);

  return row->mMin;
}

nscoord
nsGrid::GetMaxRowHeight(nsBoxLayoutState& aState, int32_t aIndex, bool aIsHorizontal)
{
  RebuildIfNeeded();

  nsGridRow* row = GetRowAt(aIndex, aIsHorizontal);

  if (row->IsCollapsed())
    return 0;

  if (row->IsMaxSet()) 
    return row->mMax;

  nsIFrame* box = row->mBox;

  
  if (box) {
    bool widthSet, heightSet;
    nsSize cssSize(-1, -1);
    nsIFrame::AddCSSMaxSize(box, cssSize, widthSet, heightSet);

    row->mMax = GET_HEIGHT(cssSize, aIsHorizontal);

    
    if (row->mMax != -1)
      return row->mMax;
  }

  
  nscoord top;
  nscoord bottom;
  GetRowOffsets(aState, aIndex, top, bottom, aIsHorizontal);

  
  
  if (row->mIsBogus)
  {
     nsSize size(NS_INTRINSICSIZE,NS_INTRINSICSIZE);
     if (box) {
       size = box->GetPrefSize(aState);
       nsBox::AddMargin(box, size);
       nsGridLayout2::AddOffset(aState, box, size);
     }

     row->mMax = GET_HEIGHT(size, aIsHorizontal);
     return row->mMax;
  }

  nsSize size(NS_INTRINSICSIZE,NS_INTRINSICSIZE);

  nsGridCell* child;

  int32_t count = GetColumnCount(aIsHorizontal); 

  for (int32_t i=0; i < count; i++)
  {  
    if (aIsHorizontal)
     child = GetCellAt(i,aIndex);
    else
     child = GetCellAt(aIndex,i);

    
    if (!child->IsCollapsed())
    {
      nsSize min = child->GetMinSize(aState);
      nsSize childSize = nsBox::BoundsCheckMinMax(min, child->GetMaxSize(aState));
      nsSprocketLayout::AddLargestSize(size, childSize, aIsHorizontal);
    }
  }

  row->mMax = GET_HEIGHT(size, aIsHorizontal) + top + bottom;

  return row->mMax;
}

bool
nsGrid::IsGrid(nsIFrame* aBox)
{
  nsIGridPart* part = GetPartFromBox(aBox);
  if (!part)
    return false;

  nsGridLayout2* grid = part->CastToGridLayout();

  if (grid)
    return true;

  return false;
}






nscoord
nsGrid::GetRowFlex(nsBoxLayoutState& aState, int32_t aIndex, bool aIsHorizontal)
{
  RebuildIfNeeded();

  nsGridRow* row = GetRowAt(aIndex, aIsHorizontal);

  if (row->IsFlexSet()) 
    return row->mFlex;

  nsIFrame* box = row->mBox;
  row->mFlex = 0;

  if (box) {

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    


    
    
    
    
    
    
    
    
    
    
    

    
    
    

    box = GetScrollBox(box);
    nsIFrame* parent = nsBox::GetParentBox(box);
    nsIFrame* parentsParent=nullptr;

    while(parent)
    {
      parent = GetScrollBox(parent);
      parentsParent = nsBox::GetParentBox(parent);

      
      
      
      if (parentsParent) {
        if (!IsGrid(parentsParent)) {
          nscoord flex = parent->GetFlex(aState);
          nsIFrame::AddCSSFlex(aState, parent, flex);
          if (flex == 0) {
            row->mFlex = 0;
            return row->mFlex;
          }
        } else 
          break;
      }

      parent = parentsParent;
    }
    
    
    row->mFlex = box->GetFlex(aState);
    nsIFrame::AddCSSFlex(aState, box, row->mFlex);
  }

  return row->mFlex;
}

void
nsGrid::SetLargestSize(nsSize& aSize, nscoord aHeight, bool aIsHorizontal)
{
  if (aIsHorizontal) {
    if (aSize.height < aHeight)
      aSize.height = aHeight;
  } else {
    if (aSize.width < aHeight)
      aSize.width = aHeight;
  }
}

void
nsGrid::SetSmallestSize(nsSize& aSize, nscoord aHeight, bool aIsHorizontal)
{
  if (aIsHorizontal) {
    if (aSize.height > aHeight)
      aSize.height = aHeight;
  } else {
    if (aSize.width < aHeight)
      aSize.width = aHeight;
  }
}

int32_t 
nsGrid::GetRowCount(int32_t aIsHorizontal)
{
  RebuildIfNeeded();

  if (aIsHorizontal)
    return mRowCount;
  else
    return mColumnCount;
}

int32_t 
nsGrid::GetColumnCount(int32_t aIsHorizontal)
{
  return GetRowCount(!aIsHorizontal);
}




void 
nsGrid::CellAddedOrRemoved(nsBoxLayoutState& aState, int32_t aIndex, bool aIsHorizontal)
{
  
  
  
  if (mMarkingDirty)
    return;

  NeedsRebuild(aState);
}




void 
nsGrid::RowAddedOrRemoved(nsBoxLayoutState& aState, int32_t aIndex, bool aIsHorizontal)
{
  
  
  if (mMarkingDirty)
    return;

  NeedsRebuild(aState);
}





nsIFrame*
nsGrid::GetScrolledBox(nsIFrame* aChild)
{
  
      nsIScrollableFrame *scrollFrame = do_QueryFrame(aChild);
      if (scrollFrame) {
         nsIFrame* scrolledFrame = scrollFrame->GetScrolledFrame();
         NS_ASSERTION(scrolledFrame,"Error no scroll frame!!");
         return scrolledFrame;
      }

      return aChild;
}





nsIFrame*
nsGrid::GetScrollBox(nsIFrame* aChild)
{
  if (!aChild)
    return nullptr;

  
  nsIFrame* parent = nsBox::GetParentBox(aChild);

  
  
  
  
  while (parent) {
    nsIScrollableFrame *scrollFrame = do_QueryFrame(parent);
    
    if (scrollFrame)
      return parent;

    nsCOMPtr<nsIGridPart> parentGridRow = GetPartFromBox(parent);
    
    if (parentGridRow) 
      break;

    parent = nsBox::GetParentBox(parent);
  }

  return aChild;
}



#ifdef DEBUG_grid
void
nsGrid::PrintCellMap()
{
  
  printf("-----Columns------\n");
  for (int x=0; x < mColumnCount; x++) 
  {
   
    nsGridRow* column = GetColumnAt(x);
    printf("%d(pf=%d, mn=%d, mx=%d) ", x, column->mPref, column->mMin, column->mMax);
  }

  printf("\n-----Rows------\n");
  for (x=0; x < mRowCount; x++) 
  {
    nsGridRow* column = GetRowAt(x);
    printf("%d(pf=%d, mn=%d, mx=%d) ", x, column->mPref, column->mMin, column->mMax);
  }

  printf("\n");
  
}
#endif
