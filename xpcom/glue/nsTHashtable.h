





#ifndef nsTHashtable_h__
#define nsTHashtable_h__

#include "nscore.h"
#include "pldhash.h"
#include "nsDebug.h"
#include "mozilla/Assertions.h"
#include "mozilla/MemoryChecking.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"
#include "mozilla/fallible.h"
#include "mozilla/PodOperations.h"

#include <new>
























































enum PLDHashOperator
{
  PL_DHASH_NEXT = 0,          
  PL_DHASH_STOP = 1,          
  PL_DHASH_REMOVE = 2         
};

template<class EntryType>
class nsTHashtable
{
  typedef mozilla::fallible_t fallible_t;

public:
  
  
  nsTHashtable()
    : mTable(Ops(), sizeof(EntryType), PL_DHASH_DEFAULT_INITIAL_LENGTH)
  {}
  explicit nsTHashtable(uint32_t aInitLength)
    : mTable(Ops(), sizeof(EntryType), aInitLength)
  {}

  


  ~nsTHashtable();

  nsTHashtable(nsTHashtable<EntryType>&& aOther);

  



  uint32_t GetGeneration() const { return mTable.Generation(); }

  


  typedef typename EntryType::KeyType KeyType;

  


  typedef typename EntryType::KeyTypePointer KeyTypePointer;

  



  uint32_t Count() const { return mTable.EntryCount(); }

  





  EntryType* GetEntry(KeyType aKey) const
  {
    return static_cast<EntryType*>(
      PL_DHashTableSearch(const_cast<PLDHashTable*>(&mTable),
                          EntryType::KeyToPointer(aKey)));
  }

  




  bool Contains(KeyType aKey) const { return !!GetEntry(aKey); }

  





  EntryType* PutEntry(KeyType aKey)
  {
    return static_cast<EntryType*>  
      (PL_DHashTableAdd(&mTable, EntryType::KeyToPointer(aKey)));
  }

  MOZ_WARN_UNUSED_RESULT
  EntryType* PutEntry(KeyType aKey, const fallible_t&)
  {
    return static_cast<EntryType*>
      (PL_DHashTableAdd(&mTable, EntryType::KeyToPointer(aKey),
                        mozilla::fallible));
  }

  



  void RemoveEntry(KeyType aKey)
  {
    PL_DHashTableRemove(&mTable,
                        EntryType::KeyToPointer(aKey));
  }

  







  void RawRemoveEntry(EntryType* aEntry)
  {
    PL_DHashTableRawRemove(&mTable, aEntry);
  }

  









  typedef PLDHashOperator (*Enumerator)(EntryType* aEntry, void* userArg);

  









  uint32_t EnumerateEntries(Enumerator aEnumFunc, void* aUserArg)
  {
    uint32_t n = 0;
    for (auto iter = mTable.RemovingIter(); !iter.Done(); iter.Next()) {
      auto entry = static_cast<EntryType*>(iter.Get());
      PLDHashOperator op = aEnumFunc(entry, aUserArg);
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

  




  void Clear()
  {
    mTable.Clear();
  }

  







  typedef size_t (*SizeOfEntryExcludingThisFun)(EntryType* aEntry,
                                                mozilla::MallocSizeOf aMallocSizeOf,
                                                void* aArg);

  











  size_t SizeOfExcludingThis(SizeOfEntryExcludingThisFun aSizeOfEntryExcludingThis,
                             mozilla::MallocSizeOf aMallocSizeOf,
                             void* aUserArg = nullptr) const
  {
    if (aSizeOfEntryExcludingThis) {
      s_SizeOfArgs args = { aSizeOfEntryExcludingThis, aUserArg };
      return PL_DHashTableSizeOfExcludingThis(&mTable, s_SizeOfStub,
                                              aMallocSizeOf, &args);
    }
    return PL_DHashTableSizeOfExcludingThis(&mTable, nullptr, aMallocSizeOf);
  }

  



  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return SizeOfExcludingThis(BasicSizeOfEntryExcludingThisFun, aMallocSizeOf);
  }

  


  size_t SizeOfIncludingThis(SizeOfEntryExcludingThisFun aSizeOfEntryExcludingThis,
                             mozilla::MallocSizeOf aMallocSizeOf,
                             void* aUserArg = nullptr) const
  {
    return aMallocSizeOf(this) +
      SizeOfExcludingThis(aSizeOfEntryExcludingThis, aMallocSizeOf, aUserArg);
  }

  



  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return SizeOfIncludingThis(BasicSizeOfEntryExcludingThisFun, aMallocSizeOf);
  }

  


  void SwapElements(nsTHashtable<EntryType>& aOther)
  {
    MOZ_ASSERT_IF(this->mTable.Ops() && aOther.mTable.Ops(),
                  this->mTable.Ops() == aOther.mTable.Ops());
    mozilla::Swap(this->mTable, aOther.mTable);
  }

#ifdef DEBUG
  





  void MarkImmutable()
  {
    PL_DHashMarkTableImmutable(&mTable);
  }
#endif

protected:
  PLDHashTable mTable;

  static const void* s_GetKey(PLDHashTable* aTable, PLDHashEntryHdr* aEntry);

  static PLDHashNumber s_HashKey(PLDHashTable* aTable, const void* aKey);

  static bool s_MatchEntry(PLDHashTable* aTable, const PLDHashEntryHdr* aEntry,
                           const void* aKey);

