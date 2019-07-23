



































#ifndef __NS_SVGMARKERFRAME_H__
#define __NS_SVGMARKERFRAME_H__

#include "nsSVGContainerFrame.h"
#include "gfxMatrix.h"

class gfxContext;
class nsSVGPathGeometryFrame;
class nsIURI;
class nsIContent;
struct nsSVGMark;

typedef nsSVGContainerFrame nsSVGMarkerFrameBase;

class nsSVGMarkerFrame : public nsSVGMarkerFrameBase
{
  friend nsIFrame*
  NS_NewSVGMarkerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGMarkerFrame(nsStyleContext* aContext) :
    nsSVGMarkerFrameBase(aContext),
    mMarkedFrame(nsnull),
    mInUse(PR_FALSE),
    mInUse2(PR_FALSE) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  
#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);
  




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

  
  virtual gfxMatrix GetCanvasTM();

  
  
  
  
  
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

#endif
