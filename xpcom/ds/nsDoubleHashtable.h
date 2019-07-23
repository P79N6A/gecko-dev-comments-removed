








































#ifndef __nsDoubleHashtable_h__
#define __nsDoubleHashtable_h__

#include "pldhash.h"
#include "nscore.h"
#include "nsString.h"
#include "nsHashKeys.h"






























































































































































#define DHASH_CALLBACKS(ENTRY_CLASS)                                          \
PR_STATIC_CALLBACK(const void *)                                              \
ENTRY_CLASS##GetKey(PLDHashTable* table, PLDHashEntryHdr* entry)              \
{                                                                             \
  ENTRY_CLASS* e = NS_STATIC_CAST(ENTRY_CLASS*, entry);                       \
  return e->GetKey();                                                         \
}                                                                             \
PR_STATIC_CALLBACK(PLDHashNumber)                                             \
ENTRY_CLASS##HashKey(PLDHashTable* table, const void* key)                    \
{                                                                             \
  return ENTRY_CLASS::HashKey(key);                                           \
}                                                                             \
PR_STATIC_CALLBACK(PRBool)                                                    \
ENTRY_CLASS##MatchEntry(PLDHashTable *table, const PLDHashEntryHdr *entry,    \
                        const void *key)                                      \
{                                                                             \
  const ENTRY_CLASS* e = NS_STATIC_CAST(const ENTRY_CLASS*, entry);           \
  return e->MatchEntry(key);                                                  \
}                                                                             \
PR_STATIC_CALLBACK(void)                                                      \
ENTRY_CLASS##ClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry)          \
{                                                                             \
  ENTRY_CLASS* e = NS_STATIC_CAST(ENTRY_CLASS *, entry);                      \
  e->~ENTRY_CLASS();                                                          \
}                                                                             \
PR_STATIC_CALLBACK(PRBool)                                                    \
ENTRY_CLASS##InitEntry(PLDHashTable *table, PLDHashEntryHdr *entry,           \
                       const void *key)                                       \
{                                                                             \
  new (entry) ENTRY_CLASS(key);                                               \
  return PR_TRUE;                                                             \
}













#define DHASH_INIT(HASHTABLE,ENTRY_CLASS,NUM_INITIAL_ENTRIES,RV)              \
PR_BEGIN_MACRO                                                                \
  static PLDHashTableOps hash_table_ops =                                     \
  {                                                                           \
    PL_DHashAllocTable,                                                       \
    PL_DHashFreeTable,                                                        \
    ENTRY_CLASS##GetKey,                                                      \
    ENTRY_CLASS##HashKey,                                                     \
    ENTRY_CLASS##MatchEntry,                                                  \
    PL_DHashMoveEntryStub,                                                    \
    ENTRY_CLASS##ClearEntry,                                                  \
    PL_DHashFinalizeStub,                                                     \
    ENTRY_CLASS##InitEntry                                                    \
  };                                                                          \
  PRBool isLive = PL_DHashTableInit(&(HASHTABLE),                             \
                                    &hash_table_ops, nsnull,                  \
                                    sizeof(ENTRY_CLASS),                      \
                                    (NUM_INITIAL_ENTRIES));                   \
  if (!isLive) {                                                              \
    (HASHTABLE).ops = nsnull;                                                 \
    RV = NS_ERROR_OUT_OF_MEMORY;                                              \
  } else {                                                                    \
    RV = NS_OK;                                                               \
  }                                                                           \
PR_END_MACRO


























































#define DECL_DHASH_WRAPPER(CLASSNAME,ENTRY_CLASS,KEY_TYPE)                    \
class DHASH_EXPORT CLASSNAME {                                                \
public:                                                                       \
  CLASSNAME();                                                                \
  ~CLASSNAME();                                                               \
  nsresult Init(PRUint32 aNumInitialEntries);                                 \
  ENTRY_CLASS* GetEntry(const KEY_TYPE aKey);                                 \
  ENTRY_CLASS* AddEntry(const KEY_TYPE aKey);                                 \
  void Remove(const KEY_TYPE aKey);                                           \
  PLDHashTable mHashTable;                                                    \
};

