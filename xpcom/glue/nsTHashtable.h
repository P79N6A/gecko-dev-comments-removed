





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


PLDHashOperator PL_DHashStubEnumRemove(PLDHashTable* aTable,
                                       PLDHashEntryHdr* aEntry,
                                       uint32_t aOrdinal,
                                       void* aUserArg);






















































template<class EntryType>
class nsTHashtable
{
  typedef mozilla::fallible_t fallible_t;

public:
  
  
  nsTHashtable() { Init(PL_DHASH_DEFAULT_INITIAL_LENGTH); }
  explicit nsTHashtable(uint32_t aInitLength) { Init(aInitLength); }

  


  ~nsTHashtable();

  nsTHashtable(nsTHashtable<EntryType>&& aOther);

  



  uint32_t GetGeneration() const { return mTable.Generation(); }

  


  typedef typename EntryType::KeyType KeyType;

  


  typedef typename EntryType::KeyTypePointer KeyTypePointer;

  



  uint32_t Count() const { return mTable.EntryCount(); }

  





  EntryType* GetEntry(KeyType aKey) const
  {
    NS_ASSERTION(mTable.IsInitialized(),
                 "nsTHashtable was not initialized properly.");

    return static_cast<EntryType*>(
      PL_DHashTableSearch(const_cast<PLDHashTable*>(&mTable),
                          EntryType::KeyToPointer(aKey)));
  }

  




  bool Contains(KeyType aKey) const { return !!GetEntry(aKey); }

  





  EntryType* PutEntry(KeyType aKey)
  {
    NS_ASSERTION(mTable.IsInitialized(),
                 "nsTHashtable was not initialized properly.");

    return static_cast<EntryType*>  
      (PL_DHashTableAdd(&mTable, EntryType::KeyToPointer(aKey)));
  }

  MOZ_WARN_UNUSED_RESULT
  EntryType* PutEntry(KeyType aKey, const fallible_t&)
  {
    NS_ASSERTION(mTable.IsInitialized(),
                 "nsTHashtable was not initialized properly.");

    return static_cast<EntryType*>
      (PL_DHashTableAdd(&mTable, EntryType::KeyToPointer(aKey),
                        mozilla::fallible));
  }

  



  void RemoveEntry(KeyType aKey)
  {
    NS_ASSERTION(mTable.IsInitialized(),
                 "nsTHashtable was not initialized properly.");

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
    NS_ASSERTION(mTable.IsInitialized(),
                 "nsTHashtable was not initialized properly.");

    s_EnumArgs args = { aEnumFunc, aUserArg };
    return PL_DHashTableEnumerate(&mTable, s_EnumStub, &args);
  }

  


  void Clear()
  {
    NS_ASSERTION(mTable.IsInitialized(),
                 "nsTHashtable was not initialized properly.");

    PL_DHashTableEnumerate(&mTable, PL_DHashStubEnumRemove, nullptr);
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
    NS_ASSERTION(mTable.IsInitialized(),
                 "nsTHashtable was not initialized properly.");

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

  






  struct s_EnumArgs
  {
    Enumerator userFunc;
    void* userArg;
  };

  static PLDHashOperator s_EnumStub(PLDHashTable* aTable,
                                    PLDHashEntryHdr* aEntry,
                                    uint32_t aNumber, void* aArg);

  






  struct s_SizeOfArgs
  {
    SizeOfEntryExcludingThisFun userFunc;
    void* userArg;
  };

  static size_t s_SizeOfStub(PLDHashEntryHdr* aEntry,
                             mozilla::MallocSizeOf aMallocSizeOf, void* aArg);

private:
  
  nsTHashtable(nsTHashtable<EntryType>& aToCopy) = delete;

  



  void Init(uint32_t aInitLength);

  



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

  
  
  aOther.mTable.SetOps(nullptr);
}

template<class EntryType>
nsTHashtable<EntryType>::~nsTHashtable()
{
  if (mTable.IsInitialized()) {
    PL_DHashTableFinish(&mTable);
  }
}

template<class EntryType>
void
nsTHashtable<EntryType>::Init(uint32_t aInitLength)
{
  static const PLDHashTableOps sOps =
  {
    s_HashKey,
    s_MatchEntry,
    EntryType::ALLOW_MEMMOVE ? ::PL_DHashMoveEntryStub : s_CopyEntry,
    s_ClearEntry,
    s_InitEntry
  };

  PL_DHashTableInit(&mTable, &sOps, sizeof(EntryType), aInitLength);
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
PLDHashOperator
nsTHashtable<EntryType>::s_EnumStub(PLDHashTable* aTable,
                                    PLDHashEntryHdr* aEntry,
                                    uint32_t aNumber,
                                    void* aArg)
{
  
  return (*reinterpret_cast<s_EnumArgs*>(aArg)->userFunc)(
    static_cast<EntryType*>(aEntry),
    reinterpret_cast<s_EnumArgs*>(aArg)->userArg);
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
