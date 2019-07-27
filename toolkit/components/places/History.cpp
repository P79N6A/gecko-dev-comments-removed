





#include "mozilla/ArrayUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/MemoryReporting.h"

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
#include "nsJSUtils.h"
#include "mozilla/ipc/URIUtils.h"
#include "nsPrintfCString.h"
#include "nsTHashtable.h"
#include "jsapi.h"


#define VISIT_OBSERVERS_INITIAL_CACHE_LENGTH 64


#define VISITS_REMOVAL_INITIAL_HASH_LENGTH 64

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
  , hidden(true)
  , typed(false)
  , transitionType(UINT32_MAX)
  , visitTime(0)
  , frecency(-1)
  , titleChanged(false)
  , shouldUpdateFrecency(true)
  {
    guid.SetIsVoid(true);
    title.SetIsVoid(true);
  }

  explicit VisitData(nsIURI* aURI,
                     nsIURI* aReferrer = nullptr)
  : placeId(0)
  , visitId(0)
  , hidden(true)
  , typed(false)
  , transitionType(UINT32_MAX)
  , visitTime(0)
  , frecency(-1)
  , titleChanged(false)
  , shouldUpdateFrecency(true)
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

  
  bool shouldUpdateFrecency;
};







struct RemoveVisitsFilter {
  RemoveVisitsFilter()
  : transitionType(UINT32_MAX)
  {
  }

  uint32_t transitionType;
};




class PlaceHashKey : public nsCStringHashKey
{
public:
  explicit PlaceHashKey(const nsACString& aSpec)
    : nsCStringHashKey(&aSpec)
    , mVisitCount(0)
    , mBookmarked(false)
#ifdef DEBUG
    , mIsInitialized(false)
#endif
  {
  }

  explicit PlaceHashKey(const nsACString* aSpec)
    : nsCStringHashKey(aSpec)
    , mVisitCount(0)
    , mBookmarked(false)
#ifdef DEBUG
    , mIsInitialized(false)
#endif
  {
  }

  PlaceHashKey(const PlaceHashKey& aOther)
    : nsCStringHashKey(&aOther.GetKey())
  {
    MOZ_ASSERT(false, "Do not call me!");
  }

  void SetProperties(uint32_t aVisitCount, bool aBookmarked)
  {
    mVisitCount = aVisitCount;
    mBookmarked = aBookmarked;
#ifdef DEBUG
    mIsInitialized = true;
#endif
  }

  uint32_t VisitCount() const
  {
#ifdef DEBUG
    MOZ_ASSERT(mIsInitialized, "PlaceHashKey::mVisitCount not set");
#endif
    return mVisitCount;
  }

  bool IsBookmarked() const
  {
#ifdef DEBUG
    MOZ_ASSERT(mIsInitialized, "PlaceHashKey::mBookmarked not set");
#endif
    return mBookmarked;
  }

  
  nsTArray<VisitData> mVisits;
private:
  
  uint32_t mVisitCount;
  
  bool mBookmarked;
#ifdef DEBUG
  
  bool mIsInitialized;
#endif
};




