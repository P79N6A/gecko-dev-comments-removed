







































#ifndef MOZILLA_DOMSVGTRANSFORM_H__
#define MOZILLA_DOMSVGTRANSFORM_H__

#include "nsIDOMSVGTransform.h"
#include "DOMSVGTransformList.h"
#include "SVGTransform.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"






#define MOZILLA_DOMSVGTRANSFORM_IID \
  { 0x0A799862, 0x9469, 0x41FE, \
    { 0xB4, 0xCD, 0x20, 0x19, 0xE6, 0x5D, 0x8D, 0xA6 } }

#define MOZ_SVG_LIST_INDEX_BIT_COUNT 22 // supports > 4 million list items

namespace mozilla {

class DOMSVGMatrix;




class DOMSVGTransform : public nsIDOMSVGTransform
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOMSVGTRANSFORM_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGTransform)
  NS_DECL_NSIDOMSVGTRANSFORM

  


  DOMSVGTransform(DOMSVGTransformList *aList,
                  PRUint32 aListIndex,
                  PRBool aIsAnimValItem);

  






  DOMSVGTransform();
  DOMSVGTransform(const gfxMatrix &aMatrix);

  


  DOMSVGTransform(const SVGTransform &aMatrix);

  ~DOMSVGTransform() {
    
    
    
    NS_ABORT_IF_FALSE(!mMatrixTearoff, "Matrix tear-off pointer not cleared."
        " Transform being destroyed before matrix?");
    
    
    
    if (mList) {
      mList->mItems[mListIndex] = nsnull;
    }
  };

  



  DOMSVGTransform* Clone() {
    NS_ASSERTION(mList, "unexpected caller");
    return new DOMSVGTransform(InternalItem());
  }

  PRBool IsInList() const {
    return !!mList;
  }

  



  PRBool HasOwner() const {
    return !!mList;
  }

  









  void InsertingIntoList(DOMSVGTransformList *aList,
                         PRUint32 aListIndex,
                         PRBool aIsAnimValItem);

  static PRUint32 MaxListIndex() {
    return (1U << MOZ_SVG_LIST_INDEX_BIT_COUNT) - 1;
  }

  
  void UpdateListIndex(PRUint32 aListIndex) {
    mListIndex = aListIndex;
  }

  





  void RemovingFromList();

  SVGTransform ToSVGTransform() const {
    return Transform();
  }

protected:
  
  friend class DOMSVGMatrix;
  const PRBool IsAnimVal() const {
    return mIsAnimValItem;
  }
  const gfxMatrix& Matrix() const {
    return Transform().Matrix();
  }
  void SetMatrix(const gfxMatrix& aMatrix);
  void ClearMatrixTearoff(DOMSVGMatrix* aMatrix);

private:
  nsSVGElement* Element() {
    return mList->Element();
  }

  



  SVGTransform& InternalItem();
  const SVGTransform& InternalItem() const;

#ifdef DEBUG
  PRBool IndexIsValid();
#endif

  const SVGTransform& Transform() const {
    return HasOwner() ? InternalItem() : *mTransform;
  }
  SVGTransform& Transform() {
    return HasOwner() ? InternalItem() : *mTransform;
  }
  void NotifyElementOfChange();

  nsRefPtr<DOMSVGTransformList> mList;

  
  

  PRUint32 mListIndex:MOZ_SVG_LIST_INDEX_BIT_COUNT;
  PRPackedBool mIsAnimValItem:1;

  
  
  
  
  
  
  nsAutoPtr<SVGTransform> mTransform;

  
  
  
  
  
  
  DOMSVGMatrix* mMatrixTearoff;
};

NS_DEFINE_STATIC_IID_ACCESSOR(DOMSVGTransform, MOZILLA_DOMSVGTRANSFORM_IID)

} 

#undef MOZ_SVG_LIST_INDEX_BIT_COUNT

#endif 
