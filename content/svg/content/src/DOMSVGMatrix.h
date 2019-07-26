



































#ifndef MOZILLA_DOMSVGMATRIX_H__
#define MOZILLA_DOMSVGMATRIX_H__

#include "DOMSVGTransform.h"
#include "gfxMatrix.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"






#define MOZILLA_DOMSVGMATRIX_IID \
  { 0x633419E5, 0x7E88, 0x4C3E, \
    { 0x8A, 0x9A, 0x85, 0x6F, 0x63, 0x5E, 0x90, 0xA3 } }

namespace mozilla {




class DOMSVGMatrix MOZ_FINAL : public nsISupports,
                               public nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOMSVGMATRIX_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGMatrix)

  


  DOMSVGMatrix(DOMSVGTransform& aTransform) : mTransform(&aTransform) {
    SetIsDOMBinding();
  }

  


  
  DOMSVGMatrix() {
    SetIsDOMBinding();
  }

  DOMSVGMatrix(const gfxMatrix &aMatrix) : mMatrix(aMatrix) {
    SetIsDOMBinding();
  }

  const gfxMatrix& Matrix() const {
    return mTransform ? mTransform->Matrixgfx() : mMatrix;
  }

  
  DOMSVGTransform* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap);

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
  already_AddRefed<DOMSVGMatrix> Multiply(DOMSVGMatrix& aMatrix);
  already_AddRefed<DOMSVGMatrix> Inverse(ErrorResult& aRv);
  already_AddRefed<DOMSVGMatrix> Translate(float x, float y);
  already_AddRefed<DOMSVGMatrix> Scale(float scaleFactor);
  already_AddRefed<DOMSVGMatrix> ScaleNonUniform(float scaleFactorX,
                                                 float scaleFactorY);
  already_AddRefed<DOMSVGMatrix> Rotate(float angle);
  already_AddRefed<DOMSVGMatrix> RotateFromVector(float x,
                                                  float y,
                                                  ErrorResult& aRv);
  already_AddRefed<DOMSVGMatrix> FlipX();
  already_AddRefed<DOMSVGMatrix> FlipY();
  already_AddRefed<DOMSVGMatrix> SkewX(float angle, ErrorResult& rv);
  already_AddRefed<DOMSVGMatrix> SkewY(float angle, ErrorResult& rv);

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

NS_DEFINE_STATIC_IID_ACCESSOR(DOMSVGMatrix, MOZILLA_DOMSVGMATRIX_IID)

} 

#endif 
