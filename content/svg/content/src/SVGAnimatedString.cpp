




#include "mozilla/dom/SVGAnimatedString.h"
#include "mozilla/dom/SVGAnimatedStringBinding.h"

namespace mozilla {
namespace dom {

JSObject*
SVGAnimatedString::WrapObject(JSContext* aCx,
                              JS::Handle<JSObject*> aScope)
{
  return SVGAnimatedStringBinding::Wrap(aCx, this);
}

} 
} 
