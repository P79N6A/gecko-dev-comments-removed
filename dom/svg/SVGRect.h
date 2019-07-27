





#ifndef mozilla_dom_SVGRect_h
#define mozilla_dom_SVGRect_h

#include "mozilla/dom/SVGIRect.h"
#include "mozilla/gfx/Rect.h"
#include "nsSVGElement.h"




namespace mozilla {
namespace dom {

class SVGRect final : public SVGIRect
{
public:
  explicit SVGRect(nsIContent* aParent, float x=0.0f, float y=0.0f, float w=0.0f,
                   float h=0.0f);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SVGRect)

  
  virtual float X() const override final
  {
    return mX;
  }

  virtual void SetX(float aX, ErrorResult& aRv) final
  {
    mX = aX;
  }

  virtual float Y() const override final
  {
    return mY;
  }

  virtual void SetY(float aY, ErrorResult& aRv) final
  {
    mY = aY;
  }

  virtual float Width() const override final
  {
    return mWidth;
  }

  virtual void SetWidth(float aWidth, ErrorResult& aRv) final
  {
    mWidth = aWidth;
  }

  virtual float Height() const override final
  {
    return mHeight;
  }

  virtual void SetHeight(float aHeight, ErrorResult& aRv) final
  {
    mHeight = aHeight;
  }

  virtual nsIContent* GetParentObject() const override
  {
    return mParent;
  }

protected:
  ~SVGRect() {}

  nsCOMPtr<nsIContent> mParent;
  float mX, mY, mWidth, mHeight;
};

} 
} 

already_AddRefed<mozilla::dom::SVGRect>
NS_NewSVGRect(nsIContent* aParent, float x=0.0f, float y=0.0f,
              float width=0.0f, float height=0.0f);

already_AddRefed<mozilla::dom::SVGRect>
NS_NewSVGRect(nsIContent* aParent, const mozilla::gfx::Rect& rect);

#endif 
