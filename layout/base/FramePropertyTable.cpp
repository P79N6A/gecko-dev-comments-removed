




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
      
      entry->mProp.DestroyValueFor(aFrame);
      entry->mProp.mValue = aValue;
      return;
    }

    
    PropertyValue current = entry->mProp;
    entry->mProp.mProperty = nsnull;
    MOZ_STATIC_ASSERT(sizeof(nsTArray<PropertyValue>) <= sizeof(void *),
                      "Property array must fit entirely within entry->mProp.mValue");
    new (&entry->mProp.mValue) nsTArray<PropertyValue>(4);
    entry->mProp.ToArray()->AppendElement(current);
  }

  nsTArray<PropertyValue>* array = entry->mProp.ToArray();
  nsTArray<PropertyValue>::index_type index =
    array->IndexOf(aProperty, 0, PropertyComparator());
  if (index != nsTArray<PropertyValue>::NoIndex) {
    PropertyValue* pv = &array->ElementAt(index);
    pv->DestroyValueFor(aFrame);
    pv->mValue = aValue;
    return;
  }

  array->AppendElement(PropertyValue(aProperty, aValue));
}

void*
FramePropertyTable::Get(const nsIFrame* aFrame,
                        const FramePropertyDescriptor* aProperty,
                        bool* aFoundResult)
{
  NS_ASSERTION(aFrame, "Null frame?");
  NS_ASSERTION(aProperty, "Null property?");

  if (aFoundResult) {
    *aFoundResult = false;
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
      *aFoundResult = true;
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
    *aFoundResult = true;
  }

  return array->ElementAt(index).mValue;
}

void*
FramePropertyTable::Remove(nsIFrame* aFrame, const FramePropertyDescriptor* aProperty,
                           bool* aFoundResult)
{
  NS_ASSERTION(aFrame, "Null frame?");
  NS_ASSERTION(aProperty, "Null property?");

  if (aFoundResult) {
    *aFoundResult = false;
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
      *aFoundResult = true;
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
    *aFoundResult = true;
  }

  void* result = array->ElementAt(index).mValue;

  PRUint32 last = array->Length() - 1;
  array->ElementAt(index) = array->ElementAt(last);
  array->RemoveElementAt(last);

  if (last == 1) {
    PropertyValue pv = array->ElementAt(0);
    array->~nsTArray<PropertyValue>();
    entry->mProp = pv;
  }
  
  return result;
}

void
FramePropertyTable::Delete(nsIFrame* aFrame, const FramePropertyDescriptor* aProperty)
{
  NS_ASSERTION(aFrame, "Null frame?");
  NS_ASSERTION(aProperty, "Null property?");

  bool found;
  void* v = Remove(aFrame, aProperty, &found);
  if (found) {
    PropertyValue pv(aProperty, v);
    pv.DestroyValueFor(aFrame);
  }
}

 void
FramePropertyTable::DeleteAllForEntry(Entry* aEntry)
{
  if (!aEntry->mProp.IsArray()) {
    aEntry->mProp.DestroyValueFor(aEntry->GetKey());
    return;
  }

  nsTArray<PropertyValue>* array = aEntry->mProp.ToArray();
  for (PRUint32 i = 0; i < array->Length(); ++i) {
    array->ElementAt(i).DestroyValueFor(aEntry->GetKey());
  }
  array->~nsTArray<PropertyValue>();
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

size_t
FramePropertyTable::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  return mEntries.SizeOfExcludingThis(SizeOfPropertyTableEntryExcludingThis,
                                      aMallocSizeOf);
}

 size_t
FramePropertyTable::SizeOfPropertyTableEntryExcludingThis(Entry* aEntry,
                      nsMallocSizeOfFun aMallocSizeOf, void *)
{
  return aEntry->mProp.SizeOfExcludingThis(aMallocSizeOf);
}

}
