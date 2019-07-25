




































#ifndef nsBaseHashtable_h__
#define nsBaseHashtable_h__

#include "nsTHashtable.h"
#include "prlock.h"
#include "nsDebug.h"

template<class KeyClass,class DataType,class UserDataType>
class nsBaseHashtable; 






template<class KeyClass,class DataType>
class nsBaseHashtableET : public KeyClass
{
public:
  DataType mData;
  friend class nsTHashtable< nsBaseHashtableET<KeyClass,DataType> >;

private:
  typedef typename KeyClass::KeyType KeyType;
  typedef typename KeyClass::KeyTypePointer KeyTypePointer;
  
  nsBaseHashtableET(KeyTypePointer aKey);
  nsBaseHashtableET(nsBaseHashtableET<KeyClass,DataType>& toCopy);
  ~nsBaseHashtableET();
};













template<class KeyClass,class DataType,class UserDataType>
class nsBaseHashtable :
  protected nsTHashtable< nsBaseHashtableET<KeyClass,DataType> >
{
public:
  typedef typename KeyClass::KeyType KeyType;
  typedef nsBaseHashtableET<KeyClass,DataType> EntryType;

  

  






  bool Init(PRUint32 initSize = PL_DHASH_MIN_SIZE)
  { return nsTHashtable<EntryType>::Init(initSize); }

  




  bool IsInitialized() const { return !!this->mTable.entrySize; }

  



  PRUint32 Count() const
  { return nsTHashtable<EntryType>::Count(); }

  








  bool Get(KeyType aKey, UserDataType* pData NS_OUTPARAM) const
  {
    EntryType* ent = this->GetEntry(aKey);

    if (!ent)
      return false;

    if (pData)
      *pData = ent->mData;

    return true;
  }

  








  UserDataType Get(KeyType aKey) const
  {
    EntryType* ent = this->GetEntry(aKey);
    if (!ent)
      return nsnull;

    return ent->mData;
  }

  





  bool Put(KeyType aKey, UserDataType aData)
  {
    EntryType* ent = this->PutEntry(aKey);

    if (!ent)
      return false;

    ent->mData = aData;

    return true;
  }

  



  void Remove(KeyType aKey) { this->RemoveEntry(aKey); }

  








  typedef PLDHashOperator
    (* EnumReadFunction)(KeyType      aKey,
                         UserDataType aData,
                         void*        userArg);

  




  PRUint32 EnumerateRead(EnumReadFunction enumFunc, void* userArg) const
  {
    NS_ASSERTION(this->mTable.entrySize,
                 "nsBaseHashtable was not initialized properly.");

    s_EnumReadArgs enumData = { enumFunc, userArg };
    return PL_DHashTableEnumerate(const_cast<PLDHashTable*>(&this->mTable),
                                  s_EnumReadStub,
                                  &enumData);
  }

  










  typedef PLDHashOperator
    (* EnumFunction)(KeyType       aKey,
                     DataType&     aData,
                     void*         userArg);

  





  PRUint32 Enumerate(EnumFunction enumFunc, void* userArg)
  {
    NS_ASSERTION(this->mTable.entrySize,
                 "nsBaseHashtable was not initialized properly.");

    s_EnumArgs enumData = { enumFunc, userArg };
    return PL_DHashTableEnumerate(&this->mTable,
                                  s_EnumStub,
                                  &enumData);
  }

  


  void Clear() { nsTHashtable<EntryType>::Clear(); }

  








  typedef size_t
    (* SizeOfEntryExcludingThisFun)(KeyType           aKey,
                                    const DataType    &aData,
                                    nsMallocSizeOfFun mallocSizeOf,
                                    void*             userArg);

  











  size_t SizeOfExcludingThis(SizeOfEntryExcludingThisFun sizeOfEntryExcludingThis,
                             nsMallocSizeOfFun mallocSizeOf, void *userArg = nsnull)
  {
    if (!IsInitialized()) {
      return 0;
    }
    if (sizeOfEntryExcludingThis) {
      s_SizeOfArgs args = { sizeOfEntryExcludingThis, userArg };
      return PL_DHashTableSizeOfExcludingThis(&this->mTable, s_SizeOfStub,
                                              mallocSizeOf, &args);
    }
    return PL_DHashTableSizeOfExcludingThis(&this->mTable, NULL, mallocSizeOf);
  }

protected:
  




  struct s_EnumReadArgs
  {
    EnumReadFunction func;
    void* userArg;
  };

  static PLDHashOperator s_EnumReadStub(PLDHashTable    *table,
                                        PLDHashEntryHdr *hdr,
                                        PRUint32         number,
                                        void            *arg);

  struct s_EnumArgs
  {
    EnumFunction func;
    void* userArg;
  };

  static PLDHashOperator s_EnumStub(PLDHashTable      *table,
                                    PLDHashEntryHdr   *hdr,
                                    PRUint32           number,
                                    void              *arg);

  struct s_SizeOfArgs
  {
    SizeOfEntryExcludingThisFun func;
    void* userArg;
  };
  
  static size_t s_SizeOfStub(PLDHashEntryHdr *entry,
                             nsMallocSizeOfFun mallocSizeOf,
                             void *arg);
};




