



































#ifndef NSSVGTEXTPATHFRAME_H
#define NSSVGTEXTPATHFRAME_H

#include "nsSVGTSpanFrame.h"
#include "nsIDOMSVGAnimatedString.h"
#include "nsSVGLengthList.h"

class nsSVGTextPathFrame;

typedef nsSVGTSpanFrame nsSVGTextPathFrameBase;

class nsSVGTextPathFrame : public nsSVGTextPathFrameBase
{
  friend nsIFrame*
  NS_NewSVGTextPathFrame(nsIPresShell* aPresShell, nsIContent* aContent,
                         nsIFrame* parentFrame, nsStyleContext* aContext);
protected:
  nsSVGTextPathFrame(nsStyleContext* aContext) : nsSVGTextPathFrameBase(aContext) {}

public:
  
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

  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetX();
  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetY();
  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetDx();
  NS_IMETHOD_(already_AddRefed<nsIDOMSVGLengthList>) GetDy();

private:
  already_AddRefed<gfxFlattenedPath> GetFlattenedPath(nsIFrame *path);
};

#endif
