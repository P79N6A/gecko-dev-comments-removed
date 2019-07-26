





#ifndef mozilla_dom_MediaError_h
#define mozilla_dom_MediaError_h

#include "nsIDOMMediaError.h"
#include "nsHTMLMediaElement.h"
#include "nsWrapperCache.h"
#include "nsISupports.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {

class MediaError MOZ_FINAL : public nsIDOMMediaError,
                             public nsWrapperCache
{
public:
  MediaError(nsHTMLMediaElement* aParent, uint16_t aCode);

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MediaError)

  
  NS_DECL_NSIDOMMEDIAERROR

  nsHTMLMediaElement* GetParentObject() const
  {
    return mParent;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap);

  uint16_t Code() const
  {
    return mCode;
  }

private:
  nsRefPtr<nsHTMLMediaElement> mParent;

  
  const uint16_t mCode;
};

} 
} 

#endif 
