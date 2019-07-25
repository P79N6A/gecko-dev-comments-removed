






































#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/ContentParent.h"
#include "nsXULAppAPI.h"

#include "History.h"
#include "nsNavHistory.h"
#include "nsNavBookmarks.h"
#include "Helpers.h"
#include "PlaceInfo.h"
#include "VisitInfo.h"

#include "mozilla/storage.h"
#include "mozilla/dom/Link.h"
#include "nsDocShellCID.h"
#include "mozilla/Services.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"
#include "nsIXPConnect.h"
#include "mozilla/Util.h"


#define VISIT_OBSERVERS_INITIAL_CACHE_SIZE 128


#define TOPIC_UPDATEPLACES_COMPLETE "places-updatePlaces-complete"

using namespace mozilla::dom;

namespace mozilla {
namespace places {




#define URI_VISITED "visited"
#define URI_NOT_VISITED "not visited"
#define URI_VISITED_RESOLUTION_TOPIC "visited-status-resolution"

#define URI_VISIT_SAVED "uri-visit-saved"




struct VisitData {
  VisitData()
  : placeId(0)
  , visitId(0)
  , sessionId(0)
  , hidden(true)
  , typed(false)
  , transitionType(PR_UINT32_MAX)
  , visitTime(0)
  , titleChanged(false)
  {
    guid.SetIsVoid(PR_TRUE);
    title.SetIsVoid(PR_TRUE);
  }

  VisitData(nsIURI* aURI,
            nsIURI* aReferrer = NULL)
  : placeId(0)
  , visitId(0)
  , sessionId(0)
  , hidden(true)
  , typed(false)
  , transitionType(PR_UINT32_MAX)
  , visitTime(0)
  , titleChanged(false)
  {
    (void)aURI->GetSpec(spec);
    (void)GetReversedHostname(aURI, revHost);
    if (aReferrer) {
      (void)aReferrer->GetSpec(referrerSpec);
    }
    guid.SetIsVoid(PR_TRUE);
    title.SetIsVoid(PR_TRUE);
  }

  







  void SetTransitionType(PRUint32 aTransitionType)
  {
    typed = aTransitionType == nsINavHistoryService::TRANSITION_TYPED;
    bool redirected =
      aTransitionType == nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY ||
      aTransitionType == nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT;
    hidden = GetHiddenState(redirected, aTransitionType);
    transitionType = aTransitionType;
  }

  








  bool IsSamePlaceAs(VisitData& aOther)
  {
    if (!spec.Equals(aOther.spec)) {
      return false;
    }

    aOther.placeId = placeId;
    aOther.guid = guid;
    return true;
  }

  PRInt64 placeId;
  nsCString guid;
  PRInt64 visitId;
  PRInt64 sessionId;
  nsCString spec;
  nsString revHost;
  bool hidden;
  bool typed;
  PRUint32 transitionType;
  PRTime visitTime;

  





  nsString title;

