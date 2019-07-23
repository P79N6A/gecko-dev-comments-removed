







































#include "nsCOMPtr.h"
#include "nsTreeSelection.h"
#include "nsIBoxObject.h"
#include "nsITreeBoxObject.h"
#include "nsITreeView.h"
#include "nsString.h"
#include "nsIDOMElement.h"
#include "nsIDOMClassInfo.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsGUIEvent.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsPLDOMEvent.h"
#include "nsEventDispatcher.h"
#include "nsAutoPtr.h"


struct nsTreeRange
{
  nsTreeSelection* mSelection;

  nsTreeRange* mPrev;
  nsTreeRange* mNext;

  PRInt32 mMin;
  PRInt32 mMax;

  nsTreeRange(nsTreeSelection* aSel, PRInt32 aSingleVal)
    :mSelection(aSel), mPrev(nsnull), mNext(nsnull), mMin(aSingleVal), mMax(aSingleVal) {}
  nsTreeRange(nsTreeSelection* aSel, PRInt32 aMin, PRInt32 aMax) 
    :mSelection(aSel), mPrev(nsnull), mNext(nsnull), mMin(aMin), mMax(aMax) {}

  ~nsTreeRange() { delete mNext; }

  void Connect(nsTreeRange* aPrev = nsnull, nsTreeRange* aNext = nsnull) {
    if (aPrev)
      aPrev->mNext = this;
    else
      mSelection->mFirstRange = this;

    if (aNext)
      aNext->mPrev = this;

    mPrev = aPrev;
    mNext = aNext;
  }

  nsresult RemoveRange(PRInt32 aStart, PRInt32 aEnd) {
    
    
    if (aEnd < mMin)
      return NS_OK;
    
    if (aEnd < mMax) {
      if (aStart <= mMin) {
        
        mMin = aEnd + 1;
      } else {
        
        nsTreeRange* range = new nsTreeRange(mSelection, aEnd + 1, mMax);
        if (!range)
          return NS_ERROR_OUT_OF_MEMORY;

        mMax = aStart - 1;
        range->Connect(this, mNext);
      }
      return NS_OK;
    }
    nsTreeRange* next = mNext;
    if (aStart <= mMin) {
      
      if (mPrev)
        mPrev->mNext = next;
      else
        mSelection->mFirstRange = next;

      if (next)
        next->mPrev = mPrev;
      mPrev = mNext = nsnull;
      delete this;
    } else if (aStart <= mMax) {
      
      mMax = aStart - 1;
    }
    return next ? next->RemoveRange(aStart, aEnd) : NS_OK;
  }

  nsresult Remove(PRInt32 aIndex) {
    if (aIndex >= mMin && aIndex <= mMax) {
      
      if (mMin == mMax) {
        
        if (mPrev)
          mPrev->mNext = mNext;
        if (mNext)
          mNext->mPrev = mPrev;
        nsTreeRange* first = mSelection->mFirstRange;
        if (first == this)
          mSelection->mFirstRange = mNext;
        mNext = mPrev = nsnull;
        delete this;
      }
      else if (aIndex == mMin)
        mMin++;
      else if (aIndex == mMax)
        mMax--;
      else {
        
        nsTreeRange* newRange = new nsTreeRange(mSelection, aIndex + 1, mMax);
        if (!newRange)
          return NS_ERROR_OUT_OF_MEMORY;

        newRange->Connect(this, mNext);
        mMax = aIndex - 1;
      }
    }
    else if (mNext)
      return mNext->Remove(aIndex);

    return NS_OK;
  }

  nsresult Add(PRInt32 aIndex) {
    if (aIndex < mMin) {
      
      if (aIndex + 1 == mMin)
        mMin = aIndex;
      else if (mPrev && mPrev->mMax+1 == aIndex)
        mPrev->mMax = aIndex;
      else {
        
        nsTreeRange* newRange = new nsTreeRange(mSelection, aIndex);
        if (!newRange)
          return NS_ERROR_OUT_OF_MEMORY;

        newRange->Connect(mPrev, this);
      }
    }
    else if (mNext)
      mNext->Add(aIndex);
    else {
      
      if (mMax+1 == aIndex)
        mMax = aIndex;
      else {
        
        nsTreeRange* newRange = new nsTreeRange(mSelection, aIndex);
        if (!newRange)
          return NS_ERROR_OUT_OF_MEMORY;

        newRange->Connect(this, nsnull);
      }
    }
    return NS_OK;
  }

