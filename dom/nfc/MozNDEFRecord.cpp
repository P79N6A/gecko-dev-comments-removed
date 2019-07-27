







#include "MozNDEFRecord.h"
#include "mozilla/dom/MozNDEFRecordBinding.h"
#include "mozilla/HoldDropJSObjects.h"
#include "nsContentUtils.h"


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
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aGlobal.GetAsSupports());
  if (!win) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  if (!ValidateTNF(aOptions, aRv)) {
    return nullptr;
  }

  nsRefPtr<MozNDEFRecord> ndefrecord = new MozNDEFRecord(aGlobal.Context(),
                                                         win, aOptions);
  if (!ndefrecord) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }
  return ndefrecord.forget();
}

MozNDEFRecord::MozNDEFRecord(JSContext* aCx, nsPIDOMWindow* aWindow,
                             const MozNDEFRecordOptions& aOptions)
{
  mWindow = aWindow; 

  mTnf = aOptions.mTnf;

  if (aOptions.mType.WasPassed()) {
    const Uint8Array& type = aOptions.mType.Value();
    type.ComputeLengthAndData();
    mType = Uint8Array::Create(aCx, this, type.Length(), type.Data());
  }

  if (aOptions.mId.WasPassed()) {
    const Uint8Array& id = aOptions.mId.Value();
    id.ComputeLengthAndData();
    mId = Uint8Array::Create(aCx, this, id.Length(), id.Data());
  }

  if (aOptions.mPayload.WasPassed()) {
    const Uint8Array& payload = aOptions.mPayload.Value();
    payload.ComputeLengthAndData();
    mPayload = Uint8Array::Create(aCx, this, payload.Length(), payload.Data());
  }

  SetIsDOMBinding();
  HoldData();
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