  nsCString referrerSpec;

  
  bool titleChanged;
};




namespace {












already_AddRefed<nsIURI>
GetURIFromJSObject(JSContext* aCtx,
                   JSObject* aObject,
                   const char* aProperty)
{
  jsval uriVal;
  JSBool rc = JS_GetProperty(aCtx, aObject, aProperty, &uriVal);
  NS_ENSURE_TRUE(rc, nsnull);

  if (!JSVAL_IS_PRIMITIVE(uriVal)) {
    nsCOMPtr<nsIXPConnect> xpc = mozilla::services::GetXPConnect();

    nsCOMPtr<nsIXPConnectWrappedNative> wrappedObj;
    nsresult rv = xpc->GetWrappedNativeOfJSObject(aCtx, JSVAL_TO_OBJECT(uriVal),
                                                  getter_AddRefs(wrappedObj));
    NS_ENSURE_SUCCESS(rv, nsnull);
    nsCOMPtr<nsIURI> uri = do_QueryWrappedNative(wrappedObj);
    return uri.forget();
  }
  return nsnull;
}













void
GetStringFromJSObject(JSContext* aCtx,
                      JSObject* aObject,
                      const char* aProperty,
                      nsString& _string)
{
  jsval val;
  JSBool rc = JS_GetProperty(aCtx, aObject, aProperty, &val);
  if (!rc || JSVAL_IS_VOID(val) ||
      !(JSVAL_IS_NULL(val) || JSVAL_IS_STRING(val))) {
    _string.SetIsVoid(PR_TRUE);
    return;
  }
  
  if (JSVAL_IS_NULL(val)) {
    _string.Truncate();
    return;
  }
  size_t length;
  const jschar* chars =
    JS_GetStringCharsZAndLength(aCtx, JSVAL_TO_STRING(val), &length);
  if (!chars) {
    _string.SetIsVoid(PR_TRUE);
    return;
  }
  _string.Assign(static_cast<const PRUnichar*>(chars), length);
}













template <typename IntType>
nsresult
GetIntFromJSObject(JSContext* aCtx,
                   JSObject* aObject,
                   const char* aProperty,
                   IntType* _int)
{
  jsval value;
  JSBool rc = JS_GetProperty(aCtx, aObject, aProperty, &value);
  NS_ENSURE_TRUE(rc, NS_ERROR_UNEXPECTED);
  if (JSVAL_IS_VOID(value)) {
    return NS_ERROR_INVALID_ARG;
  }
  NS_ENSURE_ARG(JSVAL_IS_PRIMITIVE(value));
  NS_ENSURE_ARG(JSVAL_IS_NUMBER(value));

  jsdouble num;
  rc = JS_ValueToNumber(aCtx, value, &num);
  NS_ENSURE_TRUE(rc, NS_ERROR_UNEXPECTED);
  NS_ENSURE_ARG(IntType(num) == num);

  *_int = IntType(num);
  return NS_OK;
}















nsresult
GetJSObjectFromArray(JSContext* aCtx,
                     JSObject* aArray,
                     jsuint aIndex,
                     JSObject** _rooter)
{
  NS_PRECONDITION(JS_IsArrayObject(aCtx, aArray),
                  "Must provide an object that is an array!");

  jsval value;
  JSBool rc = JS_GetElement(aCtx, aArray, aIndex, &value);
  NS_ENSURE_TRUE(rc, NS_ERROR_UNEXPECTED);
  NS_ENSURE_ARG(!JSVAL_IS_PRIMITIVE(value));
  *_rooter = JSVAL_TO_OBJECT(value);
  return NS_OK;
}

class VisitedQuery : public AsyncStatementCallback
{
public:
  static nsresult Start(nsIURI* aURI)
  {
    NS_PRECONDITION(aURI, "Null URI");

  
  
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    mozilla::dom::ContentChild* cpc =
      mozilla::dom::ContentChild::GetSingleton();
    NS_ASSERTION(cpc, "Content Protocol is NULL!");
    (void)cpc->SendStartVisitedQuery(aURI);
    return NS_OK;
  }

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
      DebugOnly<nsresult> rv =
        obsService->NotifyObservers(uri, URI_VISIT_SAVED, nsnull);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not notify observers");
    }

    History::GetService()->NotifyVisited(uri);

    return NS_OK;
  }
private:
  VisitData mPlace;
  VisitData mReferrer;
};




class NotifyTitleObservers : public nsRunnable
{
public:
  







