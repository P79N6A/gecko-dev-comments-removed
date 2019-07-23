




































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

  




  PRBool Get(KeyType aKey, UserDataType* pData) const;

  





  Interface* GetWeak(KeyType aKey, PRBool* aFound = nsnull) const;
};







template<class KeyClass,class Interface>
class nsInterfaceHashtableMT :
  public nsBaseHashtableMT< KeyClass, nsCOMPtr<Interface> , Interface* >
{
public:
  typedef typename KeyClass::KeyType KeyType;
  typedef Interface* UserDataType;

  




  PRBool Get(KeyType aKey, UserDataType* pData) const;

  
  
  
};






template<class KeyClass,class Interface>
PRBool
nsInterfaceHashtable<KeyClass,Interface>::Get
  (KeyType aKey, UserDataType* pInterface) const
{
  typename nsBaseHashtable<KeyClass, nsCOMPtr<Interface>, Interface*>::EntryType* ent =
    GetEntry(aKey);

  if (ent)
  {
    if (pInterface)
    {
      *pInterface = ent->mData;

      NS_IF_ADDREF(*pInterface);
    }

    return PR_TRUE;
  }

  
  
  if (pInterface)
    *pInterface = nsnull;

  return PR_FALSE;
}

template<class KeyClass,class Interface>
Interface*
nsInterfaceHashtable<KeyClass,Interface>::GetWeak
  (KeyType aKey, PRBool* aFound) const
{
  typename nsBaseHashtable<KeyClass, nsCOMPtr<Interface>, Interface*>::EntryType* ent =
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





template<class KeyClass,class Interface>
PRBool
nsInterfaceHashtableMT<KeyClass,Interface>::Get
  (KeyType aKey, UserDataType* pInterface) const
{
  PR_Lock(this->mLock);

  typename nsBaseHashtableMT<KeyClass, nsCOMPtr<Interface>, Interface*>::EntryType* ent =
    GetEntry(aKey);

  if (ent)
  {
    if (pInterface)
    {
      *pInterface = ent->mData;

      NS_IF_ADDREF(*pInterface);
    }

    PR_Unlock(this->mLock);

    return PR_TRUE;
  }

  
  
  if (pInterface)
    *pInterface = nsnull;

  PR_Unlock(this->mLock);

  return PR_FALSE;
}

#endif 
