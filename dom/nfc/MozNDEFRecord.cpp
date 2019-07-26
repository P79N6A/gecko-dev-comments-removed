







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


already_AddRefed<MozNDEFRecord>
MozNDEFRecord::Constructor(const GlobalObject& aGlobal,
                           uint8_t aTnf,
                           const Optional<Uint8Array>& aType,
                           const Optional<Uint8Array>& aId,
                           const Optional<Uint8Array>& aPayload,
                           ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aGlobal.GetAsSupports());
  if (!win) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<MozNDEFRecord> ndefrecord = new MozNDEFRecord(aGlobal.GetContext(),
                                                         win, aTnf, aType, aId,
                                                         aPayload);
  if (!ndefrecord) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }
  return ndefrecord.forget();
}

MozNDEFRecord::MozNDEFRecord(JSContext* aCx, nsPIDOMWindow* aWindow,
                             uint8_t aTnf,
                             const Optional<Uint8Array>& aType,
                             const Optional<Uint8Array>& aId,
                             const Optional<Uint8Array>& aPayload)
  : mTnf(aTnf)
{
  mWindow = aWindow; 

  if (aType.WasPassed()) {
    mType = Uint8Array::Create(aCx, this, aType.Value().Length(), aType.Value().Data());
  }

  if (aId.WasPassed()) {
    mId = Uint8Array::Create(aCx, this, aId.Value().Length(), aId.Value().Data());
  }

  if (aPayload.WasPassed()) {
    mPayload = Uint8Array::Create(aCx, this, aPayload.Value().Length(), aPayload.Value().Data());
  }

  SetIsDOMBinding();
  HoldData();
}

MozNDEFRecord::~MozNDEFRecord()
{
  DropData();
}

JSObject*
MozNDEFRecord::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return MozNDEFRecordBinding::Wrap(aCx, aScope, this);
}

} 
} 
