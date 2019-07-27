




#include "mozilla/dom/ImageCaptureError.h"
#include "mozilla/dom/ImageCaptureErrorEventBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(ImageCaptureError, mParent)
NS_IMPL_CYCLE_COLLECTING_ADDREF(ImageCaptureError)
NS_IMPL_CYCLE_COLLECTING_RELEASE(ImageCaptureError)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ImageCaptureError)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

ImageCaptureError::ImageCaptureError(nsISupports* aParent,
                                     uint16_t aCode,
                                     const nsAString& aMessage)
  : mParent(aParent)
  , mMessage(aMessage)
  , mCode(aCode)
{
}

ImageCaptureError::~ImageCaptureError()
{
}

nsISupports*
ImageCaptureError::GetParentObject() const
{
  return mParent;
}

JSObject*
ImageCaptureError::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return ImageCaptureErrorBinding::Wrap(aCx, this, aGivenProto);
}

uint16_t
ImageCaptureError::Code() const
{
  return mCode;
}

void
ImageCaptureError::GetMessage(nsAString& retval) const
{
  retval = mMessage;
}

} 
} 
