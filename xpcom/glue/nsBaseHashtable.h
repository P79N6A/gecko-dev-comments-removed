




#ifndef nsBaseHashtable_h__
#define nsBaseHashtable_h__

#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"
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
  nsBaseHashtableET(nsBaseHashtableET<KeyClass,DataType>&& toMove);
  ~nsBaseHashtableET();
};













template<class KeyClass,class DataType,class UserDataType>
class nsBaseHashtable :
  protected nsTHashtable< nsBaseHashtableET<KeyClass,DataType> >
{
  typedef mozilla::fallible_t fallible_t;

public:
  typedef typename KeyClass::KeyType KeyType;
  typedef nsBaseHashtableET<KeyClass,DataType> EntryType;

  nsBaseHashtable()
  {
  }
  explicit nsBaseHashtable(uint32_t aInitSize)
    : nsTHashtable<EntryType>(aInitSize)
  {
  }

  



  uint32_t Count() const
  { return nsTHashtable<EntryType>::Count(); }

  








  bool Get(KeyType aKey, UserDataType* pData) const
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
      return 0;

    return ent->mData;
  }

  





  void Put(KeyType aKey, const UserDataType& aData)
  {
    if (!Put(aKey, aData, fallible_t()))
      NS_RUNTIMEABORT("OOM");
  }

  bool Put(KeyType aKey, const UserDataType& aData, const fallible_t&) NS_WARN_UNUSED_RESULT
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

  




  uint32_t EnumerateRead(EnumReadFunction enumFunc, void* userArg) const
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

  





  uint32_t Enumerate(EnumFunction enumFunc, void* userArg)
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
                                    mozilla::MallocSizeOf mallocSizeOf,
                                    void*             userArg);

  











  size_t SizeOfIncludingThis(SizeOfEntryExcludingThisFun sizeOfEntryExcludingThis,
                             mozilla::MallocSizeOf mallocSizeOf, void *userArg = nullptr)
  {
    return mallocSizeOf(this) + this->SizeOfExcludingThis(sizeOfEntryExcludingThis,
                                                          mallocSizeOf, userArg);
  }

  











  size_t SizeOfExcludingThis(SizeOfEntryExcludingThisFun sizeOfEntryExcludingThis,
                             mozilla::MallocSizeOf mallocSizeOf, void *userArg = nullptr) const
  {
    if (sizeOfEntryExcludingThis) {
      s_SizeOfArgs args = { sizeOfEntryExcludingThis, userArg };
      return PL_DHashTableSizeOfExcludingThis(&this->mTable, s_SizeOfStub,
                                              mallocSizeOf, &args);
    }
    return PL_DHashTableSizeOfExcludingThis(&this->mTable, nullptr,
                                            mallocSizeOf);
  }

protected:
  




  struct s_EnumReadArgs
  {
    EnumReadFunction func;
    void* userArg;
  };

  static PLDHashOperator s_EnumReadStub(PLDHashTable    *table,
                                        PLDHashEntryHdr *hdr,
                                        uint32_t         number,
                                        void            *arg);

  struct s_EnumArgs
  {
    EnumFunction func;
    void* userArg;
  };

  static PLDHashOperator s_EnumStub(PLDHashTable      *table,
                                    PLDHashEntryHdr   *hdr,
                                    uint32_t           number,
                                    void              *arg);

  struct s_SizeOfArgs
  {
    SizeOfEntryExcludingThisFun func;
    void* userArg;
  };
  
  static size_t s_SizeOfStub(PLDHashEntryHdr *entry,
                             mozilla::MallocSizeOf mallocSizeOf,
                             void *arg);
};

class nsCycleCollectionTraversalCallback;

struct MOZ_STACK_CLASS nsBaseHashtableCCTraversalData
{
  nsBaseHashtableCCTraversalData(nsCycleCollectionTraversalCallback& aCallback,
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

template <typename K, typename T>
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





template<class KeyClass,class DataType>
nsBaseHashtableET<KeyClass,DataType>::nsBaseHashtableET(KeyTypePointer aKey) :
  KeyClass(aKey)
{ }

template<class KeyClass,class DataType>
nsBaseHashtableET<KeyClass,DataType>::nsBaseHashtableET
  (nsBaseHashtableET<KeyClass,DataType>&& toMove) :
  KeyClass(mozilla::Move(toMove)),
  mData(mozilla::Move(toMove.mData))
{ }

template<class KeyClass,class DataType>
nsBaseHashtableET<KeyClass,DataType>::~nsBaseHashtableET()
{ }






template<class KeyClass,class DataType,class UserDataType>
PLDHashOperator
nsBaseHashtable<KeyClass,DataType,UserDataType>::s_EnumReadStub
  (PLDHashTable *table, PLDHashEntryHdr *hdr, uint32_t number, void* arg)
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
  (PLDHashTable *table, PLDHashEntryHdr *hdr, uint32_t number, void* arg)
{
  EntryType* ent = static_cast<EntryType*>(hdr);
  s_EnumArgs* eargs = (s_EnumArgs*) arg;

  return (eargs->func)(ent->GetKey(), ent->mData, eargs->userArg);
}

template<class KeyClass,class DataType,class UserDataType>
size_t
nsBaseHashtable<KeyClass,DataType,UserDataType>::s_SizeOfStub
  (PLDHashEntryHdr *hdr, mozilla::MallocSizeOf mallocSizeOf, void *arg)
{
  EntryType* ent = static_cast<EntryType*>(hdr);
  s_SizeOfArgs* eargs = static_cast<s_SizeOfArgs*>(arg);

  return (eargs->func)(ent->GetKey(), ent->mData, mallocSizeOf, eargs->userArg);
}

#endif 
