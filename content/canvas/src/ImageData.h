





#ifndef mozilla_dom_ImageData_h
#define mozilla_dom_ImageData_h

#include "nsIDOMCanvasRenderingContext2D.h"

#include "mozilla/Attributes.h"
#include "mozilla/StandardInteger.h"

#include "nsCycleCollectionParticipant.h"
#include "nsTraceRefcnt.h"
#include "xpcpublic.h"

#include "jsapi.h"

namespace mozilla {
namespace dom {

class ImageData MOZ_FINAL : public nsIDOMImageData
{
public:
  ImageData(uint32_t aWidth, uint32_t aHeight, JSObject& aData)
    : mWidth(aWidth)
    , mHeight(aHeight)
    , mData(&aData)
  {
    MOZ_COUNT_CTOR(ImageData);
    HoldData();
  }

  ~ImageData()
  {
    MOZ_COUNT_DTOR(ImageData);
    DropData();
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMIMAGEDATA
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(ImageData)

  uint32_t GetWidth()
  {
    return mWidth;
  }
  uint32_t GetHeight()
  {
    return mHeight;
  }
  JSObject* GetData(JSContext* cx)
  {
    return GetDataObject();
  }
  JSObject* GetDataObject()
  {
    xpc_UnmarkGrayObject(mData);
    return mData;
  }

private:
  void HoldData();
  void DropData();

  ImageData() MOZ_DELETE;

  uint32_t mWidth, mHeight;
  JSObject* mData;
};

} 
} 

#endif 
