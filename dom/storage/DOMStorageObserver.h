




#ifndef nsIDOMStorageObserver_h__
#define nsIDOMStorageObserver_h__

#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsWeakReference.h"
#include "nsTArray.h"
#include "nsString.h"

namespace mozilla {
namespace dom {

class DOMStorageObserver;



class DOMStorageObserverSink
{
public:
  virtual ~DOMStorageObserverSink() {}

private:
  friend class DOMStorageObserver;
  virtual nsresult Observe(const char* aTopic, const nsACString& aScopePrefix) = 0;
};



class DOMStorageObserver : public nsIObserver
                         , public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  static nsresult Init();
  static nsresult Shutdown();
  static DOMStorageObserver* Self() { return sSelf; }

  void AddSink(DOMStorageObserverSink* aObs);
  void RemoveSink(DOMStorageObserverSink* aObs);
  void Notify(const char* aTopic, const nsACString& aData = EmptyCString());

private:
  virtual ~DOMStorageObserver() {}

  static DOMStorageObserver* sSelf;

  
  nsTArray<DOMStorageObserverSink*> mSinks;
  nsCOMPtr<nsITimer> mDBThreadStartDelayTimer;
};

} 
} 

#endif
