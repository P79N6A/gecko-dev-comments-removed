





#include "mozilla/dom/ImageData.h"

#include "nsDOMClassInfoID.h"
#include "nsContentUtils.h"

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
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_CALLBACK(tmp->mData, "mData")
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(ImageData)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(ImageData)
  tmp->DropData();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


NS_IMETHODIMP
ImageData::GetWidth(PRUint32* aWidth)
{
  *aWidth = GetWidth();
  return NS_OK;
}


NS_IMETHODIMP
ImageData::GetHeight(PRUint32* aHeight)
{
  *aHeight = GetHeight();
  return NS_OK;
}


NS_IMETHODIMP
ImageData::GetData(JS::Value* aData)
{
  *aData = GetData();
  return NS_OK;
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
