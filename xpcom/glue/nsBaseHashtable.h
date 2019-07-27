





#ifndef nsBaseHashtable_h__
#define nsBaseHashtable_h__

#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"
#include "nsTHashtable.h"
#include "prlock.h"
#include "nsDebug.h"

template<class KeyClass, class DataType, class UserDataType>
class nsBaseHashtable; 






template<class KeyClass, class DataType>
class nsBaseHashtableET : public KeyClass
{
public:
  DataType mData;
  friend class nsTHashtable<nsBaseHashtableET<KeyClass, DataType>>;

private:
  typedef typename KeyClass::KeyType KeyType;
  typedef typename KeyClass::KeyTypePointer KeyTypePointer;

  explicit nsBaseHashtableET(KeyTypePointer aKey);
  nsBaseHashtableET(nsBaseHashtableET<KeyClass, DataType>&& aToMove);
  ~nsBaseHashtableET();
};













template<class KeyClass, class DataType, class UserDataType>
class nsBaseHashtable
  : protected nsTHashtable<nsBaseHashtableET<KeyClass, DataType>>
{
  typedef mozilla::fallible_t fallible_t;

public:
  typedef typename KeyClass::KeyType KeyType;
  typedef nsBaseHashtableET<KeyClass, DataType> EntryType;

  using nsTHashtable<EntryType>::Contains;

  nsBaseHashtable() {}
  explicit nsBaseHashtable(uint32_t aInitLength)
    : nsTHashtable<EntryType>(aInitLength)
  {
  }

  



  uint32_t Count() const { return nsTHashtable<EntryType>::Count(); }

  








  bool Get(KeyType aKey, UserDataType* aData) const
  {
    EntryType* ent = this->GetEntry(aKey);
    if (!ent) {
      return false;
    }

    if (aData) {
      *aData = ent->mData;
    }

    return true;
  }

  








  UserDataType Get(KeyType aKey) const
  {
    EntryType* ent = this->GetEntry(aKey);
    if (!ent) {
      return 0;
    }

    return ent->mData;
  }

  





  void Put(KeyType aKey, const UserDataType& aData)
  {
    if (!Put(aKey, aData, mozilla::fallible)) {
      NS_ABORT_OOM(this->mTable.EntrySize() * this->mTable.EntryCount());
    }
  }

  MOZ_WARN_UNUSED_RESULT bool Put(KeyType aKey, const UserDataType& aData,
                                  const fallible_t&)
  {
    EntryType* ent = this->PutEntry(aKey);
    if (!ent) {
      return false;
    }

    ent->mData = aData;

    return true;
  }

  



  void Remove(KeyType aKey) { this->RemoveEntry(aKey); }

  








  typedef PLDHashOperator (*EnumReadFunction)(KeyType aKey,
                                              UserDataType aData,
                                              void* aUserArg);

  




  uint32_t EnumerateRead(EnumReadFunction aEnumFunc, void* aUserArg) const
  {
    uint32_t n = 0;
    for (auto iter = this->mTable.Iter(); !iter.Done(); iter.Next()) {
      auto entry = static_cast<EntryType*>(iter.Get());
      PLDHashOperator op = aEnumFunc(entry->GetKey(), entry->mData, aUserArg);
      n++;
      MOZ_ASSERT(!(op & PL_DHASH_REMOVE));
      if (op & PL_DHASH_STOP) {
        break;
      }
    }
    return n;
  }

  










  typedef PLDHashOperator (*EnumFunction)(KeyType aKey,
                                          DataType& aData,
                                          void* aUserArg);

  





  uint32_t Enumerate(EnumFunction aEnumFunc, void* aUserArg)
  {
    uint32_t n = 0;
    for (auto iter = this->mTable.RemovingIter(); !iter.Done(); iter.Next()) {
      auto entry = static_cast<EntryType*>(iter.Get());
      PLDHashOperator op = aEnumFunc(entry->GetKey(), entry->mData, aUserArg);
      n++;
      if (op & PL_DHASH_REMOVE) {
        iter.Remove();
      }
      if (op & PL_DHASH_STOP) {
        break;
      }
    }
    return n;
  }

  


  void Clear() { nsTHashtable<EntryType>::Clear(); }

  








  typedef size_t
    (*SizeOfEntryExcludingThisFun)(KeyType aKey,
                                   const DataType& aData,
                                   mozilla::MallocSizeOf aMallocSizeOf,
                                   void* aUserArg);

  











  size_t SizeOfIncludingThis(SizeOfEntryExcludingThisFun aSizeOfEntryExcludingThis,
                             mozilla::MallocSizeOf aMallocSizeOf,
                             void* aUserArg = nullptr)
  {
    return aMallocSizeOf(this) +
      this->SizeOfExcludingThis(aSizeOfEntryExcludingThis, aMallocSizeOf,
                                aUserArg);
  }

  











  size_t SizeOfExcludingThis(SizeOfEntryExcludingThisFun aSizeOfEntryExcludingThis,
                             mozilla::MallocSizeOf aMallocSizeOf,
                             void* aUserArg = nullptr) const
  {
    if (aSizeOfEntryExcludingThis) {
      s_SizeOfArgs args = { aSizeOfEntryExcludingThis, aUserArg };
      return PL_DHashTableSizeOfExcludingThis(&this->mTable, s_SizeOfStub,
                                              aMallocSizeOf, &args);
    }
    return PL_DHashTableSizeOfExcludingThis(&this->mTable, nullptr,
                                            aMallocSizeOf);
  }

#ifdef DEBUG
  using nsTHashtable<EntryType>::MarkImmutable;
#endif

protected:
  struct s_SizeOfArgs
  {
    SizeOfEntryExcludingThisFun func;
    void* userArg;
  };

  static size_t s_SizeOfStub(PLDHashEntryHdr* aEntry,
                             mozilla::MallocSizeOf aMallocSizeOf,
                             void* aArg);
};

