






































#ifdef MOZ_IPC
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/ContentParent.h"
#include "nsXULAppAPI.h"
#endif

#include "History.h"
#include "nsNavHistory.h"
#include "nsNavBookmarks.h"
#include "Helpers.h"

#include "mozilla/storage.h"
#include "mozilla/dom/Link.h"
#include "nsDocShellCID.h"
#include "nsIEventStateManager.h"
#include "mozilla/Services.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"


#define VISIT_OBSERVERS_INITIAL_CACHE_SIZE 128

using namespace mozilla::dom;

namespace mozilla {
namespace places {




#define URI_VISITED "visited"
#define URI_NOT_VISITED "not visited"
#define URI_VISITED_RESOLUTION_TOPIC "visited-status-resolution"

#define URI_VISIT_SAVED "uri-visit-saved"




namespace {

class VisitedQuery : public AsyncStatementCallback
{
public:
  static nsresult Start(nsIURI* aURI)
  {
    NS_PRECONDITION(aURI, "Null URI");

#ifdef MOZ_IPC
  
  
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    mozilla::dom::ContentChild* cpc =
      mozilla::dom::ContentChild::GetSingleton();
    NS_ASSERTION(cpc, "Content Protocol is NULL!");
    (void)cpc->SendStartVisitedQuery(aURI);
    return NS_OK;
  }
#endif

    nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
    NS_ENSURE_STATE(navHistory);
    if (navHistory->hasEmbedVisit(aURI)) {
      nsRefPtr<VisitedQuery> callback = new VisitedQuery(aURI, true);
      NS_ENSURE_TRUE(callback, NS_ERROR_OUT_OF_MEMORY);
      
      nsCOMPtr<nsIRunnable> event =
        NS_NewRunnableMethod(callback, &VisitedQuery::NotifyVisitedStatus);
      NS_DispatchToMainThread(event);

      return NS_OK;
    }

    mozIStorageAsyncStatement* stmt =
      History::GetService()->GetIsVisitedStatement();
    NS_ENSURE_STATE(stmt);

    
    nsresult rv = URIBinder::Bind(stmt, 0, aURI);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<VisitedQuery> callback = new VisitedQuery(aURI);
    NS_ENSURE_TRUE(callback, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<mozIStoragePendingStatement> handle;
    return stmt->ExecuteAsync(callback, getter_AddRefs(handle));
  }

  NS_IMETHOD HandleResult(mozIStorageResultSet* aResults)
  {
    
    
    mIsVisited = true;
    return NS_OK;
  }

  NS_IMETHOD HandleError(mozIStorageError* aError)
  {
    
    
    return NS_OK;
  }

  NS_IMETHOD HandleCompletion(PRUint16 aReason)
  {
    if (aReason != mozIStorageStatementCallback::REASON_FINISHED) {
      return NS_OK;
    }

    nsresult rv = NotifyVisitedStatus();
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  nsresult NotifyVisitedStatus()
  {
    if (mIsVisited) {
      History::GetService()->NotifyVisited(mURI);
    }

    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
    if (observerService) {
      nsAutoString status;
      if (mIsVisited) {
        status.AssignLiteral(URI_VISITED);
      }
      else {
        status.AssignLiteral(URI_NOT_VISITED);
      }
      (void)observerService->NotifyObservers(mURI,
                                             URI_VISITED_RESOLUTION_TOPIC,
                                             status.get());
    }

    return NS_OK;
  }

private:
  VisitedQuery(nsIURI* aURI, bool aIsVisited=false)
  : mURI(aURI)
  , mIsVisited(aIsVisited)
  {
  }

  nsCOMPtr<nsIURI> mURI;
  bool mIsVisited;
};

struct VisitData {
  VisitData()
  : placeId(0)
  , visitId(0)
  , sessionId(0)
  , hidden(false)
  , typed(false)
  , transitionType(-1)
  , visitTime(0)
  {
  }

