



































#ifndef NSSVGTEXTPATHFRAME_H
#define NSSVGTEXTPATHFRAME_H

#include "nsSVGTSpanFrame.h"
#include "SVGLengthList.h"
#include "nsSVGNumberList.h"

typedef nsSVGTSpanFrame nsSVGTextPathFrameBase;

class nsSVGTextPathFrame : public nsSVGTextPathFrameBase
{
  friend nsIFrame*
  NS_NewSVGTextPathFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGTextPathFrame(nsStyleContext* aContext) : nsSVGTextPathFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  
#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);
  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGTextPath"), aResult);
  }
#endif

  
  already_AddRefed<gfxFlattenedPath> GetFlattenedPath();
  nsIFrame *GetPathFrame();

  gfxFloat GetStartOffset();
  gfxFloat GetPathScale();
protected:

  virtual void GetXY(mozilla::SVGUserUnitList *aX, mozilla::SVGUserUnitList *aY);
  virtual void GetDxDy(mozilla::SVGUserUnitList *aDx, mozilla::SVGUserUnitList *aDy);
  virtual already_AddRefed<nsIDOMSVGNumberList> GetRotate();

private:
  already_AddRefed<gfxFlattenedPath> GetFlattenedPath(nsIFrame *path);
};

#endif