  NotifyTitleObservers(const nsCString& aSpec,
                       const nsString& aTitle)
  : mSpec(aSpec)
  , mTitle(aTitle)
  {
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




class NotifyCompletion : public nsRunnable
{
public:
  NotifyCompletion(mozIVisitInfoCallback* aCallback,
                   const VisitData& aPlace,
                   nsresult aResult)
  : mCallback(aCallback)
  , mPlace(aPlace)
  , mResult(aResult)
  {
    NS_PRECONDITION(aCallback, "Must pass a non-null callback!");
  }

  NS_IMETHOD Run()
  {
    NS_PRECONDITION(NS_IsMainThread(),
                    "This should be called on the main thread");

    nsCOMPtr<nsIURI> referrerURI;
    if (!mPlace.referrerSpec.IsEmpty()) {
      (void)NS_NewURI(getter_AddRefs(referrerURI), mPlace.referrerSpec);
    }

    nsCOMPtr<mozIVisitInfo> visit =
      new VisitInfo(mPlace.visitId, mPlace.visitTime, mPlace.transitionType,
                    referrerURI.forget(), mPlace.sessionId);
    PlaceInfo::VisitsArray visits;
    (void)visits.AppendElement(visit);

    nsCOMPtr<nsIURI> uri;
    (void)NS_NewURI(getter_AddRefs(uri), mPlace.spec);

    
    nsCOMPtr<mozIPlaceInfo> place =
      new PlaceInfo(mPlace.placeId, mPlace.guid, uri.forget(), mPlace.title,
                    -1, visits);

    (void)mCallback->OnComplete(mResult, place);
    return NS_OK;
  }

private:
  




  mozIVisitInfoCallback* mCallback;
  VisitData mPlace;
  const nsresult mResult;
};













bool
CanAddURI(nsIURI* aURI,
          const nsCString& aGUID = EmptyCString(),
          mozIVisitInfoCallback* aCallback = NULL)
{
  nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(navHistory, false);

  PRBool canAdd;
  nsresult rv = navHistory->CanAddURI(aURI, &canAdd);
  if (NS_SUCCEEDED(rv) && canAdd) {
    return true;
  };

  
  if (aCallback) {
    
    
    
    NS_ADDREF(aCallback);

    VisitData place(aURI);
    place.guid = aGUID;
    nsCOMPtr<nsIRunnable> event =
      new NotifyCompletion(aCallback, place, NS_ERROR_INVALID_ARG);
    (void)NS_DispatchToMainThread(event);

    
    
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    (void)NS_ProxyRelease(mainThread, aCallback, PR_TRUE);
  }

  return false;
}




class InsertVisitedURIs : public nsRunnable
{
public:
  









  static nsresult Start(mozIStorageConnection* aConnection,
                        nsTArray<VisitData>& aPlaces,
                        mozIVisitInfoCallback* aCallback = NULL)
  {
    NS_PRECONDITION(NS_IsMainThread(),
                    "This should be called on the main thread");
    NS_PRECONDITION(aPlaces.Length() > 0, "Must pass a non-empty array!");

    nsRefPtr<InsertVisitedURIs> event =
      new InsertVisitedURIs(aConnection, aPlaces, aCallback);

    
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

    mozStorageTransaction transaction(mDBConn, PR_FALSE,
                                      mozIStorageConnection::TRANSACTION_IMMEDIATE);

    VisitData* lastPlace = NULL;
    for (nsTArray<VisitData>::size_type i = 0; i < mPlaces.Length(); i++) {
      VisitData& place = mPlaces.ElementAt(i);
      VisitData& referrer = mReferrers.ElementAt(i);

      
      
      bool known = (lastPlace && lastPlace->IsSamePlaceAs(place)) ||
                   mHistory->FetchPageInfo(place);

      FetchReferrerInfo(referrer, place);

      nsresult rv = DoDatabaseInserts(known, place, referrer);
      if (mCallback) {
        nsCOMPtr<nsIRunnable> event =
          new NotifyCompletion(mCallback, place, rv);
        nsresult rv2 = NS_DispatchToMainThread(event);
        NS_ENSURE_SUCCESS(rv2, rv2);
      }
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIRunnable> event = new NotifyVisitObservers(place, referrer);
      rv = NS_DispatchToMainThread(event);
      NS_ENSURE_SUCCESS(rv, rv);

      
      if ((!known && !place.title.IsVoid()) || place.titleChanged) {
        event = new NotifyTitleObservers(place.spec, place.title);
        rv = NS_DispatchToMainThread(event);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      lastPlace = &mPlaces.ElementAt(i);
    }

    nsresult rv = transaction.Commit();
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }
private:
  InsertVisitedURIs(mozIStorageConnection* aConnection,
                    nsTArray<VisitData>& aPlaces,
                    mozIVisitInfoCallback* aCallback)
  : mDBConn(aConnection)
  , mCallback(aCallback)
  , mHistory(History::GetService())
  {
    NS_PRECONDITION(NS_IsMainThread(),
                    "This should be called on the main thread");

    (void)mPlaces.SwapElements(aPlaces);
    (void)mReferrers.SetLength(mPlaces.Length());

    nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
    NS_ABORT_IF_FALSE(navHistory, "Could not get nsNavHistory?!");

    for (nsTArray<VisitData>::size_type i = 0; i < mPlaces.Length(); i++) {
      mReferrers[i].spec = mPlaces[i].referrerSpec;

      
      
      
      if (i == 0) {
        PRInt64 newSessionId = navHistory->GetNewSessionID();
        if (mPlaces[0].sessionId > newSessionId) {
          mPlaces[0].sessionId = newSessionId;
        }
      }

      
      
      
      
      
      if (mPlaces[i].sessionId <= 0 ||
          (i > 0 && mPlaces[i].sessionId >= mPlaces[0].sessionId)) {
        mPlaces[i].sessionId = navHistory->GetNewSessionID();
      }

#ifdef DEBUG
      nsCOMPtr<nsIURI> uri;
      (void)NS_NewURI(getter_AddRefs(uri), mPlaces[i].spec);
      NS_ASSERTION(CanAddURI(uri),
                   "Passed a VisitData with a URI we cannot add to history!");
#endif
    }

    
    NS_IF_ADDREF(mCallback);
  }

  virtual ~InsertVisitedURIs()
  {
    if (mCallback) {
      nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
      (void)NS_ProxyRelease(mainThread, mCallback, PR_TRUE);
    }
  }

  











  nsresult DoDatabaseInserts(bool aKnown,
                             VisitData& aPlace,
                             VisitData& aReferrer)
  {
    NS_PRECONDITION(!NS_IsMainThread(),
                    "This should not be called on the main thread");

    
    nsresult rv;
    if (aKnown) {
      rv = mHistory->UpdatePlace(aPlace);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    
    else {
      rv = mHistory->InsertPlace(aPlace);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      if (mCallback) {
        bool exists = mHistory->FetchPageInfo(aPlace);
        if (!exists) {
          NS_NOTREACHED("should have an entry in moz_places");
        }
      }
    }

    rv = AddVisit(aPlace, aReferrer);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    rv = UpdateFrecency(aPlace);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  










  bool FetchVisitInfo(VisitData& _place,
                      PRTime aThresholdStart = 0)
  {
    NS_PRECONDITION(!_place.spec.IsEmpty(), "must have a non-empty spec!");

    nsCOMPtr<mozIStorageStatement> stmt;
    
    if (_place.visitTime) {
      stmt = mHistory->syncStatements.GetCachedStatement(
        "SELECT id, session, visit_date "
        "FROM moz_historyvisits "
        "WHERE place_id = (SELECT id FROM moz_places WHERE url = :page_url) "
        "AND visit_date = :visit_date "
      );
      NS_ENSURE_TRUE(stmt, false);

      mozStorageStatementScoper scoper(stmt);
      nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("visit_date"),
                                          _place.visitTime);
      NS_ENSURE_SUCCESS(rv, rv);

      scoper.Abandon();
    }
    
    else {
      stmt = mHistory->syncStatements.GetCachedStatement(
        "SELECT id, session, visit_date "
        "FROM moz_historyvisits "
        "WHERE place_id = (SELECT id FROM moz_places WHERE url = :page_url) "
        "ORDER BY visit_date DESC "
      );
      NS_ENSURE_TRUE(stmt, false);
    }
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

  










  void FetchReferrerInfo(VisitData& aReferrer,
                         VisitData& aPlace)
  {
    if (aReferrer.spec.IsEmpty()) {
      return;
    }

    
    
    bool recentVisit = FetchVisitInfo(aReferrer, aPlace.visitTime);
    
    
    if (recentVisit) {
      aPlace.sessionId = aReferrer.sessionId;
    }
    
    
    else {
      
      
      
      aPlace.referrerSpec.Truncate();
      aReferrer.visitId = 0;
    }
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
      rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), _place.placeId);
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

    
    
    (void)FetchVisitInfo(_place);

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
        rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), aPlace.placeId);
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
        rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), aPlace.placeId);
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

