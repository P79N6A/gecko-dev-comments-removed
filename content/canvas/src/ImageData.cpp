





#include "mozilla/dom/ImageData.h"

#include "mozilla/dom/ImageDataBinding.h"
#include "nsCycleCollectionHoldDrop.h"

#include "jsapi.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTING_ADDREF(ImageData)
NS_IMPL_CYCLE_COLLECTING_RELEASE(ImageData)

NS_IMPL_CYCLE_COLLECTION_CLASS(ImageData)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ImageData)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(ImageData)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mData)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(ImageData)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(ImageData)
  tmp->DropData();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

void
ImageData::HoldData()
{
  mozilla::HoldJSObjects(this);
}

void
ImageData::DropData()
{
  if (mData) {
    mData = nullptr;
    mozilla::DropJSObjects(this);
  }
}

JSObject*
ImageData::WrapObject(JSContext* cx, JS::Handle<JSObject*> scope)
{
  return ImageDataBinding::Wrap(cx, scope, this);
}

} 
} 
