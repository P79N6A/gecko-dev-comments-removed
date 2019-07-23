




































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

  

  






  PRBool Init(PRUint32 initSize = PL_DHASH_MIN_SIZE)
  { return nsTHashtable<EntryType>::Init(initSize); }

  




  PRBool IsInitialized() const { return this->mTable.entrySize; }

  



  PRUint32 Count() const
  { return nsTHashtable<EntryType>::Count(); }

  








  PRBool Get(KeyType aKey, UserDataType* pData) const
  {
    EntryType* ent = GetEntry(aKey);

    if (!ent)
      return PR_FALSE;

    if (pData)
      *pData = ent->mData;

    return PR_TRUE;
  }

  





  PRBool Put(KeyType aKey, UserDataType aData)
  {
    EntryType* ent = PutEntry(aKey);

    if (!ent)
      return PR_FALSE;

    ent->mData = aData;

    return PR_TRUE;
  }

  



  void Remove(KeyType aKey) { RemoveEntry(aKey); }

  








  typedef PLDHashOperator
    (*PR_CALLBACK EnumReadFunction)(KeyType      aKey,
                                    UserDataType aData,
                                    void*        userArg);

  




  PRUint32 EnumerateRead(EnumReadFunction enumFunc, void* userArg) const
  {
    NS_ASSERTION(this->mTable.entrySize,
                 "nsBaseHashtable was not initialized properly.");

    s_EnumReadArgs enumData = { enumFunc, userArg };
    return PL_DHashTableEnumerate(NS_CONST_CAST(PLDHashTable*, &this->mTable),
                                  s_EnumReadStub,
                                  &enumData);
  }

  










  typedef PLDHashOperator
    (*PR_CALLBACK EnumFunction)(KeyType       aKey,
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

  PRBool Init(PRUint32 initSize = PL_DHASH_MIN_SIZE);
  PRBool IsInitialized() const { return mLock != nsnull; }
  PRUint32 Count() const;
  PRBool Get(KeyType aKey, UserDataType* pData) const;
  PRBool Put(KeyType aKey, UserDataType aData);
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
  EntryType* ent = NS_STATIC_CAST(EntryType*, hdr);
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
  EntryType* ent = NS_STATIC_CAST(EntryType*, hdr);
  s_EnumArgs* eargs = (s_EnumArgs*) arg;

  return (eargs->func)(ent->GetKey(), ent->mData, eargs->userArg);
}






template<class KeyClass,class DataType,class UserDataType>
nsBaseHashtableMT<KeyClass,DataType,UserDataType>::~nsBaseHashtableMT()
{
  if (this->mLock)
    PR_DestroyLock(this->mLock);
}

template<class KeyClass,class DataType,class UserDataType>
PRBool
nsBaseHashtableMT<KeyClass,DataType,UserDataType>::Init(PRUint32 initSize)
{
  if (!nsTHashtable<EntryType>::IsInitialized() && !nsTHashtable<EntryType>::Init(initSize))
    return PR_FALSE;

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
PRBool
nsBaseHashtableMT<KeyClass,DataType,UserDataType>::Get(KeyType       aKey,
                                                           UserDataType* pData) const
{
  PR_Lock(this->mLock);
  PRBool res =
    nsBaseHashtable<KeyClass,DataType,UserDataType>::Get(aKey, pData);
  PR_Unlock(this->mLock);

  return res;
}

template<class KeyClass,class DataType,class UserDataType>
PRBool
nsBaseHashtableMT<KeyClass,DataType,UserDataType>::Put(KeyType      aKey,
                                                           UserDataType aData)
{
  PR_Lock(this->mLock);
  PRBool res =
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
