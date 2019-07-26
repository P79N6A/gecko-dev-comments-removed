





#include "mozilla/dom/ImageData.h"

#include "nsDOMClassInfoID.h"
#include "nsContentUtils.h"
#include "mozilla/dom/CanvasRenderingContext2DBinding.h"

#include "jsapi.h"

DOMCI_DATA(ImageData, mozilla::dom::ImageData)

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTING_ADDREF(ImageData)
NS_IMPL_CYCLE_COLLECTING_RELEASE(ImageData)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ImageData)
  NS_INTERFACE_MAP_ENTRY(nsIDOMImageData)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ImageData)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_CLASS(ImageData)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(ImageData)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mData)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(ImageData)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(ImageData)
  tmp->DropData();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


NS_IMETHODIMP
ImageData::GetWidth(uint32_t* aWidth)
{
  *aWidth = Width();
  return NS_OK;
}


NS_IMETHODIMP
ImageData::GetHeight(uint32_t* aHeight)
{
  *aHeight = Height();
  return NS_OK;
}


NS_IMETHODIMP
ImageData::GetData(JSContext* aCx, JS::Value* aData)
{
  *aData = JS::ObjectOrNullValue(GetDataObject());
  return JS_WrapValue(aCx, aData) ? NS_OK : NS_ERROR_FAILURE;
}

void
ImageData::HoldData()
{
  NS_HOLD_JS_OBJECTS(this, ImageData);
}

void
ImageData::DropData()
{
  if (mData) {
    NS_DROP_JS_OBJECTS(this, ImageData);
    mData = NULL;
  }
}

} 
} 