#define DHASH_WRAPPER(CLASSNAME,ENTRY_CLASS,KEY_TYPE)                         \
DHASH_CALLBACKS(ENTRY_CLASS)                                                  \
CLASSNAME::CLASSNAME() {                                                      \
  mHashTable.ops = nsnull;                                                    \
}                                                                             \
CLASSNAME::~CLASSNAME() {                                                     \
  if (mHashTable.ops) {                                                       \
    PL_DHashTableFinish(&mHashTable);                                         \
  }                                                                           \
}                                                                             \
nsresult CLASSNAME::Init(PRUint32 aNumInitialEntries) {                       \
  if (!mHashTable.ops) {                                                      \
    nsresult rv;                                                              \
    DHASH_INIT(mHashTable,ENTRY_CLASS,aNumInitialEntries,rv);                 \
    return rv;                                                                \
  }                                                                           \
  return NS_OK;                                                               \
}                                                                             \
ENTRY_CLASS* CLASSNAME::GetEntry(const KEY_TYPE aKey) {                       \
  ENTRY_CLASS* e = NS_STATIC_CAST(ENTRY_CLASS*,                               \
                                  PL_DHashTableOperate(&mHashTable, &aKey,    \
                                                       PL_DHASH_LOOKUP));     \
  return PL_DHASH_ENTRY_IS_BUSY(e) ? e : nsnull;                              \
}                                                                             \
ENTRY_CLASS* CLASSNAME::AddEntry(const KEY_TYPE aKey) {                       \
  return NS_STATIC_CAST(ENTRY_CLASS*,                                         \
                        PL_DHashTableOperate(&mHashTable, &aKey,              \
                                             PL_DHASH_ADD));                  \
}                                                                             \
void CLASSNAME::Remove(const KEY_TYPE aKey) {                                 \
  PL_DHashTableOperate(&mHashTable, &aKey, PL_DHASH_REMOVE);                  \
}
































class NS_COM PLDHashStringEntry : public PLDHashEntryHdr
{
public:
  PLDHashStringEntry(const void* aKey) :
    mKey(*NS_STATIC_CAST(const nsAString*, aKey)) { }
  ~PLDHashStringEntry() { }

  const void* GetKey() const {
    return NS_STATIC_CAST(const nsAString*, &mKey);
  }
  static PLDHashNumber HashKey(const void* key) {
    return HashString(*NS_STATIC_CAST(const nsAString*, key));
  }
  PRBool MatchEntry(const void* key) const {
    return NS_STATIC_CAST(const nsAString*, key)->Equals(mKey);
  }

  const nsString mKey;
};




class NS_COM PLDHashCStringEntry : public PLDHashEntryHdr
{
public:
  PLDHashCStringEntry(const void* aKey) :
    mKey(*NS_STATIC_CAST(const nsACString*, aKey)) { }
  ~PLDHashCStringEntry() { }

  const void* GetKey() const {
    return NS_STATIC_CAST(const nsACString*, &mKey);
  }
  static PLDHashNumber HashKey(const void* key) {
    return HashString(*NS_STATIC_CAST(const nsACString*, key));
  }
  PRBool MatchEntry(const void* key) const {
    return NS_STATIC_CAST(const nsACString*, key)->Equals(mKey);
  }

  const nsCString mKey;
};




class NS_COM PLDHashInt32Entry : public PLDHashEntryHdr
{
public:
  PLDHashInt32Entry(const void* aKey) :
    mKey(*(NS_STATIC_CAST(const PRInt32*, aKey))) { }
  ~PLDHashInt32Entry() { }

  const void* GetKey() const {
    return NS_STATIC_CAST(const PRInt32*, &mKey);
  }
  static PLDHashNumber HashKey(const void* key) {
    return *NS_STATIC_CAST(const PRInt32*, key);
  }
  PRBool MatchEntry(const void* key) const {
    return *(NS_STATIC_CAST(const PRInt32*, key)) == mKey;
  }

  const PRInt32 mKey;
};





class NS_COM PLDHashVoidEntry : public PLDHashEntryHdr
{
public:
  PLDHashVoidEntry(const void* aKey) :
    mKey(*(const void**)aKey) { }
  ~PLDHashVoidEntry() { }

  const void* GetKey() const {
    return (const void**)&mKey;
  }
  static PLDHashNumber HashKey(const void* key) {
    return PLDHashNumber(NS_PTR_TO_INT32(*(const void**)key)) >> 2;
  }
  PRBool MatchEntry(const void* key) const {
    return *(const void**)key == mKey;
  }

  const void* mKey;
};


#define DHASH_EXPORT


#endif
