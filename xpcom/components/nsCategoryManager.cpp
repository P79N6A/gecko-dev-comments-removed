





































#define PL_ARENA_CONST_ALIGN_MASK 7

#include "nsICategoryManager.h"
#include "nsCategoryManager.h"

#include "plarena.h"
#include "prio.h"
#include "prprf.h"
#include "prlock.h"
#include "nsCOMPtr.h"
#include "nsTHashtable.h"
#include "nsClassHashtable.h"
#include "nsIFactory.h"
#include "nsIStringEnumerator.h"
#include "nsSupportsPrimitives.h"
#include "nsServiceManagerUtils.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsQuickSort.h"
#include "nsEnumeratorUtils.h"
#include "nsIProxyObjectManager.h"
#include "nsThreadUtils.h"

using namespace mozilla;
class nsIComponentLoaderManager;













#define NS_CATEGORYMANAGER_ARENA_SIZE (1024 * 8)


char* ArenaStrdup(const char* s, PLArenaPool* aArena);





class BaseStringEnumerator
  : public nsISimpleEnumerator,
    private nsIUTF8StringEnumerator
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

NS_IMPL_ISUPPORTS2(BaseStringEnumerator, nsISimpleEnumerator, nsIUTF8StringEnumerator)

NS_IMETHODIMP
BaseStringEnumerator::HasMoreElements(PRBool *_retval)
{
  *_retval = (mSimpleCurItem < mCount);

  return NS_OK;
}

NS_IMETHODIMP
BaseStringEnumerator::GetNext(nsISupports **_retval)
{
  if (mSimpleCurItem >= mCount)
    return NS_ERROR_FAILURE;

  nsSupportsDependentCString* str =
    new nsSupportsDependentCString(mArray[mSimpleCurItem++]);
  if (!str)
    return NS_ERROR_OUT_OF_MEMORY;

  *_retval = str;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP
BaseStringEnumerator::HasMore(PRBool *_retval)
{
  *_retval = (mStringCurItem < mCount);

  return NS_OK;
}

NS_IMETHODIMP
BaseStringEnumerator::GetNext(nsACString& _retval)
{
  if (mStringCurItem >= mCount)
    return NS_ERROR_FAILURE;

  _retval = nsDependentCString(mArray[mStringCurItem++]);
  return NS_OK;
}

int
BaseStringEnumerator::SortCallback(const void *e1, const void *e2,
                                   void * )
{
  char const *const *s1 = reinterpret_cast<char const *const *>(e1);
  char const *const *s2 = reinterpret_cast<char const *const *>(e2);

  return strcmp(*s1, *s2);
}

void
BaseStringEnumerator::Sort()
{
  NS_QuickSort(mArray, mCount, sizeof(mArray[0]), SortCallback, nsnull);
}




class EntryEnumerator
  : public BaseStringEnumerator
{
public:
  static EntryEnumerator* Create(nsTHashtable<CategoryLeaf>& aTable);

private:
  static PLDHashOperator
    enumfunc_createenumerator(CategoryLeaf* aLeaf, void* userArg);
};


PLDHashOperator
EntryEnumerator::enumfunc_createenumerator(CategoryLeaf* aLeaf, void* userArg)
{
  EntryEnumerator* mythis = static_cast<EntryEnumerator*>(userArg);
  if (aLeaf->nonpValue)
    mythis->mArray[mythis->mCount++] = aLeaf->GetKey();

  return PL_DHASH_NEXT;
}

EntryEnumerator*
EntryEnumerator::Create(nsTHashtable<CategoryLeaf>& aTable)
{
  EntryEnumerator* enumObj = new EntryEnumerator();
  if (!enumObj)
    return nsnull;

  enumObj->mArray = new char const* [aTable.Count()];
  if (!enumObj->mArray) {
    delete enumObj;
    return nsnull;
  }

  aTable.EnumerateEntries(enumfunc_createenumerator, enumObj);

  enumObj->Sort();

  return enumObj;
}






CategoryNode*
CategoryNode::Create(PLArenaPool* aArena)
{
  CategoryNode* node = new(aArena) CategoryNode();
  if (!node)
    return nsnull;

  if (!node->mTable.Init()) {
    delete node;
    return nsnull;
  }

  return node;
}

CategoryNode::~CategoryNode()
{
}

void*
CategoryNode::operator new(size_t aSize, PLArenaPool* aArena)
{
  void* p;
  PL_ARENA_ALLOCATE(p, aArena, aSize);
  return p;
}

NS_METHOD
CategoryNode::GetLeaf(const char* aEntryName,
                      char** _retval)
{
  MutexAutoLock lock(mLock);
  nsresult rv = NS_ERROR_NOT_AVAILABLE;
  CategoryLeaf* ent =
    mTable.GetEntry(aEntryName);

  
  if (ent && ent->nonpValue) {
    *_retval = NS_strdup(ent->nonpValue);
    if (*_retval)
      rv = NS_OK;
  }

  return rv;
}

NS_METHOD
CategoryNode::AddLeaf(const char* aEntryName,
                      const char* aValue,
                      PRBool aPersist,
                      PRBool aReplace,
                      char** _retval,
                      PLArenaPool* aArena)
{
  MutexAutoLock lock(mLock);
  CategoryLeaf* leaf = 
    mTable.GetEntry(aEntryName);

  nsresult rv = NS_OK;
  if (leaf) {
    
    if (!aReplace && (leaf->nonpValue || (aPersist && leaf->pValue )))
      rv = NS_ERROR_INVALID_ARG;
  } else {
    const char* arenaEntryName = ArenaStrdup(aEntryName, aArena);
    if (!arenaEntryName) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
      leaf = mTable.PutEntry(arenaEntryName);
      if (!leaf)
        rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }

  if (NS_SUCCEEDED(rv)) {
    const char* arenaValue = ArenaStrdup(aValue, aArena);
    if (!arenaValue) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
      if (_retval) {
        const char *toDup = leaf->nonpValue ? leaf->nonpValue : leaf->pValue;
        if (toDup) {
          *_retval = ToNewCString(nsDependentCString(toDup));
          if (!*_retval)
            return NS_ERROR_OUT_OF_MEMORY;
        }
        else {
          *_retval = nsnull;
        }
      }

      leaf->nonpValue = arenaValue;
      if (aPersist)
        leaf->pValue = arenaValue;
    }
  }
    
  return rv;
}

