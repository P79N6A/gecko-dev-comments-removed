




#ifndef nsRefPtrHashtable_h__
#define nsRefPtrHashtable_h__

#include "nsBaseHashtable.h"
#include "nsHashKeys.h"
#include "nsAutoPtr.h"









template<class KeyClass, class RefPtr>
class nsRefPtrHashtable :
  public nsBaseHashtable< KeyClass, nsRefPtr<RefPtr> , RefPtr* >
{
public:
  typedef typename KeyClass::KeyType KeyType;
  typedef RefPtr* UserDataType;
  typedef nsBaseHashtable< KeyClass, nsRefPtr<RefPtr> , RefPtr* > base_type;

  nsRefPtrHashtable()
  {
  }
  explicit nsRefPtrHashtable(uint32_t aInitSize)
    : nsBaseHashtable<KeyClass,nsRefPtr<RefPtr>,RefPtr*>(aInitSize)
  {
  }

  




  bool Get(KeyType aKey, UserDataType* pData) const;

  





  RefPtr* GetWeak(KeyType aKey, bool* aFound = nullptr) const;

  
  using base_type::Put;

  void Put(KeyType aKey, already_AddRefed<RefPtr> aData);

  bool Put(KeyType aKey, already_AddRefed<RefPtr> aData, const mozilla::fallible_t&) MOZ_WARN_UNUSED_RESULT;

  
  using base_type::Remove;

  






  bool Remove(KeyType aKey, UserDataType* pData);
};

template <typename K, typename T>
inline void
ImplCycleCollectionUnlink(nsRefPtrHashtable<K, T>& aField)
{
  aField.Clear();
}

template <typename K, typename T>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            nsRefPtrHashtable<K, T>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  nsBaseHashtableCCTraversalData userData(aCallback, aName, aFlags);

  aField.EnumerateRead(ImplCycleCollectionTraverse_EnumFunc<typename K::KeyType,T*>,
                       &userData);
}





template<class KeyClass, class RefPtr>
bool
nsRefPtrHashtable<KeyClass,RefPtr>::Get
  (KeyType aKey, UserDataType* pRefPtr) const
{
  typename base_type::EntryType* ent = this->GetEntry(aKey);

  if (ent) {
    if (pRefPtr) {
      *pRefPtr = ent->mData;

      NS_IF_ADDREF(*pRefPtr);
    }

    return true;
  }

  
  
  if (pRefPtr) {
    *pRefPtr = nullptr;
  }

  return false;
}

template<class KeyClass, class RefPtr>
RefPtr*
nsRefPtrHashtable<KeyClass,RefPtr>::GetWeak
  (KeyType aKey, bool* aFound) const
{
  typename base_type::EntryType* ent = this->GetEntry(aKey);

  if (ent) {
    if (aFound) {
      *aFound = true;
    }

    return ent->mData;
  }

  
  if (aFound) {
    *aFound = false;
  }

  return nullptr;
}

template<class KeyClass, class RefPtr>
void
nsRefPtrHashtable<KeyClass,RefPtr>::Put(KeyType aKey, already_AddRefed<RefPtr> aData)
{
  if (!Put(aKey, mozilla::Move(aData), mozilla::fallible_t())) {
    NS_ABORT_OOM(this->mTable.entrySize * this->mTable.entryCount);
  }
}

template<class KeyClass, class RefPtr>
bool
nsRefPtrHashtable<KeyClass,RefPtr>::Put(KeyType aKey,
                                        already_AddRefed<RefPtr> aData,
                                        const mozilla::fallible_t&)
{
  typename base_type::EntryType* ent = this->PutEntry(aKey);

  if (!ent) {
    return false;
  }

  ent->mData = aData;

  return true;
}

template<class KeyClass, class RefPtr>
bool
nsRefPtrHashtable<KeyClass,RefPtr>::Remove(KeyType aKey,
                                           UserDataType* pRefPtr)
{
  MOZ_ASSERT(pRefPtr);
  typename base_type::EntryType* ent = this->GetEntry(aKey);

  if (ent) {
    ent->mData.forget(pRefPtr);
    this->Remove(aKey);
    return true;
  }

  
  
  *pRefPtr = nullptr;
  return false;
}

#endif 
