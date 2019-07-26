





#include "base/basictypes.h"
#include "BluetoothReplyRunnable.h"
#include "DOMRequest.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/dom/Promise.h"
#include "nsServiceManagerUtils.h"

using namespace mozilla::dom;

USING_BLUETOOTH_NAMESPACE

BluetoothReplyRunnable::BluetoothReplyRunnable(nsIDOMDOMRequest* aReq,
                                               Promise* aPromise)
  : mDOMRequest(aReq)
  , mPromise(aPromise)
{}

void
BluetoothReplyRunnable::SetReply(BluetoothReply* aReply)
{
  mReply = aReply;
}

void
BluetoothReplyRunnable::ReleaseMembers()
{
  mDOMRequest = nullptr;
  mPromise = nullptr;
}

BluetoothReplyRunnable::~BluetoothReplyRunnable()
{}

nsresult
BluetoothReplyRunnable::FireReplySuccess(JS::Handle<JS::Value> aVal)
{
  MOZ_ASSERT(mReply->type() == BluetoothReply::TBluetoothReplySuccess);

  
  if (mDOMRequest) {
    nsCOMPtr<nsIDOMRequestService> rs =
      do_GetService(DOMREQUEST_SERVICE_CONTRACTID);
    NS_ENSURE_TRUE(rs, NS_ERROR_FAILURE);

    return rs->FireSuccessAsync(mDOMRequest, aVal);
  }

  
  if (mPromise) {
    mPromise->MaybeResolve(aVal);
  }

  return NS_OK;
}

nsresult
BluetoothReplyRunnable::FireErrorString()
{
  
  if (mDOMRequest) {
    nsCOMPtr<nsIDOMRequestService> rs =
      do_GetService(DOMREQUEST_SERVICE_CONTRACTID);
    NS_ENSURE_TRUE(rs, NS_ERROR_FAILURE);

    return rs->FireErrorAsync(mDOMRequest, mErrorString);
  }

  
  if (mPromise) {
    





    mPromise->MaybeReject(NS_ERROR_DOM_OPERATION_ERR);
  }

  return NS_OK;
}

NS_IMETHODIMP
BluetoothReplyRunnable::Run()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mReply);

  AutoSafeJSContext cx;
  JS::Rooted<JS::Value> v(cx, JSVAL_VOID);

  nsresult rv;
  if (mReply->type() != BluetoothReply::TBluetoothReplySuccess) {
    SetError(mReply->get_BluetoothReplyError().error());
    rv = FireErrorString();
  } else if (!ParseSuccessfulReply(&v)) {
    rv = FireErrorString();
  } else {
    rv = FireReplySuccess(v);
  }

  if (NS_FAILED(rv)) {
    BT_WARNING("Could not fire DOMRequest/Promise!");
  }

  ReleaseMembers();
  MOZ_ASSERT(!mDOMRequest,
             "mDOMRequest is still alive! Deriving class should call "
             "BluetoothReplyRunnable::ReleaseMembers()!");
  MOZ_ASSERT(!mPromise,
             "mPromise is still alive! Deriving class should call "
             "BluetoothReplyRunnable::ReleaseMembers()!");

  return rv;
}

BluetoothVoidReplyRunnable::BluetoothVoidReplyRunnable(nsIDOMDOMRequest* aReq,
                                                       Promise* aPromise)
  : BluetoothReplyRunnable(aReq, aPromise)
{}

BluetoothVoidReplyRunnable::~BluetoothVoidReplyRunnable()
{}

