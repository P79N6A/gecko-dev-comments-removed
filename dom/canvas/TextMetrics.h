




#ifndef mozilla_dom_TextMetrics_h
#define mozilla_dom_TextMetrics_h

#include "mozilla/dom/CanvasRenderingContext2DBinding.h"
#include "mozilla/dom/NonRefcountedDOMObject.h"

namespace mozilla {
namespace dom {

class TextMetrics MOZ_FINAL : public NonRefcountedDOMObject
{
public:
  explicit TextMetrics(float aValue) : width(aValue)
  {
    MOZ_COUNT_CTOR(TextMetrics);
  }

  ~TextMetrics()
  {
    MOZ_COUNT_DTOR(TextMetrics);
  }

  float Width() const
  {
    return width;
  }

  JSObject* WrapObject(JSContext* aCx, bool* aTookOwnership)
  {
    return TextMetricsBinding::Wrap(aCx, this, aTookOwnership);
  }

private:
  float width;
};

} 
} 

#endif 
