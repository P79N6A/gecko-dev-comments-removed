



#ifndef CustomQS_Canvas_h
#define CustomQS_Canvas_h

#include "jsapi.h"

#include "mozilla/dom/ImageData.h"

#include "XPCQuickStubs.h"

static bool
GetImageData(JSContext* cx, JS::Value& imageData,
             uint32_t* width, uint32_t* height, JS::Anchor<JSObject*>* array)
{
  if (!imageData.isObject()) {
    return xpc_qsThrow(cx, NS_ERROR_DOM_TYPE_MISMATCH_ERR);
  }

  nsIDOMImageData* domImageData;
  xpc_qsSelfRef imageDataRef;
  nsresult rv = xpc_qsUnwrapArg<nsIDOMImageData>(cx, imageData, &domImageData,
                                                 &imageDataRef.ptr, &imageData);
  if (NS_FAILED(rv)) {
    return xpc_qsThrow(cx, rv);
  }

  mozilla::dom::ImageData* concreteImageData =
    static_cast<mozilla::dom::ImageData*>(domImageData);
  *width = concreteImageData->GetWidth();
  *height = concreteImageData->GetHeight();
  array->set(concreteImageData->GetDataObject());
  return true;
}

#endif 
