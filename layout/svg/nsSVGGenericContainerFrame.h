




#ifndef __NS_SVGGENERICCONTAINERFRAME_H__
#define __NS_SVGGENERICCONTAINERFRAME_H__

#include "gfxMatrix.h"
#include "nsFrame.h"
#include "nsLiteralString.h"
#include "nsQueryFrame.h"
#include "nsSVGContainerFrame.h"

class nsIAtom;
class nsIFrame;
class nsIPresShell;
class nsStyleContext;

typedef nsSVGDisplayContainerFrame nsSVGGenericContainerFrameBase;

class nsSVGGenericContainerFrame : public nsSVGGenericContainerFrameBase
{
  friend nsIFrame*
  NS_NewSVGGenericContainerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGGenericContainerFrame(nsStyleContext* aContext) : nsSVGGenericContainerFrameBase(aContext) {}
  
public:
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHOD  AttributeChanged(int32_t         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               int32_t         aModType);
  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGGenericContainer"), aResult);
  }
#endif

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor);
};

#endif 
