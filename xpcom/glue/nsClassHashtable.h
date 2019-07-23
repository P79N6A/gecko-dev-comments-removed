




































#ifndef nsClassHashtable_h__
#define nsClassHashtable_h__

#include "nsBaseHashtable.h"
#include "nsHashKeys.h"
#include "nsAutoPtr.h"









template<class KeyClass,class T>
class nsClassHashtable :
  public nsBaseHashtable< KeyClass, nsAutoPtr<T>, T* >
{
public:
  typedef typename KeyClass::KeyType KeyType;
  typedef T* UserDataType;

  



  PRBool Get(KeyType aKey, UserDataType* pData) const;
};









template<class KeyClass,class T>
class nsClassHashtableMT :
  public nsBaseHashtableMT< KeyClass, nsAutoPtr<T>, T* >
{
public:
  typedef typename KeyClass::KeyType KeyType;
  typedef T* UserDataType;

  



  PRBool Get(KeyType aKey, UserDataType* pData) const;
};






template<class KeyClass,class T>
PRBool
nsClassHashtable<KeyClass,T>::Get(KeyType aKey, T** retVal) const
{
  typename nsBaseHashtable<KeyClass,nsAutoPtr<T>,T*>::EntryType* ent =
    GetEntry(aKey);

  if (ent)
  {
    if (retVal)
      *retVal = ent->mData;

    return PR_TRUE;
  }

  if (retVal)
    *retVal = nsnull;

  return PR_FALSE;
}






template<class KeyClass,class T>
PRBool
nsClassHashtableMT<KeyClass,T>::Get(KeyType aKey, T** retVal) const
{
  PR_Lock(this->mLock);

  typename nsBaseHashtableMT<KeyClass,nsAutoPtr<T>,T*>::EntryType* ent =
    GetEntry(aKey);

  if (ent)
  {
    if (retVal)
      *retVal = ent->mData;

    PR_Unlock(this->mLock);

    return PR_TRUE;
  }

  if (retVal)
    *retVal = nsnull;

  PR_Unlock(this->mLock);

  return PR_FALSE;
}

#endif 