class nsCycleCollectionTraversalCallback;

struct MOZ_STACK_CLASS nsBaseHashtableCCTraversalData
{
  nsBaseHashtableCCTraversalData(nsCycleCollectionTraversalCallback& aCallback,
                                 const char* aName,
                                 uint32_t aFlags)
    : mCallback(aCallback)
    , mName(aName)
    , mFlags(aFlags)
  {
  }

  nsCycleCollectionTraversalCallback& mCallback;
  const char* mName;
  uint32_t mFlags;

};

template<typename K, typename T>
PLDHashOperator
ImplCycleCollectionTraverse_EnumFunc(K aKey,
                                     T aData,
                                     void* aUserData)
{
  nsBaseHashtableCCTraversalData* userData =
    static_cast<nsBaseHashtableCCTraversalData*>(aUserData);

  CycleCollectionNoteChild(userData->mCallback,
                           aData,
                           userData->mName,
                           userData->mFlags);
  return PL_DHASH_NEXT;
}





template<class KeyClass, class DataType>
nsBaseHashtableET<KeyClass, DataType>::nsBaseHashtableET(KeyTypePointer aKey)
  : KeyClass(aKey)
  , mData()
{
}

template<class KeyClass, class DataType>
nsBaseHashtableET<KeyClass, DataType>::nsBaseHashtableET(
      nsBaseHashtableET<KeyClass, DataType>&& aToMove)
  : KeyClass(mozilla::Move(aToMove))
  , mData(mozilla::Move(aToMove.mData))
{
}

template<class KeyClass, class DataType>
nsBaseHashtableET<KeyClass, DataType>::~nsBaseHashtableET()
{
}






template<class KeyClass, class DataType, class UserDataType>
size_t
nsBaseHashtable<KeyClass, DataType, UserDataType>::s_SizeOfStub(
    PLDHashEntryHdr* aHdr, mozilla::MallocSizeOf aMallocSizeOf, void* aArg)
{
  EntryType* ent = static_cast<EntryType*>(aHdr);
  s_SizeOfArgs* eargs = static_cast<s_SizeOfArgs*>(aArg);

  return (eargs->func)(ent->GetKey(), ent->mData, aMallocSizeOf, eargs->userArg);
}

#endif 
