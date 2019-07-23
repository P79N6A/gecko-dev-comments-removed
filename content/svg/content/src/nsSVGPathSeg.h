





































#ifndef __NS_SVGPATHSEG_H__
#define __NS_SVGPATHSEG_H__

#include "nsIDOMSVGPathSeg.h"
#include "nsIWeakReference.h"
#include "nsSVGUtils.h"
#include "nsSVGPathDataParser.h"


#define NS_SVGPATHSEG_IID \
  { 0x0dfd1b3c, 0x5638, 0x4813, \
    { 0xa6, 0xf8, 0x5a, 0x32, 0x5a, 0x35, 0xd0, 0x6e } }

#define NS_ENSURE_NATIVE_PATH_SEG(obj, retval)                 \
  {                                                            \
    nsresult rv;                                               \
    if (retval)                                                \
      *retval = nsnull;                                        \
    nsCOMPtr<nsSVGPathSeg> path = do_QueryInterface(obj, &rv); \
    NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_SVG_WRONG_TYPE_ERR);    \
  }

class nsSVGPathSegList;
class nsISVGValue;

struct nsSVGPathSegTraversalState {
  float curPosX, startPosX, quadCPX, cubicCPX;
  float curPosY, startPosY, quadCPY, cubicCPY;
  nsSVGPathSegTraversalState()
  {
    curPosX = startPosX = quadCPX = cubicCPX = 0;
    curPosY = startPosY = quadCPY = cubicCPY = 0;
  }
};

class nsSVGPathSeg : public nsIDOMSVGPathSeg
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_SVGPATHSEG_IID)

  nsSVGPathSeg() : mCurrentList(nsnull) {}
  nsresult SetCurrentList(nsISVGValue* aList);
  nsQueryReferent GetCurrentList() const;

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPATHSEG
  NS_IMETHOD GetValueString(nsAString& aValue) = 0;

  
  virtual float GetLength(nsSVGPathSegTraversalState *ts) = 0;

protected:
  virtual PRUint16 GetSegType() = 0;
  void DidModify();

private:
  static char mTypeLetters[];
  nsCOMPtr<nsIWeakReference> mCurrentList;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsSVGPathSeg, NS_SVGPATHSEG_IID)

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
                       PRBool largeArcFlag, PRBool sweepFlag);

nsIDOMSVGPathSeg*
NS_NewSVGPathSegArcRel(float x, float y,
                       float r1, float r2, float angle,
                       PRBool largeArcFlag, PRBool sweepFlag);

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


#endif 