  nsTArray<VisitData> mPlaces;
  nsTArray<VisitData> mReferrers;

  




  mozIVisitInfoCallback* mCallback;

  



  nsRefPtr<History> mHistory;
};




class SetPageTitle : public nsRunnable
{
public:
  









  static nsresult Start(mozIStorageConnection* aConnection,
                        nsIURI* aURI,
                        const nsAString& aTitle)
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

    
    bool exists = mHistory->FetchPageInfo(mPlace);
    if (!exists || !mPlace.titleChanged) {
      
      
      return NS_OK;
    }

    NS_ASSERTION(mPlace.placeId > 0,
                 "We somehow have an invalid place id here!");

    
    nsCOMPtr<mozIStorageStatement> stmt =
      mHistory->syncStatements.GetCachedStatement(
        "UPDATE moz_places "
        "SET title = :page_title "
        "WHERE id = :page_id "
      );
    NS_ENSURE_STATE(stmt);

    {
      mozStorageStatementScoper scoper(stmt);
      nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"),
                                          mPlace.placeId);
      NS_ENSURE_SUCCESS(rv, rv);
      
      
      if (mPlace.title.IsEmpty()) {
        rv = stmt->BindNullByName(NS_LITERAL_CSTRING("page_title"));
      }
      else {
        rv = stmt->BindStringByName(NS_LITERAL_CSTRING("page_title"),
                                    StringHead(mPlace.title, TITLE_LENGTH_MAX));
      }
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->Execute();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsIRunnable> event =
      new NotifyTitleObservers(mPlace.spec, mPlace.title);
    nsresult rv = NS_DispatchToMainThread(event);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

private:
  SetPageTitle(const nsCString& aSpec,
               const nsAString& aTitle)
  : mHistory(History::GetService())
  {
    mPlace.spec = aSpec;
    mPlace.title = aTitle;
  }

