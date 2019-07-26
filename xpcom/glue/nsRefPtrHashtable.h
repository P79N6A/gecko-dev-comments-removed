




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

  




  bool Get(KeyType aKey, UserDataType* pData) const;

  





  RefPtr* GetWeak(KeyType aKey, bool* aFound = nullptr) const;
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

  if (ent)
  {
    if (pRefPtr)
    {
      *pRefPtr = ent->mData;

      NS_IF_ADDREF(*pRefPtr);
    }

    return true;
  }

  
  
  if (pRefPtr)
    *pRefPtr = nullptr;

  return false;
}

template<class KeyClass, class RefPtr>
RefPtr*
nsRefPtrHashtable<KeyClass,RefPtr>::GetWeak
  (KeyType aKey, bool* aFound) const
{
  typename base_type::EntryType* ent = this->GetEntry(aKey);

  if (ent)
  {
    if (aFound)
      *aFound = true;

    return ent->mData;
  }

  
  if (aFound)
    *aFound = false;
  return nullptr;
}

#endif 
