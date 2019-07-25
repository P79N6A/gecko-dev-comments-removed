





































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

  





  RefPtr* GetWeak(KeyType aKey, bool* aFound = nsnull) const;
};







template<class KeyClass, class RefPtr>
class nsRefPtrHashtableMT :
  public nsBaseHashtableMT< KeyClass, nsRefPtr<RefPtr> , RefPtr* >
{
public:
  typedef typename KeyClass::KeyType KeyType;
  typedef RefPtr* UserDataType;
  typedef nsBaseHashtableMT< KeyClass, nsRefPtr<RefPtr> , RefPtr* > base_type;

  




  bool Get(KeyType aKey, UserDataType* pData) const;

  
  
  
};






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
    *pRefPtr = nsnull;

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
  return nsnull;
}





template<class KeyClass, class RefPtr>
bool
nsRefPtrHashtableMT<KeyClass,RefPtr>::Get
  (KeyType aKey, UserDataType* pRefPtr) const
{
  PR_Lock(this->mLock);

  typename base_type::EntryType* ent = this->GetEntry(aKey);

  if (ent)
  {
    if (pRefPtr)
    {
      *pRefPtr = ent->mData;

      NS_IF_ADDREF(*pRefPtr);
    }

    PR_Unlock(this->mLock);

    return true;
  }

  
  
  if (pRefPtr)
    *pRefPtr = nsnull;

  PR_Unlock(this->mLock);

  return false;
}

#endif 
