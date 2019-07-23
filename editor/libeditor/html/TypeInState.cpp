






































#include "TypeInState.h"
#include "nsEditor.h"





NS_IMPL_ISUPPORTS1(TypeInState, nsISelectionListener)




 
TypeInState::TypeInState() :
 mSetArray()
,mClearedArray()
,mRelativeFontSize(0)
,mLastSelectionOffset(0)
{
  Reset();
}

TypeInState::~TypeInState()
{
  
  

  Reset();
}

nsresult TypeInState::UpdateSelState(nsISelection *aSelection)
{
  if (!aSelection) return NS_ERROR_NULL_POINTER;
  
  PRBool isCollapsed = PR_FALSE;
  nsresult result = aSelection->GetIsCollapsed(&isCollapsed);

  if (NS_FAILED(result)) return result;

  if (isCollapsed)
  {
    result = nsEditor::GetStartNodeAndOffset(aSelection, address_of(mLastSelectionContainer), &mLastSelectionOffset);
  }
  return result;
}


NS_IMETHODIMP TypeInState::NotifySelectionChanged(nsIDOMDocument *, nsISelection *aSelection, PRInt16)
{
  
  
  
  
  
  
  
  
  

  if (aSelection)
  {
    PRBool isCollapsed = PR_FALSE;
    nsresult result = aSelection->GetIsCollapsed(&isCollapsed);

    if (NS_FAILED(result)) return result;

    if (isCollapsed)
    {
      nsCOMPtr<nsIDOMNode> selNode;
      PRInt32 selOffset = 0;

      result = nsEditor::GetStartNodeAndOffset(aSelection, address_of(selNode), &selOffset);

      if (NS_FAILED(result)) return result;

      if (selNode && selNode == mLastSelectionContainer && selOffset == mLastSelectionOffset)
      {
        
        return NS_OK;
      }

      mLastSelectionContainer = selNode;
      mLastSelectionOffset = selOffset;
    }
    else
    {
      mLastSelectionContainer = nsnull;
      mLastSelectionOffset = 0;
    }
  }

  Reset(); 
  return NS_OK;
}

void TypeInState::Reset()
{
  PRInt32 count;
  PropItem *propItemPtr;
  
  while ((count = mClearedArray.Count()))
  {
    
    count--; 
    propItemPtr = (PropItem*)mClearedArray.ElementAt(count);
    mClearedArray.RemoveElementAt(count);
    if (propItemPtr) delete propItemPtr;
  }
  while ((count = mSetArray.Count()))
  {
    
    count--; 
    propItemPtr = (PropItem*)mSetArray.ElementAt(count);
    mSetArray.RemoveElementAt(count);
    if (propItemPtr) delete propItemPtr;
  }
}


nsresult TypeInState::SetProp(nsIAtom *aProp)
{
  return SetProp(aProp,EmptyString(),EmptyString());
}

nsresult TypeInState::SetProp(nsIAtom *aProp, const nsString &aAttr)
{
  return SetProp(aProp,aAttr,EmptyString());
}

nsresult TypeInState::SetProp(nsIAtom *aProp, const nsString &aAttr, const nsString &aValue)
{
  
  if (nsEditProperty::big == aProp)
  {
    mRelativeFontSize++;
    return NS_OK;
  }
  if (nsEditProperty::small == aProp)
  {
    mRelativeFontSize--;
    return NS_OK;
  }

  PRInt32 index;
  PropItem *item;

  if (IsPropSet(aProp,aAttr,nsnull,index))
  {
    
    item = (PropItem*)mSetArray[index];
    item->value = aValue;
  }
  else 
  {
    
    item = new PropItem(aProp,aAttr,aValue);
    if (!item) return NS_ERROR_OUT_OF_MEMORY;
    
    
    mSetArray.AppendElement((void*)item);
    
    
    RemovePropFromClearedList(aProp,aAttr);  
  }
    
  return NS_OK;
}


nsresult TypeInState::ClearAllProps()
{
  
  return ClearProp(nsnull,EmptyString());
}

nsresult TypeInState::ClearProp(nsIAtom *aProp)
{
  return ClearProp(aProp,EmptyString());
}

nsresult TypeInState::ClearProp(nsIAtom *aProp, const nsString &aAttr)
{
  
  if (IsPropCleared(aProp,aAttr)) return NS_OK;
  
  
  PropItem *item = new PropItem(aProp,aAttr,EmptyString());
  if (!item) return NS_ERROR_OUT_OF_MEMORY;
  
  
  RemovePropFromSetList(aProp,aAttr);
  
  
  mClearedArray.AppendElement((void*)item);
  
  return NS_OK;
}





  
nsresult TypeInState::TakeClearProperty(PropItem **outPropItem)
{
  if (!outPropItem) return NS_ERROR_NULL_POINTER;
  *outPropItem = nsnull;
  PRInt32 count = mClearedArray.Count();
  if (count) 
  {
    count--; 
    *outPropItem = (PropItem*)mClearedArray[count];
    mClearedArray.RemoveElementAt(count);
  }
  return NS_OK;
}




  
nsresult TypeInState::TakeSetProperty(PropItem **outPropItem)
{
  if (!outPropItem) return NS_ERROR_NULL_POINTER;
  *outPropItem = nsnull;
  PRInt32 count = mSetArray.Count();
  if (count) 
  {
    count--; 
    *outPropItem = (PropItem*)mSetArray[count];
    mSetArray.RemoveElementAt(count);
  }
  return NS_OK;
}




