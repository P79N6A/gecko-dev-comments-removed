





#ifndef nsInterfaceHashtable_h__
#define nsInterfaceHashtable_h__

#include "nsBaseHashtable.h"
#include "nsHashKeys.h"
#include "nsCOMPtr.h"









template<class KeyClass, class Interface>
class nsInterfaceHashtable
  : public nsBaseHashtable<KeyClass, nsCOMPtr<Interface>, Interface*>
{
public:
  typedef typename KeyClass::KeyType KeyType;
  typedef Interface* UserDataType;
  typedef nsBaseHashtable<KeyClass, nsCOMPtr<Interface>, Interface*> base_type;

  nsInterfaceHashtable() {}
  explicit nsInterfaceHashtable(uint32_t aInitLength)
    : nsBaseHashtable<KeyClass, nsCOMPtr<Interface>, Interface*>(aInitLength)
  {
  }

  




  bool Get(KeyType aKey, UserDataType* aData) const;

  


  already_AddRefed<Interface> Get(KeyType aKey) const;

  





  Interface* GetWeak(KeyType aKey, bool* aFound = nullptr) const;
};

template<typename K, typename T>
inline void
ImplCycleCollectionUnlink(nsInterfaceHashtable<K, T>& aField)
{
  aField.Clear();
}

template<typename K, typename T>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            const nsInterfaceHashtable<K, T>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  nsBaseHashtableCCTraversalData userData(aCallback, aName, aFlags);

  aField.EnumerateRead(ImplCycleCollectionTraverse_EnumFunc<typename K::KeyType, T*>,
                       &userData);
}





template<class KeyClass, class Interface>
bool
nsInterfaceHashtable<KeyClass, Interface>::Get(KeyType aKey,
                                               UserDataType* aInterface) const
{
  typename base_type::EntryType* ent = this->GetEntry(aKey);

  if (ent) {
    if (aInterface) {
      *aInterface = ent->mData;

      NS_IF_ADDREF(*aInterface);
    }

    return true;
  }

  
  
  if (aInterface) {
    *aInterface = nullptr;
  }

  return false;
}

template<class KeyClass, class Interface>
already_AddRefed<Interface>
nsInterfaceHashtable<KeyClass, Interface>::Get(KeyType aKey) const
{
  typename base_type::EntryType* ent = this->GetEntry(aKey);
  if (!ent) {
    return nullptr;
  }

  nsCOMPtr<Interface> copy = ent->mData;
  return copy.forget();
}

template<class KeyClass, class Interface>
Interface*
nsInterfaceHashtable<KeyClass, Interface>::GetWeak(KeyType aKey,
                                                   bool* aFound) const
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

#endif 
