



#include "PlaceInfo.h"
#include "VisitInfo.h"
#include "nsIURI.h"
#include "nsServiceManagerUtils.h"
#include "nsIXPConnect.h"
#include "mozilla/Services.h"

namespace mozilla {
namespace places {




PlaceInfo::PlaceInfo(int64_t aId,
                     const nsCString& aGUID,
                     already_AddRefed<nsIURI> aURI,
                     const nsString& aTitle,
                     int64_t aFrecency,
                     const VisitsArray& aVisits)
: mId(aId)
, mGUID(aGUID)
, mURI(aURI)
, mTitle(aTitle)
, mFrecency(aFrecency)
, mVisits(aVisits)
{
  NS_PRECONDITION(mURI, "Must provide a non-null uri!");
}




NS_IMETHODIMP
PlaceInfo::GetPlaceId(int64_t* _placeId)
{
  *_placeId = mId;
  return NS_OK;
}

NS_IMETHODIMP
PlaceInfo::GetGuid(nsACString& _guid)
{
  _guid = mGUID;
  return NS_OK;
}

NS_IMETHODIMP
PlaceInfo::GetUri(nsIURI** _uri)
{
  NS_ADDREF(*_uri = mURI);
  return NS_OK;
}

NS_IMETHODIMP
PlaceInfo::GetTitle(nsAString& _title)
{
  _title = mTitle;
  return NS_OK;
}

NS_IMETHODIMP
PlaceInfo::GetFrecency(int64_t* _frecency)
{
  *_frecency = mFrecency;
  return NS_OK;
}

NS_IMETHODIMP
PlaceInfo::GetVisits(JSContext* aContext,
                     JS::Value* _visits)
{
  
  
  JSObject* visits = JS_NewArrayObject(aContext, 0, NULL);
  NS_ENSURE_TRUE(visits, NS_ERROR_OUT_OF_MEMORY);

  JSObject* global = JS_GetGlobalForScopeChain(aContext);
  NS_ENSURE_TRUE(global, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIXPConnect> xpc = mozilla::services::GetXPConnect();

  for (VisitsArray::size_type idx = 0; idx < mVisits.Length(); idx++) {
    nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
    nsresult rv = xpc->WrapNative(aContext, global, mVisits[idx],
                                  NS_GET_IID(mozIVisitInfo),
                                  getter_AddRefs(wrapper));
    NS_ENSURE_SUCCESS(rv, rv);

    JSObject* jsobj;
    rv = wrapper->GetJSObject(&jsobj);
    NS_ENSURE_SUCCESS(rv, rv);
    JS::Value wrappedVisit = OBJECT_TO_JSVAL(jsobj);

    JSBool rc = JS_SetElement(aContext, visits, idx, &wrappedVisit);
    NS_ENSURE_TRUE(rc, NS_ERROR_UNEXPECTED);
  }

  *_visits = OBJECT_TO_JSVAL(visits);

  return NS_OK;
}




NS_IMPL_ISUPPORTS1(
  PlaceInfo
, mozIPlaceInfo
)

} 
} 
