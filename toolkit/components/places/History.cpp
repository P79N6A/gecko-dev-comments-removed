





#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Util.h"

#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/ContentParent.h"
#include "nsXULAppAPI.h"

#include "History.h"
#include "nsNavHistory.h"
#include "nsNavBookmarks.h"
#include "nsAnnotationService.h"
#include "Helpers.h"
#include "PlaceInfo.h"
#include "VisitInfo.h"
#include "nsPlacesMacros.h"

#include "mozilla/storage.h"
#include "mozilla/dom/Link.h"
#include "nsDocShellCID.h"
#include "mozilla/Services.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"
#include "nsIXPConnect.h"
#include "mozilla/unused.h"
#include "nsContentUtils.h"
#include "nsIMemoryReporter.h"
#include "mozilla/ipc/URIUtils.h"


#define VISIT_OBSERVERS_INITIAL_CACHE_SIZE 128

using namespace mozilla::dom;
using namespace mozilla::ipc;
using mozilla::unused;

namespace mozilla {
namespace places {




#define URI_VISITED "visited"
#define URI_NOT_VISITED "not visited"
#define URI_VISITED_RESOLUTION_TOPIC "visited-status-resolution"

#define URI_VISIT_SAVED "uri-visit-saved"

#define DESTINATIONFILEURI_ANNO \
        NS_LITERAL_CSTRING("downloads/destinationFileURI")
#define DESTINATIONFILENAME_ANNO \
        NS_LITERAL_CSTRING("downloads/destinationFileName")




struct VisitData {
  VisitData()
  : placeId(0)
  , visitId(0)
  , sessionId(0)
  , hidden(true)
  , typed(false)
  , transitionType(UINT32_MAX)
  , visitTime(0)
  , frecency(-1)
  , titleChanged(false)
  {
    guid.SetIsVoid(true);
    title.SetIsVoid(true);
  }

  VisitData(nsIURI* aURI,
            nsIURI* aReferrer = NULL)
  : placeId(0)
  , visitId(0)
  , sessionId(0)
  , hidden(true)
  , typed(false)
  , transitionType(UINT32_MAX)
  , visitTime(0)
  , frecency(-1)
  , titleChanged(false)
  {
    (void)aURI->GetSpec(spec);
    (void)GetReversedHostname(aURI, revHost);
    if (aReferrer) {
      (void)aReferrer->GetSpec(referrerSpec);
    }
    guid.SetIsVoid(true);
    title.SetIsVoid(true);
  }

  






