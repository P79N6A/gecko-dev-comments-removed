











































#include "nsGrid.h"
#include "nsGridRowGroupLayout.h"
#include "nsBox.h"
#include "nsIScrollableFrame.h"
#include "nsSprocketLayout.h"
#include "nsGridLayout2.h"
#include "nsGridRow.h"
#include "nsGridCell.h"









































































nsGrid::nsGrid():mBox(nsnull),
                 mRows(nsnull),
                 mColumns(nsnull), 
                 mRowsBox(nsnull),
                 mColumnsBox(nsnull),
                 mNeedsRebuild(true),
                 mRowCount(0),
                 mColumnCount(0),
                 mExtraRowCount(0),
                 mExtraColumnCount(0),
                 mCellMap(nsnull),
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

  
  
  mRowsBox = nsnull;
  mColumnsBox = nsnull;
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

  
  PRInt32 computedRowCount = 0;
  PRInt32 computedColumnCount = 0;
  PRInt32 rowCount = 0;
  PRInt32 columnCount = 0;

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

  mRows = nsnull;
  mColumns = nsnull;
  mCellMap = nsnull;
  mColumnCount = 0;
  mRowCount = 0;
  mExtraColumnCount = 0;
  mExtraRowCount = 0;
  mRowsBox = nsnull;
  mColumnsBox = nsnull;
}




void
nsGrid::FindRowsAndColumns(nsIBox** aRows, nsIBox** aColumns)
{
  *aRows = nsnull;
  *aColumns = nsnull;

  
  nsIBox* child = nsnull;
  
  if (mBox)
    child = mBox->GetChildBox();

  while(child)
  {
    nsIBox* oldBox = child;
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

    child = child->GetNextBox();
  }
}






