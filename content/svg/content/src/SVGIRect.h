




#ifndef mozilla_dom_SVGIRect_h
#define mozilla_dom_SVGIRect_h

#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/SVGRectBinding.h"
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsWrapperCache.h"
#include "nsIContent.h"

class nsSVGElement;

namespace mozilla {
namespace dom {

class SVGIRect : public nsISupports,
                 public nsWrapperCache
{
public:
  SVGIRect()
  {
    SetIsDOMBinding();
  }

  virtual ~SVGIRect()
  {
  }

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return SVGRectBinding::Wrap(aCx, this);
  }

  virtual nsIContent* GetParentObject() const = 0;

  virtual float X() const = 0;

  virtual void SetX(float aX, ErrorResult& aRv) = 0;

  virtual float Y() const = 0;

  virtual void SetY(float aY, ErrorResult& aRv) = 0;

  virtual float Width() const = 0;

  virtual void SetWidth(float aWidth, ErrorResult& aRv) = 0;

  virtual float Height() const = 0;

  virtual void SetHeight(float aHeight, ErrorResult& aRv) = 0;
};

} 
} 

#endif 

