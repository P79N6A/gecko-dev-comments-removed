











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
    
    const_cast<SelfType*>(this)->EnumerateEntries(KeyEnumerator, &aKeys);
  }

  
  
  typedef PLDHashOperator (* Enumerator)(DataType* aValue, void* aClosure);
  void EnumerateValues(Enumerator aEnumerator, void *aClosure)
  {
    ValueEnumClosure args = { aEnumerator, aClosure };
    this->EnumerateEntries(ValueEnumerator, &args);
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

private:
  static PLDHashOperator
  KeyEnumerator(EntryType* aEntry, void* aClosure)
  {
    nsTArray<nsString>& keys = *static_cast<nsTArray<nsString>*>(aClosure);
    keys.AppendElement(aEntry->GetKey());
    return PL_DHASH_NEXT;
  }

  struct ValueEnumClosure {
    Enumerator mEnumerator;
    void* mClosure;
  };

  static PLDHashOperator
  ValueEnumerator(EntryType* aEntry, void* aClosure)
  {
    ValueEnumClosure* enumClosure = static_cast<ValueEnumClosure*>(aClosure);
    return enumClosure->mEnumerator(&aEntry->mData, enumClosure->mClosure);
  }
};

} 
} 

#endif 