void
nsGrid::CountRowsColumns(nsIBox* aRowBox, PRInt32& aRowCount, PRInt32& aComputedColumnCount)
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
nsGrid::BuildRows(nsIBox* aBox, PRInt32 aRowCount, nsGridRow** aRows, bool aIsHorizontal)
{
  
  if (aRowCount == 0) {

    
    if (*aRows)
      delete[] (*aRows);

    *aRows = nsnull;
    return;
  }

  
  nsGridRow* row;
  
  
  if (aIsHorizontal)
  { 
    if (aRowCount > mRowCount) {
       delete[] mRows;
       row = new nsGridRow[aRowCount];
    } else {
      for (PRInt32 i=0; i < mRowCount; i++)
        mRows[i].Init(nsnull, false);

      row = mRows;
    }
  } else {
    if (aRowCount > mColumnCount) {
       delete[] mColumns;
       row = new nsGridRow[aRowCount];
    } else {
       for (PRInt32 i=0; i < mColumnCount; i++)
         mColumns[i].Init(nsnull, false);

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
nsGrid::BuildCellMap(PRInt32 aRows, PRInt32 aColumns)
{
  PRInt32 size = aRows*aColumns;
  PRInt32 oldsize = mRowCount*mColumnCount;
  if (size == 0) {
    delete[] mCellMap;
  }
  else {
    if (size > oldsize) {
      delete[] mCellMap;
      return new nsGridCell[size];
    } else {
      
      for (PRInt32 i=0; i < oldsize; i++)
      {
        mCellMap[i].SetBoxInRow(nsnull);
        mCellMap[i].SetBoxInColumn(nsnull);
      }
      return mCellMap;
    }
  }
  return nsnull;
}





void
nsGrid::PopulateCellMap(nsGridRow* aRows, nsGridRow* aColumns, PRInt32 aRowCount, PRInt32 aColumnCount, bool aIsHorizontal)
{
  if (!aRows)
    return;

   
  PRInt32 j = 0;

  for(PRInt32 i=0; i < aRowCount; i++) 
  {
     nsIBox* child = nsnull;
     nsGridRow* row = &aRows[i];

     
     if (row->mIsBogus) 
       continue;

     child = row->mBox;
     if (child) {
       child = child->GetChildBox();

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

         child = child->GetNextBox();

         j++;
       }
     }
  }
}





void 
nsGrid::DirtyRows(nsIBox* aRowBox, nsBoxLayoutState& aState)
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
nsGrid::GetColumnAt(PRInt32 aIndex, bool aIsHorizontal)
{
  return GetRowAt(aIndex, !aIsHorizontal);
}

nsGridRow*
nsGrid::GetRowAt(PRInt32 aIndex, bool aIsHorizontal)
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
nsGrid::GetCellAt(PRInt32 aX, PRInt32 aY)
{
  RebuildIfNeeded();

  NS_ASSERTION(aY < mRowCount && aY >= 0, "Index out of range");
  NS_ASSERTION(aX < mColumnCount && aX >= 0, "Index out of range");
  return &mCellMap[aY*mColumnCount+aX];
}

PRInt32
nsGrid::GetExtraColumnCount(bool aIsHorizontal)
{
  return GetExtraRowCount(!aIsHorizontal);
}

PRInt32
nsGrid::GetExtraRowCount(bool aIsHorizontal)
{
  RebuildIfNeeded();

  if (aIsHorizontal)
    return mExtraRowCount;
  else
    return mExtraColumnCount;
}







nsSize
nsGrid::GetPrefRowSize(nsBoxLayoutState& aState, PRInt32 aRowIndex, bool aIsHorizontal)
{ 
  nsSize size(0,0);
  if (!(aRowIndex >=0 && aRowIndex < GetRowCount(aIsHorizontal)))
    return size;

  nscoord height = GetPrefRowHeight(aState, aRowIndex, aIsHorizontal);
  SetLargestSize(size, height, aIsHorizontal);

  return size;
}

nsSize
nsGrid::GetMinRowSize(nsBoxLayoutState& aState, PRInt32 aRowIndex, bool aIsHorizontal)
{ 
  nsSize size(0,0);
  if (!(aRowIndex >=0 && aRowIndex < GetRowCount(aIsHorizontal)))
    return size;

  nscoord height = GetMinRowHeight(aState, aRowIndex, aIsHorizontal);
  SetLargestSize(size, height, aIsHorizontal);

  return size;
}

nsSize
nsGrid::GetMaxRowSize(nsBoxLayoutState& aState, PRInt32 aRowIndex, bool aIsHorizontal)
{ 
  nsSize size(NS_INTRINSICSIZE,NS_INTRINSICSIZE);
  if (!(aRowIndex >=0 && aRowIndex < GetRowCount(aIsHorizontal)))
    return size;

  nscoord height = GetMaxRowHeight(aState, aRowIndex, aIsHorizontal);
  SetSmallestSize(size, height, aIsHorizontal);

  return size;
}


nsIGridPart*
nsGrid::GetPartFromBox(nsIBox* aBox)
{
  if (!aBox)
    return nsnull;

  nsBoxLayout* layout = aBox->GetLayoutManager();
  return layout ? layout->AsGridPart() : nsnull;
}

nsMargin
nsGrid::GetBoxTotalMargin(nsIBox* aBox, bool aIsHorizontal)
{
  nsMargin margin(0,0,0,0);
  
  
  
  nsIGridPart* part = GetPartFromBox(aBox);
  if (part)
    margin = part->GetTotalMargin(aBox, aIsHorizontal);

  return margin;
}










void
nsGrid::GetFirstAndLastRow(nsBoxLayoutState& aState, 
                          PRInt32& aFirstIndex, 
                          PRInt32& aLastIndex, 
                          nsGridRow*& aFirstRow,
                          nsGridRow*& aLastRow,
                          bool aIsHorizontal)
{
  aFirstRow = nsnull;
  aLastRow = nsnull;
  aFirstIndex = -1;
  aLastIndex = -1;

  PRInt32 count = GetRowCount(aIsHorizontal);

  if (count == 0)
    return;


  
  
  
  

  
  PRInt32 i;
  for (i=0; i < count; i++)
  {
     nsGridRow* row = GetRowAt(i,aIsHorizontal);
     if (!row->IsCollapsed(aState)) {
       aFirstIndex = i;
       aFirstRow = row;
       break;
     }
  }

  
  for (i=count-1; i >= 0; i--)
  {
     nsGridRow* row = GetRowAt(i,aIsHorizontal);
     if (!row->IsCollapsed(aState)) {
       aLastIndex = i;
       aLastRow = row;
       break;
     }

  }
}






void
nsGrid::GetRowOffsets(nsBoxLayoutState& aState, PRInt32 aIndex, nscoord& aTop, nscoord& aBottom, bool aIsHorizontal)
{

  RebuildIfNeeded();

  nsGridRow* row = GetRowAt(aIndex, aIsHorizontal);

  if (row->IsOffsetSet()) 
  {
    aTop    = row->mTop;
    aBottom = row->mBottom;
    return;
  }

  
  nsIBox* box = row->GetBox();

  
  nsMargin margin(0,0,0,0);
  nsMargin border(0,0,0,0);
  nsMargin padding(0,0,0,0);
  nsMargin totalBorderPadding(0,0,0,0);
  nsMargin totalMargin(0,0,0,0);

  
  
  if (box && !row->mIsBogus)
  {
    if (!box->IsCollapsed(aState))
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

  
  

  
  

  
  
  PRInt32 firstIndex = 0;
  PRInt32 lastIndex = 0;
  nsGridRow* firstRow = nsnull;
  nsGridRow* lastRow = nsnull;
  GetFirstAndLastRow(aState, firstIndex, lastIndex, firstRow, lastRow, aIsHorizontal);

  if (aIndex == firstIndex || aIndex == lastIndex) {
    nscoord maxTop = 0;
    nscoord maxBottom = 0;

    
    
    PRInt32 count = GetColumnCount(aIsHorizontal); 

    for (PRInt32 i=0; i < count; i++)
    {  
      nsMargin totalChildBorderPadding(0,0,0,0);

      nsGridRow* column = GetColumnAt(i,aIsHorizontal);
      nsIBox* box = column->GetBox();

      if (box) 
      {
        
        if (!box->IsCollapsed(aState))
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
nsGrid::GetPrefRowHeight(nsBoxLayoutState& aState, PRInt32 aIndex, bool aIsHorizontal)
{
  RebuildIfNeeded();

  nsGridRow* row = GetRowAt(aIndex, aIsHorizontal);

  if (row->IsCollapsed(aState))
    return 0;

  if (row->IsPrefSet()) 
    return row->mPref;

  nsIBox* box = row->mBox;

  
  if (box) 
  {
    bool widthSet, heightSet;
    nsSize cssSize(-1, -1);
    nsIBox::AddCSSPrefSize(box, cssSize, widthSet, heightSet);

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

  PRInt32 count = GetColumnCount(aIsHorizontal); 

  for (PRInt32 i=0; i < count; i++)
  {  
    if (aIsHorizontal)
     child = GetCellAt(i,aIndex);
    else
     child = GetCellAt(aIndex,i);

    
    if (!child->IsCollapsed(aState))
    {
      nsSize childSize = child->GetPrefSize(aState);

      nsSprocketLayout::AddLargestSize(size, childSize, aIsHorizontal);
    }
  }

  row->mPref = GET_HEIGHT(size, aIsHorizontal) + top + bottom;

  return row->mPref;
}

nscoord
nsGrid::GetMinRowHeight(nsBoxLayoutState& aState, PRInt32 aIndex, bool aIsHorizontal)
{
  RebuildIfNeeded();

  nsGridRow* row = GetRowAt(aIndex, aIsHorizontal);

  if (row->IsCollapsed(aState))
    return 0;

  if (row->IsMinSet()) 
    return row->mMin;

  nsIBox* box = row->mBox;

  
  if (box) {
    bool widthSet, heightSet;
    nsSize cssSize(-1, -1);
    nsIBox::AddCSSMinSize(aState, box, cssSize, widthSet, heightSet);

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

  PRInt32 count = GetColumnCount(aIsHorizontal); 

  for (PRInt32 i=0; i < count; i++)
  {  
    if (aIsHorizontal)
     child = GetCellAt(i,aIndex);
    else
     child = GetCellAt(aIndex,i);

    
    if (!child->IsCollapsed(aState))
    {
      nsSize childSize = child->GetMinSize(aState);

      nsSprocketLayout::AddLargestSize(size, childSize, aIsHorizontal);
    }
  }

  row->mMin = GET_HEIGHT(size, aIsHorizontal);

  return row->mMin;
}

nscoord
nsGrid::GetMaxRowHeight(nsBoxLayoutState& aState, PRInt32 aIndex, bool aIsHorizontal)
{
  RebuildIfNeeded();

  nsGridRow* row = GetRowAt(aIndex, aIsHorizontal);

  if (row->IsCollapsed(aState))
    return 0;

  if (row->IsMaxSet()) 
    return row->mMax;

  nsIBox* box = row->mBox;

  
  if (box) {
    bool widthSet, heightSet;
    nsSize cssSize(-1, -1);
    nsIBox::AddCSSMaxSize(box, cssSize, widthSet, heightSet);

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

  PRInt32 count = GetColumnCount(aIsHorizontal); 

  for (PRInt32 i=0; i < count; i++)
  {  
    if (aIsHorizontal)
     child = GetCellAt(i,aIndex);
    else
     child = GetCellAt(aIndex,i);

    
    if (!child->IsCollapsed(aState))
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
nsGrid::IsGrid(nsIBox* aBox)
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
nsGrid::GetRowFlex(nsBoxLayoutState& aState, PRInt32 aIndex, bool aIsHorizontal)
{
  RebuildIfNeeded();

  nsGridRow* row = GetRowAt(aIndex, aIsHorizontal);

  if (row->IsFlexSet()) 
    return row->mFlex;

  nsIBox* box = row->mBox;
  row->mFlex = 0;

  if (box) {

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    


    
    
    
    
    
    
    
    
    
    
    

    
    
    

    box = GetScrollBox(box);
    nsIBox* parent = box->GetParentBox();
    nsIBox* parentsParent=nsnull;

    while(parent)
    {
      parent = GetScrollBox(parent);
      parentsParent = parent->GetParentBox();

      
      
      
      if (parentsParent) {
        if (!IsGrid(parentsParent)) {
          nscoord flex = parent->GetFlex(aState);
          nsIBox::AddCSSFlex(aState, parent, flex);
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
    nsIBox::AddCSSFlex(aState, box, row->mFlex);
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

PRInt32 
nsGrid::GetRowCount(PRInt32 aIsHorizontal)
{
  RebuildIfNeeded();

  if (aIsHorizontal)
    return mRowCount;
  else
    return mColumnCount;
}

PRInt32 
nsGrid::GetColumnCount(PRInt32 aIsHorizontal)
{
  return GetRowCount(!aIsHorizontal);
}




void 
nsGrid::CellAddedOrRemoved(nsBoxLayoutState& aState, PRInt32 aIndex, bool aIsHorizontal)
{
  
  
  
  if (mMarkingDirty)
    return;

  NeedsRebuild(aState);
}




void 
nsGrid::RowAddedOrRemoved(nsBoxLayoutState& aState, PRInt32 aIndex, bool aIsHorizontal)
{
  
  
  if (mMarkingDirty)
    return;

  NeedsRebuild(aState);
}





nsIBox*
nsGrid::GetScrolledBox(nsIBox* aChild)
{
  
      nsIScrollableFrame *scrollFrame = do_QueryFrame(aChild);
      if (scrollFrame) {
         nsIFrame* scrolledFrame = scrollFrame->GetScrolledFrame();
         NS_ASSERTION(scrolledFrame,"Error no scroll frame!!");
         return scrolledFrame;
      }

      return aChild;
}





nsIBox*
nsGrid::GetScrollBox(nsIBox* aChild)
{
  if (!aChild)
    return nsnull;

  
  nsIBox* parent = aChild->GetParentBox();

  
  
  
  
  while (parent) {
    nsIScrollableFrame *scrollFrame = do_QueryFrame(parent);
    
    if (scrollFrame)
      return parent;

    nsCOMPtr<nsIGridPart> parentGridRow = GetPartFromBox(parent);
    
    if (parentGridRow) 
      break;

    parent = parent->GetParentBox();
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