NS_METHOD
CategoryNode::DeleteLeaf(const char* aEntryName,
                         PRBool aDontPersist)
{
  
  
  MutexAutoLock lock(mLock);

  if (aDontPersist) {
    
    mTable.RemoveEntry(aEntryName);
  } else {
    
    
    CategoryLeaf* leaf = mTable.GetEntry(aEntryName);
    if (leaf) {
      if (leaf->pValue) {
        leaf->nonpValue = nsnull;
      } else {
        
        mTable.RawRemoveEntry(leaf);
      }
    }
  }

  return NS_OK;
}

NS_METHOD 
CategoryNode::Enumerate(nsISimpleEnumerator **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  MutexAutoLock lock(mLock);
  EntryEnumerator* enumObj = EntryEnumerator::Create(mTable);

  if (!enumObj)
    return NS_ERROR_OUT_OF_MEMORY;

  *_retval = enumObj;
  NS_ADDREF(*_retval);
  return NS_OK;
}

struct persistent_userstruct {
  PRFileDesc* fd;
  const char* categoryName;
  PRBool      success;
};

PLDHashOperator
enumfunc_pentries(CategoryLeaf* aLeaf, void* userArg)
{
  persistent_userstruct* args =
    static_cast<persistent_userstruct*>(userArg);

  PLDHashOperator status = PL_DHASH_NEXT;

  if (aLeaf->pValue) {
    if (PR_fprintf(args->fd,
                   "%s,%s,%s\n",
                   args->categoryName,
                   aLeaf->GetKey(),
                   aLeaf->pValue) == (PRUint32) -1) {
      args->success = PR_FALSE;
      status = PL_DHASH_STOP;
    }
  }

  return status;
}

PRBool
CategoryNode::WritePersistentEntries(PRFileDesc* fd, const char* aCategoryName)
{
  persistent_userstruct args = {
    fd,
    aCategoryName,
    PR_TRUE
  };

  MutexAutoLock lock(mLock);
  mTable.EnumerateEntries(enumfunc_pentries, &args);

  return args.success;
}






