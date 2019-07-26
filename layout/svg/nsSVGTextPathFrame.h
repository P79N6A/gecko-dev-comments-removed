




#ifndef NSSVGTEXTPATHFRAME_H
#define NSSVGTEXTPATHFRAME_H

#include "mozilla/Attributes.h"
#include "gfxTypes.h"
#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsISVGChildFrame.h"
#include "nsLiteralString.h"
#include "nsQueryFrame.h"
#include "nsSVGTSpanFrame.h"

class gfxPath;
class nsIAtom;
class nsIContent;
class nsIFrame;
class nsIPresShell;
class nsStyleContext;

namespace mozilla {
class SVGNumberList;
}

typedef nsSVGTSpanFrame nsSVGTextPathFrameBase;

class nsSVGTextPathFrame : public nsSVGTextPathFrameBase
{
  typedef mozilla::SVGNumberList SVGNumberList;

  friend nsIFrame*
  NS_NewSVGTextPathFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGTextPathFrame(nsStyleContext* aContext) : nsSVGTextPathFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  
#ifdef DEBUG
  virtual void Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        aPrevInFlow) MOZ_OVERRIDE;
#endif

  NS_IMETHOD  AttributeChanged(int32_t         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               int32_t         aModType) MOZ_OVERRIDE;
  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGTextPath"), aResult);
  }
#endif

  
  already_AddRefed<gfxPath> GetPath();
  nsIFrame *GetPathFrame();

  




  gfxFloat GetOffsetScale();

  




  gfxFloat GetStartOffset();

protected:

  virtual void GetXY(SVGUserUnitList *aX, SVGUserUnitList *aY) MOZ_OVERRIDE;
  virtual void GetDxDy(SVGUserUnitList *aDx, SVGUserUnitList *aDy) MOZ_OVERRIDE;
  virtual const SVGNumberList *GetRotate() MOZ_OVERRIDE;
};

#endif