template<class KeyClass,class DataType,class UserDataType>
class nsBaseHashtableMT :
  protected nsBaseHashtable<KeyClass,DataType,UserDataType>
{
public:
  typedef typename
    nsBaseHashtable<KeyClass,DataType,UserDataType>::EntryType EntryType;
  typedef typename
    nsBaseHashtable<KeyClass,DataType,UserDataType>::KeyType KeyType;
  typedef typename
    nsBaseHashtable<KeyClass,DataType,UserDataType>::EnumFunction EnumFunction;
  typedef typename
    nsBaseHashtable<KeyClass,DataType,UserDataType>::EnumReadFunction EnumReadFunction;

  nsBaseHashtableMT() : mLock(nsnull) { }
  ~nsBaseHashtableMT();

  bool Init(PRUint32 initSize = PL_DHASH_MIN_SIZE);
  bool IsInitialized() const { return mLock != nsnull; }
  PRUint32 Count() const;
  bool Get(KeyType aKey, UserDataType* pData) const;
  bool Put(KeyType aKey, UserDataType aData);
  void Remove(KeyType aKey);

  PRUint32 EnumerateRead(EnumReadFunction enumFunc, void* userArg) const;
  PRUint32 Enumerate(EnumFunction enumFunc, void* userArg);
  void Clear();

protected:
  PRLock* mLock;
};
  





template<class KeyClass,class DataType>
nsBaseHashtableET<KeyClass,DataType>::nsBaseHashtableET(KeyTypePointer aKey) :
  KeyClass(aKey)
{ }

template<class KeyClass,class DataType>
nsBaseHashtableET<KeyClass,DataType>::nsBaseHashtableET
  (nsBaseHashtableET<KeyClass,DataType>& toCopy) :
  KeyClass(toCopy),
  mData(toCopy.mData)
{ }

template<class KeyClass,class DataType>
nsBaseHashtableET<KeyClass,DataType>::~nsBaseHashtableET()
{ }






template<class KeyClass,class DataType,class UserDataType>
PLDHashOperator
nsBaseHashtable<KeyClass,DataType,UserDataType>::s_EnumReadStub
  (PLDHashTable *table, PLDHashEntryHdr *hdr, PRUint32 number, void* arg)
{
  EntryType* ent = static_cast<EntryType*>(hdr);
  s_EnumReadArgs* eargs = (s_EnumReadArgs*) arg;

  PLDHashOperator res = (eargs->func)(ent->GetKey(), ent->mData, eargs->userArg);

  NS_ASSERTION( !(res & PL_DHASH_REMOVE ),
                "PL_DHASH_REMOVE return during const enumeration; ignoring.");

  if (res & PL_DHASH_STOP)
    return PL_DHASH_STOP;

  return PL_DHASH_NEXT;
}

template<class KeyClass,class DataType,class UserDataType>
PLDHashOperator
nsBaseHashtable<KeyClass,DataType,UserDataType>::s_EnumStub
  (PLDHashTable *table, PLDHashEntryHdr *hdr, PRUint32 number, void* arg)
{
  EntryType* ent = static_cast<EntryType*>(hdr);
  s_EnumArgs* eargs = (s_EnumArgs*) arg;

  return (eargs->func)(ent->GetKey(), ent->mData, eargs->userArg);
}