class CategoryEnumerator
  : public BaseStringEnumerator
{
public:
  static CategoryEnumerator* Create(nsClassHashtable<nsDepCharHashKey, CategoryNode>& aTable);

private:
  static PLDHashOperator
  enumfunc_createenumerator(const char* aStr,
                            CategoryNode* aNode,
                            void* userArg);
};

CategoryEnumerator*
CategoryEnumerator::Create(nsClassHashtable<nsDepCharHashKey, CategoryNode>& aTable)
{
  CategoryEnumerator* enumObj = new CategoryEnumerator();
  if (!enumObj)
    return nsnull;

  enumObj->mArray = new const char* [aTable.Count()];
  if (!enumObj->mArray) {
    delete enumObj;
    return nsnull;
  }

  aTable.EnumerateRead(enumfunc_createenumerator, enumObj);

  return enumObj;
}

PLDHashOperator
CategoryEnumerator::enumfunc_createenumerator(const char* aStr, CategoryNode* aNode, void* userArg)
{
  CategoryEnumerator* mythis = static_cast<CategoryEnumerator*>(userArg);

  
  if (aNode->Count())
    mythis->mArray[mythis->mCount++] = aStr;

  return PL_DHASH_NEXT;
}






NS_IMPL_THREADSAFE_ISUPPORTS1(nsCategoryManager, nsICategoryManager)

nsCategoryManager*
nsCategoryManager::Create()
{
  nsCategoryManager* manager = new nsCategoryManager();
  
  if (!manager)
    return nsnull;

  PL_INIT_ARENA_POOL(&(manager->mArena), "CategoryManagerArena",
                     NS_CATEGORYMANAGER_ARENA_SIZE); 

  if (!manager->mTable.Init()) {
    delete manager;
    return nsnull;
  }

  return manager;
}

nsCategoryManager::~nsCategoryManager()
{
  
  
  
  mTable.Clear();

  PL_FinishArenaPool(&mArena);
}

inline CategoryNode*
nsCategoryManager::get_category(const char* aName) {
  CategoryNode* node;
  if (!mTable.Get(aName, &node)) {
    return nsnull;
  }
  return node;
}

void
nsCategoryManager::NotifyObservers( const char *aTopic,
                                    const char *aCategoryName,
                                    const char *aEntryName )
{
  if (mSuppressNotifications)
    return;

  nsCOMPtr<nsIObserverService> observerService
    (do_GetService("@mozilla.org/observer-service;1"));
  if (!observerService)
    return;

  nsCOMPtr<nsIObserverService> obsProxy;
  NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                       NS_GET_IID(nsIObserverService),
                       observerService,
                       NS_PROXY_ASYNC,
                       getter_AddRefs(obsProxy));
  if (!obsProxy)
    return;

  if (aEntryName) {
    nsCOMPtr<nsISupportsCString> entry
      (do_CreateInstance (NS_SUPPORTS_CSTRING_CONTRACTID));
    if (!entry)
      return;

    nsresult rv = entry->SetData(nsDependentCString(aEntryName));
    if (NS_FAILED(rv))
      return;

    obsProxy->NotifyObservers(entry, aTopic,
                              NS_ConvertUTF8toUTF16(aCategoryName).get());
  } else {
    obsProxy->NotifyObservers(this, aTopic,
                              NS_ConvertUTF8toUTF16(aCategoryName).get());
  }
}

NS_IMETHODIMP
nsCategoryManager::GetCategoryEntry( const char *aCategoryName,
                                     const char *aEntryName,
                                     char **_retval )
{
  NS_ENSURE_ARG_POINTER(aCategoryName);
  NS_ENSURE_ARG_POINTER(aEntryName);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult status = NS_ERROR_NOT_AVAILABLE;

  CategoryNode* category;
  {
    MutexAutoLock lock(mLock);
    category = get_category(aCategoryName);
  }

  if (category) {
    status = category->GetLeaf(aEntryName, _retval);
  }

  return status;
}

