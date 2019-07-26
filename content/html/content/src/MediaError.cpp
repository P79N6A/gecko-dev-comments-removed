





#include "mozilla/dom/MediaError.h"
#include "nsDOMClassInfoID.h"
#include "mozilla/dom/MediaErrorBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(MediaError, mParent)
NS_IMPL_CYCLE_COLLECTING_ADDREF(MediaError)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MediaError)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MediaError)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMMediaError)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMediaError)
NS_INTERFACE_MAP_END

MediaError::MediaError(HTMLMediaElement* aParent, uint16_t aCode)
  : mParent(aParent)
  , mCode(aCode)
{
  SetIsDOMBinding();
}

NS_IMETHODIMP MediaError::GetCode(uint16_t* aCode)
{
  if (aCode)
    *aCode = Code();

  return NS_OK;
}

JSObject*
MediaError::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return MediaErrorBinding::Wrap(aCx, aScope, this);
}

} 
} 