  PRInt64 placeId;
  PRInt64 visitId;
  PRInt64 sessionId;
  nsCString spec;
  nsString revHost;
  bool hidden;
  bool typed;
  PRInt32 transitionType;
  PRTime visitTime;
};





class NotifyVisitObservers : public nsRunnable
{
public:
  NotifyVisitObservers(VisitData& aPlace,
                       VisitData& aReferrer)
  : mPlace(aPlace)
  , mReferrer(aReferrer)
  {
  }

  NS_IMETHOD Run()
  {
    NS_PRECONDITION(NS_IsMainThread(),
                    "This should be called on the main thread");

    nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
    if (!navHistory) {
      NS_WARNING("Trying to notify about a visit but cannot get the history service!");
      return NS_OK;
    }

    nsCOMPtr<nsIURI> uri;
    (void)NS_NewURI(getter_AddRefs(uri), mPlace.spec);

    
    
    if (!mPlace.hidden &&
        mPlace.transitionType != nsINavHistoryService::TRANSITION_EMBED &&
        mPlace.transitionType != nsINavHistoryService::TRANSITION_FRAMED_LINK) {
      navHistory->NotifyOnVisit(uri, mPlace.visitId, mPlace.visitTime,
                                mPlace.sessionId, mReferrer.visitId,
                                mPlace.transitionType);
    }

    nsCOMPtr<nsIObserverService> obsService =
      mozilla::services::GetObserverService();
    if (obsService) {
      nsresult rv = obsService->NotifyObservers(uri, URI_VISIT_SAVED, nsnull);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not notify observers");
    }

    History::GetService()->NotifyVisited(uri);

    return NS_OK;
  }
private:
  VisitData mPlace;
  VisitData mReferrer;
};




class InsertVisitedURI : public nsRunnable
{
public:
  









  static nsresult Start(mozIStorageConnection* aConnection,
                        VisitData& aPlace,
                        nsIURI* aReferrer = nsnull)
  {
    NS_PRECONDITION(NS_IsMainThread(),
                    "This should be called on the main thread");

    nsRefPtr<InsertVisitedURI> event =
      new InsertVisitedURI(aConnection, aPlace, aReferrer);

    
    
    
    
    nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(navHistory, NS_ERROR_UNEXPECTED);
    event->mPlace.sessionId = navHistory->GetNewSessionID();

    
    nsCOMPtr<nsIEventTarget> target = do_GetInterface(aConnection);
    NS_ENSURE_TRUE(target, NS_ERROR_UNEXPECTED);
    nsresult rv = target->Dispatch(event, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_IMETHOD Run()
  {
    NS_PRECONDITION(!NS_IsMainThread(),
                    "This should not be called on the main thread");

    bool known = FetchPageInfo(mPlace);

    
    
    if (!mReferrer.spec.IsEmpty()) {
      bool recentVisit = FetchVisitInfo(mReferrer, mPlace.visitTime);
      
      
      if (recentVisit) {
        mPlace.sessionId = mReferrer.sessionId;
      }
      
      
      else {
        
        
        mReferrer.visitId = 0;
      }
    }

    mozStorageTransaction transaction(mDBConn, PR_FALSE,
                                      mozIStorageConnection::TRANSACTION_IMMEDIATE);
    nsresult rv;
    nsCOMPtr<mozIStorageStatement> stmt;
    
    if (known) {
      NS_ASSERTION(mPlace.placeId > 0, "must have a valid place id!");

      stmt = mHistory->syncStatements.GetCachedStatement(
          "UPDATE moz_places "
          "SET hidden = :hidden, typed = :typed "
          "WHERE id = :page_id "
        );
      NS_ENSURE_STATE(stmt);
      rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), mPlace.placeId);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    
    else {
      NS_ASSERTION(mPlace.placeId == 0, "should not have a valid place id!");

      stmt = mHistory->syncStatements.GetCachedStatement(
          "INSERT INTO moz_places "
            "(url, rev_host, hidden, typed, guid) "
          "VALUES (:page_url, :rev_host, :hidden, :typed, GENERATE_GUID()) "
        );
      NS_ENSURE_STATE(stmt);

      rv = stmt->BindStringByName(NS_LITERAL_CSTRING("rev_host"),
                                  mPlace.revHost);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), mPlace.spec);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("typed"), mPlace.typed);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("hidden"), mPlace.hidden);
    NS_ENSURE_SUCCESS(rv, rv);