  VisitData mPlace;

  



  nsRefPtr<History> mHistory;
};









void
StoreAndNotifyEmbedVisit(VisitData& aPlace,
                         mozIVisitInfoCallback* aCallback = NULL)
{
  NS_PRECONDITION(aPlace.transitionType == nsINavHistoryService::TRANSITION_EMBED,
                  "Must only pass TRANSITION_EMBED visits to this!");
  NS_PRECONDITION(NS_IsMainThread(), "Must be called on the main thread!");

  nsCOMPtr<nsIURI> uri;
  (void)NS_NewURI(getter_AddRefs(uri), aPlace.spec);

  nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
  if (!navHistory || !uri) {
    return;
  }

  navHistory->registerEmbedVisit(uri, aPlace.visitTime);

  if (aCallback) {
    
    
    
    NS_ADDREF(aCallback);
    nsCOMPtr<nsIRunnable> event =
      new NotifyCompletion(aCallback, aPlace, NS_OK);
    (void)NS_DispatchToMainThread(event);

    
    
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    (void)NS_ProxyRelease(mainThread, aCallback, PR_TRUE);
  }

  VisitData noReferrer;
  nsCOMPtr<nsIRunnable> event = new NotifyVisitObservers(aPlace, noReferrer);
  (void)NS_DispatchToMainThread(event);
}

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

  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    mozilla::dom::ContentParent* cpp = 
      mozilla::dom::ContentParent::GetSingleton(PR_FALSE);
    if (cpp)
      (void)cpp->SendNotifyVisited(aURI);
  }

  
  
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

