







#include "MozNDEFRecord.h"
#include "mozilla/dom/MozNDEFRecordBinding.h"
#include "mozilla/HoldDropJSObjects.h"
#include "nsContentUtils.h"
#include "nsString.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(MozNDEFRecord)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(MozNDEFRecord)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mWindow)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
  tmp->DropData();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(MozNDEFRecord)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(MozNDEFRecord)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mType)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mId)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mPayload)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(MozNDEFRecord)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MozNDEFRecord)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MozNDEFRecord)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

void
MozNDEFRecord::HoldData()
{
  mozilla::HoldJSObjects(this);
}

void
MozNDEFRecord::DropData()
{
  if (mType) {
    mType = nullptr;
  }
  if (mId) {
    mId = nullptr;
  }
  if (mPayload) {
    mPayload = nullptr;
  }
  mozilla::DropJSObjects(this);
}






 bool
MozNDEFRecord::ValidateTNF(const MozNDEFRecordOptions& aOptions,
                           ErrorResult& aRv)
{
  
  
  

  
  
  if ((aOptions.mTnf == TNF::Empty) &&
      (aOptions.mType.WasPassed() || aOptions.mId.WasPassed() ||
       aOptions.mPayload.WasPassed())) {
    NS_WARNING("tnf is empty but type/id/payload is not null.");
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return false;
  }

  
  
  if ((aOptions.mTnf == TNF::Unknown || aOptions.mTnf == TNF::Unchanged) &&
      aOptions.mType.WasPassed()) {
    NS_WARNING("tnf is unknown/unchanged but type  is not null.");
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return false;
  }

  return true;
}

 already_AddRefed<MozNDEFRecord>
MozNDEFRecord::Constructor(const GlobalObject& aGlobal,
                           const MozNDEFRecordOptions& aOptions,
                           ErrorResult& aRv)
{
  if (!ValidateTNF(aOptions, aRv)) {
    return nullptr;
  }

  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aGlobal.GetAsSupports());
  if (!win) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  JSContext* context = aGlobal.Context();
  nsRefPtr<MozNDEFRecord> ndefRecord = new MozNDEFRecord(win, aOptions.mTnf);
  ndefRecord->InitType(context, aOptions.mType);
  ndefRecord->InitId(context, aOptions.mId);
  ndefRecord->InitPayload(context, aOptions.mPayload);

  return ndefRecord.forget();
}

 already_AddRefed<MozNDEFRecord>
MozNDEFRecord::Constructor(const GlobalObject& aGlobal,
                           const nsAString& aUri,
                           ErrorResult& aRv)
{
  if (aUri.IsVoid()) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return nullptr;
  }

  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aGlobal.GetAsSupports());
  if (!win) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<MozNDEFRecord> ndefRecord = new MozNDEFRecord(win, TNF::Well_known);
  ndefRecord->InitType(aGlobal.Context(), RTD::U);
  ndefRecord->InitPayload(aGlobal.Context(), aUri);
  return ndefRecord.forget();
}

MozNDEFRecord::MozNDEFRecord(nsPIDOMWindow* aWindow, TNF aTnf)
  : mWindow(aWindow) 
  , mTnf(aTnf)
  , mSize(3) 
{
  HoldData();
}

void
MozNDEFRecord::GetAsURI(nsAString& aRetVal)
{
  aRetVal.SetIsVoid(true);
  if (mTnf != TNF::Well_known) {
    return;
  }

  JS::AutoCheckCannotGC nogc;
  uint8_t* typeData = JS_GetUint8ArrayData(mType, nogc);
  const char* uVal = RTDValues::strings[static_cast<uint32_t>(RTD::U)].value;
  if (typeData[0] != uVal[0]) {
    return;
  }

  uint32_t payloadLen;
  uint8_t* payloadData;
  js::GetUint8ArrayLengthAndData(mPayload, &payloadLen, &payloadData);
  uint8_t id = payloadData[0];
  if (id >= static_cast<uint8_t>(WellKnownURIPrefix::EndGuard_)) {
    return;
  }

  using namespace mozilla::dom::WellKnownURIPrefixValues;
  aRetVal.AssignASCII(strings[id].value);
  aRetVal.Append(NS_ConvertUTF8toUTF16(
    nsDependentCSubstring(reinterpret_cast<char*>(&payloadData[1]), payloadLen - 1)));
}

void
MozNDEFRecord::InitType(JSContext* aCx, const Optional<Uint8Array>& aType)
{
  if (!aType.WasPassed()) {
    return;
  }

  const Uint8Array& type = aType.Value();
  type.ComputeLengthAndData();
  mType = Uint8Array::Create(aCx, this, type.Length(), type.Data());
  IncSize(type.Length());
}

void
MozNDEFRecord::InitType(JSContext* aCx, RTD rtd)
{
  EnumEntry rtdType = RTDValues::strings[static_cast<uint32_t>(rtd)];
  mType = Uint8Array::Create(aCx, rtdType.length,
                             reinterpret_cast<const uint8_t*>(rtdType.value));
  IncSize(rtdType.length);
}

void
MozNDEFRecord::InitId(JSContext* aCx, const Optional<Uint8Array>& aId)
{
  if (!aId.WasPassed()) {
    return;
  }

  const Uint8Array& id = aId.Value();
  id.ComputeLengthAndData();
  mId = Uint8Array::Create(aCx, this, id.Length(), id.Data());
  IncSize(1  + id.Length());
}

void
MozNDEFRecord::InitPayload(JSContext* aCx, const Optional<Uint8Array>& aPayload)
{
  if (!aPayload.WasPassed()) {
    return;
  }

  const Uint8Array& payload = aPayload.Value();
  payload.ComputeLengthAndData();
  mPayload = Uint8Array::Create(aCx, this, payload.Length(), payload.Data());
  IncSizeForPayload(payload.Length());
}

void
MozNDEFRecord::InitPayload(JSContext* aCx, const nsAString& aUri)
{
  using namespace mozilla::dom::WellKnownURIPrefixValues;

  nsCString uri = NS_ConvertUTF16toUTF8(aUri);
  uint8_t id = GetURIIdentifier(uri);
  uri = Substring(uri, strings[id].length);
  mPayload = Uint8Array::Create(aCx, this, uri.Length() + 1);

  JS::AutoCheckCannotGC nogc;
  uint8_t* data = JS_GetUint8ArrayData(mPayload, nogc);
  data[0] = id;
  memcpy(&data[1], reinterpret_cast<const uint8_t*>(uri.Data()), uri.Length());
  IncSizeForPayload(uri.Length() + 1);
}

void
MozNDEFRecord::IncSize(uint32_t aCount)
{
  mSize += aCount;
}

void
MozNDEFRecord::IncSizeForPayload(uint32_t aLen)
{
  if (aLen > 0xff) {
    IncSize(3);
  }

  IncSize(aLen);
}

 uint32_t
MozNDEFRecord::GetURIIdentifier(const nsCString& aUri)
{
  using namespace mozilla::dom::WellKnownURIPrefixValues;

  
  for (uint32_t i = 1; i < static_cast<uint32_t>(WellKnownURIPrefix::EndGuard_); i++) {
    if (StringBeginsWith(aUri, nsDependentCString(strings[i].value))) {
      return i;
    }
  }

  return 0;
}

MozNDEFRecord::~MozNDEFRecord()
{
  DropData();
}

JSObject*
MozNDEFRecord::WrapObject(JSContext* aCx)
{
  return MozNDEFRecordBinding::Wrap(aCx, this);
}

} 
} 
