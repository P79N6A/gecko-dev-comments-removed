





#include "BluetoothCommon.h"
#include "MediaMetaData.h"

#include "nsCxPusher.h"
#include "nsContentUtils.h"
#include "nsJSUtils.h"
#include "nsThreadUtils.h"

using namespace mozilla;
USING_BLUETOOTH_NAMESPACE

MediaMetaData::MediaMetaData() : mDuration(-1)
                               , mMediaNumber(-1)
                               , mTotalMediaCount(-1)
{
}

nsresult
MediaMetaData::Init(JSContext* aCx, const jsval* aVal)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!aCx || !aVal) {
    return NS_OK;
  }

  if (!aVal->isObject()) {
    return aVal->isNullOrUndefined() ? NS_OK : NS_ERROR_TYPE_ERR;
  }

  JS::RootedObject obj(aCx, &aVal->toObject());
  nsCxPusher pusher;
  pusher.Push(aCx);
  JSAutoCompartment ac(aCx, obj);

  JS::Rooted<JS::Value> value(aCx);
  NS_ENSURE_STATE(JS_GetProperty(aCx, obj, "mAlbum", &value));
  if (JSVAL_IS_STRING(value)) {
    nsDependentJSString jsString;
    NS_ENSURE_STATE(jsString.init(aCx, value.toString()));
    mAlbum = jsString;
  }

  NS_ENSURE_STATE(JS_GetProperty(aCx, obj, "mArtist", &value));
  if (JSVAL_IS_STRING(value)) {
    nsDependentJSString jsString;
    NS_ENSURE_STATE(JSVAL_IS_STRING(value));
    NS_ENSURE_STATE(jsString.init(aCx, value.toString()));
    mArtist = jsString;
  }

  NS_ENSURE_STATE(JS_GetProperty(aCx, obj, "mDuration", &value));
  if (JSVAL_IS_INT(value)) {
    NS_ENSURE_STATE(JS_ValueToInt64(aCx, value, &mDuration));
  }

  NS_ENSURE_STATE(JS_GetProperty(aCx, obj, "mMediaNumber", &value));
  if (JSVAL_IS_INT(value)) {
    NS_ENSURE_STATE(JS_ValueToInt64(aCx, value, &mMediaNumber));
  }

  NS_ENSURE_STATE(JS_GetProperty(aCx, obj, "mTitle", &value));
  if (JSVAL_IS_STRING(value)) {
    nsDependentJSString jsString;
    NS_ENSURE_STATE(JSVAL_IS_STRING(value));
    NS_ENSURE_STATE(jsString.init(aCx, value.toString()));
    mTitle = jsString;
  }

  NS_ENSURE_STATE(JS_GetProperty(aCx, obj, "mTotalMediaCount", &value));
  if (JSVAL_IS_INT(value)) {
    NS_ENSURE_STATE(JS_ValueToInt64(aCx, value, &mTotalMediaCount));
  }

  return NS_OK;
}