    mozStorageStatementScoper scoper(stmt);
    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddVisit(mPlace, mReferrer);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = UpdateFrecency(mPlace);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = transaction.Commit();
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIRunnable> event = new NotifyVisitObservers(mPlace, mReferrer);
    rv = NS_DispatchToMainThread(event);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }
private:
  InsertVisitedURI(mozIStorageConnection* aConnection,
                   VisitData& aPlace,
                   nsIURI* aReferrer)
  : mDBConn(aConnection)
  , mPlace(aPlace)
  , mHistory(History::GetService())
  {
    if (aReferrer) {
      (void)aReferrer->GetSpec(mReferrer.spec);
    }
  }

  






  bool FetchPageInfo(VisitData& _place)
  {
    NS_PRECONDITION(!_place.spec.IsEmpty(), "must have a non-empty spec!");

    nsCOMPtr<mozIStorageStatement> stmt =
      mHistory->syncStatements.GetCachedStatement(
        "SELECT id, typed, hidden "
        "FROM moz_places "
        "WHERE url = :page_url "
      );
    NS_ENSURE_TRUE(stmt, false);
    mozStorageStatementScoper scoper(stmt);

    nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"),
                                  _place.spec);
    NS_ENSURE_SUCCESS(rv, false);

    PRBool hasResult;
    rv = stmt->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, false);
    if (!hasResult) {
      return false;
    }

    rv = stmt->GetInt64(0, &_place.placeId);
    NS_ENSURE_SUCCESS(rv, false);

    if (!_place.typed) {
      
      
      PRInt32 typed;
      rv = stmt->GetInt32(1, &typed);
      _place.typed = !!typed;
      NS_ENSURE_SUCCESS(rv, true);
    }
    if (_place.hidden) {
      
      
      
      PRInt32 hidden;
      rv = stmt->GetInt32(2, &hidden);
      _place.hidden = !!hidden;
      NS_ENSURE_SUCCESS(rv, true);
    }

    return true;
  }

  










  bool FetchVisitInfo(VisitData& _place,
                      PRTime aThresholdStart = 0)
  {
    NS_PRECONDITION(!_place.spec.IsEmpty(), "must have a non-empty spec!");

    nsCOMPtr<mozIStorageStatement> stmt =
      mHistory->syncStatements.GetCachedStatement(
        "SELECT id, session, visit_date "
        "FROM moz_historyvisits "
        "WHERE place_id = (SELECT id FROM moz_places WHERE url = :page_url) "
        "ORDER BY visit_date DESC "
      );
    NS_ENSURE_TRUE(stmt, false);
    mozStorageStatementScoper scoper(stmt);

    nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"),
                                  _place.spec);
    NS_ENSURE_SUCCESS(rv, false);