NS_IMETHODIMP
nsCategoryManager::AddCategoryEntry( const char *aCategoryName,
                                     const char *aEntryName,
                                     const char *aValue,
                                     PRBool aPersist,
                                     PRBool aReplace,
                                     char **_retval )
{
  NS_ENSURE_ARG_POINTER(aCategoryName);
  NS_ENSURE_ARG_POINTER(aEntryName);
  NS_ENSURE_ARG_POINTER(aValue);

  
  
  CategoryNode* category;
  {
    MutexAutoLock lock(mLock);
    category = get_category(aCategoryName);

    if (!category) {
      
      category = CategoryNode::Create(&mArena);
        
      char* categoryName = ArenaStrdup(aCategoryName, &mArena);
      mTable.Put(categoryName, category);
    }
  }

  if (!category)
    return NS_ERROR_OUT_OF_MEMORY;

  
  char *oldEntry = nsnull;

  nsresult rv = category->AddLeaf(aEntryName,
                                  aValue,
                                  aPersist,
                                  aReplace,
                                  &oldEntry,
                                  &mArena);

  if (NS_SUCCEEDED(rv)) {
    if (oldEntry) {
      NotifyObservers(NS_XPCOM_CATEGORY_ENTRY_REMOVED_OBSERVER_ID,
                      aCategoryName, oldEntry);
    }
    NotifyObservers(NS_XPCOM_CATEGORY_ENTRY_ADDED_OBSERVER_ID,
                    aCategoryName, aEntryName);

    if (_retval)
      *_retval = oldEntry;
    else if (oldEntry)
      nsMemory::Free(oldEntry);
  }

  return rv;
}

NS_IMETHODIMP
nsCategoryManager::DeleteCategoryEntry( const char *aCategoryName,
                                        const char *aEntryName,
                                        PRBool aDontPersist)
{
  NS_ENSURE_ARG_POINTER(aCategoryName);
  NS_ENSURE_ARG_POINTER(aEntryName);

  





  CategoryNode* category;
  {
    MutexAutoLock lock(mLock);
    category = get_category(aCategoryName);
  }

  if (!category)
    return NS_OK;

  nsresult rv = category->DeleteLeaf(aEntryName,
                                     aDontPersist);

  if (NS_SUCCEEDED(rv)) {
    NotifyObservers(NS_XPCOM_CATEGORY_ENTRY_REMOVED_OBSERVER_ID,
                    aCategoryName, aEntryName);
  }

  return rv;
}

