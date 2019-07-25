



#ifndef CustomQS_Canvas_h
#define CustomQS_Canvas_h

#include "jsapi.h"

#include "mozilla/dom/ImageData.h"

static bool
GetPositiveInt(JSContext* cx, JSObject& obj, const char* name, uint32_t* out)
{
  JS::Value temp;
  int32_t signedInt;
  if (!JS_GetProperty(cx, &obj, name, &temp) ||
      !JS_ValueToECMAInt32(cx, temp, &signedInt)) {
    return false;
  }
  if (signedInt <= 0) {
    return xpc_qsThrow(cx, NS_ERROR_DOM_TYPE_MISMATCH_ERR);
  }
  *out = uint32_t(signedInt);
  return true;
}

static bool
GetImageData(JSContext* cx, JS::Value& imageData,
             uint32_t* width, uint32_t* height, JS::Anchor<JSObject*>* array)
{
  if (!imageData.isObject()) {
    return xpc_qsThrow(cx, NS_ERROR_DOM_TYPE_MISMATCH_ERR);
  }

  nsIDOMImageData* domImageData;
  xpc_qsSelfRef imageDataRef;
  if (NS_SUCCEEDED(xpc_qsUnwrapArg<nsIDOMImageData>(cx, imageData,
                                                    &domImageData,
                                                    &imageDataRef.ptr,
                                                    &imageData))) {
    mozilla::dom::ImageData* concreteImageData =
      static_cast<mozilla::dom::ImageData*>(domImageData);
    *width = concreteImageData->GetWidth();
    *height = concreteImageData->GetHeight();
    array->set(concreteImageData->GetDataObject());
    return true;
  }

  
  JSObject& dataObject = imageData.toObject();

  if (!GetPositiveInt(cx, dataObject, "width", width) ||
      !GetPositiveInt(cx, dataObject, "height", height)) {
    return false;
  }

  JS::Value temp;
  if (!JS_GetProperty(cx, &dataObject, "data", &temp)) {
    return false;
  }
  if (!temp.isObject()) {
    return xpc_qsThrow(cx, NS_ERROR_DOM_TYPE_MISMATCH_ERR);
  }
  array->set(&temp.toObject());
  return true;
}

#endif 
