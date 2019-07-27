





#ifndef nsObserverList_h___
#define nsObserverList_h___

#include "nsISupports.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIObserver.h"
#include "nsIWeakReference.h"
#include "nsHashKeys.h"
#include "nsISimpleEnumerator.h"
#include "mozilla/Attributes.h"

struct ObserverRef
{
  ObserverRef(const ObserverRef& aO) : isWeakRef(aO.isWeakRef), ref(aO.ref) {}
  explicit ObserverRef(nsIObserver* aObserver) : isWeakRef(false), ref(aObserver) {}
  explicit ObserverRef(nsIWeakReference* aWeak) : isWeakRef(true), ref(aWeak) {}

  bool isWeakRef;
  nsCOMPtr<nsISupports> ref;

  nsIObserver* asObserver()
  {
    NS_ASSERTION(!isWeakRef, "Isn't a strong ref.");
    return static_cast<nsIObserver*>((nsISupports*)ref);
  }

  nsIWeakReference* asWeak()
  {
    NS_ASSERTION(isWeakRef, "Isn't a weak ref.");
    return static_cast<nsIWeakReference*>((nsISupports*)ref);
  }

  bool operator==(nsISupports* aRhs) const { return ref == aRhs; }
};

class nsObserverList : public nsCharPtrHashKey
{
  friend class nsObserverService;

public:
  explicit nsObserverList(const char* aKey) : nsCharPtrHashKey(aKey)
  {
    MOZ_COUNT_CTOR(nsObserverList);
  }

  ~nsObserverList()
  {
    MOZ_COUNT_DTOR(nsObserverList);
  }

  nsresult AddObserver(nsIObserver* aObserver, bool aOwnsWeak);
  nsresult RemoveObserver(nsIObserver* aObserver);

  void NotifyObservers(nsISupports* aSubject,
                       const char* aTopic,
                       const char16_t* aSomeData);
  nsresult GetObserverList(nsISimpleEnumerator** aEnumerator);

  
  
  void FillObserverArray(nsCOMArray<nsIObserver>& aArray);

  
  void AppendStrongObservers(nsCOMArray<nsIObserver>& aArray);

private:
  nsTArray<ObserverRef> mObservers;
};

class nsObserverEnumerator final : public nsISimpleEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR

  explicit nsObserverEnumerator(nsObserverList* aObserverList);

private:
  ~nsObserverEnumerator() {}

  int32_t mIndex; 
  nsCOMArray<nsIObserver> mObservers;
};

#endif 
