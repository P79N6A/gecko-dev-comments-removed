




#ifndef MOZILLA_DOMSVGPOINT_H__
#define MOZILLA_DOMSVGPOINT_H__

#include "DOMSVGPointList.h"
#include "gfxPoint.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsISVGPoint.h"
#include "nsTArray.h"
#include "SVGPoint.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"

class nsSVGElement;






#define MOZILLA_DOMSVGPOINT_IID \
  { 0xd6b6c440, 0xaf8d, 0x40ee, \
    { 0x85, 0x6b, 0x02, 0xa3, 0x17, 0xca, 0xb2, 0x75 } }

#define MOZ_SVG_LIST_INDEX_BIT_COUNT 30

namespace mozilla {

namespace dom {
class SVGMatrix;
}















class DOMSVGPoint MOZ_FINAL : public nsISVGPoint
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOMSVGPOINT_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGPoint)

  


  DOMSVGPoint(DOMSVGPointList *aList,
              uint32_t aListIndex,
              bool aIsAnimValItem)
    : nsISVGPoint()
    , mList(aList)
    , mListIndex(aListIndex)
    , mIsReadonly(false)
    , mIsAnimValItem(aIsAnimValItem)
  {
    
    NS_ABORT_IF_FALSE(aList &&
                      aListIndex <= MaxListIndex(), "bad arg");

    NS_ABORT_IF_FALSE(IndexIsValid(), "Bad index for DOMSVGPoint!");
  }

  explicit DOMSVGPoint(const DOMSVGPoint *aPt = nullptr)
    : nsISVGPoint()
    , mList(nullptr)
    , mListIndex(0)
    , mIsReadonly(false)
    , mIsAnimValItem(false)
  {
    if (aPt) {
      mPt = aPt->ToSVGPoint();
    }
  }

  DOMSVGPoint(float aX, float aY)
    : nsISVGPoint()
    , mList(nullptr)
    , mListIndex(0)
    , mIsReadonly(false)
    , mIsAnimValItem(false)
  {
    mPt.mX = aX;
    mPt.mY = aY;
  }

  explicit DOMSVGPoint(const gfxPoint &aPt)
    : nsISVGPoint()
    , mList(nullptr)
    , mListIndex(0)
    , mIsReadonly(false)
    , mIsAnimValItem(false)
  {
    mPt.mX = float(aPt.x);
    mPt.mY = float(aPt.y);
    NS_ASSERTION(NS_finite(mPt.mX) && NS_finite(mPt.mX),
                 "DOMSVGPoint coords are not finite");
  }


  virtual ~DOMSVGPoint() {
    
    
    
    if (mList) {
      mList->mItems[mListIndex] = nullptr;
    }
  }

  
  virtual float X();
  virtual void SetX(float aX, ErrorResult& rv);
  virtual float Y();
  virtual void SetY(float aY, ErrorResult& rv);
  virtual already_AddRefed<nsISVGPoint> MatrixTransform(dom::SVGMatrix& matrix);
  nsISupports* GetParentObject() MOZ_OVERRIDE {
    return mList;
  }

  



  DOMSVGPoint* Clone() {
    return new DOMSVGPoint(this);
  }

  bool IsInList() const {
    return !!mList;
  }

  





  bool HasOwner() const {
    return !!mList;
  }

  








  void InsertingIntoList(DOMSVGPointList *aList,
                         uint32_t aListIndex,
                         bool aIsAnimValItem);

  static uint32_t MaxListIndex() {
    return (1U << MOZ_SVG_LIST_INDEX_BIT_COUNT) - 1;
  }

  
  void UpdateListIndex(uint32_t aListIndex) {
    mListIndex = aListIndex;
  }

  





  void RemovingFromList();

  SVGPoint ToSVGPoint() const {
    return HasOwner() ? const_cast<DOMSVGPoint*>(this)->InternalItem() : mPt;
  }

  bool IsReadonly() const {
    return mIsReadonly;
  }
  void SetReadonly(bool aReadonly) {
    mIsReadonly = aReadonly;
  }

protected:

  nsSVGElement* Element() {
    return mList->Element();
  }

  








  SVGPoint& InternalItem();

#ifdef DEBUG
  bool IndexIsValid();
#endif

  nsRefPtr<DOMSVGPointList> mList;

  
  

  uint32_t mListIndex:MOZ_SVG_LIST_INDEX_BIT_COUNT;
  uint32_t mIsReadonly:1;    
  uint32_t mIsAnimValItem:1; 

  
  SVGPoint mPt;
};

NS_DEFINE_STATIC_IID_ACCESSOR(DOMSVGPoint, MOZILLA_DOMSVGPOINT_IID)

} 

#undef MOZ_SVG_LIST_INDEX_BIT_COUNT

#endif 
