




#ifndef __NS_SVGGENERICCONTAINERFRAME_H__
#define __NS_SVGGENERICCONTAINERFRAME_H__

#include "mozilla/Attributes.h"
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
  explicit nsSVGGenericContainerFrame(nsStyleContext* aContext) : nsSVGGenericContainerFrameBase(aContext) {}
  
public:
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual nsresult  AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType) MOZ_OVERRIDE;
  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGGenericContainer"), aResult);
  }
#endif

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor,
                                nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE;
};

#endif 