template<class KeyClass,class DataType,class UserDataType>
size_t
nsBaseHashtable<KeyClass,DataType,UserDataType>::s_SizeOfStub
  (PLDHashEntryHdr *hdr, nsMallocSizeOfFun mallocSizeOf, void *arg)
{
  EntryType* ent = static_cast<EntryType*>(hdr);
  s_SizeOfArgs* eargs = static_cast<s_SizeOfArgs*>(arg);

  return (eargs->func)(ent->GetKey(), ent->mData, mallocSizeOf, eargs->userArg);
}





template<class KeyClass,class DataType,class UserDataType>
nsBaseHashtableMT<KeyClass,DataType,UserDataType>::~nsBaseHashtableMT()
{
  if (this->mLock)
    PR_DestroyLock(this->mLock);
}

template<class KeyClass,class DataType,class UserDataType>
bool
nsBaseHashtableMT<KeyClass,DataType,UserDataType>::Init(PRUint32 initSize)
{
  if (!nsTHashtable<EntryType>::IsInitialized() && !nsTHashtable<EntryType>::Init(initSize))
    return false;

  this->mLock = PR_NewLock();
  NS_ASSERTION(this->mLock, "Error creating lock during nsBaseHashtableL::Init()");

  return (this->mLock != nsnull);
}

template<class KeyClass,class DataType,class UserDataType>
PRUint32
nsBaseHashtableMT<KeyClass,DataType,UserDataType>::Count() const
{
  PR_Lock(this->mLock);
  PRUint32 count = nsTHashtable<EntryType>::Count();
  PR_Unlock(this->mLock);

  return count;
}

template<class KeyClass,class DataType,class UserDataType>
bool
nsBaseHashtableMT<KeyClass,DataType,UserDataType>::Get(KeyType       aKey,
                                                           UserDataType* pData) const
{
  PR_Lock(this->mLock);
  bool res =
    nsBaseHashtable<KeyClass,DataType,UserDataType>::Get(aKey, pData);
  PR_Unlock(this->mLock);

  return res;
}

template<class KeyClass,class DataType,class UserDataType>
bool
nsBaseHashtableMT<KeyClass,DataType,UserDataType>::Put(KeyType      aKey,
                                                           UserDataType aData)
{
  PR_Lock(this->mLock);
  bool res =
    nsBaseHashtable<KeyClass,DataType,UserDataType>::Put(aKey, aData);
  PR_Unlock(this->mLock);

  return res;
}

template<class KeyClass,class DataType,class UserDataType>
void
nsBaseHashtableMT<KeyClass,DataType,UserDataType>::Remove(KeyType aKey)
{
  PR_Lock(this->mLock);
  nsBaseHashtable<KeyClass,DataType,UserDataType>::Remove(aKey);
  PR_Unlock(this->mLock);
}

template<class KeyClass,class DataType,class UserDataType>
PRUint32
nsBaseHashtableMT<KeyClass,DataType,UserDataType>::EnumerateRead
  (EnumReadFunction fEnumCall, void* userArg) const
{
  PR_Lock(this->mLock);
  PRUint32 count =
    nsBaseHashtable<KeyClass,DataType,UserDataType>::EnumerateRead(fEnumCall, userArg);
  PR_Unlock(this->mLock);

  return count;
}

template<class KeyClass,class DataType,class UserDataType>
PRUint32
nsBaseHashtableMT<KeyClass,DataType,UserDataType>::Enumerate
  (EnumFunction fEnumCall, void* userArg)
{
  PR_Lock(this->mLock);
  PRUint32 count =
    nsBaseHashtable<KeyClass,DataType,UserDataType>::Enumerate(fEnumCall, userArg);
  PR_Unlock(this->mLock);

  return count;
}

template<class KeyClass,class DataType,class UserDataType>
void
nsBaseHashtableMT<KeyClass,DataType,UserDataType>::Clear()
{
  PR_Lock(this->mLock);
  nsBaseHashtable<KeyClass,DataType,UserDataType>::Clear();
  PR_Unlock(this->mLock);
}

#endif 
