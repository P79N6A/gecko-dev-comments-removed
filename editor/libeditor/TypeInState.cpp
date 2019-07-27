





#include <stddef.h>

#include "TypeInState.h"
#include "mozilla/mozalloc.h"
#include "nsAString.h"
#include "nsDebug.h"
#include "nsEditor.h"
#include "nsError.h"
#include "nsGkAtoms.h"
#include "nsIDOMNode.h"
#include "nsISelection.h"
#include "nsISupportsBase.h"
#include "nsISupportsImpl.h"
#include "nsReadableUtils.h"
#include "nsStringFwd.h"

class nsIAtom;
class nsIDOMDocument;





NS_IMPL_CYCLE_COLLECTION(TypeInState, mLastSelectionContainer)
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


NS_IMETHODIMP TypeInState::NotifySelectionChanged(nsIDOMDocument *, nsISelection *aSelection, int16_t)
{
  
  
  
  
  
  
  
  
  

  if (aSelection) {
    int32_t rangeCount = 0;
    nsresult result = aSelection->GetRangeCount(&rangeCount);
    NS_ENSURE_SUCCESS(result, result);

    if (aSelection->Collapsed() && rangeCount) {
      nsCOMPtr<nsIDOMNode> selNode;
      int32_t selOffset = 0;

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
      mLastSelectionContainer = nullptr;
      mLastSelectionOffset = 0;
    }
  }

  Reset(); 
  return NS_OK;
}

void TypeInState::Reset()
{
  for(uint32_t i = 0, n = mClearedArray.Length(); i < n; i++) {
    delete mClearedArray[i];
  }
  mClearedArray.Clear();
  for(uint32_t i = 0, n = mSetArray.Length(); i < n; i++) {
    delete mSetArray[i];
  }
  mSetArray.Clear();
}


void
TypeInState::SetProp(nsIAtom* aProp, const nsAString& aAttr,
                     const nsAString& aValue)
{
  
  if (nsGkAtoms::big == aProp) {
    mRelativeFontSize++;
    return;
  }
  if (nsGkAtoms::small == aProp) {
    mRelativeFontSize--;
    return;
  }

  int32_t index;
  if (IsPropSet(aProp, aAttr, nullptr, index)) {
    
    mSetArray[index]->value = aValue;
    return;
  }

  
  mSetArray.AppendElement(new PropItem(aProp, aAttr, aValue));

  
  RemovePropFromClearedList(aProp, aAttr);
}


void
TypeInState::ClearAllProps()
{
  
  ClearProp(nullptr, EmptyString());
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
  uint32_t count = mClearedArray.Length();
  if (!count) {
    return nullptr;
  }

  --count; 
  PropItem* propItem = mClearedArray[count];
  mClearedArray.RemoveElementAt(count);
  return propItem;
}




  
PropItem*
TypeInState::TakeSetProperty()
{
  uint32_t count = mSetArray.Length();
  if (!count) {
    return nullptr;
  }
  count--; 
  PropItem* propItem = mSetArray[count];
  mSetArray.RemoveElementAt(count);
  return propItem;
}




int32_t
TypeInState::TakeRelativeFontSize()
{
  int32_t relSize = mRelativeFontSize;
  mRelativeFontSize = 0;
  return relSize;
}

void
TypeInState::GetTypingState(bool &isSet, bool &theSetting, nsIAtom *aProp)
{
  GetTypingState(isSet, theSetting, aProp, EmptyString(), nullptr);
}

void
TypeInState::GetTypingState(bool &isSet,
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
}






 
void
TypeInState::RemovePropFromSetList(nsIAtom* aProp, const nsAString& aAttr)
{
  int32_t index;
  if (!aProp)
  {
    
    for(uint32_t i = 0, n = mSetArray.Length(); i < n; i++) {
      delete mSetArray[i];
    }
    mSetArray.Clear();
    mRelativeFontSize=0;
  }
  else if (FindPropInList(aProp, aAttr, nullptr, mSetArray, index))
  {
    delete mSetArray[index];
    mSetArray.RemoveElementAt(index);
  }
}


void
TypeInState::RemovePropFromClearedList(nsIAtom* aProp, const nsAString& aAttr)
{
  int32_t index;
  if (FindPropInList(aProp, aAttr, nullptr, mClearedArray, index))
  {
    delete mClearedArray[index];
    mClearedArray.RemoveElementAt(index);
  }
}


bool TypeInState::IsPropSet(nsIAtom *aProp,
                            const nsAString& aAttr,
                            nsAString* outValue)
{
  int32_t i;
  return IsPropSet(aProp, aAttr, outValue, i);
}


bool TypeInState::IsPropSet(nsIAtom* aProp,
                            const nsAString& aAttr,
                            nsAString* outValue,
                            int32_t& outIndex)
{
  
  uint32_t i, count = mSetArray.Length();
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
  int32_t i;
  return IsPropCleared(aProp, aAttr, i);
}


bool TypeInState::IsPropCleared(nsIAtom* aProp,
                                const nsAString& aAttr,
                                int32_t& outIndex)
{
  if (FindPropInList(aProp, aAttr, nullptr, mClearedArray, outIndex))
    return true;
  if (FindPropInList(0, EmptyString(), nullptr, mClearedArray, outIndex))
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
                                   int32_t &outIndex)
{
  
  uint32_t i, count = aList.Length();
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
 tag(nullptr)
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
