





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
  
  if (!aSelection->Collapsed()) {
    return NS_OK;
  }

  return nsEditor::GetStartNodeAndOffset(aSelection, getter_AddRefs(mLastSelectionContainer), &mLastSelectionOffset);
}


NS_IMETHODIMP TypeInState::NotifySelectionChanged(nsIDOMDocument *, nsISelection *aSelection, PRInt16)
{
  
  
  
  
  
  
  
  
  

  if (aSelection) {
    PRInt32 rangeCount = 0;
    nsresult result = aSelection->GetRangeCount(&rangeCount);
    NS_ENSURE_SUCCESS(result, result);

    if (aSelection->Collapsed() && rangeCount) {
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


void
TypeInState::SetProp(nsIAtom* aProp, const nsAString& aAttr,
                     const nsAString& aValue)
{
  
  if (nsEditProperty::big == aProp) {
    mRelativeFontSize++;
    return;
  }
  if (nsEditProperty::small == aProp) {
    mRelativeFontSize--;
    return;
  }

  PRInt32 index;
  if (IsPropSet(aProp, aAttr, NULL, index)) {
    
    mSetArray[index]->value = aValue;
    return;
  }

  
  mSetArray.AppendElement(new PropItem(aProp, aAttr, aValue));

  
  RemovePropFromClearedList(aProp, aAttr);
}


void
TypeInState::ClearAllProps()
{
  
  ClearProp(nsnull, EmptyString());
}

void
TypeInState::ClearProp(nsIAtom* aProp, const nsAString& aAttr)
{
  
  if (IsPropCleared(aProp, aAttr)) {
    return;
  }

  
  PropItem* item = new PropItem(aProp, aAttr, EmptyString());

  
  RemovePropFromSetList(aProp, aAttr);

  
  mClearedArray.AppendElement(item);
}





  
PropItem*
TypeInState::TakeClearProperty()
{
  PRUint32 count = mClearedArray.Length();
  if (!count) {
    return NULL;
  }

  --count; 
  PropItem* propItem = mClearedArray[count];
  mClearedArray.RemoveElementAt(count);
  return propItem;
}




  
PropItem*
TypeInState::TakeSetProperty()
{
  PRUint32 count = mSetArray.Length();
  if (!count) {
    return NULL;
  }
  count--; 
  PropItem* propItem = mSetArray[count];
  mSetArray.RemoveElementAt(count);
  return propItem;
}




PRInt32
TypeInState::TakeRelativeFontSize()
{
  PRInt32 relSize = mRelativeFontSize;
  mRelativeFontSize = 0;
  return relSize;
}

nsresult TypeInState::GetTypingState(bool &isSet, bool &theSetting, nsIAtom *aProp)
{
  return GetTypingState(isSet, theSetting, aProp, EmptyString(), nsnull);
}

nsresult TypeInState::GetTypingState(bool &isSet, 
                                     bool &theSetting, 
                                     nsIAtom *aProp,
                                     const nsString &aAttr, 
                                     nsString *aValue)
{
  if (IsPropSet(aProp, aAttr, aValue))
  {
    isSet = true;
    theSetting = true;
  }
  else if (IsPropCleared(aProp, aAttr))
  {
    isSet = true;
    theSetting = false;
  }
  else
  {
    isSet = false;
  }
  return NS_OK;
}






 
nsresult TypeInState::RemovePropFromSetList(nsIAtom* aProp,
                                            const nsAString& aAttr)
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


nsresult TypeInState::RemovePropFromClearedList(nsIAtom* aProp,
                                                const nsAString& aAttr)
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
                            const nsAString& aAttr,
                            nsAString* outValue)
{
  PRInt32 i;
  return IsPropSet(aProp, aAttr, outValue, i);
}


bool TypeInState::IsPropSet(nsIAtom* aProp,
                            const nsAString& aAttr,
                            nsAString* outValue,
                            PRInt32& outIndex)
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
      return true;
    }
  }
  return false;
}


bool TypeInState::IsPropCleared(nsIAtom* aProp,
                                const nsAString& aAttr)
{
  PRInt32 i;
  return IsPropCleared(aProp, aAttr, i);
}


bool TypeInState::IsPropCleared(nsIAtom* aProp,
                                const nsAString& aAttr,
                                PRInt32& outIndex)
{
  if (FindPropInList(aProp, aAttr, nsnull, mClearedArray, outIndex))
    return true;
  if (FindPropInList(0, EmptyString(), nsnull, mClearedArray, outIndex))
  {
    
    outIndex = -1;
    return true;
  }
  return false;
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
      return true;
    }
  }
  return false;
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
