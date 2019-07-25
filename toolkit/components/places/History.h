






































#ifndef mozilla_places_History_h_
#define mozilla_places_History_h_

#include "mozilla/IHistory.h"
#include "mozIAsyncHistory.h"
#include "nsIDownloadHistory.h"
#include "Database.h"

#include "mozilla/dom/Link.h"
#include "nsTHashtable.h"
#include "nsString.h"
#include "nsURIHashKey.h"
#include "nsTObserverArray.h"
#include "nsDeque.h"
#include "nsIObserver.h"
#include "mozIStorageConnection.h"

namespace mozilla {
namespace places {

struct VisitData;

#define NS_HISTORYSERVICE_CID \
  {0x0937a705, 0x91a6, 0x417a, {0x82, 0x92, 0xb2, 0x2e, 0xb1, 0x0d, 0xa8, 0x6c}}

class History : public IHistory
              , public nsIDownloadHistory
              , public mozIAsyncHistory
              , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IHISTORY
  NS_DECL_NSIDOWNLOADHISTORY
  NS_DECL_MOZIASYNCHISTORY
  NS_DECL_NSIOBSERVER

  History();

  





  void NotifyVisited(nsIURI* aURI);

  


  mozIStorageAsyncStatement* GetIsVisitedStatement();

  





  nsresult InsertPlace(const VisitData& aVisitData);

  





  nsresult UpdatePlace(const VisitData& aVisitData);

  






  bool FetchPageInfo(VisitData& _place);

  



  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf);

  


  static History* GetService();

  



  static History* GetSingleton();

  template<int N>
  already_AddRefed<mozIStorageStatement>
  GetStatement(const char (&aQuery)[N])
  {
    mozIStorageConnection* dbConn = GetDBConn();
    NS_ENSURE_TRUE(dbConn, nsnull);
    return mDB->GetStatement(aQuery);
  }

private:
  virtual ~History();

  


  mozIStorageConnection* GetDBConn();

  




  nsRefPtr<mozilla::places::Database> mDB;

  




  nsCOMPtr<mozIStorageConnection> mReadOnlyDBConn;

  




  nsCOMPtr<mozIStorageAsyncStatement> mIsVisitedStatement;

  


  void Shutdown();

  static History* gService;

  
  bool mShuttingDown;

  typedef nsTObserverArray<mozilla::dom::Link* > ObserverArray;

  class KeyClass : public nsURIHashKey
  {
  public:
    KeyClass(const nsIURI* aURI)
    : nsURIHashKey(aURI)
    {
    }
    KeyClass(const KeyClass& aOther)
    : nsURIHashKey(aOther)
    {
      NS_NOTREACHED("Do not call me!");
    }
    ObserverArray array;
  };

  



  static size_t SizeOfEntryExcludingThis(KeyClass* aEntry,
                                         nsMallocSizeOfFun aMallocSizeOf,
                                         void*);

  nsTHashtable<KeyClass> mObservers;
};

} 
} 

#endif 