    PRBool hasResult;
    rv = stmt->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, false);
    if (!hasResult) {
      return false;
    }

    rv = stmt->GetInt64(0, &_place.visitId);
    NS_ENSURE_SUCCESS(rv, false);
    rv = stmt->GetInt64(1, &_place.sessionId);
    NS_ENSURE_SUCCESS(rv, false);
    rv = stmt->GetInt64(2, &_place.visitTime);
    NS_ENSURE_SUCCESS(rv, false);

    
    
    if (aThresholdStart &&
        aThresholdStart - _place.visitTime <= RECENT_EVENT_THRESHOLD) {
      return true;
    }

    return false;
  }

  







  nsresult AddVisit(VisitData& _place,
                    const VisitData& aReferrer)
  {
    nsresult rv;
    nsCOMPtr<mozIStorageStatement> stmt;
    if (_place.placeId) {
      stmt = mHistory->syncStatements.GetCachedStatement(
        "INSERT INTO moz_historyvisits "
          "(from_visit, place_id, visit_date, visit_type, session) "
        "VALUES (:from_visit, :page_id, :visit_date, :visit_type, :session) "
      );
      NS_ENSURE_STATE(stmt);
      rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), mPlace.placeId);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      stmt = mHistory->syncStatements.GetCachedStatement(
        "INSERT INTO moz_historyvisits "
          "(from_visit, place_id, visit_date, visit_type, session) "
        "VALUES (:from_visit, (SELECT id FROM moz_places WHERE url = :page_url), :visit_date, :visit_type, :session) "
      );
      NS_ENSURE_STATE(stmt);
      rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), _place.spec);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("from_visit"),
                               aReferrer.visitId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("visit_date"),
                               _place.visitTime);
    NS_ENSURE_SUCCESS(rv, rv);
    PRInt32 transitionType = _place.transitionType;
    NS_ASSERTION(transitionType >= nsINavHistoryService::TRANSITION_LINK &&
                 transitionType <= nsINavHistoryService::TRANSITION_FRAMED_LINK,
                 "Invalid transition type!");
    rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("visit_type"),
                               transitionType);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("session"),
                               _place.sessionId);
    NS_ENSURE_SUCCESS(rv, rv);

    mozStorageStatementScoper scoper(stmt);
    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    bool visited = FetchVisitInfo(_place);
    if (visited) {
      NS_NOTREACHED("Not visited after adding a visit!");
    }

    return NS_OK;
  }

  





  nsresult UpdateFrecency(const VisitData& aPlace)
  {
    nsresult rv;
    { 
      nsCOMPtr<mozIStorageStatement> stmt;
      if (aPlace.placeId) {
        stmt = mHistory->syncStatements.GetCachedStatement(
          "UPDATE moz_places "
          "SET frecency = CALCULATE_FRECENCY(:page_id) "
          "WHERE id = :page_id"
        );
        NS_ENSURE_STATE(stmt);
        rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), mPlace.placeId);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        stmt = mHistory->syncStatements.GetCachedStatement(
          "UPDATE moz_places "
          "SET frecency = CALCULATE_FRECENCY(id) "
          "WHERE url = :page_url"
        );
        NS_ENSURE_STATE(stmt);
        rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aPlace.spec);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      mozStorageStatementScoper scoper(stmt);

      rv = stmt->Execute();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    { 
      
      nsCOMPtr<mozIStorageStatement> stmt;
      if (aPlace.placeId) {
        stmt = mHistory->syncStatements.GetCachedStatement(
          "UPDATE moz_places "
          "SET hidden = 0 "
          "WHERE id = :page_id AND frecency <> 0"
        );
        NS_ENSURE_STATE(stmt);
        rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), mPlace.placeId);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        stmt = mHistory->syncStatements.GetCachedStatement(
          "UPDATE moz_places "
          "SET hidden = 0 "
          "WHERE url = :page_url AND frecency <> 0"
        );
        NS_ENSURE_STATE(stmt);
        rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aPlace.spec);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      mozStorageStatementScoper scoper(stmt);
      rv = stmt->Execute();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  mozIStorageConnection* mDBConn;

  VisitData mPlace;
  VisitData mReferrer;

  



  nsRefPtr<History> mHistory;
};




class NotifyTitleObservers : public nsRunnable
{
public:
  







  NotifyTitleObservers(const nsCString& aSpec,
                       const nsString& aTitle)
  : mSpec(aSpec)
  , mTitle(aTitle)
  {
    NS_PRECONDITION(!NS_IsMainThread(),
                    "This should not be called on the main thread");
  }

