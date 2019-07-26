




#ifndef nsTHashtable_h__
#define nsTHashtable_h__

#include "nscore.h"
#include "pldhash.h"
#include "nsDebug.h"
#include NEW_H
#include "mozilla/fallible.h"


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
  


  nsTHashtable();

  


  ~nsTHashtable();

  





  void Init(uint32_t initSize = PL_DHASH_MIN_SIZE)
  {
    if (!Init(initSize, fallible_t()))
      NS_RUNTIMEABORT("OOM");
  }
  bool Init(const fallible_t&) NS_WARN_UNUSED_RESULT
  { return Init(PL_DHASH_MIN_SIZE, fallible_t()); }
  bool Init(uint32_t initSize, const fallible_t&) NS_WARN_UNUSED_RESULT;

  



  bool IsInitialized() const { return !!mTable.entrySize; }

  



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
      NS_RUNTIMEABORT("OOM");
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
                                                 nsMallocSizeOfFun mallocSizeOf,
                                                 void *arg);

  











  size_t SizeOfExcludingThis(SizeOfEntryExcludingThisFun sizeOfEntryExcludingThis,
                             nsMallocSizeOfFun mallocSizeOf, void *userArg = NULL) const
  {
    if (!IsInitialized()) {
      return 0;
    }
    if (sizeOfEntryExcludingThis) {
      s_SizeOfArgs args = { sizeOfEntryExcludingThis, userArg };
      return PL_DHashTableSizeOfExcludingThis(&mTable, s_SizeOfStub, mallocSizeOf, &args);
    }
    return PL_DHashTableSizeOfExcludingThis(&mTable, NULL, mallocSizeOf);
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
                             nsMallocSizeOfFun mallocSizeOf,
                             void *arg);

private:
  
  nsTHashtable(nsTHashtable<EntryType>& toCopy);

  
  nsTHashtable<EntryType>& operator= (nsTHashtable<EntryType>& toEqual);
};





template<class EntryType>
nsTHashtable<EntryType>::nsTHashtable()
{
  
  mTable.entrySize = 0;
}

template<class EntryType>
nsTHashtable<EntryType>::~nsTHashtable()
{
  if (mTable.entrySize)
    PL_DHashTableFinish(&mTable);
}

template<class EntryType>
bool
nsTHashtable<EntryType>::Init(uint32_t initSize, const fallible_t&)
{
  if (mTable.entrySize)
  {
    NS_ERROR("nsTHashtable::Init() should not be called twice.");
    return true;
  }

  static PLDHashTableOps sOps = 
  {
    ::PL_DHashAllocTable,
    ::PL_DHashFreeTable,
    s_HashKey,
    s_MatchEntry,
    ::PL_DHashMoveEntryStub,
    s_ClearEntry,
    ::PL_DHashFinalizeStub,
    s_InitEntry
  };

  if (!EntryType::ALLOW_MEMMOVE)
  {
    sOps.moveEntry = s_CopyEntry;
  }
  
  if (!PL_DHashTableInit(&mTable, &sOps, nullptr, sizeof(EntryType), initSize))
  {
    
    mTable.entrySize = 0;
    return false;
  }

  return true;
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

  new(to) EntryType(*fromEntry);

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
                                      nsMallocSizeOfFun mallocSizeOf,
                                      void *arg)
{
  
  return (* reinterpret_cast<s_SizeOfArgs*>(arg)->userFunc)(
    reinterpret_cast<EntryType*>(entry),
    mallocSizeOf,
    reinterpret_cast<s_SizeOfArgs*>(arg)->userArg);
}

#endif 