nsresult TypeInState::TakeRelativeFontSize(PRInt32 *outRelSize)
{
  if (!outRelSize) return NS_ERROR_NULL_POINTER;
  *outRelSize = mRelativeFontSize;
  mRelativeFontSize = 0;
  return NS_OK;
}

nsresult TypeInState::GetTypingState(PRBool &isSet, PRBool &theSetting, nsIAtom *aProp)
{
  return GetTypingState(isSet, theSetting, aProp, EmptyString(), nsnull);
}

nsresult TypeInState::GetTypingState(PRBool &isSet, 
                                     PRBool &theSetting, 
                                     nsIAtom *aProp, 
                                     const nsString &aAttr)
{
  return GetTypingState(isSet, theSetting, aProp, aAttr, nsnull);
}


nsresult TypeInState::GetTypingState(PRBool &isSet, 
                                     PRBool &theSetting, 
                                     nsIAtom *aProp,
                                     const nsString &aAttr, 
                                     nsString *aValue)
{
  if (IsPropSet(aProp, aAttr, aValue))
  {
    isSet = PR_TRUE;
    theSetting = PR_TRUE;
  }
  else if (IsPropCleared(aProp, aAttr))
  {
    isSet = PR_TRUE;
    theSetting = PR_FALSE;
  }
  else
  {
    isSet = PR_FALSE;
  }
  return NS_OK;
}






 
nsresult TypeInState::RemovePropFromSetList(nsIAtom *aProp, 
                                            const nsString &aAttr)
{
  PRInt32 index;
  PropItem *item;
  if (!aProp)
  {
    
    mRelativeFontSize=0;
    while ((index = mSetArray.Count()))
    {
      
      index--; 
      item = (PropItem*)mSetArray.ElementAt(index);
      mSetArray.RemoveElementAt(index);
      if (item) delete item;
    }
  }
  else if (FindPropInList(aProp, aAttr, nsnull, mSetArray, index))
  {
    item = (PropItem*)mSetArray.ElementAt(index);
    mSetArray.RemoveElementAt(index);
    if (item) delete item;
  }
  return NS_OK;
}


nsresult TypeInState::RemovePropFromClearedList(nsIAtom *aProp, 
                                            const nsString &aAttr)
{
  PRInt32 index;
  if (FindPropInList(aProp, aAttr, nsnull, mClearedArray, index))
  {
    PropItem *item = (PropItem*)mClearedArray.ElementAt(index);
    mClearedArray.RemoveElementAt(index);
    if (item) delete item;
  }
  return NS_OK;
}


PRBool TypeInState::IsPropSet(nsIAtom *aProp, 
                              const nsString &aAttr,
                              nsString* outValue)
{
  PRInt32 i;
  return IsPropSet(aProp, aAttr, outValue, i);
}


PRBool TypeInState::IsPropSet(nsIAtom *aProp, 
                              const nsString &aAttr,
                              nsString *outValue,
                              PRInt32 &outIndex)
{
  
  PRInt32 i, count = mSetArray.Count();
  for (i=0; i<count; i++)
  {
    PropItem *item = (PropItem*)mSetArray[i];
    if ( (item->tag == aProp) &&
         (item->attr == aAttr) )
    {
      if (outValue) *outValue = item->value;
      outIndex = i;
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}


PRBool TypeInState::IsPropCleared(nsIAtom *aProp, 
                                  const nsString &aAttr)
{
  PRInt32 i;
  return IsPropCleared(aProp, aAttr, i);
}


PRBool TypeInState::IsPropCleared(nsIAtom *aProp, 
                                  const nsString &aAttr,
                                  PRInt32 &outIndex)
{
  if (FindPropInList(aProp, aAttr, nsnull, mClearedArray, outIndex))
    return PR_TRUE;
  if (FindPropInList(0, EmptyString(), nsnull, mClearedArray, outIndex))
  {
    
    outIndex = -1;
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool TypeInState::FindPropInList(nsIAtom *aProp, 
                                   const nsAString &aAttr,
                                   nsAString *outValue,
                                   nsVoidArray &aList,
                                   PRInt32 &outIndex)
{
  
  PRInt32 i, count = aList.Count();
  for (i=0; i<count; i++)
  {
    PropItem *item = (PropItem*)aList[i];
    if ( (item->tag == aProp) &&
         (item->attr == aAttr) ) 
    {
      if (outValue) *outValue = item->value;
      outIndex = i;
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}







PropItem::PropItem(nsIAtom *aTag, const nsAString &aAttr, const nsAString &aValue) :
 tag(aTag)
,attr(aAttr)
,value(aValue)
{
}

PropItem::~PropItem()
{
}