  NS_IMETHOD Run()
  {
    NS_PRECONDITION(NS_IsMainThread(),
                    "This should be called on the main thread");

    nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
    NS_ENSURE_TRUE(navHistory, NS_ERROR_OUT_OF_MEMORY);
    nsCOMPtr<nsIURI> uri;
    (void)NS_NewURI(getter_AddRefs(uri), mSpec);
    navHistory->NotifyTitleChange(uri, mTitle);

    return NS_OK;
  }
private:
  const nsCString mSpec;
  const nsString mTitle;
};





class SetPageTitle : public nsRunnable
{
public:
  









  static nsresult Start(mozIStorageConnection* aConnection,
                        nsIURI* aURI,
                        const nsString& aTitle)
  {
    NS_PRECONDITION(NS_IsMainThread(),
                    "This should be called on the main thread");
    NS_PRECONDITION(aURI, "Must pass a non-null URI object!");

    nsCString spec;
    nsresult rv = aURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<SetPageTitle> event = new SetPageTitle(spec, aTitle);

    
    nsCOMPtr<nsIEventTarget> target = do_GetInterface(aConnection);
    NS_ENSURE_TRUE(target, NS_ERROR_UNEXPECTED);
    rv = target->Dispatch(event, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_IMETHOD Run()
  {
    NS_PRECONDITION(!NS_IsMainThread(),
                    "This should not be called on the main thread");

    
    nsCOMPtr<mozIStorageStatement> stmt =
      mHistory->syncStatements.GetCachedStatement(
        "SELECT id, title "
        "FROM moz_places "
        "WHERE url = :page_url "
      );
    NS_ENSURE_STATE(stmt);

    PRInt64 placeId = 0;
    nsAutoString title;
    {
      mozStorageStatementScoper scoper(stmt);
      nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"),
                                    mSpec);
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool hasResult;
      rv = stmt->ExecuteStep(&hasResult);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!hasResult) {
        
        
        return NS_OK;
      }

      rv = stmt->GetInt64(0, &placeId);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = stmt->GetString(1, title);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    NS_ASSERTION(placeId > 0, "We somehow have an invalid place id here!");

    
    
    if (mTitle.Equals(title) || (mTitle.IsVoid() && title.IsVoid())) {
      return NS_OK;
    }

    
    stmt = mHistory->syncStatements.GetCachedStatement(
        "UPDATE moz_places "
        "SET title = :page_title "
        "WHERE id = :page_id "
      );
    NS_ENSURE_STATE(stmt);

    {
      mozStorageStatementScoper scoper(stmt);
      nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"),
                                          placeId);
      NS_ENSURE_SUCCESS(rv, rv);
      if (mTitle.IsVoid()) {
        rv = stmt->BindNullByName(NS_LITERAL_CSTRING("page_title"));
      }
      else {
        rv = stmt->BindStringByName(NS_LITERAL_CSTRING("page_title"),
                                    StringHead(mTitle, TITLE_LENGTH_MAX));
      }
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->Execute();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsIRunnable> event = new NotifyTitleObservers(mSpec, mTitle);
    nsresult rv = NS_DispatchToMainThread(event);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

private:
  SetPageTitle(const nsCString& aSpec,
               const nsString& aTitle)
  : mSpec(aSpec)
  , mTitle(aTitle)
  , mHistory(History::GetService())
  {
  }

  const nsCString mSpec;
  const nsString mTitle;

  



  nsRefPtr<History> mHistory;
};

} 




History* History::gService = NULL;

History::History()
  : syncStatements(mDBConn)
  , mShuttingDown(false)
{
  NS_ASSERTION(!gService, "Ruh-roh!  This service has already been created!");
  gService = this;

  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  NS_WARN_IF_FALSE(os, "Observer service was not found!");
  if (os) {
    (void)os->AddObserver(this, TOPIC_PLACES_SHUTDOWN, PR_FALSE);
  }
}

History::~History()
{
  gService = NULL;

#ifdef DEBUG
  if (mObservers.IsInitialized()) {
    NS_ASSERTION(mObservers.Count() == 0,
                 "Not all Links were removed before we disappear!");
  }
#endif
}

void
History::NotifyVisited(nsIURI* aURI)
{
  NS_ASSERTION(aURI, "Ruh-roh!  A NULL URI was passed to us!");

#ifdef MOZ_IPC
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    mozilla::dom::ContentParent* cpp = 
      mozilla::dom::ContentParent::GetSingleton(PR_FALSE);
    if (cpp)
      (void)cpp->SendNotifyVisited(aURI);
  }
#endif

  
  