  void SetTransitionType(uint32_t aTransitionType)
  {
    typed = aTransitionType == nsINavHistoryService::TRANSITION_TYPED;
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

  int64_t placeId;
  nsCString guid;
  int64_t visitId;
  int64_t sessionId;
  nsCString spec;
  nsString revHost;
  bool hidden;
  bool typed;
  uint32_t transitionType;
  PRTime visitTime;
  int32_t frecency;

  





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
  NS_ENSURE_TRUE(rc, nullptr);

  if (!JSVAL_IS_PRIMITIVE(uriVal)) {
    nsCOMPtr<nsIXPConnect> xpc = mozilla::services::GetXPConnect();

    nsCOMPtr<nsIXPConnectWrappedNative> wrappedObj;
    nsresult rv = xpc->GetWrappedNativeOfJSObject(aCtx, JSVAL_TO_OBJECT(uriVal),
                                                  getter_AddRefs(wrappedObj));
    NS_ENSURE_SUCCESS(rv, nullptr);
    nsCOMPtr<nsIURI> uri = do_QueryWrappedNative(wrappedObj);
    return uri.forget();
  }
  return nullptr;
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
    _string.SetIsVoid(true);
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
    _string.SetIsVoid(true);
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

  double num;
  rc = JS_ValueToNumber(aCtx, value, &num);
  NS_ENSURE_TRUE(rc, NS_ERROR_UNEXPECTED);
  NS_ENSURE_ARG(IntType(num) == num);

  *_int = IntType(num);
  return NS_OK;
}















nsresult
GetJSObjectFromArray(JSContext* aCtx,
                     JSObject* aArray,
                     uint32_t aIndex,
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
  static nsresult Start(nsIURI* aURI,
                        mozIVisitedStatusCallback* aCallback=nullptr)
  {
    NS_PRECONDITION(aURI, "Null URI");

  
  
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    URIParams uri;
    SerializeURI(aURI, uri);

    mozilla::dom::ContentChild* cpc =
      mozilla::dom::ContentChild::GetSingleton();
    NS_ASSERTION(cpc, "Content Protocol is NULL!");
    (void)cpc->SendStartVisitedQuery(uri);
    return NS_OK;
  }

    nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
    NS_ENSURE_STATE(navHistory);
    if (navHistory->hasEmbedVisit(aURI)) {
      nsRefPtr<VisitedQuery> callback = new VisitedQuery(aURI, aCallback, true);
      NS_ENSURE_TRUE(callback, NS_ERROR_OUT_OF_MEMORY);
      
      nsCOMPtr<nsIRunnable> event =
        NS_NewRunnableMethod(callback, &VisitedQuery::NotifyVisitedStatus);
      NS_DispatchToMainThread(event);

      return NS_OK;
    }

    History* history = History::GetService();
    NS_ENSURE_STATE(history);
    mozIStorageAsyncStatement* stmt = history->GetIsVisitedStatement();
    NS_ENSURE_STATE(stmt);

    
    nsresult rv = URIBinder::Bind(stmt, 0, aURI);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<VisitedQuery> callback = new VisitedQuery(aURI, aCallback);
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

  NS_IMETHOD HandleCompletion(uint16_t aReason)
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
    
    if (mCallback) {
      mCallback->IsVisited(mURI, mIsVisited);
      return NS_OK;
    }

    if (mIsVisited) {
      History* history = History::GetService();
      NS_ENSURE_STATE(history);
      history->NotifyVisited(mURI);
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
  VisitedQuery(nsIURI* aURI,
               mozIVisitedStatusCallback *aCallback=nullptr,
               bool aIsVisited=false)
  : mURI(aURI)
  , mCallback(aCallback)
  , mIsVisited(aIsVisited)
  {
  }

  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<mozIVisitedStatusCallback> mCallback;
  bool mIsVisited;
};




class NotifyVisitObservers : public nsRunnable
{
public:
  NotifyVisitObservers(VisitData& aPlace,
                       VisitData& aReferrer)
  : mPlace(aPlace)
  , mReferrer(aReferrer)
  , mHistory(History::GetService())
  {
  }

  NS_IMETHOD Run()
  {
    NS_PRECONDITION(NS_IsMainThread(),
                    "This should be called on the main thread");
    
    if (mHistory->IsShuttingDown()) {
      
      return NS_OK;
    }

    nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
    if (!navHistory) {
      NS_WARNING("Trying to notify about a visit but cannot get the history service!");
      return NS_OK;
    }

    nsCOMPtr<nsIURI> uri;
    (void)NS_NewURI(getter_AddRefs(uri), mPlace.spec);

    
    
    if (mPlace.transitionType != nsINavHistoryService::TRANSITION_EMBED) {
      navHistory->NotifyOnVisit(uri, mPlace.visitId, mPlace.visitTime,
                                mPlace.sessionId, mReferrer.visitId,
                                mPlace.transitionType, mPlace.guid,
                                mPlace.hidden);
    }

    nsCOMPtr<nsIObserverService> obsService =
      mozilla::services::GetObserverService();
    if (obsService) {
      DebugOnly<nsresult> rv =
        obsService->NotifyObservers(uri, URI_VISIT_SAVED, nullptr);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Could not notify observers");
    }

    History* history = History::GetService();
    NS_ENSURE_STATE(history);
    history->AppendToRecentlyVisitedURIs(uri);
    history->NotifyVisited(uri);

    return NS_OK;
  }
private:
  VisitData mPlace;
  VisitData mReferrer;
  nsRefPtr<History> mHistory;
};




class NotifyTitleObservers : public nsRunnable
{
public:
  







  NotifyTitleObservers(const nsCString& aSpec,
                       const nsString& aTitle,
                       const nsCString& aGUID)
  : mSpec(aSpec)
  , mTitle(aTitle)
  , mGUID(aGUID)
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
    navHistory->NotifyTitleChange(uri, mTitle, mGUID);

    return NS_OK;
  }
private:
  const nsCString mSpec;
  const nsString mTitle;
  const nsCString mGUID;
};




class NotifyVisitInfoCallback : public nsRunnable
{
public:
  NotifyVisitInfoCallback(mozIVisitInfoCallback* aCallback,
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
    if (NS_SUCCEEDED(mResult)) {
      (void)mCallback->HandleResult(place);
    }
    else {
      (void)mCallback->HandleError(mResult, place);
    }

    return NS_OK;
  }

private:
  




