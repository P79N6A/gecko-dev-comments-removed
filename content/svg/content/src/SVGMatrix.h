



































#ifndef mozilla_dom_SVGMatrix_h
#define mozilla_dom_SVGMatrix_h

#include "mozilla/dom/SVGTransform.h"
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

  


  explicit SVGMatrix(SVGTransform& aTransform) : mTransform(&aTransform) {
    SetIsDOMBinding();
  }

  


  
  SVGMatrix() {
    SetIsDOMBinding();
  }

  explicit SVGMatrix(const gfxMatrix &aMatrix) : mMatrix(aMatrix) {
    SetIsDOMBinding();
  }

  const gfxMatrix& GetMatrix() const {
    return mTransform ? mTransform->Matrixgfx() : mMatrix;
  }

  
  SVGTransform* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  float A() const { return static_cast<float>(GetMatrix()._11); }
  void SetA(float aA, ErrorResult& rv);
  float B() const { return static_cast<float>(GetMatrix()._12); }
  void SetB(float aB, ErrorResult& rv);
  float C() const { return static_cast<float>(GetMatrix()._21); }
  void SetC(float aC, ErrorResult& rv);
  float D() const { return static_cast<float>(GetMatrix()._22); }
  void SetD(float aD, ErrorResult& rv);
  float E() const { return static_cast<float>(GetMatrix()._31); }
  void SetE(float aE, ErrorResult& rv);
  float F() const { return static_cast<float>(GetMatrix()._32); }
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
  ~SVGMatrix() {}

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

  nsRefPtr<SVGTransform> mTransform;

  
  
  gfxMatrix mMatrix;
};

} 
} 

#endif 
