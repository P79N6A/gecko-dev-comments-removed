





#ifndef mozilla_dom_ImageCaptureError_h
#define mozilla_dom_ImageCaptureError_h

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {






class ImageCaptureError MOZ_FINAL : public nsISupports,
                                    public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(ImageCaptureError)

  ImageCaptureError(nsISupports* aParent, uint16_t aCode, const nsAString& aMessage);

  nsISupports* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  uint16_t Code() const;

  enum {
    FRAME_GRAB_ERROR = 1,
    SETTINGS_ERROR = 2,
    PHOTO_ERROR = 3,
    ERROR_UNKNOWN = 4,
  };

  void GetMessage(nsAString& retval) const;

private:
  ~ImageCaptureError();

  nsCOMPtr<nsISupports> mParent;
  nsString mMessage;
  uint16_t mCode;
};

} 
} 

#endif 
