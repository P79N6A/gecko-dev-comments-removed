




#include "mozilla/dom/SVGAnimatedString.h"
#include "mozilla/dom/SVGAnimatedStringBinding.h"

namespace mozilla {
namespace dom {

JSObject*
SVGAnimatedString::WrapObject(JSContext* aCx)
{
  return SVGAnimatedStringBinding::Wrap(aCx, this);
}

} 
} 
