



































#ifndef __NS_SVGMARKERFRAME_H__
#define __NS_SVGMARKERFRAME_H__

#include "nsSVGContainerFrame.h"
#include "nsIDOMSVGAnimatedEnum.h"
#include "nsIDOMSVGAnimatedAngle.h"
#include "nsIDOMSVGRect.h"
#include "nsIDOMSVGAngle.h"

class gfxContext;
class nsSVGPathGeometryFrame;
class nsIURI;
class nsIContent;
struct nsSVGMark;

typedef nsSVGContainerFrame nsSVGMarkerFrameBase;

class nsSVGMarkerFrame : public nsSVGMarkerFrameBase
{
protected:
  friend nsIFrame*
  NS_NewSVGMarkerFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);

  NS_IMETHOD InitSVG();

public:
  nsSVGMarkerFrame(nsStyleContext* aContext) : nsSVGMarkerFrameBase(aContext) {}

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGMarker"), aResult);
  }
#endif

  
  nsresult PaintMark(nsSVGRenderState *aContext,
                     nsSVGPathGeometryFrame *aMarkedFrame,
                     nsSVGMark *aMark,
                     float aStrokeWidth);

  nsRect RegionMark(nsSVGPathGeometryFrame *aMarkedFrame,
                    const nsSVGMark *aMark, float aStrokeWidth);

private:
  
  nsSVGPathGeometryFrame *mMarkedFrame;
  float mStrokeWidth, mX, mY, mAngle;

  
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();

  
  
  class AutoMarkerReferencer;
  friend class AutoMarkerReferencer;

  
  
  
  
  
  class AutoMarkerReferencer
  {
  public:
    AutoMarkerReferencer(nsSVGMarkerFrame *aFrame,
                         nsSVGPathGeometryFrame *aMarkedFrame);
    ~AutoMarkerReferencer();
  private:
    nsSVGMarkerFrame *mFrame;
  };

  
  void SetParentCoordCtxProvider(nsSVGSVGElement *aContext);

  
  PRPackedBool mInUse;

  
  PRPackedBool mInUse2;
};

nsresult
NS_GetSVGMarkerFrame(nsSVGMarkerFrame **aResult,
                     nsIURI *aURI,
                     nsIContent *aContent);

#endif