nsresult
History::InsertPlace(const VisitData& aPlace)
{
  NS_PRECONDITION(aPlace.placeId == 0, "should not have a valid place id!");
  NS_PRECONDITION(!NS_IsMainThread(), "must be called off of the main thread!");

  nsCOMPtr<mozIStorageStatement> stmt = syncStatements.GetCachedStatement(
      "INSERT INTO moz_places "
        "(url, title, rev_host, hidden, typed, guid) "
      "VALUES (:url, :title, :rev_host, :hidden, :typed, :guid) "
    );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindStringByName(NS_LITERAL_CSTRING("rev_host"),
                                       aPlace.revHost);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("url"), aPlace.spec);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (aPlace.title.IsEmpty()) {
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("title"));
  }
  else {
    rv = stmt->BindStringByName(NS_LITERAL_CSTRING("title"),
                                StringHead(aPlace.title, TITLE_LENGTH_MAX));
  }
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("typed"), aPlace.typed);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("hidden"), aPlace.hidden);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString guid(aPlace.guid);
  if (aPlace.guid.IsVoid()) {
    rv = GenerateGUID(guid);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("guid"), guid);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
History::UpdatePlace(const VisitData& aPlace)
{
  NS_PRECONDITION(!NS_IsMainThread(), "must be called off of the main thread!");
  NS_PRECONDITION(aPlace.placeId > 0, "must have a valid place id!");
  NS_PRECONDITION(!aPlace.guid.IsVoid(), "must have a guid!");

  nsCOMPtr<mozIStorageStatement> stmt = syncStatements.GetCachedStatement(
      "UPDATE moz_places "
      "SET title = :title, "
          "hidden = :hidden, "
          "typed = :typed, "
          "guid = :guid "
      "WHERE id = :page_id "
    );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv;
  
  if (aPlace.title.IsEmpty()) {
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("title"));
  }
  else {
    rv = stmt->BindStringByName(NS_LITERAL_CSTRING("title"),
                                StringHead(aPlace.title, TITLE_LENGTH_MAX));
  }
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("typed"), aPlace.typed);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("hidden"), aPlace.hidden);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("guid"), aPlace.guid);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"),
                             aPlace.placeId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

