





#include "mozilla/dom/StructuredClone.h"

#include "js/StructuredClone.h"
#include "mozilla/dom/ImageData.h"
#include "mozilla/dom/StructuredCloneTags.h"

namespace mozilla {
namespace dom {

JSObject*
ReadStructuredCloneImageData(JSContext* aCx, JSStructuredCloneReader* aReader)
{
  
  uint32_t width, height;
  JS::Rooted<JS::Value> dataArray(aCx);
  if (!JS_ReadUint32Pair(aReader, &width, &height) ||
      !JS_ReadTypedArray(aReader, &dataArray)) {
    return nullptr;
  }
  MOZ_ASSERT(dataArray.isObject());

  
  JS::Rooted<JSObject*> result(aCx);
  {
    
    nsRefPtr<ImageData> imageData = new ImageData(width, height,
                                                  dataArray.toObject());
    
    result = imageData->WrapObject(aCx);
  }
  return result;
}

bool
WriteStructuredCloneImageData(JSContext* aCx, JSStructuredCloneWriter* aWriter,
                              ImageData* aImageData)
{
  uint32_t width = aImageData->Width();
  uint32_t height = aImageData->Height();
  JS::Rooted<JSObject*> dataArray(aCx, aImageData->GetDataObject());

  JSAutoCompartment ac(aCx, dataArray);
  JS::Rooted<JS::Value> arrayValue(aCx, JS::ObjectValue(*dataArray));
  return JS_WriteUint32Pair(aWriter, SCTAG_DOM_IMAGEDATA, 0) &&
         JS_WriteUint32Pair(aWriter, width, height) &&
         JS_WriteTypedArray(aWriter, arrayValue);
}

} 
} 