  static void s_CopyEntry(PLDHashTable* aTable, const PLDHashEntryHdr* aFrom,
                          PLDHashEntryHdr* aTo);

  static void s_ClearEntry(PLDHashTable* aTable, PLDHashEntryHdr* aEntry);

  static void s_InitEntry(PLDHashEntryHdr* aEntry, const void* aKey);

  






  struct s_SizeOfArgs
  {
    SizeOfEntryExcludingThisFun userFunc;
    void* userArg;
  };

  static size_t s_SizeOfStub(PLDHashEntryHdr* aEntry,
                             mozilla::MallocSizeOf aMallocSizeOf, void* aArg);

private:
  
  nsTHashtable(nsTHashtable<EntryType>& aToCopy) = delete;

  


  static const PLDHashTableOps* Ops();

  



  static size_t BasicSizeOfEntryExcludingThisFun(EntryType* aEntry,
                                                 mozilla::MallocSizeOf aMallocSizeOf,
                                                 void*);

  
  nsTHashtable<EntryType>& operator=(nsTHashtable<EntryType>& aToEqual) = delete;
};





template<class EntryType>
nsTHashtable<EntryType>::nsTHashtable(nsTHashtable<EntryType>&& aOther)
  : mTable(mozilla::Move(aOther.mTable))
{
  
  
  MOZ_MAKE_MEM_UNDEFINED(&aOther.mTable, sizeof(aOther.mTable));
}

template<class EntryType>
nsTHashtable<EntryType>::~nsTHashtable()
{
}

template<class EntryType>
 const PLDHashTableOps*
nsTHashtable<EntryType>::Ops()
{
  
  
  
  static const PLDHashTableOps sOps =
  {
    s_HashKey,
    s_MatchEntry,
    EntryType::ALLOW_MEMMOVE ? ::PL_DHashMoveEntryStub : s_CopyEntry,
    s_ClearEntry,
    s_InitEntry
  };
  return &sOps;
}


template<class EntryType>
size_t
nsTHashtable<EntryType>::BasicSizeOfEntryExcludingThisFun(EntryType* aEntry,
                                                          mozilla::MallocSizeOf aMallocSizeOf,
                                                          void*)
{
  return aEntry->SizeOfExcludingThis(aMallocSizeOf);
}



template<class EntryType>
PLDHashNumber
nsTHashtable<EntryType>::s_HashKey(PLDHashTable* aTable, const void* aKey)
{
  return EntryType::HashKey(reinterpret_cast<const KeyTypePointer>(aKey));
}

template<class EntryType>
bool
nsTHashtable<EntryType>::s_MatchEntry(PLDHashTable* aTable,
                                      const PLDHashEntryHdr* aEntry,
                                      const void* aKey)
{
  return ((const EntryType*)aEntry)->KeyEquals(
    reinterpret_cast<const KeyTypePointer>(aKey));
}

template<class EntryType>
void
nsTHashtable<EntryType>::s_CopyEntry(PLDHashTable* aTable,
                                     const PLDHashEntryHdr* aFrom,
                                     PLDHashEntryHdr* aTo)
{
  EntryType* fromEntry =
    const_cast<EntryType*>(reinterpret_cast<const EntryType*>(aFrom));

  new (aTo) EntryType(mozilla::Move(*fromEntry));

  fromEntry->~EntryType();
}

template<class EntryType>
void
nsTHashtable<EntryType>::s_ClearEntry(PLDHashTable* aTable,
                                      PLDHashEntryHdr* aEntry)
{
  static_cast<EntryType*>(aEntry)->~EntryType();
}

template<class EntryType>
void
nsTHashtable<EntryType>::s_InitEntry(PLDHashEntryHdr* aEntry,
                                     const void* aKey)
{
  new (aEntry) EntryType(reinterpret_cast<KeyTypePointer>(aKey));
}

template<class EntryType>
size_t
nsTHashtable<EntryType>::s_SizeOfStub(PLDHashEntryHdr* aEntry,
                                      mozilla::MallocSizeOf aMallocSizeOf,
                                      void* aArg)
{
  
  return (*reinterpret_cast<s_SizeOfArgs*>(aArg)->userFunc)(
    static_cast<EntryType*>(aEntry),
    aMallocSizeOf,
    reinterpret_cast<s_SizeOfArgs*>(aArg)->userArg);
}

class nsCycleCollectionTraversalCallback;

struct MOZ_STACK_CLASS nsTHashtableCCTraversalData
{
  nsTHashtableCCTraversalData(nsCycleCollectionTraversalCallback& aCallback,
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

template<class EntryType>
PLDHashOperator
ImplCycleCollectionTraverse_EnumFunc(EntryType* aEntry, void* aUserData)
{
  auto userData = static_cast<nsTHashtableCCTraversalData*>(aUserData);

  ImplCycleCollectionTraverse(userData->mCallback,
                              *aEntry,
                              userData->mName,
                              userData->mFlags);
  return PL_DHASH_NEXT;
}

template<class EntryType>
inline void
ImplCycleCollectionUnlink(nsTHashtable<EntryType>& aField)
{
  aField.Clear();
}

template<class EntryType>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            nsTHashtable<EntryType>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  nsTHashtableCCTraversalData userData(aCallback, aName, aFlags);

  aField.EnumerateEntries(ImplCycleCollectionTraverse_EnumFunc<EntryType>,
                          &userData);
}

#endif 