namespace {













nsresult
GetJSArrayFromJSValue(JS::Handle<JS::Value> aValue,
                      JSContext* aCtx,
                      JS::MutableHandle<JSObject*> _array,
                      uint32_t* _arrayLength) {
  if (aValue.isObjectOrNull()) {
    JS::Rooted<JSObject*> val(aCtx, aValue.toObjectOrNull());
    if (JS_IsArrayObject(aCtx, val)) {
      _array.set(val);
      (void)JS_GetArrayLength(aCtx, _array, _arrayLength);
      NS_ENSURE_ARG(*_arrayLength > 0);
      return NS_OK;
    }
  }

  
  
  *_arrayLength = 1;
  _array.set(JS_NewArrayObject(aCtx, 0));
  NS_ENSURE_TRUE(_array, NS_ERROR_OUT_OF_MEMORY);

  bool rc = JS_DefineElement(aCtx, _array, 0, aValue, 0);
  NS_ENSURE_TRUE(rc, NS_ERROR_UNEXPECTED);
  return NS_OK;
}









already_AddRefed<nsIURI>
GetJSValueAsURI(JSContext* aCtx,
                const JS::Value& aValue) {
  if (!aValue.isPrimitive()) {
    nsCOMPtr<nsIXPConnect> xpc = mozilla::services::GetXPConnect();

    nsCOMPtr<nsIXPConnectWrappedNative> wrappedObj;
    nsresult rv = xpc->GetWrappedNativeOfJSObject(aCtx, aValue.toObjectOrNull(),
                                                  getter_AddRefs(wrappedObj));
    NS_ENSURE_SUCCESS(rv, nullptr);
    nsCOMPtr<nsIURI> uri = do_QueryWrappedNative(wrappedObj);
    return uri.forget();
  }
  return nullptr;
}












already_AddRefed<nsIURI>
GetURIFromJSObject(JSContext* aCtx,
                   JS::Handle<JSObject *> aObject,
                   const char* aProperty)
{
  JS::Rooted<JS::Value> uriVal(aCtx);
  bool rc = JS_GetProperty(aCtx, aObject, aProperty, &uriVal);
  NS_ENSURE_TRUE(rc, nullptr);
  return GetJSValueAsURI(aCtx, uriVal);
}










void
GetJSValueAsString(JSContext* aCtx,
                   const JS::Value& aValue,
                   nsString& _string) {
  if (aValue.isUndefined() ||
      !(aValue.isNull() || aValue.isString())) {
    _string.SetIsVoid(true);
    return;
  }

  
  if (aValue.isNull()) {
    _string.Truncate();
    return;
  }

  if (!AssignJSString(aCtx, _string, aValue.toString())) {
    _string.SetIsVoid(true);
  }
}













void
GetStringFromJSObject(JSContext* aCtx,
                      JS::Handle<JSObject *> aObject,
                      const char* aProperty,
                      nsString& _string)
{
  JS::Rooted<JS::Value> val(aCtx);
  bool rc = JS_GetProperty(aCtx, aObject, aProperty, &val);
  if (!rc) {
    _string.SetIsVoid(true);
    return;
  }
  else {
    GetJSValueAsString(aCtx, val, _string);
  }
}













template <typename IntType>
nsresult
GetIntFromJSObject(JSContext* aCtx,
                   JS::Handle<JSObject *> aObject,
                   const char* aProperty,
                   IntType* _int)
{
  JS::Rooted<JS::Value> value(aCtx);
  bool rc = JS_GetProperty(aCtx, aObject, aProperty, &value);
  NS_ENSURE_TRUE(rc, NS_ERROR_UNEXPECTED);
  if (value.isUndefined()) {
    return NS_ERROR_INVALID_ARG;
  }
  NS_ENSURE_ARG(value.isPrimitive());
  NS_ENSURE_ARG(value.isNumber());

  double num;
  rc = JS::ToNumber(aCtx, value, &num);
  NS_ENSURE_TRUE(rc, NS_ERROR_UNEXPECTED);
  NS_ENSURE_ARG(IntType(num) == num);

  *_int = IntType(num);
  return NS_OK;
}















nsresult
GetJSObjectFromArray(JSContext* aCtx,
                     JS::Handle<JSObject*> aArray,
                     uint32_t aIndex,
                     JS::MutableHandle<JSObject*> objOut)
{
  NS_PRECONDITION(JS_IsArrayObject(aCtx, aArray),
                  "Must provide an object that is an array!");

  JS::Rooted<JS::Value> value(aCtx);
  bool rc = JS_GetElement(aCtx, aArray, aIndex, &value);
  NS_ENSURE_TRUE(rc, NS_ERROR_UNEXPECTED);
  NS_ENSURE_ARG(!value.isPrimitive());
  objOut.set(&value.toObject());
  return NS_OK;
}

class VisitedQuery MOZ_FINAL: public AsyncStatementCallback,
                              public mozIStorageCompletionCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED

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
    nsRefPtr<VisitedQuery> callback = new VisitedQuery(aURI, aCallback);
    NS_ENSURE_TRUE(callback, NS_ERROR_OUT_OF_MEMORY);
    nsresult rv = history->GetIsVisitedStatement(callback);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  
  
  NS_IMETHOD Complete(nsresult aResult, nsISupports* aStatement)
  {
    NS_ENSURE_SUCCESS(aResult, aResult);
    nsCOMPtr<mozIStorageAsyncStatement> stmt = do_QueryInterface(aStatement);
    NS_ENSURE_STATE(stmt);
    
    nsresult rv = URIBinder::Bind(stmt, 0, mURI);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<mozIStoragePendingStatement> handle;
    return stmt->ExecuteAsync(this, getter_AddRefs(handle));
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
  explicit VisitedQuery(nsIURI* aURI,
                        mozIVisitedStatusCallback *aCallback=nullptr,
                        bool aIsVisited=false)
  : mURI(aURI)
  , mCallback(aCallback)
  , mIsVisited(aIsVisited)
  {
  }

  ~VisitedQuery()
  {
  }

  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<mozIVisitedStatusCallback> mCallback;
  bool mIsVisited;
};

NS_IMPL_ISUPPORTS_INHERITED(
  VisitedQuery
, AsyncStatementCallback
, mozIStorageCompletionCallback
)




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
    MOZ_ASSERT(NS_IsMainThread(), "This should be called on the main thread");

    
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
                                mReferrer.visitId, mPlace.transitionType,
                                mPlace.guid, mPlace.hidden);
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
    MOZ_ASSERT(NS_IsMainThread(), "This should be called on the main thread");

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





class NotifyPlaceInfoCallback : public nsRunnable
{
public:
  NotifyPlaceInfoCallback(mozIVisitInfoCallback* aCallback,
                          const VisitData& aPlace,
                          bool aIsSingleVisit,
                          nsresult aResult)
  : mCallback(aCallback)
  , mPlace(aPlace)
  , mResult(aResult)
  , mIsSingleVisit(aIsSingleVisit)
  {
    MOZ_ASSERT(aCallback, "Must pass a non-null callback!");
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread(), "This should be called on the main thread");

    nsCOMPtr<nsIURI> referrerURI;
    if (!mPlace.referrerSpec.IsEmpty()) {
      (void)NS_NewURI(getter_AddRefs(referrerURI), mPlace.referrerSpec);
    }

    nsCOMPtr<nsIURI> uri;
    (void)NS_NewURI(getter_AddRefs(uri), mPlace.spec);

    nsCOMPtr<mozIPlaceInfo> place;
    if (mIsSingleVisit) {
      nsCOMPtr<mozIVisitInfo> visit =
        new VisitInfo(mPlace.visitId, mPlace.visitTime, mPlace.transitionType,
                      referrerURI.forget());
      PlaceInfo::VisitsArray visits;
      (void)visits.AppendElement(visit);

      
      
      place =
        new PlaceInfo(mPlace.placeId, mPlace.guid, uri.forget(), mPlace.title,
                      -1, visits);
    }
    else {
      
      place =
        new PlaceInfo(mPlace.placeId, mPlace.guid, uri.forget(), mPlace.title,
                      -1);
    }

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
  bool mIsSingleVisit;
};