bool
History::FetchPageInfo(VisitData& _place)
{
  NS_PRECONDITION(!_place.spec.IsEmpty(), "must have a non-empty spec!");
  NS_PRECONDITION(!NS_IsMainThread(), "must be called off of the main thread!");

  nsCOMPtr<mozIStorageStatement> stmt = syncStatements.GetCachedStatement(
      "SELECT id, title, hidden, typed, guid "
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

  nsAutoString title;
  rv = stmt->GetString(1, title);
  NS_ENSURE_SUCCESS(rv, true);

  
  
  
  
  if (_place.title.IsVoid()) {
    _place.title = title;
  }
  
  else {
    _place.titleChanged = !(_place.title.Equals(title) ||
                            (_place.title.IsEmpty() && title.IsVoid()));
  }

  if (_place.hidden) {
    
    
    
    PRInt32 hidden;
    rv = stmt->GetInt32(2, &hidden);
    _place.hidden = !!hidden;
    NS_ENSURE_SUCCESS(rv, true);
  }

  if (!_place.typed) {
    
    
    PRInt32 typed;
    rv = stmt->GetInt32(3, &typed);
    _place.typed = !!typed;
    NS_ENSURE_SUCCESS(rv, true);
  }

  if (_place.guid.IsVoid()) {
    rv = stmt->GetUTF8String(4, _place.guid);
    NS_ENSURE_SUCCESS(rv, true);
  }

  return true;
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

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    mozilla::dom::ContentChild* cpc =
      mozilla::dom::ContentChild::GetSingleton();
    NS_ASSERTION(cpc, "Content Protocol is NULL!");
    (void)cpc->SendVisitURI(aURI, aLastVisitedURI, aFlags);
    return NS_OK;
  } 

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

  nsTArray<VisitData> placeArray(1);
  NS_ENSURE_TRUE(placeArray.AppendElement(VisitData(aURI, aLastVisitedURI)),
                 NS_ERROR_OUT_OF_MEMORY);
  VisitData& place = placeArray.ElementAt(0);
  NS_ENSURE_FALSE(place.spec.IsEmpty(), NS_ERROR_INVALID_ARG);

  place.visitTime = PR_Now();

  
  
  PRUint32 recentFlags = navHistory->GetRecentFlags(aURI);
  bool isFollowedLink = recentFlags & nsNavHistory::RECENT_ACTIVATED;

  
  
  
  
  

  if (!(aFlags & IHistory::TOP_LEVEL) && !isFollowedLink) {
    
    place.SetTransitionType(nsINavHistoryService::TRANSITION_EMBED);
  }
  else if (aFlags & IHistory::REDIRECT_TEMPORARY) {
    place.SetTransitionType(nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY);
  }
  else if (aFlags & IHistory::REDIRECT_PERMANENT) {
    place.SetTransitionType(nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT);
  }
  else if (recentFlags & nsNavHistory::RECENT_TYPED) {
    place.SetTransitionType(nsINavHistoryService::TRANSITION_TYPED);
  }
  else if (recentFlags & nsNavHistory::RECENT_BOOKMARKED) {
    place.SetTransitionType(nsINavHistoryService::TRANSITION_BOOKMARK);
  }
  else if (!(aFlags & IHistory::TOP_LEVEL) && isFollowedLink) {
    
    place.SetTransitionType(nsINavHistoryService::TRANSITION_FRAMED_LINK);
  }
  else {
    
    place.SetTransitionType(nsINavHistoryService::TRANSITION_LINK);
  }

  
  
  if (place.transitionType == nsINavHistoryService::TRANSITION_EMBED) {
    StoreAndNotifyEmbedVisit(place);
  }
  else {
    mozIStorageConnection* dbConn = GetDBConn();
    NS_ENSURE_STATE(dbConn);

    rv = InsertVisitedURIs::Start(dbConn, placeArray);
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
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    NS_PRECONDITION(aLink, "Must pass a non-null Link!");
  }

  
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
  
  
  
  else if (!aLink) {
    NS_ASSERTION(XRE_GetProcessType() == GeckoProcessType_Default,
                 "We should only ever get a null Link in the default process!");
    return NS_OK;
  }

  
  
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

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    mozilla::dom::ContentChild * cpc = 
      mozilla::dom::ContentChild::GetSingleton();
    NS_ASSERTION(cpc, "Content Protocol is NULL!");
    (void)cpc->SendSetURITitle(aURI, nsDependentString(aTitle));
    return NS_OK;
  } 

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

  mozIStorageConnection* dbConn = GetDBConn();
  NS_ENSURE_STATE(dbConn);

  rv = SetPageTitle::Start(dbConn, aURI, aTitle);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




