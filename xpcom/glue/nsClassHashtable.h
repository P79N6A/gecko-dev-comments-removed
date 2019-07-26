




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
  typedef nsBaseHashtable< KeyClass, nsAutoPtr<T>, T* > base_type;

  nsClassHashtable()
  {
  }
  explicit nsClassHashtable(uint32_t aInitSize)
    : nsBaseHashtable<KeyClass,nsAutoPtr<T>,T*>(aInitSize)
  {
  }

  



  bool Get(KeyType aKey, UserDataType* pData) const;

  



  UserDataType Get(KeyType aKey) const;

  









  void RemoveAndForget(KeyType aKey, nsAutoPtr<T> &aOut);
};





template<class KeyClass,class T>
bool
nsClassHashtable<KeyClass,T>::Get(KeyType aKey, T** retVal) const
{
  typename base_type::EntryType* ent = this->GetEntry(aKey);

  if (ent)
  {
    if (retVal)
      *retVal = ent->mData;

    return true;
  }

  if (retVal)
    *retVal = nullptr;

  return false;
}

template<class KeyClass,class T>
T*
nsClassHashtable<KeyClass,T>::Get(KeyType aKey) const
{
  typename base_type::EntryType* ent = this->GetEntry(aKey);

  if (!ent)
    return NULL;

  return ent->mData;
}

template<class KeyClass,class T>
void
nsClassHashtable<KeyClass,T>::RemoveAndForget(KeyType aKey, nsAutoPtr<T> &aOut)
{
  aOut = nullptr;
  nsAutoPtr<T> ptr;

  typename base_type::EntryType *ent = this->GetEntry(aKey);
  if (!ent)
    return;

  
  aOut = ent->mData;

  this->Remove(aKey);
}

#endif 
