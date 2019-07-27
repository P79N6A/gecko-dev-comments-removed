





#ifndef mozilla_places_History_h_
#define mozilla_places_History_h_

#include "mozilla/IHistory.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Mutex.h"
#include "mozIAsyncHistory.h"
#include "nsIDownloadHistory.h"
#include "Database.h"

#include "mozilla/dom/Link.h"
#include "nsTHashtable.h"
#include "nsString.h"
#include "nsURIHashKey.h"
#include "nsTObserverArray.h"
#include "nsDeque.h"
#include "nsIMemoryReporter.h"
#include "nsIObserver.h"
#include "mozIStorageConnection.h"

namespace mozilla {
namespace places {

struct VisitData;
class ConcurrentStatementsHolder;

#define NS_HISTORYSERVICE_CID \
  {0x0937a705, 0x91a6, 0x417a, {0x82, 0x92, 0xb2, 0x2e, 0xb1, 0x0d, 0xa8, 0x6c}}


#define RECENTLY_VISITED_URI_SIZE 8

class History : public IHistory
              , public nsIDownloadHistory
              , public mozIAsyncHistory
              , public nsIObserver
              , public nsIMemoryReporter
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_IHISTORY
  NS_DECL_NSIDOWNLOADHISTORY
  NS_DECL_MOZIASYNCHISTORY
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIMEMORYREPORTER

  History();

  


  nsresult GetIsVisitedStatement(mozIStorageCompletionCallback* aCallback);

  





  nsresult InsertPlace(const VisitData& aVisitData);

  





  nsresult UpdatePlace(const VisitData& aVisitData);

  







  nsresult FetchPageInfo(VisitData& _place, bool* _exists);

  



  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf);

  


  static History* GetService();

  



  static History* GetSingleton();

  template<int N>
  already_AddRefed<mozIStorageStatement>
  GetStatement(const char (&aQuery)[N])
  {
    mozIStorageConnection* dbConn = GetDBConn();
    NS_ENSURE_TRUE(dbConn, nullptr);
    return mDB->GetStatement(aQuery);
  }

  already_AddRefed<mozIStorageStatement>
  GetStatement(const nsACString& aQuery)
  {
    mozIStorageConnection* dbConn = GetDBConn();
    NS_ENSURE_TRUE(dbConn, nullptr);
    return mDB->GetStatement(aQuery);
  }

  bool IsShuttingDown() const {
    return mShuttingDown;
  }
  Mutex& GetShutdownMutex() {
    return mShutdownMutex;
  }

  



  void AppendToRecentlyVisitedURIs(nsIURI* aURI);

private:
  virtual ~History();

  void InitMemoryReporter();

  


  mozIStorageConnection* GetDBConn();

  




  nsRefPtr<mozilla::places::Database> mDB;

  nsRefPtr<ConcurrentStatementsHolder> mConcurrentStatementsHolder;

  


  void Shutdown();

  static History* gService;

  
  bool mShuttingDown;
  
  
  
  
  
  Mutex mShutdownMutex;

  typedef nsTObserverArray<mozilla::dom::Link* > ObserverArray;

  class KeyClass : public nsURIHashKey
  {
  public:
    explicit KeyClass(const nsIURI* aURI)
    : nsURIHashKey(aURI)
    {
    }
    KeyClass(const KeyClass& aOther)
    : nsURIHashKey(aOther)
    {
      NS_NOTREACHED("Do not call me!");
    }
    size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
    {
      return array.SizeOfExcludingThis(aMallocSizeOf);
    }
    ObserverArray array;
  };

  nsTHashtable<KeyClass> mObservers;

  



  typedef nsAutoTArray<nsCOMPtr<nsIURI>, RECENTLY_VISITED_URI_SIZE>
          RecentlyVisitedArray;
  RecentlyVisitedArray mRecentlyVisitedURIs;
  RecentlyVisitedArray::index_type mRecentlyVisitedURIsNextIndex;

  bool IsRecentlyVisitedURI(nsIURI* aURI);
};

} 
} 

#endif 
