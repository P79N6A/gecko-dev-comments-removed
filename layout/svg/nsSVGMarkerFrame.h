




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

class nsRenderingContext;
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
  friend class nsSVGMarkerAnonChildFrame;
  friend nsContainerFrame*
  NS_NewSVGMarkerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  explicit nsSVGMarkerFrame(nsStyleContext* aContext)
    : nsSVGMarkerFrameBase(aContext)
    , mMarkedFrame(nullptr)
    , mInUse(false)
    , mInUse2(false)
  {
    AddStateBits(NS_FRAME_IS_NONDISPLAY);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

  
#ifdef DEBUG
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
#endif

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE {}

  virtual nsresult AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType) MOZ_OVERRIDE;
  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGMarker"), aResult);
  }
#endif

  virtual nsContainerFrame* GetContentInsertionFrame() MOZ_OVERRIDE {
    
    NS_ABORT_IF_FALSE(GetFirstPrincipalChild() &&
                      GetFirstPrincipalChild()->GetType() ==
                        nsGkAtoms::svgMarkerAnonChildFrame,
                      "Where is our anonymous child?");
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  }

  
  nsresult PaintMark(nsRenderingContext *aContext,
                     nsSVGPathGeometryFrame *aMarkedFrame,
                     nsSVGMark *aMark,
                     float aStrokeWidth);

  SVGBBox GetMarkBBoxContribution(const Matrix &aToBBoxUserspace,
                                  uint32_t aFlags,
                                  nsSVGPathGeometryFrame *aMarkedFrame,
                                  const nsSVGMark *aMark,
                                  float aStrokeWidth);

private:
  
  nsSVGPathGeometryFrame *mMarkedFrame;
  float mStrokeWidth, mX, mY, mAutoAngle;
  bool mIsStart;  

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor,
                                nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE;

  
  
  
  
  
  class MOZ_STACK_CLASS AutoMarkerReferencer
  {
  public:
    AutoMarkerReferencer(nsSVGMarkerFrame *aFrame,
                         nsSVGPathGeometryFrame *aMarkedFrame
                         MOZ_GUARD_OBJECT_NOTIFIER_PARAM);
    ~AutoMarkerReferencer();
  private:
    nsSVGMarkerFrame *mFrame;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
  };

  
  void SetParentCoordCtxProvider(mozilla::dom::SVGSVGElement *aContext);

  
  bool mInUse;

  
  bool mInUse2;
};




typedef nsSVGDisplayContainerFrame nsSVGMarkerAnonChildFrameBase;



class nsSVGMarkerAnonChildFrame
  : public nsSVGMarkerAnonChildFrameBase
{
  friend nsContainerFrame*
  NS_NewSVGMarkerAnonChildFrame(nsIPresShell* aPresShell,
                                nsStyleContext* aContext);

  explicit nsSVGMarkerAnonChildFrame(nsStyleContext* aContext)
    : nsSVGMarkerAnonChildFrameBase(aContext)
  {}

public:
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
#endif

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE {
    return MakeFrameName(NS_LITERAL_STRING("SVGMarkerAnonChild"), aResult);
  }
#endif

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor,
                                nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE
  {
    nsSVGMarkerFrame* marker = static_cast<nsSVGMarkerFrame*>(GetParent());
    return marker->GetCanvasTM(aFor, aTransformRoot);
  }
};
#endif