  mozIVisitInfoCallback* mCallback;
  VisitData mPlace;
  const nsresult mResult;
};




class NotifyCompletion : public nsRunnable
{
public:
  NotifyCompletion(mozIVisitInfoCallback* aCallback)
  : mCallback(aCallback)
  {
    NS_PRECONDITION(aCallback, "Must pass a non-null callback!");
  }

  NS_IMETHOD Run()
  {
    if (NS_IsMainThread()) {
      (void)mCallback->HandleCompletion();
    }
    else {
      (void)NS_DispatchToMainThread(this);

      
      
      nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
      (void)NS_ProxyRelease(mainThread, mCallback, true);
    }
    return NS_OK;
  }

private:
  





  mozIVisitInfoCallback* mCallback;
};













bool
CanAddURI(nsIURI* aURI,
          const nsCString& aGUID = EmptyCString(),
          mozIVisitInfoCallback* aCallback = NULL)
{
  nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(navHistory, false);

  bool canAdd;
  nsresult rv = navHistory->CanAddURI(aURI, &canAdd);
  if (NS_SUCCEEDED(rv) && canAdd) {
    return true;
  };

  
  if (aCallback) {
    
    
    
    NS_ADDREF(aCallback);

    VisitData place(aURI);
    place.guid = aGUID;
    nsCOMPtr<nsIRunnable> event =
      new NotifyVisitInfoCallback(aCallback, place, NS_ERROR_INVALID_ARG);
    (void)NS_DispatchToMainThread(event);

    
    
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    (void)NS_ProxyRelease(mainThread, aCallback, true);
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

    
    MutexAutoLock lockedScope(mHistory->GetShutdownMutex());
    if(mHistory->IsShuttingDown()) {
      
      return NS_OK;
    }

    mozStorageTransaction transaction(mDBConn, false,
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
          new NotifyVisitInfoCallback(mCallback, place, rv);
        nsresult rv2 = NS_DispatchToMainThread(event);
        NS_ENSURE_SUCCESS(rv2, rv2);
      }
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIRunnable> event = new NotifyVisitObservers(place, referrer);
      rv = NS_DispatchToMainThread(event);
      NS_ENSURE_SUCCESS(rv, rv);

      
      if ((!known && !place.title.IsVoid()) || place.titleChanged) {
        event = new NotifyTitleObservers(place.spec, place.title, place.guid);
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
        int64_t newSessionId = navHistory->GetNewSessionID();
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
      (void)NS_ProxyRelease(mainThread, mCallback, true);
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

      
      
      
      if (mCallback || aPlace.guid.IsEmpty()) {
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
      stmt = mHistory->GetStatement(
        "SELECT id, session, visit_date "
        "FROM moz_historyvisits "
        "WHERE place_id = (SELECT id FROM moz_places WHERE url = :page_url) "
        "AND visit_date = :visit_date "
      );
      NS_ENSURE_TRUE(stmt, false);

      mozStorageStatementScoper scoper(stmt);
      nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("visit_date"),
                                          _place.visitTime);
      NS_ENSURE_SUCCESS(rv, false);

      scoper.Abandon();
    }
    
    else {
      stmt = mHistory->GetStatement(
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

    bool hasResult;
    rv = stmt->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, false);
    if (!hasResult) {
      return false;
    }

    rv = stmt->GetInt64(0, &_place.visitId);
    NS_ENSURE_SUCCESS(rv, false);
    rv = stmt->GetInt64(1, &_place.sessionId);
    NS_ENSURE_SUCCESS(rv, false);
    rv = stmt->GetInt64(2, reinterpret_cast<int64_t*>(&_place.visitTime));
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
      stmt = mHistory->GetStatement(
        "INSERT INTO moz_historyvisits "
          "(from_visit, place_id, visit_date, visit_type, session) "
        "VALUES (:from_visit, :page_id, :visit_date, :visit_type, :session) "
      );
      NS_ENSURE_STATE(stmt);
      rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), _place.placeId);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      stmt = mHistory->GetStatement(
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
    uint32_t transitionType = _place.transitionType;
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
    
    if (aPlace.frecency == 0) {
      return NS_OK;
    }

    nsresult rv;
    { 
      nsCOMPtr<mozIStorageStatement> stmt;
      if (aPlace.placeId) {
        stmt = mHistory->GetStatement(
          "UPDATE moz_places "
          "SET frecency = CALCULATE_FRECENCY(:page_id) "
          "WHERE id = :page_id"
        );
        NS_ENSURE_STATE(stmt);
        rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), aPlace.placeId);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        stmt = mHistory->GetStatement(
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

    if (!aPlace.hidden) {
      
      nsCOMPtr<mozIStorageStatement> stmt;
      if (aPlace.placeId) {
        stmt = mHistory->GetStatement(
          "UPDATE moz_places "
          "SET hidden = 0 "
          "WHERE id = :page_id AND frecency <> 0"
        );
        NS_ENSURE_STATE(stmt);
        rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), aPlace.placeId);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        stmt = mHistory->GetStatement(
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
      mHistory->GetStatement(
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
      new NotifyTitleObservers(mPlace.spec, mPlace.title, mPlace.guid);
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




class SetDownloadAnnotations MOZ_FINAL : public mozIVisitInfoCallback
{
public:
  NS_DECL_ISUPPORTS

  SetDownloadAnnotations(nsIURI* aDestination)
  : mDestination(aDestination)
  , mHistory(History::GetService())
  {
    MOZ_ASSERT(mDestination);
    MOZ_ASSERT(NS_IsMainThread());
  }

  NS_IMETHOD HandleError(nsresult aResultCode, mozIPlaceInfo *aPlaceInfo)
  {
    
    return NS_OK;
  }

  NS_IMETHOD HandleResult(mozIPlaceInfo *aPlaceInfo)
  {
    
    nsCOMPtr<nsIFileURL> destinationFileURL = do_QueryInterface(mDestination);
    if (!destinationFileURL) {
      return NS_OK;
    }

    nsCOMPtr<nsIURI> source;
    nsresult rv = aPlaceInfo->GetUri(getter_AddRefs(source));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> destinationFile;
    rv = destinationFileURL->GetFile(getter_AddRefs(destinationFile));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString destinationFileName;
    rv = destinationFile->GetLeafName(destinationFileName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoCString destinationURISpec;
    rv = destinationFileURL->GetSpec(destinationURISpec);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
    NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);

    rv = annosvc->SetPageAnnotationString(
      source,
      DESTINATIONFILEURI_ANNO,
      NS_ConvertUTF8toUTF16(destinationURISpec),
      0,
      nsIAnnotationService::EXPIRE_WITH_HISTORY
    );
    NS_ENSURE_SUCCESS(rv, rv);

    rv = annosvc->SetPageAnnotationString(
      source,
      DESTINATIONFILENAME_ANNO,
      destinationFileName,
      0,
      nsIAnnotationService::EXPIRE_WITH_HISTORY
    );
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString title;
    rv = aPlaceInfo->GetTitle(title);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    if (title.IsEmpty()) {
      rv = mHistory->SetURITitle(source, destinationFileName);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  NS_IMETHOD HandleCompletion()
  {
    return NS_OK;
  }

private:
  nsCOMPtr<nsIURI> mDestination;

  



  nsRefPtr<History> mHistory;
};
NS_IMPL_ISUPPORTS1(
  SetDownloadAnnotations,
  mozIVisitInfoCallback
)









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
      new NotifyVisitInfoCallback(aCallback, aPlace, NS_OK);
    (void)NS_DispatchToMainThread(event);

    
    
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    (void)NS_ProxyRelease(mainThread, aCallback, true);
  }

  VisitData noReferrer;
  nsCOMPtr<nsIRunnable> event = new NotifyVisitObservers(aPlace, noReferrer);
  (void)NS_DispatchToMainThread(event);
}

NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(HistoryLinksHashtableMallocSizeOf,
                                     "history-links-hashtable")

int64_t GetHistoryObserversSize()
{
  History* history = History::GetService();
  return history ?
         history->SizeOfIncludingThis(HistoryLinksHashtableMallocSizeOf) : 0;
}

NS_MEMORY_REPORTER_IMPLEMENT(HistoryService,
  "explicit/history-links-hashtable",
  KIND_HEAP,
  UNITS_BYTES,
  GetHistoryObserversSize,
  "Memory used by the hashtable of observers Places uses to notify objects of "
  "changes to links' visited state.")

} 




History* History::gService = NULL;

History::History()
  : mShuttingDown(false)
  , mShutdownMutex("History::mShutdownMutex")
  , mRecentlyVisitedURIsNextIndex(0)
{
  NS_ASSERTION(!gService, "Ruh-roh!  This service has already been created!");
  gService = this;

  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  NS_WARN_IF_FALSE(os, "Observer service was not found!");
  if (os) {
    (void)os->AddObserver(this, TOPIC_PLACES_SHUTDOWN, false);
  }

  NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(HistoryService));
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

NS_IMETHODIMP
History::NotifyVisited(nsIURI* aURI)
{
  NS_ENSURE_ARG(aURI);

  nsAutoScriptBlocker scriptBlocker;

  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    nsTArray<ContentParent*> cplist;
    ContentParent::GetAll(cplist);

    if (!cplist.IsEmpty()) {
      URIParams uri;
      SerializeURI(aURI, uri);
      for (uint32_t i = 0; i < cplist.Length(); ++i) {
        unused << cplist[i]->SendNotifyVisited(uri);
      }
    }
  }

  
  
  if (!mObservers.IsInitialized()) {
    return NS_OK;
  }

  
  
  KeyClass* key = mObservers.GetEntry(aURI);
  if (!key) {
    return NS_OK;
  }

  
  {
    
    ObserverArray::ForwardIterator iter(key->array);
    while (iter.HasMore()) {
      Link* link = iter.GetNext();
      link->SetLinkState(eLinkState_Visited);
      
      
      NS_ABORT_IF_FALSE(key == mObservers.GetEntry(aURI),
                        "The URIs hash mutated!");
    }
  }

  
  mObservers.RemoveEntry(aURI);
  return NS_OK;
}

mozIStorageAsyncStatement*
History::GetIsVisitedStatement()
{
  if (mIsVisitedStatement) {
    return mIsVisitedStatement;
  }

  
  if (!mReadOnlyDBConn) {
    mozIStorageConnection* dbConn = GetDBConn();
    NS_ENSURE_TRUE(dbConn, nullptr);

    (void)dbConn->Clone(true, getter_AddRefs(mReadOnlyDBConn));
    NS_ENSURE_TRUE(mReadOnlyDBConn, nullptr);
  }

  
  nsresult rv = mReadOnlyDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "SELECT 1 "
    "FROM moz_places h "
    "WHERE url = ?1 "
      "AND last_visit_date NOTNULL "
  ),  getter_AddRefs(mIsVisitedStatement));
  NS_ENSURE_SUCCESS(rv, nullptr);
  return mIsVisitedStatement;
}

nsresult
History::InsertPlace(const VisitData& aPlace)
{
  NS_PRECONDITION(aPlace.placeId == 0, "should not have a valid place id!");
  NS_PRECONDITION(!NS_IsMainThread(), "must be called off of the main thread!");

  nsCOMPtr<mozIStorageStatement> stmt = GetStatement(
      "INSERT INTO moz_places "
        "(url, title, rev_host, hidden, typed, frecency, guid) "
      "VALUES (:url, :title, :rev_host, :hidden, :typed, :frecency, :guid) "
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
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("frecency"), aPlace.frecency);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("hidden"), aPlace.hidden);
  NS_ENSURE_SUCCESS(rv, rv);
  nsAutoCString guid(aPlace.guid);
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

  nsCOMPtr<mozIStorageStatement> stmt = GetStatement(
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

  nsCOMPtr<mozIStorageStatement> stmt = GetStatement(
      "SELECT id, title, hidden, typed, guid "
      "FROM moz_places "
      "WHERE url = :page_url "
    );
  NS_ENSURE_TRUE(stmt, false);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"),
                                _place.spec);
  NS_ENSURE_SUCCESS(rv, false);

  bool hasResult;
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
    
    
    
    int32_t hidden;
    rv = stmt->GetInt32(2, &hidden);
    _place.hidden = !!hidden;
    NS_ENSURE_SUCCESS(rv, true);
  }

