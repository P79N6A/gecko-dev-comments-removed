





































































#ifndef MOZILLA_DOMSVGMATRIX_H__
#define MOZILLA_DOMSVGMATRIX_H__

#include "nsIDOMSVGMatrix.h"
#include "DOMSVGTransform.h"
#include "gfxMatrix.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"






#define MOZILLA_DOMSVGMATRIX_IID \
  { 0x633419E5, 0x7E88, 0x4C3E, \
    { 0x8A, 0x9A, 0x85, 0x6F, 0x63, 0x5E, 0x90, 0xA3 } }

namespace mozilla {




class DOMSVGMatrix : public nsIDOMSVGMatrix
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOMSVGMATRIX_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGMatrix)
  NS_DECL_NSIDOMSVGMATRIX

  


  DOMSVGMatrix(DOMSVGTransform& aTransform) : mTransform(&aTransform) { }

  


  DOMSVGMatrix() { } 
  DOMSVGMatrix(const gfxMatrix &aMatrix) : mMatrix(aMatrix) { }
  ~DOMSVGMatrix() {
    if (mTransform) {
      mTransform->ClearMatrixTearoff(this);
    }
  }

  const gfxMatrix& Matrix() const {
    return mTransform ? mTransform->Matrix() : mMatrix;
  }

private:
  void SetMatrix(const gfxMatrix& aMatrix) {
    if (mTransform) {
      mTransform->SetMatrix(aMatrix);
    } else {
      mMatrix = aMatrix;
    }
  }

  PRBool IsAnimVal() const {
    return mTransform ? mTransform->IsAnimVal() : PR_FALSE;
  }

  nsRefPtr<DOMSVGTransform> mTransform;

  
  
  gfxMatrix mMatrix;
};

NS_DEFINE_STATIC_IID_ACCESSOR(DOMSVGMatrix, MOZILLA_DOMSVGMATRIX_IID)

} 

#endif 
