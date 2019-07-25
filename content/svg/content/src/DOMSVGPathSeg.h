



































#ifndef MOZILLA_DOMSVGPATHSEG_H__
#define MOZILLA_DOMSVGPATHSEG_H__

#include "nsIDOMSVGPathSeg.h"
#include "DOMSVGPathSegList.h"
#include "SVGPathSegUtils.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"

class nsSVGElement;






#define MOZILLA_DOMSVGPATHSEG_IID \
  { 0x494A7566, 0xDC26, 0x40C8, { 0x91, 0x22, 0x52, 0xAB, 0xD7, 0x68, 0x70, 0xC4 } }

#define MOZ_SVG_LIST_INDEX_BIT_COUNT 31

namespace mozilla {



















class DOMSVGPathSeg : public nsIDOMSVGPathSeg
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOMSVGPATHSEG_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGPathSeg)
  NS_DECL_NSIDOMSVGPATHSEG

  




  static DOMSVGPathSeg *CreateFor(DOMSVGPathSegList *aList,
                                  PRUint32 aListIndex,
                                  bool aIsAnimValItem);

  



  virtual DOMSVGPathSeg* Clone() = 0;

  bool IsInList() const {
    return !!mList;
  }

  



  bool HasOwner() const {
    return !!mList;
  }

  








  void InsertingIntoList(DOMSVGPathSegList *aList,
                         PRUint32 aListIndex,
                         bool aIsAnimValItem);

  static PRUint32 MaxListIndex() {
    return (1U << MOZ_SVG_LIST_INDEX_BIT_COUNT) - 1;
  }

  
  void UpdateListIndex(PRUint32 aListIndex) {
    mListIndex = aListIndex;
  }

  





  void RemovingFromList();

  





  void ToSVGPathSegEncodedData(float *aData);

  


  virtual PRUint32 Type() const = 0;

protected:

  


  DOMSVGPathSeg(DOMSVGPathSegList *aList,
                PRUint32 aListIndex,
                bool aIsAnimValItem);

  




  DOMSVGPathSeg();

  virtual ~DOMSVGPathSeg() {
    
    
    
    if (mList) {
      mList->ItemAt(mListIndex) = nsnull;
    }
  }

  nsSVGElement* Element() {
    return mList->Element();
  }

  








  float* InternalItem();

  virtual float* PtrToMemberArgs() = 0;

#ifdef DEBUG
  bool IndexIsValid();
#endif

  nsRefPtr<DOMSVGPathSegList> mList;

  
  

  PRUint32 mListIndex:MOZ_SVG_LIST_INDEX_BIT_COUNT;
  PRUint32 mIsAnimValItem:1; 
};

NS_DEFINE_STATIC_IID_ACCESSOR(DOMSVGPathSeg, MOZILLA_DOMSVGPATHSEG_IID)

} 

nsIDOMSVGPathSeg*
NS_NewSVGPathSegClosePath();

nsIDOMSVGPathSeg*
NS_NewSVGPathSegMovetoAbs(float x, float y);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegMovetoRel(float x, float y);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegLinetoAbs(float x, float y);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegLinetoRel(float x, float y);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoCubicAbs(float x, float y,
                                float x1, float y1,
                                float x2, float y2);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoCubicRel(float x, float y,
                                float x1, float y1,
                                float x2, float y2);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoQuadraticAbs(float x, float y,
                                    float x1, float y1);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoQuadraticRel(float x, float y,
                                    float x1, float y1);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegArcAbs(float x, float y,
                       float r1, float r2, float angle,
                       bool largeArcFlag, bool sweepFlag);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegArcRel(float x, float y,
                       float r1, float r2, float angle,
                       bool largeArcFlag, bool sweepFlag);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegLinetoHorizontalAbs(float x);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegLinetoHorizontalRel(float x);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegLinetoVerticalAbs(float y);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegLinetoVerticalRel(float y);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoCubicSmoothAbs(float x, float y,
                                      float x2, float y2);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoCubicSmoothRel(float x, float y,
                                      float x2, float y2);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoQuadraticSmoothAbs(float x, float y);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegCurvetoQuadraticSmoothRel(float x, float y);

#undef MOZ_SVG_LIST_INDEX_BIT_COUNT

#endif 
