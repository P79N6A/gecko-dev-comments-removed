





#ifndef NSCATEGORYMANAGER_H
#define NSCATEGORYMANAGER_H

#include "prio.h"
#include "plarena.h"
#include "nsClassHashtable.h"
#include "nsICategoryManager.h"
#include "mozilla/Mutex.h"
#include "mozilla/Attributes.h"

class nsIMemoryReporter;


#define NS_CATEGORYMANAGER_CID \
{ 0x16d222a6, 0x1dd2, 0x11b2, \
  {0xb6, 0x93, 0xf3, 0x8b, 0x02, 0xc0, 0x21, 0xb2} }









class CategoryLeaf : public nsDepCharHashKey
{
public:
  CategoryLeaf(const char* aKey)
    : nsDepCharHashKey(aKey),
      value(NULL) { }
  const char* value;
};







class CategoryNode
{
public:
  NS_METHOD GetLeaf(const char* aEntryName,
                    char** _retval);

  NS_METHOD AddLeaf(const char* aEntryName,
                    const char* aValue,
                    bool aReplace,
                    char** _retval,
                    PLArenaPool* aArena);

  void DeleteLeaf(const char* aEntryName);

  void Clear() {
    mozilla::MutexAutoLock lock(mLock);
    mTable.Clear();
  }

  uint32_t Count() {
    mozilla::MutexAutoLock lock(mLock);
    uint32_t tCount = mTable.Count();
    return tCount;
  }

  NS_METHOD Enumerate(nsISimpleEnumerator** _retval);

  
  static CategoryNode* Create(PLArenaPool* aArena);
  ~CategoryNode();
  void operator delete(void*) { }

  size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf);

private:
  CategoryNode()
    : mLock("CategoryLeaf")
  { }

  void* operator new(size_t aSize, PLArenaPool* aArena);

  nsTHashtable<CategoryLeaf> mTable;
  mozilla::Mutex mLock;
};







class nsCategoryManager MOZ_FINAL
  : public nsICategoryManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICATEGORYMANAGER

  




  NS_METHOD SuppressNotifications(bool aSuppress);

  void AddCategoryEntry(const char* aCategory,
                        const char* aKey,
                        const char* aValue,
                        bool aReplace = true,
                        char** aOldValue = NULL);

  static nsresult Create(nsISupports* aOuter, REFNSIID aIID, void** aResult);
  void InitMemoryReporter();

  static nsCategoryManager* GetSingleton();
  static void Destroy();

  static int64_t GetCategoryManagerSize();
  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf);

private:
  static nsCategoryManager* gCategoryManager;

  nsCategoryManager();
  ~nsCategoryManager();

  CategoryNode* get_category(const char* aName);
  void NotifyObservers(const char* aTopic,
                       const char* aCategoryName, 
                       const char* aEntryName);

  PLArenaPool mArena;
  nsClassHashtable<nsDepCharHashKey, CategoryNode> mTable;
  mozilla::Mutex mLock;
  bool mSuppressNotifications;

  nsIMemoryReporter* mReporter;
};

#endif
