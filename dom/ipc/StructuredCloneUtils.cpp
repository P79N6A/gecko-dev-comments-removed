





#include "StructuredCloneUtils.h"

#include "nsIDOMDOMException.h"
#include "nsIMutable.h"
#include "nsIXPConnect.h"

#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/BlobBinding.h"
#include "mozilla/dom/File.h"
#include "nsContentUtils.h"
#include "nsJSEnvironment.h"
#include "MainThreadUtils.h"
#include "StructuredCloneTags.h"
#include "jsapi.h"

using namespace mozilla::dom;

namespace {

void
Error(JSContext* aCx, uint32_t aErrorId)
{
  if (NS_IsMainThread()) {
    NS_DOMStructuredCloneError(aCx, aErrorId);
  } else {
    Throw(aCx, NS_ERROR_DOM_DATA_CLONE_ERR);
  }
}

JSObject*
Read(JSContext* aCx, JSStructuredCloneReader* aReader, uint32_t aTag,
     uint32_t aData, void* aClosure)
{
  MOZ_ASSERT(aClosure);

  StructuredCloneClosure* closure =
    static_cast<StructuredCloneClosure*>(aClosure);

  if (aTag == SCTAG_DOM_BLOB) {
    
    
    
    
    
    JS::Rooted<JS::Value> val(aCx);
    {
      MOZ_ASSERT(aData < closure->mBlobImpls.Length());
      nsRefPtr<BlobImpl> blobImpl = closure->mBlobImpls[aData];

#ifdef DEBUG
      {
        
        bool isMutable;
        MOZ_ASSERT(NS_SUCCEEDED(blobImpl->GetMutable(&isMutable)));
        MOZ_ASSERT(!isMutable);
      }
#endif

      
      nsIGlobalObject *global = xpc::NativeGlobal(JS::CurrentGlobalOrNull(aCx));
      MOZ_ASSERT(global);

      nsRefPtr<Blob> newBlob = Blob::Create(global, blobImpl);
      if (!ToJSValue(aCx, newBlob, &val)) {
        return nullptr;
      }
    }

    return &val.toObject();
  }

  return NS_DOMReadStructuredClone(aCx, aReader, aTag, aData, nullptr);
}

bool
Write(JSContext* aCx, JSStructuredCloneWriter* aWriter,
      JS::Handle<JSObject*> aObj, void* aClosure)
{
  MOZ_ASSERT(aClosure);

  StructuredCloneClosure* closure =
    static_cast<StructuredCloneClosure*>(aClosure);

  
  {
    Blob* blob = nullptr;
    if (NS_SUCCEEDED(UNWRAP_OBJECT(Blob, aObj, blob)) &&
        NS_SUCCEEDED(blob->SetMutable(false)) &&
        JS_WriteUint32Pair(aWriter, SCTAG_DOM_BLOB,
                           closure->mBlobImpls.Length())) {
      closure->mBlobImpls.AppendElement(blob->Impl());
      return true;
    }
  }

  return NS_DOMWriteStructuredClone(aCx, aWriter, aObj, nullptr);
}

const JSStructuredCloneCallbacks gCallbacks = {
  Read,
  Write,
  Error,
  nullptr,
  nullptr,
  nullptr
};

} 

namespace mozilla {
namespace dom {

bool
ReadStructuredClone(JSContext* aCx, uint64_t* aData, size_t aDataLength,
                    const StructuredCloneClosure& aClosure,
                    JS::MutableHandle<JS::Value> aClone)
{
  void* closure = &const_cast<StructuredCloneClosure&>(aClosure);
  return !!JS_ReadStructuredClone(aCx, aData, aDataLength,
                                  JS_STRUCTURED_CLONE_VERSION, aClone,
                                  &gCallbacks, closure);
}

bool
WriteStructuredClone(JSContext* aCx, JS::Handle<JS::Value> aSource,
                     JSAutoStructuredCloneBuffer& aBuffer,
                     StructuredCloneClosure& aClosure)
{
  return aBuffer.write(aCx, aSource, &gCallbacks, &aClosure);
}

} 
} 
