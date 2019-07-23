



































#ifndef nsCategoryCache_h_
#define nsCategoryCache_h_

#include "nsICategoryManager.h"
#include "nsIObserver.h"
#include "nsISimpleEnumerator.h"
#include "nsISupportsPrimitives.h"

#include "nsServiceManagerUtils.h"

#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsDataHashtable.h"

#include "nsXPCOM.h"

class NS_NO_VTABLE nsCategoryListener {
  protected:
    
    
    ~nsCategoryListener() {}

  public:
    virtual void EntryAdded(const nsCString& aValue) = 0;
    virtual void EntryRemoved(const nsCString& aValue) = 0;
    virtual void CategoryCleared() = 0;
};

class NS_COM_GLUE nsCategoryObserver : public nsIObserver {
  public:
    nsCategoryObserver(const char* aCategory,
                       nsCategoryListener* aCategoryListener);
    ~nsCategoryObserver();

    void ListenerDied();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
  private:
    NS_HIDDEN_(void) RemoveObservers();

    nsDataHashtable<nsCStringHashKey, nsCString> mHash;
    nsCategoryListener*                          mListener;
    nsCString                                    mCategory;
};








template<class T>
class nsCategoryCache : protected nsCategoryListener {
  public:
    explicit nsCategoryCache(const char* aCategory);
    ~nsCategoryCache() { if (mObserver) mObserver->ListenerDied(); }

    const nsCOMArray<T>& GetEntries() {
      
      
      if (!mObserver)
        mObserver = new nsCategoryObserver(mCategoryName.get(), this);
      return mEntries;
    }
  protected:
    virtual void EntryAdded(const nsCString& aValue);
    virtual void EntryRemoved(const nsCString& aValue);
    virtual void CategoryCleared();
  private:
    friend class CategoryObserver;

    
    nsCategoryCache(const nsCategoryCache<T>&);

    nsCString mCategoryName;
    nsCOMArray<T> mEntries;
    nsRefPtr<nsCategoryObserver> mObserver;
};




template<class T>
nsCategoryCache<T>::nsCategoryCache(const char* aCategory)
: mCategoryName(aCategory)
{
}

template<class T>
void nsCategoryCache<T>::EntryAdded(const nsCString& aValue) {
  nsCOMPtr<T> catEntry = do_GetService(aValue.get());
  if (catEntry)
    mEntries.AppendObject(catEntry);
}

template<class T>
void nsCategoryCache<T>::EntryRemoved(const nsCString& aValue) {
  nsCOMPtr<T> catEntry = do_GetService(aValue.get());
  if (catEntry)
    mEntries.RemoveObject(catEntry);
}

template<class T>
void nsCategoryCache<T>::CategoryCleared() {
  mEntries.Clear();
}

#endif
