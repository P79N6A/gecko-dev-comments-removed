





#ifndef MOZILLA_DOMQUAD_H_
#define MOZILLA_DOMQUAD_H_

#include "nsWrapperCache.h"
#include "nsISupports.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/ErrorResult.h"
#include "Units.h"

namespace mozilla {
namespace dom {

class DOMRectReadOnly;
class DOMPoint;
struct DOMPointInit;

class DOMQuad final : public nsWrapperCache
{
  ~DOMQuad();

public:
  DOMQuad(nsISupports* aParent, CSSPoint aPoints[4]);
  explicit DOMQuad(nsISupports* aParent);

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(DOMQuad)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(DOMQuad)

  nsISupports* GetParentObject() const { return mParent; }
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  static already_AddRefed<DOMQuad>
  Constructor(const GlobalObject& aGlobal,
              const DOMPointInit& aP1,
              const DOMPointInit& aP2,
              const DOMPointInit& aP3,
              const DOMPointInit& aP4,
              ErrorResult& aRV);
  static already_AddRefed<DOMQuad>
  Constructor(const GlobalObject& aGlobal, const DOMRectReadOnly& aRect,
              ErrorResult& aRV);

  DOMRectReadOnly* Bounds() const;
  DOMPoint* P1() const { return mPoints[0]; }
  DOMPoint* P2() const { return mPoints[1]; }
  DOMPoint* P3() const { return mPoints[2]; }
  DOMPoint* P4() const { return mPoints[3]; }

  DOMPoint* Point(uint32_t aIndex) { return mPoints[aIndex]; }

protected:
  class QuadBounds;

  nsCOMPtr<nsISupports> mParent;
  nsRefPtr<DOMPoint> mPoints[4];
  mutable nsRefPtr<QuadBounds> mBounds; 
};

}
}

#endif 
