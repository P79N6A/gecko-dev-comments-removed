






































#include "TypeInState.h"
#include "nsEditor.h"





NS_IMPL_CYCLE_COLLECTION_1(TypeInState, mLastSelectionContainer)
NS_IMPL_CYCLE_COLLECTING_ADDREF(TypeInState)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TypeInState)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TypeInState)
  NS_INTERFACE_MAP_ENTRY(nsISelectionListener)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END




 
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
  NS_ENSURE_TRUE(aSelection, NS_ERROR_NULL_POINTER);
  
  bool isCollapsed = false;
  nsresult result = aSelection->GetIsCollapsed(&isCollapsed);

  NS_ENSURE_SUCCESS(result, result);

  if (isCollapsed)
  {
    result = nsEditor::GetStartNodeAndOffset(aSelection, getter_AddRefs(mLastSelectionContainer), &mLastSelectionOffset);
  }
  return result;
}


NS_IMETHODIMP TypeInState::NotifySelectionChanged(nsIDOMDocument *, nsISelection *aSelection, PRInt16)
{
  
  
  
  
  
  
  
  
  

  if (aSelection)
  {
    bool isCollapsed = false;
    nsresult result = aSelection->GetIsCollapsed(&isCollapsed);
    NS_ENSURE_SUCCESS(result, result);

    PRInt32 rangeCount = 0;
    result = aSelection->GetRangeCount(&rangeCount);
    NS_ENSURE_SUCCESS(result, result);

    if (isCollapsed && rangeCount)
    {
      nsCOMPtr<nsIDOMNode> selNode;
      PRInt32 selOffset = 0;

      result = nsEditor::GetStartNodeAndOffset(aSelection, getter_AddRefs(selNode), &selOffset);

      NS_ENSURE_SUCCESS(result, result);

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
  for(PRUint32 i = 0, n = mClearedArray.Length(); i < n; i++) {
    delete mClearedArray[i];
  }
  mClearedArray.Clear();
  for(PRUint32 i = 0, n = mSetArray.Length(); i < n; i++) {
    delete mSetArray[i];
  }
  mSetArray.Clear();
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
    
    item = mSetArray[index];
    item->value = aValue;
  }
  else 
  {
    
    item = new PropItem(aProp,aAttr,aValue);
    NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);
    
    
    mSetArray.AppendElement(item);
    
    
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
  NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);
  
  
  RemovePropFromSetList(aProp,aAttr);
  
  
  mClearedArray.AppendElement(item);
  
  return NS_OK;
}





  
nsresult TypeInState::TakeClearProperty(PropItem **outPropItem)
{
  NS_ENSURE_TRUE(outPropItem, NS_ERROR_NULL_POINTER);
  *outPropItem = nsnull;
  PRUint32 count = mClearedArray.Length();
  if (count)
  {
    count--; 
    *outPropItem = mClearedArray[count];
    mClearedArray.RemoveElementAt(count);
  }
  return NS_OK;
}




  
nsresult TypeInState::TakeSetProperty(PropItem **outPropItem)
{
  NS_ENSURE_TRUE(outPropItem, NS_ERROR_NULL_POINTER);
  *outPropItem = nsnull;
  PRUint32 count = mSetArray.Length();
  if (count)
  {
    count--; 
    *outPropItem = mSetArray[count];
    mSetArray.RemoveElementAt(count);
  }
  return NS_OK;
}




nsresult TypeInState::TakeRelativeFontSize(PRInt32 *outRelSize)
{
  NS_ENSURE_TRUE(outRelSize, NS_ERROR_NULL_POINTER);
  *outRelSize = mRelativeFontSize;
  mRelativeFontSize = 0;
  return NS_OK;
}

nsresult TypeInState::GetTypingState(bool &isSet, bool &theSetting, nsIAtom *aProp)
{
  return GetTypingState(isSet, theSetting, aProp, EmptyString(), nsnull);
}

nsresult TypeInState::GetTypingState(bool &isSet, 
                                     bool &theSetting, 
                                     nsIAtom *aProp, 
                                     const nsString &aAttr)
{
  return GetTypingState(isSet, theSetting, aProp, aAttr, nsnull);
}


nsresult TypeInState::GetTypingState(bool &isSet, 
                                     bool &theSetting, 
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
  if (!aProp)
  {
    
    for(PRUint32 i = 0, n = mSetArray.Length(); i < n; i++) {
      delete mSetArray[i];
    }
    mSetArray.Clear();
    mRelativeFontSize=0;
  }
  else if (FindPropInList(aProp, aAttr, nsnull, mSetArray, index))
  {
    delete mSetArray[index];
    mSetArray.RemoveElementAt(index);
  }
  return NS_OK;
}


nsresult TypeInState::RemovePropFromClearedList(nsIAtom *aProp, 
                                            const nsString &aAttr)
{
  PRInt32 index;
  if (FindPropInList(aProp, aAttr, nsnull, mClearedArray, index))
  {
    delete mClearedArray[index];
    mClearedArray.RemoveElementAt(index);
  }
  return NS_OK;
}


bool TypeInState::IsPropSet(nsIAtom *aProp, 
                              const nsString &aAttr,
                              nsString* outValue)
{
  PRInt32 i;
  return IsPropSet(aProp, aAttr, outValue, i);
}


bool TypeInState::IsPropSet(nsIAtom *aProp, 
                              const nsString &aAttr,
                              nsString *outValue,
                              PRInt32 &outIndex)
{
  
  PRUint32 i, count = mSetArray.Length();
  for (i=0; i<count; i++)
  {
    PropItem *item = mSetArray[i];
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


bool TypeInState::IsPropCleared(nsIAtom *aProp, 
                                  const nsString &aAttr)
{
  PRInt32 i;
  return IsPropCleared(aProp, aAttr, i);
}


bool TypeInState::IsPropCleared(nsIAtom *aProp, 
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

bool TypeInState::FindPropInList(nsIAtom *aProp, 
                                   const nsAString &aAttr,
                                   nsAString *outValue,
                                   nsTArray<PropItem*> &aList,
                                   PRInt32 &outIndex)
{
  
  PRUint32 i, count = aList.Length();
  for (i=0; i<count; i++)
  {
    PropItem *item = aList[i];
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







PropItem::PropItem() : 
 tag(nsnull)
,attr()
,value()
{
  MOZ_COUNT_CTOR(PropItem);
}

PropItem::PropItem(nsIAtom *aTag, const nsAString &aAttr, const nsAString &aValue) :
 tag(aTag)
,attr(aAttr)
,value(aValue)
{
  MOZ_COUNT_CTOR(PropItem);
}

PropItem::~PropItem()
{
  MOZ_COUNT_DTOR(PropItem);
}
