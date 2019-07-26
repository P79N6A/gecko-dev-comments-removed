





#include "StructuredCloneUtils.h"

#include "nsIDOMFile.h"
#include "nsIDOMDOMException.h"
#include "nsIMutable.h"
#include "nsIXPConnect.h"

#include "nsContentUtils.h"
#include "nsJSEnvironment.h"
#include "nsThreadUtils.h"
#include "StructuredCloneTags.h"

using namespace mozilla::dom;

namespace {

void
Error(JSContext* aCx, uint32_t aErrorId)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_DOMStructuredCloneError(aCx, aErrorId);
}

JSObject*
Read(JSContext* aCx, JSStructuredCloneReader* aReader, uint32_t aTag,
     uint32_t aData, void* aClosure)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aClosure);

  StructuredCloneClosure* closure =
    static_cast<StructuredCloneClosure*>(aClosure);

  if (aTag == SCTAG_DOM_FILE) {
    MOZ_ASSERT(aData < closure->mBlobs.Length());

    nsCOMPtr<nsIDOMFile> file = do_QueryInterface(closure->mBlobs[aData]);
    MOZ_ASSERT(file);

#ifdef DEBUG
    {
      
      nsCOMPtr<nsIMutable> mutableFile = do_QueryInterface(file);
      bool isMutable;
      if (NS_FAILED(mutableFile->GetMutable(&isMutable))) {
        MOZ_NOT_REACHED("GetMutable failed!");
      }
      else {
        MOZ_ASSERT(!isMutable);
      }
    }
#endif

    JS::Value wrappedFile;
    nsresult rv =
      nsContentUtils::WrapNative(aCx, JS_GetGlobalForScopeChain(aCx), file,
                                  &NS_GET_IID(nsIDOMFile), &wrappedFile);
    if (NS_FAILED(rv)) {
      Error(aCx, nsIDOMDOMException::DATA_CLONE_ERR);
      return nullptr;
    }

    return &wrappedFile.toObject();
  }

  if (aTag == SCTAG_DOM_BLOB) {
    MOZ_ASSERT(aData < closure->mBlobs.Length());

    nsCOMPtr<nsIDOMBlob> blob = do_QueryInterface(closure->mBlobs[aData]);
    MOZ_ASSERT(blob);

#ifdef DEBUG
    {
      
      nsCOMPtr<nsIMutable> mutableBlob = do_QueryInterface(blob);
      bool isMutable;
      if (NS_FAILED(mutableBlob->GetMutable(&isMutable))) {
        MOZ_NOT_REACHED("GetMutable failed!");
      }
      else {
        MOZ_ASSERT(!isMutable);
      }
    }
#endif

    JS::Value wrappedBlob;
    nsresult rv =
      nsContentUtils::WrapNative(aCx, JS_GetGlobalForScopeChain(aCx), blob,
                                  &NS_GET_IID(nsIDOMBlob), &wrappedBlob);
    if (NS_FAILED(rv)) {
      Error(aCx, nsIDOMDOMException::DATA_CLONE_ERR);
      return nullptr;
    }

    return &wrappedBlob.toObject();
  }

  return NS_DOMReadStructuredClone(aCx, aReader, aTag, aData, nullptr);
}

JSBool
Write(JSContext* aCx, JSStructuredCloneWriter* aWriter, JSObject* aObj,
      void* aClosure)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aClosure);

  StructuredCloneClosure* closure =
    static_cast<StructuredCloneClosure*>(aClosure);

  
  nsCOMPtr<nsIXPConnectWrappedNative> wrappedNative;
  nsContentUtils::XPConnect()->
    GetWrappedNativeOfJSObject(aCx, aObj, getter_AddRefs(wrappedNative));

  if (wrappedNative) {
    
    nsISupports* wrappedObject = wrappedNative->Native();
    MOZ_ASSERT(wrappedObject);

    
    nsCOMPtr<nsIDOMFile> file = do_QueryInterface(wrappedObject);
    if (file) {
      nsCOMPtr<nsIMutable> mutableFile = do_QueryInterface(file);
      if (mutableFile &&
          NS_SUCCEEDED(mutableFile->SetMutable(false)) &&
          JS_WriteUint32Pair(aWriter, SCTAG_DOM_FILE,
                             closure->mBlobs.Length())) {
        closure->mBlobs.AppendElement(file);
        return true;
      }
    }

    
    nsCOMPtr<nsIDOMBlob> blob = do_QueryInterface(wrappedObject);
    if (blob) {
      nsCOMPtr<nsIMutable> mutableBlob = do_QueryInterface(blob);
      if (mutableBlob &&
          NS_SUCCEEDED(mutableBlob->SetMutable(false)) &&
          JS_WriteUint32Pair(aWriter, SCTAG_DOM_BLOB,
                             closure->mBlobs.Length())) {
        closure->mBlobs.AppendElement(blob);
        return true;
      }
    }
  }

  return NS_DOMWriteStructuredClone(aCx, aWriter, aObj, nullptr);
}

JSStructuredCloneCallbacks gCallbacks = {
  Read,
  Write,
  Error
};

} 

namespace mozilla {
namespace dom {

bool
ReadStructuredClone(JSContext* aCx, uint64_t* aData, size_t aDataLength,
                    const StructuredCloneClosure& aClosure, JS::Value* aClone)
{
  void* closure = &const_cast<StructuredCloneClosure&>(aClosure);
  return !!JS_ReadStructuredClone(aCx, aData, aDataLength,
                                  JS_STRUCTURED_CLONE_VERSION, aClone,
                                  &gCallbacks, closure);
}

bool
WriteStructuredClone(JSContext* aCx, const JS::Value& aSource,
                     JSAutoStructuredCloneBuffer& aBuffer,
                     StructuredCloneClosure& aClosure)
{
  return aBuffer.write(aCx, aSource, &gCallbacks, &aClosure);
}

} 
} 
