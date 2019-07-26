



































#ifndef mozilla_dom_SVGMatrix_h
#define mozilla_dom_SVGMatrix_h

#include "DOMSVGTransform.h"
#include "gfxMatrix.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {




class SVGMatrix MOZ_FINAL : public nsWrapperCache
{
public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(SVGMatrix)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(SVGMatrix)

  


  SVGMatrix(DOMSVGTransform& aTransform) : mTransform(&aTransform) {
    SetIsDOMBinding();
  }

  


  
  SVGMatrix() {
    SetIsDOMBinding();
  }

  SVGMatrix(const gfxMatrix &aMatrix) : mMatrix(aMatrix) {
    SetIsDOMBinding();
  }

  const gfxMatrix& Matrix() const {
    return mTransform ? mTransform->Matrixgfx() : mMatrix;
  }

  
  DOMSVGTransform* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  float A() const { return static_cast<float>(Matrix().xx); }
  void SetA(float aA, ErrorResult& rv);
  float B() const { return static_cast<float>(Matrix().yx); }
  void SetB(float aB, ErrorResult& rv);
  float C() const { return static_cast<float>(Matrix().xy); }
  void SetC(float aC, ErrorResult& rv);
  float D() const { return static_cast<float>(Matrix().yy); }
  void SetD(float aD, ErrorResult& rv);
  float E() const { return static_cast<float>(Matrix().x0); }
  void SetE(float aE, ErrorResult& rv);
  float F() const { return static_cast<float>(Matrix().y0); }
  void SetF(float aF, ErrorResult& rv);
  already_AddRefed<SVGMatrix> Multiply(SVGMatrix& aMatrix);
  already_AddRefed<SVGMatrix> Inverse(ErrorResult& aRv);
  already_AddRefed<SVGMatrix> Translate(float x, float y);
  already_AddRefed<SVGMatrix> Scale(float scaleFactor);
  already_AddRefed<SVGMatrix> ScaleNonUniform(float scaleFactorX,
                                              float scaleFactorY);
  already_AddRefed<SVGMatrix> Rotate(float angle);
  already_AddRefed<SVGMatrix> RotateFromVector(float x,
                                               float y,
                                               ErrorResult& aRv);
  already_AddRefed<SVGMatrix> FlipX();
  already_AddRefed<SVGMatrix> FlipY();
  already_AddRefed<SVGMatrix> SkewX(float angle, ErrorResult& rv);
  already_AddRefed<SVGMatrix> SkewY(float angle, ErrorResult& rv);

private:
  void SetMatrix(const gfxMatrix& aMatrix) {
    if (mTransform) {
      mTransform->SetMatrix(aMatrix);
    } else {
      mMatrix = aMatrix;
    }
  }

  bool IsAnimVal() const {
    return mTransform ? mTransform->IsAnimVal() : false;
  }

  nsRefPtr<DOMSVGTransform> mTransform;

  
  
  gfxMatrix mMatrix;
};

} 
} 

#endif 
