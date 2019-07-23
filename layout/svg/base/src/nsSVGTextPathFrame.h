



































#ifndef NSSVGTEXTPATHFRAME_H
#define NSSVGTEXTPATHFRAME_H

#include "nsSVGTSpanFrame.h"
#include "nsSVGLengthList.h"
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

  virtual already_AddRefed<nsIDOMSVGLengthList> GetX();
  virtual already_AddRefed<nsIDOMSVGLengthList> GetY();
  virtual already_AddRefed<nsIDOMSVGLengthList> GetDx();
  virtual already_AddRefed<nsIDOMSVGLengthList> GetDy();
  virtual already_AddRefed<nsIDOMSVGNumberList> GetRotate();

private:
  already_AddRefed<gfxFlattenedPath> GetFlattenedPath(nsIFrame *path);
};

#endif