  PRBool Contains(PRInt32 aIndex) {
    if (aIndex >= mMin && aIndex <= mMax)
      return PR_TRUE;

    if (mNext)
      return mNext->Contains(aIndex);

    return PR_FALSE;
  }

  PRInt32 Count() {
    PRInt32 total = mMax - mMin + 1;
    if (mNext)
      total += mNext->Count();
    return total;
  }

  void Invalidate() {
    if (mSelection->mTree)
      mSelection->mTree->InvalidateRange(mMin, mMax);
    if (mNext)
      mNext->Invalidate();
  }

  void RemoveAllBut(PRInt32 aIndex) {
    if (aIndex >= mMin && aIndex <= mMax) {

      
      mSelection->mFirstRange->Invalidate();

      mMin = aIndex;
      mMax = aIndex;
      
      nsTreeRange* first = mSelection->mFirstRange;
      if (mPrev)
        mPrev->mNext = mNext;
      if (mNext)
        mNext->mPrev = mPrev;
      mNext = mPrev = nsnull;
      
      if (first != this) {
        delete mSelection->mFirstRange;
        mSelection->mFirstRange = this;
      }
    }
    else if (mNext)
      mNext->RemoveAllBut(aIndex);
  }

  void Insert(nsTreeRange* aRange) {
    if (mMin >= aRange->mMax)
      aRange->Connect(mPrev, this);
    else if (mNext)
      mNext->Insert(aRange);
    else 
      aRange->Connect(this, nsnull);
  }
};

nsTreeSelection::nsTreeSelection(nsITreeBoxObject* aTree)
  : mTree(aTree),
    mSuppressed(PR_FALSE),
    mCurrentIndex(-1),
    mShiftSelectPivot(-1),
    mFirstRange(nsnull)
{
}

nsTreeSelection::~nsTreeSelection()
{
  delete mFirstRange;
  if (mSelectTimer)
    mSelectTimer->Cancel();
}

NS_IMPL_CYCLE_COLLECTION_1(nsTreeSelection, mCurrentColumn)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsTreeSelection)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsTreeSelection)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsTreeSelection)
  NS_INTERFACE_MAP_ENTRY(nsITreeSelection)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(TreeSelection)
NS_INTERFACE_MAP_END

