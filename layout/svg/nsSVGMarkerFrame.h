




#ifndef __NS_SVGMARKERFRAME_H__
#define __NS_SVGMARKERFRAME_H__

#include "mozilla/Attributes.h"
#include "gfxMatrix.h"
#include "gfxRect.h"
#include "nsFrame.h"
#include "nsLiteralString.h"
#include "nsQueryFrame.h"
#include "nsSVGContainerFrame.h"
#include "nsSVGUtils.h"

class nsIAtom;
class nsIContent;
class nsIFrame;
class nsIPresShell;
class nsRenderingContext;
class nsStyleContext;
class nsSVGPathGeometryFrame;

namespace mozilla {
namespace dom {
class SVGSVGElement;
}
}

struct nsSVGMark;

typedef nsSVGContainerFrame nsSVGMarkerFrameBase;

class nsSVGMarkerFrame : public nsSVGMarkerFrameBase
{
  friend nsIFrame*
  NS_NewSVGMarkerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGMarkerFrame(nsStyleContext* aContext)
    : nsSVGMarkerFrameBase(aContext)
    , mMarkedFrame(nullptr)
    , mInUse(false)
    , mInUse2(false)
  {
    AddStateBits(NS_STATE_SVG_NONDISPLAY_CHILD);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

  
#ifdef DEBUG
  virtual void Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        aPrevInFlow) MOZ_OVERRIDE;
#endif

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE {}

  NS_IMETHOD AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType) MOZ_OVERRIDE;
  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGMarker"), aResult);
  }
#endif

  
  nsresult PaintMark(nsRenderingContext *aContext,
                     nsSVGPathGeometryFrame *aMarkedFrame,
                     nsSVGMark *aMark,
                     float aStrokeWidth);

  SVGBBox GetMarkBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                  uint32_t aFlags,
                                  nsSVGPathGeometryFrame *aMarkedFrame,
                                  const nsSVGMark *aMark,
                                  float aStrokeWidth);

private:
  
  nsSVGPathGeometryFrame *mMarkedFrame;
  float mStrokeWidth, mX, mY, mAutoAngle;

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor) MOZ_OVERRIDE;

  
  
  
  
  
  class AutoMarkerReferencer
  {
  public:
    AutoMarkerReferencer(nsSVGMarkerFrame *aFrame,
                         nsSVGPathGeometryFrame *aMarkedFrame);
    ~AutoMarkerReferencer();
  private:
    nsSVGMarkerFrame *mFrame;
  };

  
  void SetParentCoordCtxProvider(mozilla::dom::SVGSVGElement *aContext);

  
  bool mInUse;

  
  bool mInUse2;
};

#endif
