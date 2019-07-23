





































#ifndef __NS_SVGGENERICCONTAINERFRAME_H__
#define __NS_SVGGENERICCONTAINERFRAME_H__

#include "nsIDOMSVGGElement.h"
#include "nsPresContext.h"
#include "nsSVGContainerFrame.h"
#include "nsGkAtoms.h"
#include "gfxMatrix.h"

typedef nsSVGDisplayContainerFrame nsSVGGenericContainerFrameBase;

class nsSVGGenericContainerFrame : public nsSVGGenericContainerFrameBase
{
  friend nsIFrame*
  NS_NewSVGGenericContainerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGGenericContainerFrame(nsStyleContext* aContext) : nsSVGGenericContainerFrameBase(aContext) {}
  
public:
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);
  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGGenericContainer"), aResult);
  }
#endif

  
  virtual gfxMatrix GetCanvasTM();
};

#endif 
