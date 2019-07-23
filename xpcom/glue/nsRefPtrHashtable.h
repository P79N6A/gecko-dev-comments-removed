





































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

  




  PRBool Get(KeyType aKey, UserDataType* pData) const;

  





  RefPtr* GetWeak(KeyType aKey, PRBool* aFound = nsnull) const;
};







template<class KeyClass, class RefPtr>
class nsRefPtrHashtableMT :
  public nsBaseHashtableMT< KeyClass, nsRefPtr<RefPtr> , RefPtr* >
{
public:
  typedef typename KeyClass::KeyType KeyType;
  typedef RefPtr* UserDataType;

  




  PRBool Get(KeyType aKey, UserDataType* pData) const;

  
  
  
};






template<class KeyClass, class RefPtr>
PRBool
nsRefPtrHashtable<KeyClass,RefPtr>::Get
  (KeyType aKey, UserDataType* pRefPtr) const
{
  typename nsBaseHashtable<KeyClass, nsRefPtr<RefPtr>, RefPtr*>::EntryType* ent =
    GetEntry(aKey);

  if (ent)
  {
    if (pRefPtr)
    {
      *pRefPtr = ent->mData;

      NS_IF_ADDREF(*pRefPtr);
    }

    return PR_TRUE;
  }

  
  
  if (pRefPtr)
    *pRefPtr = nsnull;

  return PR_FALSE;
}

template<class KeyClass, class RefPtr>
RefPtr*
nsRefPtrHashtable<KeyClass,RefPtr>::GetWeak
  (KeyType aKey, PRBool* aFound) const
{
  typename nsBaseHashtable<KeyClass, nsRefPtr<RefPtr>, RefPtr*>::EntryType* ent =
    GetEntry(aKey);

  if (ent)
  {
    if (aFound)
      *aFound = PR_TRUE;

    return ent->mData;
  }

  
  if (aFound)
    *aFound = PR_FALSE;
  return nsnull;
}





template<class KeyClass, class RefPtr>
PRBool
nsRefPtrHashtableMT<KeyClass,RefPtr>::Get
  (KeyType aKey, UserDataType* pRefPtr) const
{
  PR_Lock(this->mLock);

  typename nsBaseHashtableMT<KeyClass, nsRefPtr<RefPtr>, RefPtr*>::EntryType* ent =
    GetEntry(aKey);

  if (ent)
  {
    if (pRefPtr)
    {
      *pRefPtr = ent->mData;

      NS_IF_ADDREF(*pRefPtr);
    }

    PR_Unlock(this->mLock);

    return PR_TRUE;
  }

  
  
  if (pRefPtr)
    *pRefPtr = nsnull;

  PR_Unlock(this->mLock);

  return PR_FALSE;
}

#endif 
