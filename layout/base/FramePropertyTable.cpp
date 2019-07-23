




































#include "FramePropertyTable.h"
#include "prlog.h"

namespace mozilla {

void
FramePropertyTable::Set(nsIFrame* aFrame, const FramePropertyDescriptor* aProperty,
                        void* aValue)
{
  NS_ASSERTION(aFrame, "Null frame?");
  NS_ASSERTION(aProperty, "Null property?");

  if (mLastFrame != aFrame || !mLastEntry) {
    mLastFrame = aFrame;
    mLastEntry = mEntries.PutEntry(aFrame);
  }
  Entry* entry = mLastEntry;

  if (!entry->mProp.IsArray()) {
    if (!entry->mProp.mProperty) {
      
      entry->mProp.mProperty = aProperty;
      entry->mProp.mValue = aValue;
      return;
    }
    if (entry->mProp.mProperty == aProperty) {
      
      entry->mProp.DestroyValue();
      entry->mProp.mValue = aValue;
      return;
    }

    
    PropertyValue current = entry->mProp;
    entry->mProp.mProperty = nsnull;
    PR_STATIC_ASSERT(sizeof(nsTArray<PropertyValue>) <= sizeof(void *));
    new (&entry->mProp.mValue) nsTArray<PropertyValue>(4);
    entry->mProp.ToArray()->AppendElement(current);
  }

  nsTArray<PropertyValue>* array = entry->mProp.ToArray();
  nsTArray<PropertyValue>::index_type index =
    array->IndexOf(aProperty, 0, PropertyComparator());
  if (index != nsTArray<PropertyValue>::NoIndex) {
    PropertyValue* pv = &array->ElementAt(index);
    pv->DestroyValue();
    pv->mValue = aValue;
    return;
  }

  array->AppendElement(PropertyValue(aProperty, aValue));
}

void*
FramePropertyTable::Get(const nsIFrame* aFrame,
                        const FramePropertyDescriptor* aProperty,
                        PRBool* aFoundResult)
{
  NS_ASSERTION(aFrame, "Null frame?");
  NS_ASSERTION(aProperty, "Null property?");

  if (aFoundResult) {
    *aFoundResult = PR_FALSE;
  }

  if (mLastFrame != aFrame) {
    mLastFrame = const_cast<nsIFrame*>(aFrame);
    mLastEntry = mEntries.GetEntry(mLastFrame);
  }
  Entry* entry = mLastEntry;
  if (!entry)
    return nsnull;

  if (entry->mProp.mProperty == aProperty) {
    if (aFoundResult) {
      *aFoundResult = PR_TRUE;
    }
    return entry->mProp.mValue;
  }
  if (!entry->mProp.IsArray()) {
    
    return nsnull;
  }

  nsTArray<PropertyValue>* array = entry->mProp.ToArray();
  nsTArray<PropertyValue>::index_type index =
    array->IndexOf(aProperty, 0, PropertyComparator());
  if (index == nsTArray<PropertyValue>::NoIndex)
    return nsnull;

  if (aFoundResult) {
    *aFoundResult = PR_TRUE;
  }

  return array->ElementAt(index).mValue;
}

void*
FramePropertyTable::Remove(nsIFrame* aFrame, const FramePropertyDescriptor* aProperty,
                           PRBool* aFoundResult)
{
  NS_ASSERTION(aFrame, "Null frame?");
  NS_ASSERTION(aProperty, "Null property?");

  if (aFoundResult) {
    *aFoundResult = PR_FALSE;
  }

  if (mLastFrame != aFrame) {
    mLastFrame = aFrame;
    mLastEntry = mEntries.GetEntry(aFrame);
  }
  Entry* entry = mLastEntry;
  if (!entry)
    return nsnull;

  if (entry->mProp.mProperty == aProperty) {
    
    void* value = entry->mProp.mValue;
    mEntries.RawRemoveEntry(entry);
    mLastEntry = nsnull;
    if (aFoundResult) {
      *aFoundResult = PR_TRUE;
    }
    return value;
  }
  if (!entry->mProp.IsArray()) {
    
    return nsnull;
  }

  nsTArray<PropertyValue>* array = entry->mProp.ToArray();
  nsTArray<PropertyValue>::index_type index =
    array->IndexOf(aProperty, 0, PropertyComparator());
  if (index == nsTArray<PropertyValue>::NoIndex) {
    
    return nsnull;
  }

  if (aFoundResult) {
    *aFoundResult = PR_TRUE;
  }

  void* result = array->ElementAt(index).mValue;

  PRUint32 last = array->Length() - 1;
  array->ElementAt(index) = array->ElementAt(last);
  array->RemoveElementAt(last);

  if (last == 1) {
    PropertyValue pv = array->ElementAt(0);
    array->nsTArray<PropertyValue>::~nsTArray<PropertyValue>();
    entry->mProp = pv;
  }
  
  return result;
}

void
FramePropertyTable::Delete(nsIFrame* aFrame, const FramePropertyDescriptor* aProperty)
{
  NS_ASSERTION(aFrame, "Null frame?");
  NS_ASSERTION(aProperty, "Null property?");

  PRBool found;
  void* v = Remove(aFrame, aProperty, &found);
  if (found && aProperty->mDestructor) {
    aProperty->mDestructor(v);
  }
}

 void
FramePropertyTable::DeleteAllForEntry(Entry* aEntry)
{
  if (!aEntry->mProp.IsArray()) {
    aEntry->mProp.DestroyValue();
    return;
  }

  nsTArray<PropertyValue>* array = aEntry->mProp.ToArray();
  for (PRUint32 i = 0; i < array->Length(); ++i) {
    array->ElementAt(i).DestroyValue();
  }
  array->nsTArray<PropertyValue>::~nsTArray<PropertyValue>();
}

void
FramePropertyTable::DeleteAllFor(nsIFrame* aFrame)
{
  NS_ASSERTION(aFrame, "Null frame?");

  Entry* entry = mEntries.GetEntry(aFrame);
  if (!entry)
    return;

  if (mLastFrame == aFrame) {
    
    
    mLastFrame = nsnull;
    mLastEntry = nsnull;
  }

  DeleteAllForEntry(entry);
  mEntries.RawRemoveEntry(entry);
}

 PLDHashOperator
FramePropertyTable::DeleteEnumerator(Entry* aEntry, void* aArg)
{
  DeleteAllForEntry(aEntry);
  return PL_DHASH_REMOVE;
}

void
FramePropertyTable::DeleteAll()
{
  mLastFrame = nsnull;
  mLastEntry = nsnull;

  mEntries.EnumerateEntries(DeleteEnumerator, nsnull);
}

}
