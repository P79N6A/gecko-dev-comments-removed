




#ifndef mozilla_dom_SVGZoomEvent_h
#define mozilla_dom_SVGZoomEvent_h

#include "DOMSVGPoint.h"
#include "mozilla/dom/UIEvent.h"
#include "mozilla/dom/SVGZoomEventBinding.h"
#include "mozilla/EventForwards.h"
#include "nsAutoPtr.h"

class nsPresContext;

namespace mozilla {

class nsISVGPoint;

namespace dom {

class SVGZoomEvent MOZ_FINAL : public UIEvent
{
public:
  SVGZoomEvent(EventTarget* aOwner, nsPresContext* aPresContext,
               WidgetGUIEvent* aEvent);

  
  NS_FORWARD_TO_UIEVENT

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return SVGZoomEventBinding::Wrap(aCx, aScope, this);
  }

  float PreviousScale() const
  {
    return mPreviousScale;
  }

  nsISVGPoint* GetPreviousTranslate() const
  {
    return mPreviousTranslate;
  }

  float NewScale() const
  {
    return mNewScale;
  }

  nsISVGPoint* GetNewTranslate() const
  {
    return mNewTranslate;
  }

private:
  float mPreviousScale;
  float mNewScale;
  nsRefPtr<DOMSVGPoint> mPreviousTranslate;
  nsRefPtr<DOMSVGPoint> mNewTranslate;
};

} 
} 

#endif 