NS_IMETHODIMP
History::UpdatePlaces(const jsval& aPlaceInfos,
                      mozIVisitInfoCallback* aCallback,
                      JSContext* aCtx)
{
  NS_ENSURE_TRUE(NS_IsMainThread(), NS_ERROR_UNEXPECTED);
  NS_ENSURE_TRUE(!JSVAL_IS_PRIMITIVE(aPlaceInfos), NS_ERROR_INVALID_ARG);

  jsuint infosLength = 1;
  JSObject* infos;
  if (JS_IsArrayObject(aCtx, JSVAL_TO_OBJECT(aPlaceInfos))) {
    infos = JSVAL_TO_OBJECT(aPlaceInfos);
    (void)JS_GetArrayLength(aCtx, infos, &infosLength);
    NS_ENSURE_ARG(infosLength > 0);
  }
  else {
    
    
    infos = JS_NewArrayObject(aCtx, 0, NULL);
    NS_ENSURE_TRUE(infos, NS_ERROR_OUT_OF_MEMORY);

    JSBool rc = JS_DefineElement(aCtx, infos, 0, aPlaceInfos, NULL, NULL, 0);
    NS_ENSURE_TRUE(rc, NS_ERROR_UNEXPECTED);
  }

  nsTArray<VisitData> visitData;
  for (jsuint i = 0; i < infosLength; i++) {
    JSObject* info;
    nsresult rv = GetJSObjectFromArray(aCtx, infos, i, &info);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIURI> uri = GetURIFromJSObject(aCtx, info, "uri");
    nsCString guid;
    {
      nsString fatGUID;
      GetStringFromJSObject(aCtx, info, "guid", fatGUID);
      if (fatGUID.IsVoid()) {
        guid.SetIsVoid(PR_TRUE);
      }
      else {
        guid = NS_ConvertUTF16toUTF8(fatGUID);
      }
    }

    
    
    if (uri && !CanAddURI(uri, guid, aCallback)) {
      continue;
    }

    
    NS_ENSURE_ARG(uri || !guid.IsVoid());

    
    bool isValidGUID = IsValidGUID(guid);
    NS_ENSURE_ARG(guid.IsVoid() || isValidGUID);

    nsString title;
    GetStringFromJSObject(aCtx, info, "title", title);

    JSObject* visits = NULL;
    {
      jsval visitsVal;
      JSBool rc = JS_GetProperty(aCtx, info, "visits", &visitsVal);
      NS_ENSURE_TRUE(rc, NS_ERROR_UNEXPECTED);
      if (!JSVAL_IS_PRIMITIVE(visitsVal)) {
        visits = JSVAL_TO_OBJECT(visitsVal);
        NS_ENSURE_ARG(JS_IsArrayObject(aCtx, visits));
      }
    }
    NS_ENSURE_ARG(visits);

    jsuint visitsLength = 0;
    if (visits) {
      (void)JS_GetArrayLength(aCtx, visits, &visitsLength);
    }
    NS_ENSURE_ARG(visitsLength > 0);

    
    visitData.SetCapacity(visitData.Length() + visitsLength);
    for (jsuint j = 0; j < visitsLength; j++) {
      JSObject* visit;
      rv = GetJSObjectFromArray(aCtx, visits, j, &visit);
      NS_ENSURE_SUCCESS(rv, rv);

      VisitData& data = *visitData.AppendElement(VisitData(uri));
      data.title = title;
      data.guid = guid;

      
      rv = GetIntFromJSObject(aCtx, visit, "visitDate", &data.visitTime);
      NS_ENSURE_SUCCESS(rv, rv);
      PRUint32 transitionType = 0;
      rv = GetIntFromJSObject(aCtx, visit, "transitionType", &transitionType);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_ARG_RANGE(transitionType,
                          nsINavHistoryService::TRANSITION_LINK,
                          nsINavHistoryService::TRANSITION_FRAMED_LINK);
      data.SetTransitionType(transitionType);

      
      
      if (transitionType == nsINavHistoryService::TRANSITION_EMBED) {
        StoreAndNotifyEmbedVisit(data, aCallback);
        visitData.RemoveElementAt(visitData.Length() - 1);
        continue;
      }

      
      rv = GetIntFromJSObject(aCtx, visit, "sessionId", &data.sessionId);
      if (rv == NS_ERROR_INVALID_ARG) {
        data.sessionId = 0;
      }
      else {
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      nsCOMPtr<nsIURI> referrer = GetURIFromJSObject(aCtx, visit,
                                                     "referrerURI");
      if (referrer) {
        (void)referrer->GetSpec(data.referrerSpec);
      }
    }
  }

  mozIStorageConnection* dbConn = GetDBConn();
  NS_ENSURE_STATE(dbConn);

  
  
  
  if (visitData.Length()) {
    nsresult rv = InsertVisitedURIs::Start(dbConn, visitData, aCallback);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  nsCOMPtr<nsIEventTarget> backgroundThread = do_GetInterface(dbConn);
  NS_ENSURE_TRUE(backgroundThread, NS_ERROR_UNEXPECTED);
  nsRefPtr<PlacesEvent> completeEvent =
    new PlacesEvent(TOPIC_UPDATEPLACES_COMPLETE, true);
  (void)backgroundThread->Dispatch(completeEvent, 0);

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




NS_IMPL_THREADSAFE_ISUPPORTS3(
  History
, IHistory
, mozIAsyncHistory
, nsIObserver
)

} 
} 