NS_IMETHODIMP nsTreeSelection::GetTree(nsITreeBoxObject * *aTree)
{
  NS_IF_ADDREF(mTree);
  *aTree = mTree;
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::SetTree(nsITreeBoxObject * aTree)
{
  if (mSelectTimer) {
    mSelectTimer->Cancel();
    mSelectTimer = nsnull;
  }
  mTree = aTree; 
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::GetSingle(PRBool* aSingle)
{
  if (!mTree)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIBoxObject> boxObject = do_QueryInterface(mTree);

  nsCOMPtr<nsIDOMElement> element;
  boxObject->GetElement(getter_AddRefs(element));

  nsCOMPtr<nsIContent> content = do_QueryInterface(element);

  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::single, &nsGkAtoms::cell, &nsGkAtoms::text, nsnull};

  *aSingle = content->FindAttrValueIn(kNameSpaceID_None,
                                      nsGkAtoms::seltype,
                                      strings, eCaseMatters) >= 0;

  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::IsSelected(PRInt32 aIndex, PRBool* aResult)
{
  if (mFirstRange)
    *aResult = mFirstRange->Contains(aIndex);
  else
    *aResult = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::TimedSelect(PRInt32 aIndex, PRInt32 aMsec)
{
  PRBool suppressSelect = mSuppressed;

  if (aMsec != -1)
    mSuppressed = PR_TRUE;

  nsresult rv = Select(aIndex);
  if (NS_FAILED(rv))
    return rv;

  if (aMsec != -1) {
    mSuppressed = suppressSelect;
    if (!mSuppressed) {
      if (mSelectTimer)
        mSelectTimer->Cancel();

      mSelectTimer = do_CreateInstance("@mozilla.org/timer;1");
      mSelectTimer->InitWithFuncCallback(SelectCallback, this, aMsec, 
                                         nsITimer::TYPE_ONE_SHOT);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::Select(PRInt32 aIndex)
{
  mShiftSelectPivot = -1;

  nsresult rv = SetCurrentIndex(aIndex);
  if (NS_FAILED(rv))
    return rv;

  if (mFirstRange) {
    PRBool alreadySelected = mFirstRange->Contains(aIndex);

    if (alreadySelected) {
      PRInt32 count = mFirstRange->Count();
      if (count > 1) {
        
        mFirstRange->RemoveAllBut(aIndex);
        FireOnSelectHandler();
      }
      return NS_OK;
    }
    else {
      
      mFirstRange->Invalidate();
      delete mFirstRange;
    }
  }

  
  mFirstRange = new nsTreeRange(this, aIndex);
  if (!mFirstRange)
    return NS_ERROR_OUT_OF_MEMORY;

  mFirstRange->Invalidate();

  
  FireOnSelectHandler();
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::ToggleSelect(PRInt32 aIndex)
{
  
  
  
  
  
  
  
  
  mShiftSelectPivot = -1;
  nsresult rv = SetCurrentIndex(aIndex);
  if (NS_FAILED(rv))
    return rv;

  if (!mFirstRange)
    Select(aIndex);
  else {
    if (!mFirstRange->Contains(aIndex)) {
      PRBool single;
      rv = GetSingle(&single);
      if (NS_SUCCEEDED(rv) && !single)
        rv = mFirstRange->Add(aIndex);
    }
    else
      rv = mFirstRange->Remove(aIndex);
    if (NS_SUCCEEDED(rv)) {
      if (mTree)
        mTree->InvalidateRow(aIndex);

      FireOnSelectHandler();
    }
  }

  return rv;
}

NS_IMETHODIMP nsTreeSelection::RangedSelect(PRInt32 aStartIndex, PRInt32 aEndIndex, PRBool aAugment)
{
  PRBool single;
  nsresult rv = GetSingle(&single);
  if (NS_FAILED(rv))
    return rv;

  if ((mFirstRange || (aStartIndex != aEndIndex)) && single)
    return NS_OK;

  if (!aAugment) {
    
    if (mFirstRange) {
        mFirstRange->Invalidate();
        delete mFirstRange;
    }
  }

  if (aStartIndex == -1) {
    if (mShiftSelectPivot != -1)
      aStartIndex = mShiftSelectPivot;
    else if (mCurrentIndex != -1)
      aStartIndex = mCurrentIndex;
    else
      aStartIndex = aEndIndex;
  }

  mShiftSelectPivot = aStartIndex;
  rv = SetCurrentIndex(aEndIndex);
  if (NS_FAILED(rv))
    return rv;
  
  PRInt32 start = aStartIndex < aEndIndex ? aStartIndex : aEndIndex;
  PRInt32 end = aStartIndex < aEndIndex ? aEndIndex : aStartIndex;

  if (aAugment && mFirstRange) {
    
    
    nsresult rv = mFirstRange->RemoveRange(start, end);
    if (NS_FAILED(rv))
      return rv;
  }

  nsTreeRange* range = new nsTreeRange(this, start, end);
  if (!range)
    return NS_ERROR_OUT_OF_MEMORY;

  range->Invalidate();

  if (aAugment && mFirstRange)
    mFirstRange->Insert(range);
  else
    mFirstRange = range;

  FireOnSelectHandler();

  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::ClearRange(PRInt32 aStartIndex, PRInt32 aEndIndex)
{
  nsresult rv = SetCurrentIndex(aEndIndex);
  if (NS_FAILED(rv))
    return rv;

  if (mFirstRange) {
    PRInt32 start = aStartIndex < aEndIndex ? aStartIndex : aEndIndex;
    PRInt32 end = aStartIndex < aEndIndex ? aEndIndex : aStartIndex;

    mFirstRange->RemoveRange(start, end);

    if (mTree)
      mTree->InvalidateRange(start, end);
  }
  
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::ClearSelection()
{
  if (mFirstRange) {
    mFirstRange->Invalidate();
    delete mFirstRange;
    mFirstRange = nsnull;
  }
  mShiftSelectPivot = -1;

  FireOnSelectHandler();

  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::InvertSelection()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsTreeSelection::SelectAll()
{
  if (!mTree)
    return NS_OK;

  nsCOMPtr<nsITreeView> view;
  mTree->GetView(getter_AddRefs(view));
  if (!view)
    return NS_OK;

  PRInt32 rowCount;
  view->GetRowCount(&rowCount);
  PRBool single;
  nsresult rv = GetSingle(&single);
  if (NS_FAILED(rv))
    return rv;

  if (rowCount == 0 || (rowCount > 1 && single))
    return NS_OK;

  mShiftSelectPivot = -1;

  
  
  delete mFirstRange;

  mFirstRange = new nsTreeRange(this, 0, rowCount-1);
  mFirstRange->Invalidate();

  FireOnSelectHandler();

  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::GetRangeCount(PRInt32* aResult)
{
  PRInt32 count = 0;
  nsTreeRange* curr = mFirstRange;
  while (curr) {
    count++;
    curr = curr->mNext;
  }

  *aResult = count;
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::GetRangeAt(PRInt32 aIndex, PRInt32* aMin, PRInt32* aMax)
{
  *aMin = *aMax = -1;
  PRInt32 i = -1;
  nsTreeRange* curr = mFirstRange;
  while (curr) {
    i++;
    if (i == aIndex) {
      *aMin = curr->mMin;
      *aMax = curr->mMax;
      break;
    }
    curr = curr->mNext;
  }

  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::GetCount(PRInt32 *count)
{
  if (mFirstRange)
    *count = mFirstRange->Count();
  else 
    *count = 0;
  
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::GetSelectEventsSuppressed(PRBool *aSelectEventsSuppressed)
{
  *aSelectEventsSuppressed = mSuppressed;
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::SetSelectEventsSuppressed(PRBool aSelectEventsSuppressed)
{
  mSuppressed = aSelectEventsSuppressed;
  if (!mSuppressed)
    FireOnSelectHandler();
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::GetCurrentIndex(PRInt32 *aCurrentIndex)
{
  *aCurrentIndex = mCurrentIndex;
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::SetCurrentIndex(PRInt32 aIndex)
{
  if (!mTree) {
    return NS_ERROR_UNEXPECTED;
  }
  if (mCurrentIndex == aIndex) {
    return NS_OK;
  }
  if (mCurrentIndex != -1 && mTree)
    mTree->InvalidateRow(mCurrentIndex);
  
  mCurrentIndex = aIndex;
  if (!mTree)
    return NS_OK;
  
  if (aIndex != -1)
    mTree->InvalidateRow(aIndex);

  
  nsCOMPtr<nsIBoxObject> boxObject = do_QueryInterface(mTree);
  NS_ASSERTION(boxObject, "no box object!");
  if (!boxObject)
    return NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIDOMElement> treeElt;
  boxObject->GetElement(getter_AddRefs(treeElt));

  nsCOMPtr<nsINode> treeDOMNode(do_QueryInterface(treeElt));
  NS_ENSURE_STATE(treeDOMNode);

  nsRefPtr<nsPLDOMEvent> event =
    new nsPLDOMEvent(treeDOMNode, NS_LITERAL_STRING("DOMMenuItemActive"),
                     PR_FALSE);
  if (!event)
    return NS_ERROR_OUT_OF_MEMORY;

  return event->PostDOMEvent();
}

NS_IMETHODIMP nsTreeSelection::GetCurrentColumn(nsITreeColumn** aCurrentColumn)
{
  NS_IF_ADDREF(*aCurrentColumn = mCurrentColumn);
  return NS_OK;
}

NS_IMETHODIMP nsTreeSelection::SetCurrentColumn(nsITreeColumn* aCurrentColumn)
{
  if (!mTree) {
    return NS_ERROR_UNEXPECTED;
  }
  if (mCurrentColumn == aCurrentColumn) {
    return NS_OK;
  }

  if (mCurrentColumn) {
    if (mFirstRange)
      mTree->InvalidateCell(mFirstRange->mMin, mCurrentColumn);
    if (mCurrentIndex != -1)
      mTree->InvalidateCell(mCurrentIndex, mCurrentColumn);
  }
  
  mCurrentColumn = aCurrentColumn;
  
  if (mCurrentColumn) {
    if (mFirstRange)
      mTree->InvalidateCell(mFirstRange->mMin, mCurrentColumn);
    if (mCurrentIndex != -1)
      mTree->InvalidateCell(mCurrentIndex, mCurrentColumn);
  }

  return NS_OK;
}

#define ADD_NEW_RANGE(macro_range, macro_selection, macro_start, macro_end) \
  { \
    nsTreeRange* macro_new_range = new nsTreeRange(macro_selection, (macro_start), (macro_end)); \
    if (macro_range) \
      macro_range->Insert(macro_new_range); \
    else \
      macro_range = macro_new_range; \
  }

NS_IMETHODIMP
nsTreeSelection::AdjustSelection(PRInt32 aIndex, PRInt32 aCount)
{
  NS_ASSERTION(aCount != 0, "adjusting by zero");
  if (!aCount) return NS_OK;

  
  if ((mShiftSelectPivot != 1) && (aIndex <= mShiftSelectPivot)) {
    
    if (aCount < 0 && (mShiftSelectPivot <= (aIndex -aCount -1))) {
        mShiftSelectPivot = -1;
    }
    else {
        mShiftSelectPivot += aCount;
    }
  }

  
  if ((mCurrentIndex != -1) && (aIndex <= mCurrentIndex)) {
    
    if (aCount < 0 && (mCurrentIndex <= (aIndex -aCount -1))) {
        mCurrentIndex = -1;
    }
    else {
        mCurrentIndex += aCount;
    }
  }

  
  if (!mFirstRange) return NS_OK;

  nsTreeRange* newRange = nsnull;

  PRBool selChanged = PR_FALSE;
  nsTreeRange* curr = mFirstRange;
  while (curr) {
    if (aCount > 0) {
      
      if (aIndex > curr->mMax) {
        
        ADD_NEW_RANGE(newRange, this, curr->mMin, curr->mMax);
      }
      else if (aIndex <= curr->mMin) {  
        
        ADD_NEW_RANGE(newRange, this, curr->mMin + aCount, curr->mMax + aCount);
        selChanged = PR_TRUE;
      }
      else {
        
        
        ADD_NEW_RANGE(newRange, this, curr->mMin, aIndex - 1);
        ADD_NEW_RANGE(newRange, this, aIndex + aCount, curr->mMax + aCount);
        selChanged = PR_TRUE;
      }
    }
    else {
      
      if (aIndex > curr->mMax) {
        
        ADD_NEW_RANGE(newRange, this, curr->mMin, curr->mMax);
      }
      else {
        
        selChanged = PR_TRUE;
        PRInt32 lastIndexOfAdjustment = aIndex - aCount - 1;
        if (aIndex <= curr->mMin) {
          if (lastIndexOfAdjustment < curr->mMin) {
            
            ADD_NEW_RANGE(newRange, this, curr->mMin + aCount, curr->mMax + aCount);
          }
          else if (lastIndexOfAdjustment >= curr->mMax) {
            
          }
          else {
            
            ADD_NEW_RANGE(newRange, this, aIndex, curr->mMax + aCount)
          }
        }
        else if (lastIndexOfAdjustment >= curr->mMax) {
         
         ADD_NEW_RANGE(newRange, this, curr->mMin, aIndex - 1)
        }
        else {
          
          ADD_NEW_RANGE(newRange, this, curr->mMin, curr->mMax + aCount)
        }
      }
    }
    curr = curr->mNext;
  }

  delete mFirstRange;
  mFirstRange = newRange;

  
  if (selChanged)
    FireOnSelectHandler();

  return NS_OK;
}

NS_IMETHODIMP
nsTreeSelection::InvalidateSelection()
{
  if (mFirstRange)
    mFirstRange->Invalidate();
  return NS_OK;
}

NS_IMETHODIMP
nsTreeSelection::GetShiftSelectPivot(PRInt32* aIndex)
{
  *aIndex = mShiftSelectPivot;
  return NS_OK;
}


nsresult
nsTreeSelection::FireOnSelectHandler()
{
  if (mSuppressed || !mTree)
    return NS_OK;

  nsCOMPtr<nsIBoxObject> boxObject = do_QueryInterface(mTree);
  NS_ASSERTION(boxObject, "no box object!");
  if (!boxObject)
     return NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIDOMElement> elt;
  boxObject->GetElement(getter_AddRefs(elt));
  NS_ENSURE_STATE(elt);

  nsCOMPtr<nsINode> node(do_QueryInterface(elt));
  NS_ENSURE_STATE(node);

  nsRefPtr<nsPLDOMEvent> event =
    new nsPLDOMEvent(node, NS_LITERAL_STRING("select"), PR_FALSE);
  event->RunDOMEventWhenSafe();
  return NS_OK;
}

void
nsTreeSelection::SelectCallback(nsITimer *aTimer, void *aClosure)
{
  nsRefPtr<nsTreeSelection> self = static_cast<nsTreeSelection*>(aClosure);
  if (self) {
    self->FireOnSelectHandler();
    aTimer->Cancel();
    self->mSelectTimer = nsnull;
  }
}



nsresult
NS_NewTreeSelection(nsITreeBoxObject* aTree, nsITreeSelection** aResult)
{
  *aResult = new nsTreeSelection(aTree);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}
