




#ifndef nsTHashtable_h__
#define nsTHashtable_h__

#include "nscore.h"
#include "pldhash.h"
#include "nsDebug.h"
#include "mozilla/MemoryChecking.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"
#include "mozilla/fallible.h"
#include "mozilla/PodOperations.h"

#include <new>


NS_COM_GLUE PLDHashOperator
PL_DHashStubEnumRemove(PLDHashTable    *table,
                       PLDHashEntryHdr *entry,
                       uint32_t         ordinal,
                       void            *userArg);






















































template<class EntryType>
class nsTHashtable
{
  typedef mozilla::fallible_t fallible_t;

public:
  
  
  nsTHashtable()
  {
    Init(PL_DHASH_MIN_SIZE);
  }
  explicit nsTHashtable(uint32_t aInitSize)
  {
    Init(aInitSize);
  }

  


  ~nsTHashtable();

  nsTHashtable(nsTHashtable<EntryType>&& aOther);

  



  uint32_t GetGeneration() const { return mTable.generation; }

  


  typedef typename EntryType::KeyType KeyType;

  


  typedef typename EntryType::KeyTypePointer KeyTypePointer;

  



  uint32_t Count() const { return mTable.entryCount; }

  





  EntryType* GetEntry(KeyType aKey) const
  {
    NS_ASSERTION(mTable.entrySize, "nsTHashtable was not initialized properly.");
  
    EntryType* entry =
      reinterpret_cast<EntryType*>
                      (PL_DHashTableOperate(
                            const_cast<PLDHashTable*>(&mTable),
                            EntryType::KeyToPointer(aKey),
                            PL_DHASH_LOOKUP));
    return PL_DHASH_ENTRY_IS_BUSY(entry) ? entry : nullptr;
  }

  




  bool Contains(KeyType aKey) const
  {
    return !!GetEntry(aKey);
  }

  





  EntryType* PutEntry(KeyType aKey)
  {
    EntryType* e = PutEntry(aKey, fallible_t());
    if (!e)
      NS_ABORT_OOM(mTable.entrySize * mTable.entryCount);
    return e;
  }

  EntryType* PutEntry(KeyType aKey, const fallible_t&) NS_WARN_UNUSED_RESULT
  {
    NS_ASSERTION(mTable.entrySize, "nsTHashtable was not initialized properly.");

    return static_cast<EntryType*>
                      (PL_DHashTableOperate(
                            &mTable,
                            EntryType::KeyToPointer(aKey),
                            PL_DHASH_ADD));
  }

  



  void RemoveEntry(KeyType aKey)
  {
    NS_ASSERTION(mTable.entrySize, "nsTHashtable was not initialized properly.");

    PL_DHashTableOperate(&mTable,
                         EntryType::KeyToPointer(aKey),
                         PL_DHASH_REMOVE);
  }

  







  void RawRemoveEntry(EntryType* aEntry)
  {
    PL_DHashTableRawRemove(&mTable, aEntry);
  }

  









  typedef PLDHashOperator (* Enumerator)(EntryType* aEntry, void* userArg);

  






  uint32_t EnumerateEntries(Enumerator enumFunc, void* userArg)
  {
    NS_ASSERTION(mTable.entrySize, "nsTHashtable was not initialized properly.");
    
    s_EnumArgs args = { enumFunc, userArg };
    return PL_DHashTableEnumerate(&mTable, s_EnumStub, &args);
  }

  


  void Clear()
  {
    NS_ASSERTION(mTable.entrySize, "nsTHashtable was not initialized properly.");

    PL_DHashTableEnumerate(&mTable, PL_DHashStubEnumRemove, nullptr);
  }

  







  typedef size_t (* SizeOfEntryExcludingThisFun)(EntryType* aEntry,
                                                 mozilla::MallocSizeOf mallocSizeOf,
                                                 void *arg);

  











  size_t SizeOfExcludingThis(SizeOfEntryExcludingThisFun sizeOfEntryExcludingThis,
                             mozilla::MallocSizeOf mallocSizeOf,
                             void *userArg = nullptr) const
  {
    if (sizeOfEntryExcludingThis) {
      s_SizeOfArgs args = { sizeOfEntryExcludingThis, userArg };
      return PL_DHashTableSizeOfExcludingThis(&mTable, s_SizeOfStub, mallocSizeOf, &args);
    }
    return PL_DHashTableSizeOfExcludingThis(&mTable, nullptr, mallocSizeOf);
  }

  


  size_t SizeOfIncludingThis(SizeOfEntryExcludingThisFun sizeOfEntryExcludingThis,
                             mozilla::MallocSizeOf mallocSizeOf,
                             void *userArg = nullptr) const
  {
    return mallocSizeOf(this) +
        SizeOfExcludingThis(sizeOfEntryExcludingThis, mallocSizeOf, userArg);
  }

#ifdef DEBUG
  





  void MarkImmutable()
  {
    NS_ASSERTION(mTable.entrySize, "nsTHashtable was not initialized properly.");

    PL_DHashMarkTableImmutable(&mTable);
  }
#endif

protected:
  PLDHashTable mTable;

  static const void* s_GetKey(PLDHashTable    *table,
                              PLDHashEntryHdr *entry);

  static PLDHashNumber s_HashKey(PLDHashTable *table,
                                 const void   *key);

  static bool s_MatchEntry(PLDHashTable           *table,
                             const PLDHashEntryHdr  *entry,
                             const void             *key);
  
  static void s_CopyEntry(PLDHashTable          *table,
                          const PLDHashEntryHdr *from,
                          PLDHashEntryHdr       *to);
  
  static void s_ClearEntry(PLDHashTable *table,
                           PLDHashEntryHdr *entry);

