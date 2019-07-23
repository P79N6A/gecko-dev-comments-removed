




































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

struct ObserverRef
{
  ObserverRef(const ObserverRef& o) :
    isWeakRef(o.isWeakRef), ref(o.ref) { }
  
  ObserverRef(nsIObserver* aObserver) : isWeakRef(PR_FALSE), ref(aObserver) { }
  ObserverRef(nsIWeakReference* aWeak) : isWeakRef(PR_TRUE), ref(aWeak) { }

  PRBool isWeakRef;
  nsCOMPtr<nsISupports> ref;

  nsIObserver* asObserver() {
    NS_ASSERTION(!isWeakRef, "Isn't a strong ref.");
    return NS_STATIC_CAST(nsIObserver*, (nsISupports*) ref);
  }

  nsIWeakReference* asWeak() {
    NS_ASSERTION(isWeakRef, "Isn't a weak ref.");
    return NS_STATIC_CAST(nsIWeakReference*, (nsISupports*) ref);
  }

  PRBool operator==(nsISupports* b) const { return ref == b; }
};

class nsObserverList : public nsCharPtrHashKey
{
public:
  nsObserverList(const char *key) : nsCharPtrHashKey(key)
  { MOZ_COUNT_CTOR(nsObserverList); }

  ~nsObserverList() { MOZ_COUNT_DTOR(nsObserverList); }

  nsresult AddObserver(nsIObserver* anObserver, PRBool ownsWeak);
  nsresult RemoveObserver(nsIObserver* anObserver);

  void NotifyObservers(nsISupports *aSubject,
                       const char *aTopic,
                       const PRUnichar *someData);
  nsresult GetObserverList(nsISimpleEnumerator** anEnumerator);

  
  
  void FillObserverArray(nsCOMArray<nsIObserver> &aArray);

private:
  nsTArray<ObserverRef> mObservers;
};

class nsObserverEnumerator : public nsISimpleEnumerator
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISIMPLEENUMERATOR

    nsObserverEnumerator(nsObserverList* aObserverList);

private:
    ~nsObserverEnumerator() { }

    PRInt32 mIndex; 
    nsCOMArray<nsIObserver> mObservers;
};

#endif 
