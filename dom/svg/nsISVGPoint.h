




#pragma once

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/SVGPointBinding.h"
#include "DOMSVGPointList.h"


#define MOZILLA_NSISVGPOINT_IID \
  { 0xd6b6c440, 0xaf8d, 0x40ee, \
    { 0x85, 0x6b, 0x02, 0xa3, 0x17, 0xca, 0xb2, 0x75 } }

#define MOZ_SVG_LIST_INDEX_BIT_COUNT 29

namespace mozilla {

namespace dom {
class SVGMatrix;
}







class nsISVGPoint : public nsISupports,
                    public nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_NSISVGPOINT_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsISVGPoint)

  


  explicit nsISVGPoint()
    : mList(nullptr)
    , mListIndex(0)
    , mIsReadonly(false)
    , mIsAnimValItem(false)
    , mIsTranslatePoint(false)
  {
  }

  explicit nsISVGPoint(SVGPoint* aPt, bool aIsTranslatePoint)
    : mList(nullptr)
    , mListIndex(0)
    , mIsReadonly(false)
    , mIsAnimValItem(false)
    , mIsTranslatePoint(aIsTranslatePoint)
  {
    mPt.mX = aPt->GetX();
    mPt.mY = aPt->GetY();
  }

protected:
  virtual ~nsISVGPoint()
  {
    
    
    
    if (mList) {
      mList->mItems[mListIndex] = nullptr;
    }
  }

public:
  


  virtual DOMSVGPoint* Copy() = 0;

  SVGPoint ToSVGPoint() const {
    return HasOwner() ? const_cast<nsISVGPoint*>(this)->InternalItem() : mPt;
  }

  bool IsInList() const {
    return !!mList;
  }

  





  bool HasOwner() const {
    return !!mList;
  }

  bool IsTranslatePoint() const {
    return mIsTranslatePoint;
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

  bool IsReadonly() const {
    return mIsReadonly;
  }
  void SetReadonly(bool aReadonly) {
    mIsReadonly = aReadonly;
  }

  
  virtual float X() = 0;
  virtual void SetX(float aX, ErrorResult& rv) = 0;
  virtual float Y() = 0;
  virtual void SetY(float aY, ErrorResult& rv) = 0;
  virtual already_AddRefed<nsISVGPoint> MatrixTransform(dom::SVGMatrix& matrix) = 0;
  virtual JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto) override
    { return dom::SVGPointBinding::Wrap(cx, this, aGivenProto); }

  virtual nsISupports* GetParentObject() = 0;

protected:
#ifdef DEBUG
  bool IndexIsValid();
#endif

  nsRefPtr<DOMSVGPointList> mList;

  
  

  uint32_t mListIndex:MOZ_SVG_LIST_INDEX_BIT_COUNT;
  uint32_t mIsReadonly:1;       
  uint32_t mIsAnimValItem:1;    
  uint32_t mIsTranslatePoint:1;

  








  SVGPoint& InternalItem();

  
  SVGPoint mPt;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGPoint, MOZILLA_NSISVGPOINT_IID)

} 

#undef MOZ_SVG_LIST_INDEX_BIT_COUNT


