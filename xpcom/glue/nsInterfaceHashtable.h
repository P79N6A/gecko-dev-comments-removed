




#ifndef nsInterfaceHashtable_h__
#define nsInterfaceHashtable_h__

#include "nsBaseHashtable.h"
#include "nsHashKeys.h"
#include "nsCOMPtr.h"









template<class KeyClass,class Interface>
class nsInterfaceHashtable :
  public nsBaseHashtable< KeyClass, nsCOMPtr<Interface> , Interface* >
{
public:
  typedef typename KeyClass::KeyType KeyType;
  typedef Interface* UserDataType;
  typedef nsBaseHashtable< KeyClass, nsCOMPtr<Interface> , Interface* >
          base_type;

  




  bool Get(KeyType aKey, UserDataType* pData) const;

  


  already_AddRefed<Interface> Get(KeyType aKey) const;

  





  Interface* GetWeak(KeyType aKey, bool* aFound = nullptr) const;
};

template <typename K, typename T>
inline void
ImplCycleCollectionUnlink(nsInterfaceHashtable<K, T>& aField)
{
  aField.Clear();
}

template <typename K, typename T>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            nsInterfaceHashtable<K, T>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  nsBaseHashtableCCTraversalData userData(aCallback, aName, aFlags);

  aField.EnumerateRead(ImplCycleCollectionTraverse_EnumFunc<typename K::KeyType,T*>,
                       &userData);
}







template<class KeyClass,class Interface>
class nsInterfaceHashtableMT :
  public nsBaseHashtableMT< KeyClass, nsCOMPtr<Interface> , Interface* >
{
public:
  typedef typename KeyClass::KeyType KeyType;
  typedef Interface* UserDataType;
  typedef nsBaseHashtableMT< KeyClass, nsCOMPtr<Interface> , Interface* >
          base_type;

  




  bool Get(KeyType aKey, UserDataType* pData) const;

  
  
  
};






template<class KeyClass,class Interface>
bool
nsInterfaceHashtable<KeyClass,Interface>::Get
  (KeyType aKey, UserDataType* pInterface) const
{
  typename base_type::EntryType* ent = this->GetEntry(aKey);

  if (ent)
  {
    if (pInterface)
    {
      *pInterface = ent->mData;

      NS_IF_ADDREF(*pInterface);
    }

    return true;
  }

  
  
  if (pInterface)
    *pInterface = nullptr;

  return false;
}

template<class KeyClass, class Interface>
already_AddRefed<Interface>
nsInterfaceHashtable<KeyClass,Interface>::Get(KeyType aKey) const
{
  typename base_type::EntryType* ent = this->GetEntry(aKey);
  if (!ent)
    return NULL;

  nsCOMPtr<Interface> copy = ent->mData;
  return copy.forget();
}

template<class KeyClass,class Interface>
Interface*
nsInterfaceHashtable<KeyClass,Interface>::GetWeak
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





template<class KeyClass,class Interface>
bool
nsInterfaceHashtableMT<KeyClass,Interface>::Get
  (KeyType aKey, UserDataType* pInterface) const
{
  PR_Lock(this->mLock);

  typename base_type::EntryType* ent = this->GetEntry(aKey);

  if (ent)
  {
    if (pInterface)
    {
      *pInterface = ent->mData;

      NS_IF_ADDREF(*pInterface);
    }

    PR_Unlock(this->mLock);

    return true;
  }

  
  
  if (pInterface)
    *pInterface = nullptr;

  PR_Unlock(this->mLock);

  return false;
}

#endif 
