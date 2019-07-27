




#include "mozilla/dom/SVGAnimatedString.h"
#include "mozilla/dom/SVGAnimatedStringBinding.h"

namespace mozilla {
namespace dom {

JSObject*
SVGAnimatedString::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return SVGAnimatedStringBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 
