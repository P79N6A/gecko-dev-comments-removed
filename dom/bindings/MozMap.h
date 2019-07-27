











#ifndef mozilla_dom_MozMap_h
#define mozilla_dom_MozMap_h

#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "mozilla/Attributes.h"
#include "mozilla/Move.h"

namespace mozilla {
namespace dom {

namespace binding_detail {
template<typename DataType>
class MozMapEntry : public nsStringHashKey
{
public:
  explicit MozMapEntry(const nsAString* aKeyTypePointer)
    : nsStringHashKey(aKeyTypePointer)
  {
  }

  
  MozMapEntry(MozMapEntry<DataType>&& aOther)
    : nsStringHashKey(aOther),
      mData(Move(aOther.mData))
  {
  }

  DataType mData;
};

} 

template<typename DataType>
class MozMap : protected nsTHashtable<binding_detail::MozMapEntry<DataType>>
{
public:
  typedef typename binding_detail::MozMapEntry<DataType> EntryType;
  typedef nsTHashtable<EntryType> Base;
  typedef MozMap<DataType> SelfType;

  MozMap()
  {
  }

  
  MozMap(SelfType&& aOther) :
    Base(Move(aOther))
  {
  }

  
  const DataType& Get(const nsAString& aKey) const
  {
    const EntryType* ent = this->GetEntry(aKey);
    MOZ_ASSERT(ent, "Why are you using a key we didn't claim to have?");
    return ent->mData;
  }

  DataType& Get(const nsAString& aKey)
  {
    EntryType* ent = this->GetEntry(aKey);
    MOZ_ASSERT(ent, "Why are you using a key we didn't claim to have?");
    return ent->mData;
  }

  
  const DataType* GetIfExists(const nsAString& aKey) const
  {
    const EntryType* ent = this->GetEntry(aKey);
    if (!ent) {
      return nullptr;
    }
    return &ent->mData;
  }

  void GetKeys(nsTArray<nsString>& aKeys) const {
    for (auto iter = this->ConstIter(); !iter.Done(); iter.Next()) {
      aKeys.AppendElement(iter.Get()->GetKey());
    }
  }

  
  
  typedef void (* Enumerator)(DataType* aValue, void* aClosure);
  void EnumerateValues(Enumerator aEnumerator, void *aClosure)
  {
    for (auto iter = this->Iter(); !iter.Done(); iter.Next()) {
      aEnumerator(&iter.Get()->mData, aClosure);
    }
  }

  MOZ_WARN_UNUSED_RESULT
  DataType* AddEntry(const nsAString& aKey)
  {
    EntryType* ent = this->PutEntry(aKey, fallible);
    if (!ent) {
      return nullptr;
    }
    return &ent->mData;
  }
};

} 
} 

#endif 