  static bool s_InitEntry(PLDHashTable     *table,
                            PLDHashEntryHdr  *entry,
                            const void       *key);

  






  struct s_EnumArgs
  {
    Enumerator userFunc;
    void* userArg;
  };
  
  static PLDHashOperator s_EnumStub(PLDHashTable    *table,
                                    PLDHashEntryHdr *entry,
                                    uint32_t         number,
                                    void            *arg);

  






  struct s_SizeOfArgs
  {
    SizeOfEntryExcludingThisFun userFunc;
    void* userArg;
  };
  
  static size_t s_SizeOfStub(PLDHashEntryHdr *entry,
                             mozilla::MallocSizeOf mallocSizeOf,
                             void *arg);

private:
  
  nsTHashtable(nsTHashtable<EntryType>& toCopy) MOZ_DELETE;

  



  void Init(uint32_t aInitSize);

  
  nsTHashtable<EntryType>& operator= (nsTHashtable<EntryType>& toEqual) MOZ_DELETE;
};





template<class EntryType>
nsTHashtable<EntryType>::nsTHashtable(
  nsTHashtable<EntryType>&& aOther)
  : mTable(mozilla::Move(aOther.mTable))
{
  
  
  MOZ_MAKE_MEM_UNDEFINED(&aOther.mTable, sizeof(aOther.mTable));

  
  
  aOther.mTable.entrySize = 0;
}

template<class EntryType>
nsTHashtable<EntryType>::~nsTHashtable()
{
  if (mTable.entrySize)
    PL_DHashTableFinish(&mTable);
}

template<class EntryType>
void
nsTHashtable<EntryType>::Init(uint32_t aInitSize)
{
  static const PLDHashTableOps sOps =
  {
    ::PL_DHashAllocTable,
    ::PL_DHashFreeTable,
    s_HashKey,
    s_MatchEntry,
    EntryType::ALLOW_MEMMOVE ? ::PL_DHashMoveEntryStub : s_CopyEntry,
    s_ClearEntry,
    ::PL_DHashFinalizeStub,
    s_InitEntry
  };

  PL_DHashTableInit(&mTable, &sOps, nullptr, sizeof(EntryType), aInitSize);
}



template<class EntryType>
PLDHashNumber
nsTHashtable<EntryType>::s_HashKey(PLDHashTable  *table,
                                   const void    *key)
{
  return EntryType::HashKey(reinterpret_cast<const KeyTypePointer>(key));
}

template<class EntryType>
bool
nsTHashtable<EntryType>::s_MatchEntry(PLDHashTable          *table,
                                      const PLDHashEntryHdr *entry,
                                      const void            *key)
{
  return ((const EntryType*) entry)->KeyEquals(
    reinterpret_cast<const KeyTypePointer>(key));
}

template<class EntryType>
void
nsTHashtable<EntryType>::s_CopyEntry(PLDHashTable          *table,
                                     const PLDHashEntryHdr *from,
                                     PLDHashEntryHdr       *to)
{
  EntryType* fromEntry =
    const_cast<EntryType*>(reinterpret_cast<const EntryType*>(from));

  new(to) EntryType(mozilla::Move(*fromEntry));

  fromEntry->~EntryType();
}

template<class EntryType>
void
nsTHashtable<EntryType>::s_ClearEntry(PLDHashTable    *table,
                                      PLDHashEntryHdr *entry)
{
  reinterpret_cast<EntryType*>(entry)->~EntryType();
}

template<class EntryType>
bool
nsTHashtable<EntryType>::s_InitEntry(PLDHashTable    *table,
                                     PLDHashEntryHdr *entry,
                                     const void      *key)
{
  new(entry) EntryType(reinterpret_cast<KeyTypePointer>(key));
  return true;
}

template<class EntryType>
PLDHashOperator
nsTHashtable<EntryType>::s_EnumStub(PLDHashTable    *table,
                                    PLDHashEntryHdr *entry,
                                    uint32_t         number,
                                    void            *arg)
{
  
  return (* reinterpret_cast<s_EnumArgs*>(arg)->userFunc)(
    reinterpret_cast<EntryType*>(entry),
    reinterpret_cast<s_EnumArgs*>(arg)->userArg);
}

template<class EntryType>
size_t
nsTHashtable<EntryType>::s_SizeOfStub(PLDHashEntryHdr *entry,
                                      mozilla::MallocSizeOf mallocSizeOf,
                                      void *arg)
{
  
  return (* reinterpret_cast<s_SizeOfArgs*>(arg)->userFunc)(
    reinterpret_cast<EntryType*>(entry),
    mallocSizeOf,
    reinterpret_cast<s_SizeOfArgs*>(arg)->userArg);
}

class nsCycleCollectionTraversalCallback;

struct MOZ_STACK_CLASS nsTHashtableCCTraversalData
{
  nsTHashtableCCTraversalData(nsCycleCollectionTraversalCallback& aCallback,
                              const char* aName,
                              uint32_t aFlags)
  : mCallback(aCallback),
    mName(aName),
    mFlags(aFlags)
  {
  }

  nsCycleCollectionTraversalCallback& mCallback;
  const char* mName;
  uint32_t mFlags;
};

template <class EntryType>
PLDHashOperator
ImplCycleCollectionTraverse_EnumFunc(EntryType *aEntry,
                                     void* aUserData)
{
  auto userData = static_cast<nsTHashtableCCTraversalData*>(aUserData);

  ImplCycleCollectionTraverse(userData->mCallback,
                              *aEntry,
                              userData->mName,
                              userData->mFlags);
  return PL_DHASH_NEXT;
}

template <class EntryType>
inline void
ImplCycleCollectionUnlink(nsTHashtable<EntryType>& aField)
{
  aField.Clear();
}

template <class EntryType>
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