class NotifyCompletion : public nsRunnable
{
public:
  explicit NotifyCompletion(mozIVisitInfoCallback* aCallback)
  : mCallback(aCallback)
  {
    MOZ_ASSERT(aCallback, "Must pass a non-null callback!");
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
          mozIVisitInfoCallback* aCallback = nullptr)
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
      new NotifyPlaceInfoCallback(aCallback, place, true, NS_ERROR_INVALID_ARG);
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
                        mozIVisitInfoCallback* aCallback = nullptr)
  {
    MOZ_ASSERT(NS_IsMainThread(), "This should be called on the main thread");
    MOZ_ASSERT(aPlaces.Length() > 0, "Must pass a non-empty array!");

    
    nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
    MOZ_ASSERT(navHistory, "Could not get nsNavHistory?!");
    if (!navHistory) {
      return NS_ERROR_FAILURE;
    }

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
    MOZ_ASSERT(!NS_IsMainThread(), "This should not be called on the main thread");

    
    MutexAutoLock lockedScope(mHistory->GetShutdownMutex());
    if (mHistory->IsShuttingDown()) {
      
      return NS_OK;
    }

    mozStorageTransaction transaction(mDBConn, false,
                                      mozIStorageConnection::TRANSACTION_IMMEDIATE);

    VisitData* lastPlace = nullptr;
    for (nsTArray<VisitData>::size_type i = 0; i < mPlaces.Length(); i++) {
      VisitData& place = mPlaces.ElementAt(i);
      VisitData& referrer = mReferrers.ElementAt(i);

      
      
      bool known = lastPlace && lastPlace->IsSamePlaceAs(place);
      if (!known) {
        nsresult rv = mHistory->FetchPageInfo(place, &known);
        if (NS_FAILED(rv)) {
          if (mCallback) {
            nsCOMPtr<nsIRunnable> event =
              new NotifyPlaceInfoCallback(mCallback, place, true, rv);
            return NS_DispatchToMainThread(event);
          }
          return NS_OK;
        }
      }

      FetchReferrerInfo(referrer, place);

      nsresult rv = DoDatabaseInserts(known, place, referrer);
      if (mCallback) {
        nsCOMPtr<nsIRunnable> event =
          new NotifyPlaceInfoCallback(mCallback, place, true, rv);
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
    MOZ_ASSERT(NS_IsMainThread(), "This should be called on the main thread");

    (void)mPlaces.SwapElements(aPlaces);
    (void)mReferrers.SetLength(mPlaces.Length());

    for (nsTArray<VisitData>::size_type i = 0; i < mPlaces.Length(); i++) {
      mReferrers[i].spec = mPlaces[i].referrerSpec;

#ifdef DEBUG
      nsCOMPtr<nsIURI> uri;
      (void)NS_NewURI(getter_AddRefs(uri), mPlaces[i].spec);
      NS_ASSERTION(CanAddURI(uri),
                   "Passed a VisitData with a URI we cannot add to history!");
#endif
    }
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
    MOZ_ASSERT(!NS_IsMainThread(), "This should not be called on the main thread");

    
    nsresult rv;
    if (aKnown) {
      rv = mHistory->UpdatePlace(aPlace);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    
    else {
      rv = mHistory->InsertPlace(aPlace);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      if (mCallback || aPlace.guid.IsEmpty()) {
        bool exists;
        rv = mHistory->FetchPageInfo(aPlace, &exists);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!exists) {
          NS_NOTREACHED("should have an entry in moz_places");
        }
      }
    }

    rv = AddVisit(aPlace, aReferrer);
    NS_ENSURE_SUCCESS(rv, rv);

    
    

    
    if (aPlace.shouldUpdateFrecency) {
      rv = UpdateFrecency(aPlace);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  










  bool FetchVisitInfo(VisitData& _place,
                      PRTime aThresholdStart = 0)
  {
    NS_PRECONDITION(!_place.spec.IsEmpty(), "must have a non-empty spec!");

    nsCOMPtr<mozIStorageStatement> stmt;
    
    if (_place.visitTime) {
      stmt = mHistory->GetStatement(
        "SELECT id, visit_date "
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
        "SELECT id, visit_date "
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
    rv = stmt->GetInt64(1, reinterpret_cast<int64_t*>(&_place.visitTime));
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

    if (!FetchVisitInfo(aReferrer, aPlace.visitTime)) {
      
      
      
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
        "VALUES (:from_visit, :page_id, :visit_date, :visit_type, 0) "
      );
      NS_ENSURE_STATE(stmt);
      rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), _place.placeId);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      stmt = mHistory->GetStatement(
        "INSERT INTO moz_historyvisits "
          "(from_visit, place_id, visit_date, visit_type, session) "
        "VALUES (:from_visit, (SELECT id FROM moz_places WHERE url = :page_url), :visit_date, :visit_type, 0) "
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

    mozStorageStatementScoper scoper(stmt);
    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    (void)FetchVisitInfo(_place);

    return NS_OK;
  }

  





  nsresult UpdateFrecency(const VisitData& aPlace)
  {
    MOZ_ASSERT(aPlace.shouldUpdateFrecency);

    nsresult rv;
    { 
      nsCOMPtr<mozIStorageStatement> stmt;
      if (aPlace.placeId) {
        stmt = mHistory->GetStatement(
          "UPDATE moz_places "
          "SET frecency = NOTIFY_FRECENCY("
            "CALCULATE_FRECENCY(:page_id), "
            "url, guid, hidden, last_visit_date"
          ") "
          "WHERE id = :page_id"
        );
        NS_ENSURE_STATE(stmt);
        rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), aPlace.placeId);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        stmt = mHistory->GetStatement(
          "UPDATE moz_places "
          "SET frecency = NOTIFY_FRECENCY("
            "CALCULATE_FRECENCY(id), url, guid, hidden, last_visit_date"
          ") "
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

  nsCOMPtr<mozIVisitInfoCallback> mCallback;

  



  nsRefPtr<History> mHistory;
};

class GetPlaceInfo MOZ_FINAL : public nsRunnable {
public:
  


  static nsresult Start(mozIStorageConnection* aConnection,
                        VisitData& aPlace,
                        mozIVisitInfoCallback* aCallback) {
    MOZ_ASSERT(NS_IsMainThread(), "This should be called on the main thread");

    nsRefPtr<GetPlaceInfo> event = new GetPlaceInfo(aPlace, aCallback);

    
    nsCOMPtr<nsIEventTarget> target = do_GetInterface(aConnection);
    NS_ENSURE_TRUE(target, NS_ERROR_UNEXPECTED);
    nsresult rv = target->Dispatch(event, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(!NS_IsMainThread(), "This should not be called on the main thread");

    bool exists;
    nsresult rv = mHistory->FetchPageInfo(mPlace, &exists);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!exists)
      rv = NS_ERROR_NOT_AVAILABLE;

    nsCOMPtr<nsIRunnable> event =
      new NotifyPlaceInfoCallback(mCallback, mPlace, false, rv);

    rv = NS_DispatchToMainThread(event);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }
private:
  GetPlaceInfo(VisitData& aPlace,
               mozIVisitInfoCallback* aCallback)
  : mPlace(aPlace)
  , mCallback(aCallback)
  , mHistory(History::GetService())
  {
    MOZ_ASSERT(NS_IsMainThread(), "This should be called on the main thread");
  }

  virtual ~GetPlaceInfo()
  {
    if (mCallback) {
      nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
      (void)NS_ProxyRelease(mainThread, mCallback, true);
    }
  }

  VisitData mPlace;
  nsCOMPtr<mozIVisitInfoCallback> mCallback;
  nsRefPtr<History> mHistory;
};




class SetPageTitle : public nsRunnable
{
public:
  









  static nsresult Start(mozIStorageConnection* aConnection,
                        nsIURI* aURI,
                        const nsAString& aTitle)
  {
    MOZ_ASSERT(NS_IsMainThread(), "This should be called on the main thread");
    MOZ_ASSERT(aURI, "Must pass a non-null URI object!");

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
    MOZ_ASSERT(!NS_IsMainThread(), "This should not be called on the main thread");

    
    bool exists;
    nsresult rv = mHistory->FetchPageInfo(mPlace, &exists);
    NS_ENSURE_SUCCESS(rv, rv);

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
      rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), mPlace.placeId);
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
    rv = NS_DispatchToMainThread(event);
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

  explicit SetDownloadAnnotations(nsIURI* aDestination)
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
  ~SetDownloadAnnotations() {}

  nsCOMPtr<nsIURI> mDestination;

  



  nsRefPtr<History> mHistory;
};
NS_IMPL_ISUPPORTS(
  SetDownloadAnnotations,
  mozIVisitInfoCallback
)




static PLDHashOperator TransferHashEntries(PlaceHashKey* aEntry,
                                           void* aHash)
{
  nsTHashtable<PlaceHashKey>* hash =
    static_cast<nsTHashtable<PlaceHashKey> *>(aHash);
  PlaceHashKey* copy = hash->PutEntry(aEntry->GetKey());
  copy->SetProperties(aEntry->VisitCount(), aEntry->IsBookmarked());
  aEntry->mVisits.SwapElements(copy->mVisits);
  return PL_DHASH_NEXT;
}




static PLDHashOperator NotifyVisitRemoval(PlaceHashKey* aEntry,
                                          void* aHistory)
{
  nsNavHistory* history = static_cast<nsNavHistory *>(aHistory);
  const nsTArray<VisitData>& visits = aEntry->mVisits;
  nsCOMPtr<nsIURI> uri;
  (void)NS_NewURI(getter_AddRefs(uri), visits[0].spec);
  bool removingPage =
    visits.Length() == aEntry->VisitCount() &&
    !aEntry->IsBookmarked();
  
  
  
  
  
  
  
  uint32_t transition = visits[0].transitionType < UINT32_MAX ?
                          visits[0].transitionType : 0;
  history->NotifyOnPageExpired(uri, visits[0].visitTime, removingPage,
                               visits[0].guid,
                               nsINavHistoryObserver::REASON_DELETED,
                               transition);
  return PL_DHASH_NEXT;
}




class NotifyRemoveVisits : public nsRunnable
{
public:

  explicit NotifyRemoveVisits(nsTHashtable<PlaceHashKey>& aPlaces)
    : mPlaces(VISITS_REMOVAL_INITIAL_HASH_LENGTH)
    , mHistory(History::GetService())
  {
    MOZ_ASSERT(!NS_IsMainThread(),
               "This should not be called on the main thread");
    aPlaces.EnumerateEntries(TransferHashEntries, &mPlaces);
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread(), "This should be called on the main thread");

    
    if (mHistory->IsShuttingDown()) {
      
      return NS_OK;
    }

    nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
    if (!navHistory) {
      NS_WARNING("Cannot notify without the history service!");
      return NS_OK;
    }

    
    
    
    (void)navHistory->BeginUpdateBatch();
    mPlaces.EnumerateEntries(NotifyVisitRemoval, navHistory);
    (void)navHistory->EndUpdateBatch();

    return NS_OK;
  }

private:
  nsTHashtable<PlaceHashKey> mPlaces;

  



  nsRefPtr<History> mHistory;
};




static PLDHashOperator ListToBeRemovedPlaceIds(PlaceHashKey* aEntry,
                                               void* aIdsList)
{
  const nsTArray<VisitData>& visits = aEntry->mVisits;
  
  if (visits.Length() == aEntry->VisitCount() &&
      !aEntry->IsBookmarked()) {
    nsCString* list = static_cast<nsCString*>(aIdsList);
    if (!list->IsEmpty())
      list->Append(',');
    list->AppendInt(visits[0].placeId);
  }
  return PL_DHASH_NEXT;
}




class RemoveVisits : public nsRunnable
{
public:
  







  static nsresult Start(mozIStorageConnection* aConnection,
                        RemoveVisitsFilter& aFilter)
  {
    MOZ_ASSERT(NS_IsMainThread(), "This should be called on the main thread");

    nsRefPtr<RemoveVisits> event = new RemoveVisits(aConnection, aFilter);

    
    nsCOMPtr<nsIEventTarget> target = do_GetInterface(aConnection);
    NS_ENSURE_TRUE(target, NS_ERROR_UNEXPECTED);
    nsresult rv = target->Dispatch(event, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(!NS_IsMainThread(),
               "This should not be called on the main thread");

    
    MutexAutoLock lockedScope(mHistory->GetShutdownMutex());
    if (mHistory->IsShuttingDown()) {
      
      return NS_OK;
    }

    
    
    nsTHashtable<PlaceHashKey> places(VISITS_REMOVAL_INITIAL_HASH_LENGTH);
    nsresult rv = FindRemovableVisits(places);
    NS_ENSURE_SUCCESS(rv, rv);

    if (places.Count() == 0)
      return NS_OK;

    mozStorageTransaction transaction(mDBConn, false,
                                      mozIStorageConnection::TRANSACTION_IMMEDIATE);

    rv = RemoveVisitsFromDatabase();
    NS_ENSURE_SUCCESS(rv, rv);
    rv = RemovePagesFromDatabase(places);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = transaction.Commit();
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIRunnable> event = new NotifyRemoveVisits(places);
    rv = NS_DispatchToMainThread(event);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

private:
  RemoveVisits(mozIStorageConnection* aConnection,
               RemoveVisitsFilter& aFilter)
  : mDBConn(aConnection)
  , mHasTransitionType(false)
  , mHistory(History::GetService())
  {
    MOZ_ASSERT(NS_IsMainThread(), "This should be called on the main thread");

    
    nsTArray<nsCString> conditions;
    
    if (aFilter.transitionType < UINT32_MAX) {
      conditions.AppendElement(nsPrintfCString("visit_type = %d", aFilter.transitionType));
      mHasTransitionType = true;
    }
    if (conditions.Length() > 0) {
      mWhereClause.AppendLiteral (" WHERE ");
      for (uint32_t i = 0; i < conditions.Length(); ++i) {
        if (i > 0)
          mWhereClause.AppendLiteral(" AND ");
        mWhereClause.Append(conditions[i]);
      }
    }
  }

  nsresult
  FindRemovableVisits(nsTHashtable<PlaceHashKey>& aPlaces)
  {
    MOZ_ASSERT(!NS_IsMainThread(),
               "This should not be called on the main thread");

    nsCString query("SELECT h.id, url, guid, visit_date, visit_type, "
                    "(SELECT count(*) FROM moz_historyvisits WHERE place_id = h.id) as full_visit_count, "
                    "EXISTS(SELECT 1 FROM moz_bookmarks WHERE fk = h.id) as bookmarked "
                    "FROM moz_historyvisits "
                    "JOIN moz_places h ON place_id = h.id");
    query.Append(mWhereClause);

    nsCOMPtr<mozIStorageStatement> stmt = mHistory->GetStatement(query);
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scoper(stmt);

    bool hasResult;
    nsresult rv;
    while (NS_SUCCEEDED((rv = stmt->ExecuteStep(&hasResult))) && hasResult) {
      VisitData visit;
      rv = stmt->GetInt64(0, &visit.placeId);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->GetUTF8String(1, visit.spec);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->GetUTF8String(2, visit.guid);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->GetInt64(3, &visit.visitTime);
      NS_ENSURE_SUCCESS(rv, rv);
      if (mHasTransitionType) {
        int32_t transition;
        rv = stmt->GetInt32(4, &transition);
        NS_ENSURE_SUCCESS(rv, rv);
        visit.transitionType = static_cast<uint32_t>(transition);
      }
      int32_t visitCount, bookmarked;
      rv = stmt->GetInt32(5, &visitCount);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->GetInt32(6, &bookmarked);
      NS_ENSURE_SUCCESS(rv, rv);

      PlaceHashKey* entry = aPlaces.GetEntry(visit.spec);
      if (!entry) {
        entry = aPlaces.PutEntry(visit.spec);
      }
      entry->SetProperties(static_cast<uint32_t>(visitCount), static_cast<bool>(bookmarked));
      entry->mVisits.AppendElement(visit);
    }
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  nsresult
  RemoveVisitsFromDatabase()
  {
    MOZ_ASSERT(!NS_IsMainThread(),
               "This should not be called on the main thread");

    nsCString query("DELETE FROM moz_historyvisits");
    query.Append(mWhereClause);

    nsCOMPtr<mozIStorageStatement> stmt = mHistory->GetStatement(query);
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scoper(stmt);
    nsresult rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  nsresult
  RemovePagesFromDatabase(nsTHashtable<PlaceHashKey>& aPlaces)
  {
    MOZ_ASSERT(!NS_IsMainThread(),
               "This should not be called on the main thread");

    nsCString placeIdsToRemove;
    aPlaces.EnumerateEntries(ListToBeRemovedPlaceIds, &placeIdsToRemove);

#ifdef DEBUG
    {
      
      nsCString query("SELECT id FROM moz_places h WHERE id IN (");
      query.Append(placeIdsToRemove);
      query.AppendLiteral(") AND ("
          "EXISTS(SELECT 1 FROM moz_bookmarks WHERE fk = h.id) OR "
          "EXISTS(SELECT 1 FROM moz_historyvisits WHERE place_id = h.id) OR "
          "SUBSTR(h.url, 1, 6) = 'place:' "
        ")");
      nsCOMPtr<mozIStorageStatement> stmt = mHistory->GetStatement(query);
      NS_ENSURE_STATE(stmt);
      mozStorageStatementScoper scoper(stmt);
      bool hasResult;
      MOZ_ASSERT(NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && !hasResult,
                 "Trying to remove a non-oprhan place from the database");
    }
#endif

    nsCString query("DELETE FROM moz_places "
                    "WHERE id IN (");
    query.Append(placeIdsToRemove);
    query.Append(')');

    nsCOMPtr<mozIStorageStatement> stmt = mHistory->GetStatement(query);
    NS_ENSURE_STATE(stmt);
    mozStorageStatementScoper scoper(stmt);
    nsresult rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  mozIStorageConnection* mDBConn;
  bool mHasTransitionType;
  nsCString mWhereClause;

  



  nsRefPtr<History> mHistory;
};









void
StoreAndNotifyEmbedVisit(VisitData& aPlace,
                         mozIVisitInfoCallback* aCallback = nullptr)
{
  MOZ_ASSERT(aPlace.transitionType == nsINavHistoryService::TRANSITION_EMBED,
             "Must only pass TRANSITION_EMBED visits to this!");
  MOZ_ASSERT(NS_IsMainThread(), "Must be called on the main thread!");

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
      new NotifyPlaceInfoCallback(aCallback, aPlace, true, NS_OK);
    (void)NS_DispatchToMainThread(event);

    
    
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    (void)NS_ProxyRelease(mainThread, aCallback, true);
  }

  VisitData noReferrer;
  nsCOMPtr<nsIRunnable> event = new NotifyVisitObservers(aPlace, noReferrer);
  (void)NS_DispatchToMainThread(event);
}

} 




History* History::gService = nullptr;

History::History()
  : mShuttingDown(false)
  , mShutdownMutex("History::mShutdownMutex")
  , mObservers(VISIT_OBSERVERS_INITIAL_CACHE_LENGTH)
  , mRecentlyVisitedURIsNextIndex(0)
{
  NS_ASSERTION(!gService, "Ruh-roh!  This service has already been created!");
  gService = this;

  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  NS_WARN_IF_FALSE(os, "Observer service was not found!");
  if (os) {
    (void)os->AddObserver(this, TOPIC_PLACES_SHUTDOWN, false);
  }
}

History::~History()
{
  UnregisterWeakMemoryReporter(this);

  gService = nullptr;

  NS_ASSERTION(mObservers.Count() == 0,
               "Not all Links were removed before we disappear!");
}

void
History::InitMemoryReporter()
{
  RegisterWeakMemoryReporter(this);
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

class ConcurrentStatementsHolder MOZ_FINAL : public mozIStorageCompletionCallback {
public:
  NS_DECL_ISUPPORTS

  explicit ConcurrentStatementsHolder(mozIStorageConnection* aDBConn)
  {
    DebugOnly<nsresult> rv = aDBConn->AsyncClone(true, this);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
  }

  NS_IMETHOD Complete(nsresult aStatus, nsISupports* aConnection) {
    if (NS_FAILED(aStatus))
      return NS_OK;
    mReadOnlyDBConn = do_QueryInterface(aConnection);

    

    if (!mIsVisitedStatement) {
      (void)mReadOnlyDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
        "SELECT 1 FROM moz_places h "
        "WHERE url = ?1 AND last_visit_date NOTNULL "
      ),  getter_AddRefs(mIsVisitedStatement));
      MOZ_ASSERT(mIsVisitedStatement);
      nsresult result = mIsVisitedStatement ? NS_OK : NS_ERROR_NOT_AVAILABLE;
      for (int32_t i = 0; i < mIsVisitedCallbacks.Count(); ++i) {
        DebugOnly<nsresult> rv;
        rv = mIsVisitedCallbacks[i]->Complete(result, mIsVisitedStatement);
        MOZ_ASSERT(NS_SUCCEEDED(rv));
      }
      mIsVisitedCallbacks.Clear();
    }

    return NS_OK;
  }

  void GetIsVisitedStatement(mozIStorageCompletionCallback* aCallback)
  {
    if (mIsVisitedStatement) {
      DebugOnly<nsresult> rv;
      rv = aCallback->Complete(NS_OK, mIsVisitedStatement);
      MOZ_ASSERT(NS_SUCCEEDED(rv));
    } else {
      DebugOnly<bool> added = mIsVisitedCallbacks.AppendObject(aCallback);
      MOZ_ASSERT(added);
    }
  }

  void Shutdown() {
    if (mReadOnlyDBConn) {
      mIsVisitedCallbacks.Clear();
      DebugOnly<nsresult> rv;
      if (mIsVisitedStatement) {
        rv = mIsVisitedStatement->Finalize();
        MOZ_ASSERT(NS_SUCCEEDED(rv));
      }
      rv = mReadOnlyDBConn->AsyncClose(nullptr);
      MOZ_ASSERT(NS_SUCCEEDED(rv));
    }
  }

private:
  ~ConcurrentStatementsHolder()
  {
  }

  nsCOMPtr<mozIStorageAsyncConnection> mReadOnlyDBConn;
  nsCOMPtr<mozIStorageAsyncStatement> mIsVisitedStatement;
  nsCOMArray<mozIStorageCompletionCallback> mIsVisitedCallbacks;
};

NS_IMPL_ISUPPORTS(
  ConcurrentStatementsHolder
, mozIStorageCompletionCallback
)

nsresult
History::GetIsVisitedStatement(mozIStorageCompletionCallback* aCallback)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mShuttingDown)
    return NS_ERROR_NOT_AVAILABLE;

  if (!mConcurrentStatementsHolder) {
    mozIStorageConnection* dbConn = GetDBConn();
    NS_ENSURE_STATE(dbConn);
    mConcurrentStatementsHolder = new ConcurrentStatementsHolder(dbConn);
  }
  mConcurrentStatementsHolder->GetIsVisitedStatement(aCallback);
  return NS_OK;
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
  nsString title = aPlace.title;
  
  if (title.IsEmpty()) {
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("title"));
  }
  else {
    title.Assign(StringHead(aPlace.title, TITLE_LENGTH_MAX));
    rv = stmt->BindStringByName(NS_LITERAL_CSTRING("title"), title);
  }
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("typed"), aPlace.typed);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  int32_t frecency = aPlace.shouldUpdateFrecency ? aPlace.frecency : 0;
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("frecency"), frecency);
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

  
  const nsNavHistory* navHistory = nsNavHistory::GetConstHistoryService();
  NS_ENSURE_STATE(navHistory);
  navHistory->DispatchFrecencyChangedNotification(aPlace.spec, frecency, guid,
                                                  aPlace.hidden,
                                                  aPlace.visitTime);

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

nsresult
History::FetchPageInfo(VisitData& _place, bool* _exists)
{
  NS_PRECONDITION(!_place.spec.IsEmpty() || !_place.guid.IsEmpty(), "must have either a non-empty spec or guid!");
  NS_PRECONDITION(!NS_IsMainThread(), "must be called off of the main thread!");

  nsresult rv;

  
  nsCOMPtr<mozIStorageStatement> stmt;
  bool selectByURI = !_place.spec.IsEmpty();
  if (selectByURI) {
    stmt = GetStatement(
      "SELECT guid, id, title, hidden, typed, frecency "
      "FROM moz_places "
      "WHERE url = :page_url "
    );
    NS_ENSURE_STATE(stmt);

    rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), _place.spec);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    stmt = GetStatement(
      "SELECT url, id, title, hidden, typed, frecency "
      "FROM moz_places "
      "WHERE guid = :guid "
    );
    NS_ENSURE_STATE(stmt);

    rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("guid"), _place.guid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mozStorageStatementScoper scoper(stmt);

  rv = stmt->ExecuteStep(_exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!*_exists) {
    return NS_OK;
  }

  if (selectByURI) {
    if (_place.guid.IsEmpty()) {
      rv = stmt->GetUTF8String(0, _place.guid);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    nsAutoCString spec;
    rv = stmt->GetUTF8String(0, spec);
    NS_ENSURE_SUCCESS(rv, rv);
    _place.spec = spec;
  }

  rv = stmt->GetInt64(1, &_place.placeId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString title;
  rv = stmt->GetString(2, title);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  if (_place.title.IsVoid()) {
    _place.title = title;
  }
  
  else {
    _place.titleChanged = !(_place.title.Equals(title) ||
                            (_place.title.IsEmpty() && title.IsVoid()));
  }

  if (_place.hidden) {
    
    
    
    int32_t hidden;
    rv = stmt->GetInt32(3, &hidden);
    NS_ENSURE_SUCCESS(rv, rv);
    _place.hidden = !!hidden;
  }

  if (!_place.typed) {
    
    
    int32_t typed;
    rv = stmt->GetInt32(4, &typed);
    NS_ENSURE_SUCCESS(rv, rv);
    _place.typed = !!typed;
  }

  rv = stmt->GetInt32(5, &_place.frecency);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

MOZ_DEFINE_MALLOC_SIZE_OF(HistoryMallocSizeOf)

NS_IMETHODIMP
History::CollectReports(nsIHandleReportCallback* aHandleReport,
                        nsISupports* aData, bool aAnonymize)
{
  return MOZ_COLLECT_REPORT(
    "explicit/history-links-hashtable", KIND_HEAP, UNITS_BYTES,
    SizeOfIncludingThis(HistoryMallocSizeOf),
    "Memory used by the hashtable that records changes to the visited state "
    "of links.");
}

size_t
History::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOfThis)
{
  return aMallocSizeOfThis(this) +
         mObservers.SizeOfExcludingThis(aMallocSizeOfThis);
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
    gService->InitMemoryReporter();
  }

  NS_ADDREF(gService);
  return gService;
}

mozIStorageConnection*
History::GetDBConn()
{
  if (mShuttingDown)
    return nullptr;
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

  if (mConcurrentStatementsHolder) {
    mConcurrentStatementsHolder->Shutdown();
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
  else if ((recentFlags & nsNavHistory::RECENT_TYPED) &&
           !(aFlags & IHistory::UNRECOVERABLE_ERROR)) {
    
    
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
    place.shouldUpdateFrecency = false;
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
History::RemoveAllDownloads()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mShuttingDown) {
    return NS_OK;
  }

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    NS_ERROR("Cannot remove downloads to history from content process!");
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(navHistory, NS_ERROR_OUT_OF_MEMORY);
  mozIStorageConnection* dbConn = GetDBConn();
  NS_ENSURE_STATE(dbConn);

  RemoveVisitsFilter filter;
  filter.transitionType = nsINavHistoryService::TRANSITION_DOWNLOAD;

  nsresult rv = RemoveVisits::Start(dbConn, filter);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




NS_IMETHODIMP
History::GetPlacesInfo(JS::Handle<JS::Value> aPlaceIdentifiers,
                       mozIVisitInfoCallback* aCallback,
                       JSContext* aCtx)
{
  
  nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
  MOZ_ASSERT(navHistory, "Could not get nsNavHistory?!");
  if (!navHistory) {
    return NS_ERROR_FAILURE;
  }

  uint32_t placesIndentifiersLength;
  JS::Rooted<JSObject*> placesIndentifiers(aCtx);
  nsresult rv = GetJSArrayFromJSValue(aPlaceIdentifiers, aCtx,
                                      &placesIndentifiers,
                                      &placesIndentifiersLength);
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray<VisitData> placesInfo;
  placesInfo.SetCapacity(placesIndentifiersLength);
  for (uint32_t i = 0; i < placesIndentifiersLength; i++) {
    JS::Rooted<JS::Value> placeIdentifier(aCtx);
    bool rc = JS_GetElement(aCtx, placesIndentifiers, i, &placeIdentifier);
    NS_ENSURE_TRUE(rc, NS_ERROR_UNEXPECTED);

    
    nsAutoString fatGUID;
    GetJSValueAsString(aCtx, placeIdentifier, fatGUID);
    if (!fatGUID.IsVoid()) {
      NS_ConvertUTF16toUTF8 guid(fatGUID);
      if (!IsValidGUID(guid))
        return NS_ERROR_INVALID_ARG;

      VisitData& placeInfo = *placesInfo.AppendElement(VisitData());
      placeInfo.guid = guid;
    }
    else {
      nsCOMPtr<nsIURI> uri = GetJSValueAsURI(aCtx, placeIdentifier);
      if (!uri)
        return NS_ERROR_INVALID_ARG; 
      placesInfo.AppendElement(VisitData(uri));
    }
  }

  mozIStorageConnection* dbConn = GetDBConn();
  NS_ENSURE_STATE(dbConn);

  for (nsTArray<VisitData>::size_type i = 0; i < placesInfo.Length(); i++) {
    nsresult rv = GetPlaceInfo::Start(dbConn, placesInfo.ElementAt(i), aCallback);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  if (aCallback) {
    
    
    
    
    NS_ADDREF(aCallback);

    nsCOMPtr<nsIEventTarget> backgroundThread = do_GetInterface(dbConn);
    NS_ENSURE_TRUE(backgroundThread, NS_ERROR_UNEXPECTED);
    nsCOMPtr<nsIRunnable> event = new NotifyCompletion(aCallback);
    return backgroundThread->Dispatch(event, NS_DISPATCH_NORMAL);
  }

  return NS_OK;
}

NS_IMETHODIMP
History::UpdatePlaces(JS::Handle<JS::Value> aPlaceInfos,
                      mozIVisitInfoCallback* aCallback,
                      JSContext* aCtx)
{
  NS_ENSURE_TRUE(NS_IsMainThread(), NS_ERROR_UNEXPECTED);
  NS_ENSURE_TRUE(!aPlaceInfos.isPrimitive(), NS_ERROR_INVALID_ARG);

  uint32_t infosLength;
  JS::Rooted<JSObject*> infos(aCtx);
  nsresult rv = GetJSArrayFromJSValue(aPlaceInfos, aCtx, &infos, &infosLength);
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray<VisitData> visitData;
  for (uint32_t i = 0; i < infosLength; i++) {
    JS::Rooted<JSObject*> info(aCtx);
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

    JS::Rooted<JSObject*> visits(aCtx, nullptr);
    {
      JS::Rooted<JS::Value> visitsVal(aCtx);
      bool rc = JS_GetProperty(aCtx, info, "visits", &visitsVal);
      NS_ENSURE_TRUE(rc, NS_ERROR_UNEXPECTED);
      if (!visitsVal.isPrimitive()) {
        visits = visitsVal.toObjectOrNull();
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
      JS::Rooted<JSObject*> visit(aCtx);
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
    return backgroundThread->Dispatch(event, NS_DISPATCH_NORMAL);
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
                 const char16_t* aData)
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




NS_IMPL_ISUPPORTS(
  History
, IHistory
, nsIDownloadHistory
, mozIAsyncHistory
, nsIObserver
, nsIMemoryReporter
)

} 
} 