NS_IMETHODIMP
nsCategoryManager::DeleteCategory( const char *aCategoryName )
{
  NS_ENSURE_ARG_POINTER(aCategoryName);

  
  
  

  CategoryNode* category;
  {
    MutexAutoLock lock(mLock);
    category = get_category(aCategoryName);
  }

  if (category) {
    category->Clear();
    NotifyObservers(NS_XPCOM_CATEGORY_CLEARED_OBSERVER_ID,
                    aCategoryName, nsnull);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCategoryManager::EnumerateCategory( const char *aCategoryName,
                                      nsISimpleEnumerator **_retval )
{
  NS_ENSURE_ARG_POINTER(aCategoryName);
  NS_ENSURE_ARG_POINTER(_retval);

  CategoryNode* category;
  {
    MutexAutoLock lock(mLock);
    category = get_category(aCategoryName);
  }
  
  if (!category) {
    return NS_NewEmptyEnumerator(_retval);
  }

  return category->Enumerate(_retval);
}

NS_IMETHODIMP 
nsCategoryManager::EnumerateCategories(nsISimpleEnumerator **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  MutexAutoLock lock(mLock);
  CategoryEnumerator* enumObj = CategoryEnumerator::Create(mTable);

  if (!enumObj)
    return NS_ERROR_OUT_OF_MEMORY;

  *_retval = enumObj;
  NS_ADDREF(*_retval);
  return NS_OK;
}

struct writecat_struct {
  PRFileDesc* fd;
  PRBool      success;
};

PLDHashOperator
enumfunc_categories(const char* aKey, CategoryNode* aCategory, void* userArg)
{
  writecat_struct* args = static_cast<writecat_struct*>(userArg);

  PLDHashOperator result = PL_DHASH_NEXT;

  if (!aCategory->WritePersistentEntries(args->fd, aKey)) {
    args->success = PR_FALSE;
    result = PL_DHASH_STOP;
  }

  return result;
}

NS_METHOD
nsCategoryManager::WriteCategoryManagerToRegistry(PRFileDesc* fd)
{
  writecat_struct args = {
    fd,
    PR_TRUE
  };

  MutexAutoLock lock(mLock);
  mTable.EnumerateRead(enumfunc_categories, &args);

  if (!args.success) {
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

NS_METHOD
nsCategoryManager::SuppressNotifications(PRBool aSuppress)
{
  mSuppressNotifications = aSuppress;
  return NS_OK;
}

class nsCategoryManagerFactory : public nsIFactory
   {
     public:
       nsCategoryManagerFactory() { }

       NS_DECL_ISUPPORTS
       NS_DECL_NSIFACTORY
   };

NS_IMPL_ISUPPORTS1(nsCategoryManagerFactory, nsIFactory)

NS_IMETHODIMP
nsCategoryManagerFactory::CreateInstance( nsISupports* aOuter, const nsIID& aIID, void** aResult )
  {
    NS_ENSURE_ARG_POINTER(aResult);

    *aResult = 0;

    nsresult status = NS_OK;
    if ( aOuter )
      status = NS_ERROR_NO_AGGREGATION;
    else
      {
        nsCategoryManager* raw_category_manager = nsCategoryManager::Create();
        nsCOMPtr<nsICategoryManager> new_category_manager = raw_category_manager;
        if ( new_category_manager )
              status = new_category_manager->QueryInterface(aIID, aResult);
        else
          status = NS_ERROR_OUT_OF_MEMORY;
      }

    return status;
  }

NS_IMETHODIMP
nsCategoryManagerFactory::LockFactory( PRBool )
  {
      
    return NS_OK;
  }

nsresult
NS_CategoryManagerGetFactory( nsIFactory** aFactory )
  {
    

    nsresult status;

    *aFactory = 0;
    nsIFactory* new_factory = static_cast<nsIFactory*>(new nsCategoryManagerFactory);
    if (new_factory)
      {
        *aFactory = new_factory;
        NS_ADDREF(*aFactory);
        status = NS_OK;
      }
    else
      status = NS_ERROR_OUT_OF_MEMORY;

    return status;
  }












NS_COM nsresult
NS_CreateServicesFromCategory(const char *category,
                              nsISupports *origin,
                              const char *observerTopic)
{
    nsresult rv = NS_OK;
    
    int nFailed = 0; 
    nsCOMPtr<nsICategoryManager> categoryManager = 
        do_GetService("@mozilla.org/categorymanager;1", &rv);
    if (!categoryManager) return rv;

    nsCOMPtr<nsISimpleEnumerator> enumerator;
    rv = categoryManager->EnumerateCategory(category, 
            getter_AddRefs(enumerator));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISupports> entry;
    while (NS_SUCCEEDED(enumerator->GetNext(getter_AddRefs(entry)))) {
        
        nsCOMPtr<nsISupportsCString> catEntry = do_QueryInterface(entry, &rv);
        if (NS_FAILED(rv)) {
            nFailed++;
            continue;
        }
        nsCAutoString entryString;
        rv = catEntry->GetData(entryString);
        if (NS_FAILED(rv)) {
            nFailed++;
            continue;
        }
        nsXPIDLCString contractID;
        rv = categoryManager->GetCategoryEntry(category,entryString.get(), getter_Copies(contractID));
        if (NS_FAILED(rv)) {
            nFailed++;
            continue;
        }
        
        nsCOMPtr<nsISupports> instance = do_GetService(contractID, &rv);
        if (NS_FAILED(rv)) {
            nFailed++;
            continue;
        }

        if (observerTopic) {
            
            nsCOMPtr<nsIObserver> observer = do_QueryInterface(instance, &rv);
            if (NS_SUCCEEDED(rv) && observer)
                observer->Observe(origin, observerTopic, EmptyString().get());
        }
    }
    return (nFailed ? NS_ERROR_FAILURE : NS_OK);
}
