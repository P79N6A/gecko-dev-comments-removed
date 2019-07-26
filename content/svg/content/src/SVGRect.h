




#ifndef mozilla_dom_SVGRect_h
#define mozilla_dom_SVGRect_h

#include "gfxRect.h"
#include "mozilla/dom/SVGIRect.h"




namespace mozilla {
namespace dom {

class SVGRect MOZ_FINAL : public SVGIRect
{
public:
  SVGRect(float x=0.0f, float y=0.0f, float w=0.0f, float h=0.0f);

  
  NS_DECL_ISUPPORTS

  
  virtual float X() const MOZ_OVERRIDE MOZ_FINAL
  {
    return mX;
  }

  virtual void SetX(float aX, ErrorResult& aRv) MOZ_FINAL
  {
    mX = aX;
  }

  virtual float Y() const MOZ_OVERRIDE MOZ_FINAL
  {
    return mY;
  }

  virtual void SetY(float aY, ErrorResult& aRv) MOZ_FINAL
  {
    mY = aY;
  }

  virtual float Width() const MOZ_OVERRIDE MOZ_FINAL
  {
    return mWidth;
  }

  virtual void SetWidth(float aWidth, ErrorResult& aRv) MOZ_FINAL
  {
    mWidth = aWidth;
  }

  virtual float Height() const MOZ_OVERRIDE MOZ_FINAL
  {
    return mHeight;
  }

  virtual void SetHeight(float aHeight, ErrorResult& aRv) MOZ_FINAL
  {
    mHeight = aHeight;
  }

  using mozilla::dom::SVGIRect::SetX;
  using mozilla::dom::SVGIRect::SetY;
  using mozilla::dom::SVGIRect::SetWidth;
  using mozilla::dom::SVGIRect::SetHeight;

protected:
  float mX, mY, mWidth, mHeight;
};

} 
} 

nsresult
NS_NewSVGRect(mozilla::dom::SVGRect** result,
              float x=0.0f, float y=0.0f,
              float width=0.0f, float height=0.0f);

nsresult
NS_NewSVGRect(mozilla::dom::SVGRect** result, const gfxRect& rect);

#endif 
