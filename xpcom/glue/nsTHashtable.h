




































#ifndef nsTHashtable_h__
#define nsTHashtable_h__

#include "nscore.h"
#include "pldhash.h"
#include "nsDebug.h"
#include NEW_H


NS_COM_GLUE PLDHashOperator
PL_DHashStubEnumRemove(PLDHashTable    *table,
                       PLDHashEntryHdr *entry,
                       PRUint32         ordinal,
                       void            *userArg);





















































template<class EntryType>
class nsTHashtable
{
public:
  


  nsTHashtable();

  


  ~nsTHashtable();

  





  PRBool Init(PRUint32 initSize = PL_DHASH_MIN_SIZE);

  



  PRBool IsInitialized() const { return mTable.entrySize; }

  


  typedef typename EntryType::KeyType KeyType;

  


  typedef typename EntryType::KeyTypePointer KeyTypePointer;

  



  PRUint32 Count() const { return mTable.entryCount; }

  





  EntryType* GetEntry(KeyType aKey) const
  {
    NS_ASSERTION(mTable.entrySize, "nsTHashtable was not initialized properly.");
  
    EntryType* entry =
      NS_REINTERPRET_CAST(EntryType*,
                          PL_DHashTableOperate(
                            NS_CONST_CAST(PLDHashTable*,&mTable),
                            EntryType::KeyToPointer(aKey),
                            PL_DHASH_LOOKUP));
    return PL_DHASH_ENTRY_IS_BUSY(entry) ? entry : nsnull;
  }

  





  EntryType* PutEntry(KeyType aKey)
  {
    NS_ASSERTION(mTable.entrySize, "nsTHashtable was not initialized properly.");
    
    return NS_STATIC_CAST(EntryType*,
                          PL_DHashTableOperate(
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

  









  typedef PLDHashOperator (*PR_CALLBACK Enumerator)(EntryType* aEntry, void* userArg);

  






  PRUint32 EnumerateEntries(Enumerator enumFunc, void* userArg)
  {
    NS_ASSERTION(mTable.entrySize, "nsTHashtable was not initialized properly.");
    
    s_EnumArgs args = { enumFunc, userArg };
    return PL_DHashTableEnumerate(&mTable, s_EnumStub, &args);
  }

  


  void Clear()
  {
    NS_ASSERTION(mTable.entrySize, "nsTHashtable was not initialized properly.");

    PL_DHashTableEnumerate(&mTable, PL_DHashStubEnumRemove, nsnull);
  }

protected:
  PLDHashTable mTable;

  static const void* PR_CALLBACK s_GetKey(PLDHashTable    *table,
                                          PLDHashEntryHdr *entry);

  static PLDHashNumber PR_CALLBACK s_HashKey(PLDHashTable *table,
                                             const void   *key);

  static PRBool PR_CALLBACK s_MatchEntry(PLDHashTable           *table,
                                         const PLDHashEntryHdr  *entry,
                                         const void             *key);
  
  static void PR_CALLBACK s_CopyEntry(PLDHashTable          *table,
                                      const PLDHashEntryHdr *from,
                                      PLDHashEntryHdr       *to);
  
  static void PR_CALLBACK s_ClearEntry(PLDHashTable *table,
                                       PLDHashEntryHdr *entry);

  static PRBool PR_CALLBACK s_InitEntry(PLDHashTable     *table,
                                        PLDHashEntryHdr  *entry,
                                        const void       *key);

  






  struct s_EnumArgs
  {
    Enumerator userFunc;
    void* userArg;
  };
  
  static PLDHashOperator PR_CALLBACK s_EnumStub(PLDHashTable    *table,
                                                PLDHashEntryHdr *entry,
                                                PRUint32         number,
                                                void            *arg);
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
PRBool
nsTHashtable<EntryType>::Init(PRUint32 initSize)
{
  if (mTable.entrySize)
  {
    NS_ERROR("nsTHashtable::Init() should not be called twice.");
    return PR_TRUE;
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
  
  if (!PL_DHashTableInit(&mTable, &sOps, nsnull, sizeof(EntryType), initSize))
  {
    
    mTable.entrySize = 0;
    return PR_FALSE;
  }

  return PR_TRUE;
}



template<class EntryType>
PLDHashNumber
nsTHashtable<EntryType>::s_HashKey(PLDHashTable  *table,
                                   const void    *key)
{
  return EntryType::HashKey(NS_REINTERPRET_CAST(const KeyTypePointer, key));
}

template<class EntryType>
PRBool
nsTHashtable<EntryType>::s_MatchEntry(PLDHashTable          *table,
                                      const PLDHashEntryHdr *entry,
                                      const void            *key)
{
  return ((const EntryType*) entry)->KeyEquals(
    NS_REINTERPRET_CAST(const KeyTypePointer, key));
}

template<class EntryType>
void
nsTHashtable<EntryType>::s_CopyEntry(PLDHashTable          *table,
                                     const PLDHashEntryHdr *from,
                                     PLDHashEntryHdr       *to)
{
  EntryType* fromEntry =
    NS_CONST_CAST(EntryType*, NS_REINTERPRET_CAST(const EntryType*, from));

  new(to) EntryType(*fromEntry);

  fromEntry->~EntryType();
}

template<class EntryType>
void
nsTHashtable<EntryType>::s_ClearEntry(PLDHashTable    *table,
                                      PLDHashEntryHdr *entry)
{
  NS_REINTERPRET_CAST(EntryType*,entry)->~EntryType();
}

template<class EntryType>
PRBool
nsTHashtable<EntryType>::s_InitEntry(PLDHashTable    *table,
                                     PLDHashEntryHdr *entry,
                                     const void      *key)
{
  new(entry) EntryType(NS_REINTERPRET_CAST(KeyTypePointer,key));
  return PR_TRUE;
}

template<class EntryType>
PLDHashOperator
nsTHashtable<EntryType>::s_EnumStub(PLDHashTable    *table,
                                    PLDHashEntryHdr *entry,
                                    PRUint32         number,
                                    void            *arg)
{
  
  return (* NS_REINTERPRET_CAST(s_EnumArgs*,arg)->userFunc)(
    NS_REINTERPRET_CAST(EntryType*,entry),
    NS_REINTERPRET_CAST(s_EnumArgs*,arg)->userArg);
}

#endif 