  if (!mObservers.IsInitialized()) {
    return;
  }

  
  
  KeyClass* key = mObservers.GetEntry(aURI);
  if (!key) {
    return;
  }

  
  const ObserverArray& observers = key->array;
  ObserverArray::index_type len = observers.Length();
  for (ObserverArray::index_type i = 0; i < len; i++) {
    Link* link = observers[i];
    link->SetLinkState(eLinkState_Visited);
    NS_ASSERTION(len == observers.Length(),
                 "Calling SetLinkState added or removed an observer!");
  }

  
  mObservers.RemoveEntry(aURI);
}

mozIStorageAsyncStatement*
History::GetIsVisitedStatement()
{
  if (mIsVisitedStatement) {
    return mIsVisitedStatement;
  }

  
  if (!mReadOnlyDBConn) {
    mozIStorageConnection* dbConn = GetDBConn();
    NS_ENSURE_TRUE(dbConn, nsnull);

    (void)dbConn->Clone(PR_TRUE, getter_AddRefs(mReadOnlyDBConn));
    NS_ENSURE_TRUE(mReadOnlyDBConn, nsnull);
  }

  
  nsresult rv = mReadOnlyDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "SELECT h.id "
    "FROM moz_places h "
    "WHERE url = ?1 "
      "AND EXISTS(SELECT id FROM moz_historyvisits WHERE place_id = h.id LIMIT 1) "
  ),  getter_AddRefs(mIsVisitedStatement));
  NS_ENSURE_SUCCESS(rv, nsnull);
  return mIsVisitedStatement;
}


History*
History::GetService()
{
  if (gService) {
    return gService;
  }

  nsCOMPtr<IHistory> service(do_GetService(NS_IHISTORY_CONTRACTID));
  NS_ABORT_IF_FALSE(service, "Cannot obtain IHistory service!");
  NS_ASSERTION(gService, "Our constructor was not run?!");

  return gService;
}


History*
History::GetSingleton()
{
  if (!gService) {
    gService = new History();
    NS_ENSURE_TRUE(gService, nsnull);
  }

  NS_ADDREF(gService);
  return gService;
}

mozIStorageConnection*
History::GetDBConn()
{
  if (mDBConn) {
    return mDBConn;
  }

  nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(navHistory, nsnull);

  nsresult rv = navHistory->GetDBConnection(getter_AddRefs(mDBConn));
  NS_ENSURE_SUCCESS(rv, nsnull);

  return mDBConn;
}

void
History::Shutdown()
{
  NS_ASSERTION(!mShuttingDown, "Shutdown was called more than once!");

  mShuttingDown = true;

  
  nsISupports* obj = static_cast<IHistory*>(this);
  nsCOMPtr<nsIRunnable> event =
    new FinalizeStatementCacheProxy<mozIStorageStatement>(syncStatements, obj);
  nsCOMPtr<nsIEventTarget> target = do_GetInterface(mDBConn);
  if (target) {
    (void)target->Dispatch(event, NS_DISPATCH_NORMAL);
  }

  if (mReadOnlyDBConn) {
    if (mIsVisitedStatement) {
      (void)mIsVisitedStatement->Finalize();
    }
    (void)mReadOnlyDBConn->AsyncClose(nsnull);
  }
}