  if (!_place.typed) {
    
    
    int32_t typed;
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

 size_t
History::SizeOfEntryExcludingThis(KeyClass* aEntry, nsMallocSizeOfFun aMallocSizeOf, void *)
{
  return aEntry->array.SizeOfExcludingThis(aMallocSizeOf);
}

size_t
History::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOfThis)
{
  return aMallocSizeOfThis(this) +
         mObservers.SizeOfExcludingThis(SizeOfEntryExcludingThis, aMallocSizeOfThis);
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
    NS_ENSURE_TRUE(gService, nullptr);
  }

  NS_ADDREF(gService);
  return gService;
}

mozIStorageConnection*
History::GetDBConn()
{
  if (!mDB) {
    mDB = Database::GetDatabase();
    NS_ENSURE_TRUE(mDB, nullptr);
  }
  return mDB->MainConn();
}

void
History::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  MutexAutoLock lockedScope(mShutdownMutex);
  MOZ_ASSERT(!mShuttingDown && "Shutdown was called more than once!");

  mShuttingDown = true;

  if (mReadOnlyDBConn) {
    if (mIsVisitedStatement) {
      (void)mIsVisitedStatement->Finalize();
    }
    (void)mReadOnlyDBConn->AsyncClose(nullptr);
  }
}

void
History::AppendToRecentlyVisitedURIs(nsIURI* aURI) {
  if (mRecentlyVisitedURIs.Length() < RECENTLY_VISITED_URI_SIZE) {
    
    mRecentlyVisitedURIs.AppendElement(aURI);
  } else {
    
    mRecentlyVisitedURIsNextIndex %= RECENTLY_VISITED_URI_SIZE;
    mRecentlyVisitedURIs.ElementAt(mRecentlyVisitedURIsNextIndex) = aURI;
    mRecentlyVisitedURIsNextIndex++;
  }
}

inline bool
History::IsRecentlyVisitedURI(nsIURI* aURI) {
  bool equals = false;
  RecentlyVisitedArray::index_type i;
  for (i = 0; i < mRecentlyVisitedURIs.Length() && !equals; ++i) {
    aURI->Equals(mRecentlyVisitedURIs.ElementAt(i), &equals);
  }
  return equals;
}




NS_IMETHODIMP
History::VisitURI(nsIURI* aURI,
                  nsIURI* aLastVisitedURI,
                  uint32_t aFlags)
{
  NS_PRECONDITION(aURI, "URI should not be NULL.");
  if (mShuttingDown) {
    return NS_OK;
  }

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    URIParams uri;
    SerializeURI(aURI, uri);

    OptionalURIParams lastVisitedURI;
    SerializeURI(aLastVisitedURI, lastVisitedURI);

    mozilla::dom::ContentChild* cpc =
      mozilla::dom::ContentChild::GetSingleton();
    NS_ASSERTION(cpc, "Content Protocol is NULL!");
    (void)cpc->SendVisitURI(uri, lastVisitedURI, aFlags);
    return NS_OK;
  } 

  nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(navHistory, NS_ERROR_OUT_OF_MEMORY);

  
  bool canAdd;
  nsresult rv = navHistory->CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd) {
    return NS_OK;
  }

  if (aLastVisitedURI) {
    bool same;
    rv = aURI->Equals(aLastVisitedURI, &same);
    NS_ENSURE_SUCCESS(rv, rv);
    if (same && IsRecentlyVisitedURI(aURI)) {
      
      return NS_OK;
    }
  }

  nsTArray<VisitData> placeArray(1);
  NS_ENSURE_TRUE(placeArray.AppendElement(VisitData(aURI, aLastVisitedURI)),
                 NS_ERROR_OUT_OF_MEMORY);
  VisitData& place = placeArray.ElementAt(0);
  NS_ENSURE_FALSE(place.spec.IsEmpty(), NS_ERROR_INVALID_ARG);

  place.visitTime = PR_Now();

  
  
  uint32_t recentFlags = navHistory->GetRecentFlags(aURI);
  bool isFollowedLink = recentFlags & nsNavHistory::RECENT_ACTIVATED;

  
  
  
  
  

  uint32_t transitionType = nsINavHistoryService::TRANSITION_LINK;

  if (!(aFlags & IHistory::TOP_LEVEL) && !isFollowedLink) {
    
    transitionType = nsINavHistoryService::TRANSITION_EMBED;
  }
  else if (aFlags & IHistory::REDIRECT_TEMPORARY) {
    transitionType = nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY;
  }
  else if (aFlags & IHistory::REDIRECT_PERMANENT) {
    transitionType = nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT;
  }
  else if (recentFlags & nsNavHistory::RECENT_TYPED) {
    transitionType = nsINavHistoryService::TRANSITION_TYPED;
  }
  else if (recentFlags & nsNavHistory::RECENT_BOOKMARKED) {
    transitionType = nsINavHistoryService::TRANSITION_BOOKMARK;
  }
  else if (!(aFlags & IHistory::TOP_LEVEL) && isFollowedLink) {
    
    transitionType = nsINavHistoryService::TRANSITION_FRAMED_LINK;
  }

  place.SetTransitionType(transitionType);
  place.hidden = GetHiddenState(aFlags & IHistory::REDIRECT_SOURCE,
                                transitionType);

  
  if (aFlags & IHistory::UNRECOVERABLE_ERROR) {
    place.frecency = 0;
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
    obsService->NotifyObservers(aURI, NS_LINK_VISITED_EVENT_TOPIC, nullptr);
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
    mObservers.Init(VISIT_OBSERVERS_INITIAL_CACHE_SIZE);
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
    URIParams uri;
    SerializeURI(aURI, uri);

    mozilla::dom::ContentChild * cpc = 
      mozilla::dom::ContentChild::GetSingleton();
    NS_ASSERTION(cpc, "Content Protocol is NULL!");
    (void)cpc->SendSetURITitle(uri, PromiseFlatString(aTitle));
    return NS_OK;
  } 

  nsNavHistory* navHistory = nsNavHistory::GetHistoryService();

  
  
  
  
  
  
  
  
  
  NS_ENSURE_TRUE(navHistory, NS_ERROR_FAILURE);

  bool canAdd;
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
History::AddDownload(nsIURI* aSource, nsIURI* aReferrer,
                     PRTime aStartTime, nsIURI* aDestination)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ENSURE_ARG(aSource);

  ENSURE_NOT_PRIVATE_BROWSING;

  if (mShuttingDown) {
    return NS_OK;
  }

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    NS_ERROR("Cannot add downloads to history from content process!");
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(navHistory, NS_ERROR_OUT_OF_MEMORY);

  
  bool canAdd;
  nsresult rv = navHistory->CanAddURI(aSource, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd) {
    return NS_OK;
  }

  nsTArray<VisitData> placeArray(1);
  NS_ENSURE_TRUE(placeArray.AppendElement(VisitData(aSource, aReferrer)),
                 NS_ERROR_OUT_OF_MEMORY);
  VisitData& place = placeArray.ElementAt(0);
  NS_ENSURE_FALSE(place.spec.IsEmpty(), NS_ERROR_INVALID_ARG);

  place.visitTime = aStartTime;
  place.SetTransitionType(nsINavHistoryService::TRANSITION_DOWNLOAD);
  place.hidden = false;

  mozIStorageConnection* dbConn = GetDBConn();
  NS_ENSURE_STATE(dbConn);

  nsCOMPtr<mozIVisitInfoCallback> callback = aDestination
                                  ? new SetDownloadAnnotations(aDestination)
                                  : nullptr;

  rv = InsertVisitedURIs::Start(dbConn, placeArray, callback);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIObserverService> obsService =
    mozilla::services::GetObserverService();
  if (obsService) {
    obsService->NotifyObservers(aSource, NS_LINK_VISITED_EVENT_TOPIC, nullptr);
  }

  return NS_OK;
}




