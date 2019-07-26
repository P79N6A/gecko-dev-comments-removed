





#ifndef mozilla_dom_MediaError_h
#define mozilla_dom_MediaError_h

#include "nsIDOMMediaError.h"
#include "nsISupports.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {

class MediaError MOZ_FINAL : public nsIDOMMediaError
{
public:
  MediaError(uint16_t aCode);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMMEDIAERROR

private:
  
  uint16_t mCode;
};

} 
} 

#endif 