NS_IMETHODIMP
History::VisitURI(nsIURI* aURI,
                  nsIURI* aLastVisitedURI,
                  PRUint32 aFlags)
{
  NS_PRECONDITION(aURI, "URI should not be NULL.");
  if (mShuttingDown) {
    return NS_OK;
  }

#ifdef MOZ_IPC
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    mozilla::dom::ContentChild* cpc =
      mozilla::dom::ContentChild::GetSingleton();
    NS_ASSERTION(cpc, "Content Protocol is NULL!");
    (void)cpc->SendVisitURI(aURI, aLastVisitedURI, aFlags);
    return NS_OK;
  } 
#endif 

  nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(navHistory, NS_ERROR_OUT_OF_MEMORY);

  
  PRBool canAdd;
  nsresult rv = navHistory->CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd) {
    return NS_OK;
  }

  if (aLastVisitedURI) {
    PRBool same;
    rv = aURI->Equals(aLastVisitedURI, &same);
    NS_ENSURE_SUCCESS(rv, rv);
    if (same) {
      
      return NS_OK;
    }
  }

  VisitData place;
  rv = aURI->GetSpec(place.spec);
  NS_ENSURE_SUCCESS(rv, rv);
  (void)GetReversedHostname(aURI, place.revHost);

  
  
  PRUint32 recentFlags = navHistory->GetRecentFlags(aURI);
  bool redirected = false;
  bool isFollowedLink = recentFlags & nsNavHistory::RECENT_ACTIVATED;

  
  
  
  
  

  if (!(aFlags & IHistory::TOP_LEVEL) && !isFollowedLink) {
    
    place.transitionType = nsINavHistoryService::TRANSITION_EMBED;
  }
  else if (aFlags & IHistory::REDIRECT_TEMPORARY) {
    place.transitionType = nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY;
    redirected = true;
  }
  else if (aFlags & IHistory::REDIRECT_PERMANENT) {
    place.transitionType = nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT;
    redirected = true;
  }
  else if (recentFlags & nsNavHistory::RECENT_TYPED) {
    place.transitionType = nsINavHistoryService::TRANSITION_TYPED;
  }
  else if (recentFlags & nsNavHistory::RECENT_BOOKMARKED) {
    place.transitionType = nsINavHistoryService::TRANSITION_BOOKMARK;
  }
  else if (!(aFlags & IHistory::TOP_LEVEL) && isFollowedLink) {
    
    place.transitionType = nsINavHistoryService::TRANSITION_FRAMED_LINK;
  }
  else {
    
    place.transitionType = nsINavHistoryService::TRANSITION_LINK;
  }

  place.typed = place.transitionType == nsINavHistoryService::TRANSITION_TYPED;
  place.hidden = GetHiddenState(redirected, place.transitionType);
  place.visitTime = PR_Now();

  
  
  if (place.transitionType == nsINavHistoryService::TRANSITION_EMBED) {
    navHistory->registerEmbedVisit(aURI, place.visitTime);
    
    VisitData noReferrer;
    nsCOMPtr<nsIRunnable> event = new NotifyVisitObservers(place, noReferrer);
    rv = NS_DispatchToMainThread(event);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    mozIStorageConnection* dbConn = GetDBConn();
    NS_ENSURE_STATE(dbConn);

    rv = InsertVisitedURI::Start(dbConn, place, aLastVisitedURI);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIObserverService> obsService =
    mozilla::services::GetObserverService();
  if (obsService) {
    obsService->NotifyObservers(aURI, NS_LINK_VISITED_EVENT_TOPIC, nsnull);
  }

  return NS_OK;
}

NS_IMETHODIMP
History::RegisterVisitedCallback(nsIURI* aURI,
                                 Link* aLink)
{
  NS_ASSERTION(aURI, "Must pass a non-null URI!");
#ifdef MOZ_IPC
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    NS_PRECONDITION(aLink, "Must pass a non-null Link!");
  }
#else
  NS_PRECONDITION(aLink, "Must pass a non-null Link!");
#endif

  
  if (!mObservers.IsInitialized()) {
    NS_ENSURE_TRUE(mObservers.Init(VISIT_OBSERVERS_INITIAL_CACHE_SIZE),
                   NS_ERROR_OUT_OF_MEMORY);
  }

  