NS_IMETHODIMP
History::UpdatePlaces(const jsval& aPlaceInfos,
                      mozIVisitInfoCallback* aCallback,
                      JSContext* aCtx)
{
  NS_ENSURE_TRUE(NS_IsMainThread(), NS_ERROR_UNEXPECTED);
  NS_ENSURE_TRUE(!JSVAL_IS_PRIMITIVE(aPlaceInfos), NS_ERROR_INVALID_ARG);

  uint32_t infosLength = 1;
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
  for (uint32_t i = 0; i < infosLength; i++) {
    JSObject* info;
    nsresult rv = GetJSObjectFromArray(aCtx, infos, i, &info);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIURI> uri = GetURIFromJSObject(aCtx, info, "uri");
    nsCString guid;
    {
      nsString fatGUID;
      GetStringFromJSObject(aCtx, info, "guid", fatGUID);
      if (fatGUID.IsVoid()) {
        guid.SetIsVoid(true);
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

    uint32_t visitsLength = 0;
    if (visits) {
      (void)JS_GetArrayLength(aCtx, visits, &visitsLength);
    }
    NS_ENSURE_ARG(visitsLength > 0);

    
    visitData.SetCapacity(visitData.Length() + visitsLength);
    for (uint32_t j = 0; j < visitsLength; j++) {
      JSObject* visit;
      rv = GetJSObjectFromArray(aCtx, visits, j, &visit);
      NS_ENSURE_SUCCESS(rv, rv);

      VisitData& data = *visitData.AppendElement(VisitData(uri));
      data.title = title;
      data.guid = guid;

      
      rv = GetIntFromJSObject(aCtx, visit, "visitDate", &data.visitTime);
      NS_ENSURE_SUCCESS(rv, rv);
      uint32_t transitionType = 0;
      rv = GetIntFromJSObject(aCtx, visit, "transitionType", &transitionType);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_ARG_RANGE(transitionType,
                          nsINavHistoryService::TRANSITION_LINK,
                          nsINavHistoryService::TRANSITION_FRAMED_LINK);
      data.SetTransitionType(transitionType);
      data.hidden = GetHiddenState(false, transitionType);

      
      
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

  
  
  
  
  if (aCallback) {
    
    
    
    
    NS_ADDREF(aCallback);

    nsCOMPtr<nsIEventTarget> backgroundThread = do_GetInterface(dbConn);
    NS_ENSURE_TRUE(backgroundThread, NS_ERROR_UNEXPECTED);
    nsCOMPtr<nsIRunnable> event = new NotifyCompletion(aCallback);
    (void)backgroundThread->Dispatch(event, NS_DISPATCH_NORMAL);
  }

  return NS_OK;
}

NS_IMETHODIMP
History::IsURIVisited(nsIURI* aURI,
                      mozIVisitedStatusCallback* aCallback)
{
  NS_ENSURE_STATE(NS_IsMainThread());
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG(aCallback);

  nsresult rv = VisitedQuery::Start(aURI, aCallback);
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




NS_IMPL_THREADSAFE_ISUPPORTS4(
  History
, IHistory
, nsIDownloadHistory
, mozIAsyncHistory
, nsIObserver
)

} 
} 
