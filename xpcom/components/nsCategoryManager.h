





































#ifndef NSCATEGORYMANAGER_H
#define NSCATEGORYMANAGER_H

#include "prio.h"
#include "prlock.h"
#include "plarena.h"
#include "nsClassHashtable.h"
#include "nsICategoryManager.h"
#include "nsIFactory.h"
#include "nsISimpleEnumerator.h"
#include "nsIStringEnumerator.h"

#define NS_CATEGORYMANAGER_CLASSNAME     "Category Manager"


#define NS_CATEGORYMANAGER_CID \
{ 0x16d222a6, 0x1dd2, 0x11b2, \
  {0xb6, 0x93, 0xf3, 0x8b, 0x02, 0xc0, 0x21, 0xb2} }









class CategoryLeaf : public nsDepCharHashKey
{
public:
  CategoryLeaf(const char* aKey)
    : nsDepCharHashKey(aKey),
      pValue(nsnull),
      nonpValue(nsnull) { }
  const char* pValue;
  const char* nonpValue;
};







class CategoryNode
{
public:
  NS_METHOD GetLeaf(const char* aEntryName,
                    char** _retval);

  NS_METHOD AddLeaf(const char* aEntryName,
                    const char* aValue,
                    PRBool aPersist,
                    PRBool aReplace,
                    char** _retval,
                    PLArenaPool* aArena);

  NS_METHOD DeleteLeaf(const char* aEntryName,
                       PRBool aDontPersist);

  void Clear() {
    PR_Lock(mLock);
    mTable.Clear();
    PR_Unlock(mLock);
  }

  PRUint32 Count() {
    PR_Lock(mLock);
    PRUint32 tCount = mTable.Count();
    PR_Unlock(mLock);
    return tCount;
  }

  NS_METHOD Enumerate(nsISimpleEnumerator** _retval);

  PRBool WritePersistentEntries(PRFileDesc* fd, const char* aCategoryName);

  
  static CategoryNode* Create(PLArenaPool* aArena);
  ~CategoryNode();
  void operator delete(void*) { }

private:
  CategoryNode() { }
  void* operator new(size_t aSize, PLArenaPool* aArena);

  nsTHashtable<CategoryLeaf> mTable;
  PRLock* mLock;
};







class nsCategoryManager
  : public nsICategoryManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICATEGORYMANAGER

  



  NS_METHOD WriteCategoryManagerToRegistry(PRFileDesc* fd);

  




  NS_METHOD SuppressNotifications(PRBool aSuppress);

  nsCategoryManager()
    : mSuppressNotifications(PR_FALSE) { }
private:
  friend class nsCategoryManagerFactory;
  static nsCategoryManager* Create();

  ~nsCategoryManager();

  CategoryNode* get_category(const char* aName);
  void NotifyObservers(const char* aTopic,
                       const char* aCategoryName,
                       const char* aEntryName);

  PLArenaPool mArena;
  nsClassHashtable<nsDepCharHashKey, CategoryNode> mTable;
  PRLock* mLock;
  PRBool mSuppressNotifications;
};

class nsCategoryManagerFactory : public nsIFactory
   {
     public:
       nsCategoryManagerFactory() { }

       NS_DECL_ISUPPORTS
       NS_DECL_NSIFACTORY
   };





class BaseStringEnumerator
  : public nsISimpleEnumerator,
           nsIUTF8StringEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR
  NS_DECL_NSIUTF8STRINGENUMERATOR

protected:
  
  static int SortCallback(const void *, const void *, void *);

  BaseStringEnumerator()
    : mArray(nsnull),
      mCount(0),
      mSimpleCurItem(0),
      mStringCurItem(0) { }

  
  

  virtual ~BaseStringEnumerator()
  {
    if (mArray)
      delete[] mArray;
  }

  void Sort();

  const char** mArray;
  PRUint32 mCount;
  PRUint32 mSimpleCurItem;
  PRUint32 mStringCurItem;
};

#endif