#ifdef DEBUG
  bool keyAlreadyExists = !!mObservers.GetEntry(aURI);
#endif
  KeyClass* key = mObservers.PutEntry(aURI);
  NS_ENSURE_TRUE(key, NS_ERROR_OUT_OF_MEMORY);
  ObserverArray& observers = key->array;

  if (observers.IsEmpty()) {
    NS_ASSERTION(!keyAlreadyExists,
                 "An empty key was kept around in our hashtable!");

    
    
    
    nsresult rv = VisitedQuery::Start(aURI);

    
    
    
    
    if (NS_FAILED(rv) || !aLink) {
      
      mObservers.RemoveEntry(aURI);
      return rv;
    }
  }
#ifdef MOZ_IPC
  
  
  
  else if (!aLink) {
    NS_ASSERTION(XRE_GetProcessType() == GeckoProcessType_Default,
                 "We should only ever get a null Link in the default process!");
    return NS_OK;
  }
#endif

  
  
  NS_ASSERTION(!observers.Contains(aLink),
               "Already tracking this Link object!");

  
  if (!observers.AppendElement(aLink)) {
    
    (void)UnregisterVisitedCallback(aURI, aLink);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

NS_IMETHODIMP
History::UnregisterVisitedCallback(nsIURI* aURI,
                                   Link* aLink)
{
  NS_ASSERTION(aURI, "Must pass a non-null URI!");
  NS_ASSERTION(aLink, "Must pass a non-null Link object!");

  
  KeyClass* key = mObservers.GetEntry(aURI);
  if (!key) {
    NS_ERROR("Trying to unregister for a URI that wasn't registered!");
    return NS_ERROR_UNEXPECTED;
  }
  ObserverArray& observers = key->array;
  if (!observers.RemoveElement(aLink)) {
    NS_ERROR("Trying to unregister a node that wasn't registered!");
    return NS_ERROR_UNEXPECTED;
  }

  
  if (observers.IsEmpty()) {
    mObservers.RemoveEntry(aURI);
  }

  return NS_OK;
}

NS_IMETHODIMP
History::SetURITitle(nsIURI* aURI, const nsAString& aTitle)
{
  NS_PRECONDITION(aURI, "Must pass a non-null URI!");
  if (mShuttingDown) {
    return NS_OK;
  }

#ifdef MOZ_IPC
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    mozilla::dom::ContentChild * cpc = 
      mozilla::dom::ContentChild::GetSingleton();
    NS_ASSERTION(cpc, "Content Protocol is NULL!");
    (void)cpc->SendSetURITitle(aURI, nsDependentString(aTitle));
    return NS_OK;
  } 
#endif 

  nsNavHistory* navHistory = nsNavHistory::GetHistoryService();

  
  
  
  
  
  
  
  
  
  NS_ENSURE_TRUE(navHistory, NS_ERROR_FAILURE);

  PRBool canAdd;
  nsresult rv = navHistory->CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd) {
    return NS_OK;
  }

  
  if (navHistory->hasEmbedVisit(aURI)) {
    return NS_OK;
  }

  nsAutoString title;
  if (aTitle.IsEmpty()) {
    title.SetIsVoid(PR_TRUE);
  }
  else {
    title.Assign(aTitle);
  }

  mozIStorageConnection* dbConn = GetDBConn();
  NS_ENSURE_STATE(dbConn);

  rv = SetPageTitle::Start(dbConn, aURI, title);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




NS_IMETHODIMP
History::Observe(nsISupports* aSubject, const char* aTopic,
                 const PRUnichar* aData)
{
  if (strcmp(aTopic, TOPIC_PLACES_SHUTDOWN) == 0) {
    Shutdown();

    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os) {
      (void)os->RemoveObserver(this, TOPIC_PLACES_SHUTDOWN);
    }
  }

  return NS_OK;
}




NS_IMPL_THREADSAFE_ISUPPORTS2(
  History
, IHistory
, nsIObserver
)

} 
} 
