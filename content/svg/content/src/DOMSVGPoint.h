



































#ifndef MOZILLA_DOMSVGPOINT_H__
#define MOZILLA_DOMSVGPOINT_H__

#include "nsIDOMSVGPoint.h"
#include "DOMSVGPointList.h"
#include "SVGPoint.h"
#include "gfxPoint.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"

class nsSVGElement;






#define MOZILLA_DOMSVGPOINT_IID \
  { 0xd6b6c440, 0xaf8d, 0x40ee, \
    { 0x85, 0x6b, 0x02, 0xa3, 0x17, 0xca, 0xb2, 0x75 } }

namespace mozilla {















class DOMSVGPoint : public nsIDOMSVGPoint
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOMSVGPOINT_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGPoint)
  NS_DECL_NSIDOMSVGPOINT

  


  DOMSVGPoint(DOMSVGPointList *aList,
              PRUint32 aListIndex,
              PRBool aIsAnimValItem)
    : mList(aList)
    , mListIndex(aListIndex)
    , mIsReadonly(PR_FALSE)
    , mIsAnimValItem(aIsAnimValItem)
  {
    
    NS_ABORT_IF_FALSE(aList &&
                      aListIndex < (1U << 30), "bad arg");

    NS_ABORT_IF_FALSE(IndexIsValid(), "Bad index for DOMSVGPoint!");
  }

  DOMSVGPoint(const DOMSVGPoint *aPt = nsnull)
    : mList(nsnull)
    , mListIndex(0)
    , mIsReadonly(PR_FALSE)
    , mIsAnimValItem(PR_FALSE)
  {
    if (aPt) {
      mPt = aPt->ToSVGPoint();
    }
  }

  DOMSVGPoint(float aX, float aY)
    : mList(nsnull)
    , mListIndex(0)
    , mIsReadonly(PR_FALSE)
    , mIsAnimValItem(PR_FALSE)
  {
    mPt.mX = aX;
    mPt.mY = aY;
  }

  DOMSVGPoint(const gfxPoint &aPt)
    : mList(nsnull)
    , mListIndex(0)
    , mIsReadonly(PR_FALSE)
    , mIsAnimValItem(PR_FALSE)
  {
    mPt.mX = float(aPt.x);
    mPt.mY = float(aPt.y);
    NS_ASSERTION(NS_FloatIsFinite(mPt.mX) && NS_FloatIsFinite(mPt.mX),
                 "DOMSVGPoint coords are not finite");
  }


  ~DOMSVGPoint() {
    
    
    
    if (mList) {
      mList->mItems[mListIndex] = nsnull;
    }
  }

  



  DOMSVGPoint* Clone() {
    return new DOMSVGPoint(this);
  }

  PRBool IsInList() const {
    return !!mList;
  }

  





  PRBool HasOwner() const {
    return !!mList;
  }

  








  void InsertingIntoList(DOMSVGPointList *aList,
                         PRUint32 aListIndex,
                         PRBool aIsAnimValItem);

  
  void UpdateListIndex(PRUint8 aListIndex) {
    mListIndex = aListIndex;
  }

  





  void RemovingFromList();

  SVGPoint ToSVGPoint() const {
    return HasOwner() ? const_cast<DOMSVGPoint*>(this)->InternalItem() : mPt;
  }

  PRBool IsReadonly() const {
    return mIsReadonly;
  }
  void SetReadonly(PRBool aReadonly) {
    mIsReadonly = aReadonly;
  }

protected:

  nsSVGElement* Element() {
    return mList->Element();
  }

  








  SVGPoint& InternalItem();

#ifdef DEBUG
  PRBool IndexIsValid();
#endif

  nsRefPtr<DOMSVGPointList> mList;

  
  

  PRUint32 mListIndex:30;
  PRUint32 mIsReadonly:1;    
  PRUint32 mIsAnimValItem:1; 

  
  SVGPoint mPt;
};

NS_DEFINE_STATIC_IID_ACCESSOR(DOMSVGPoint, MOZILLA_DOMSVGPOINT_IID)

} 

#endif 
