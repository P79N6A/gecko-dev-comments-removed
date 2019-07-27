




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

class gfxContext;
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
                    nsIFrame*         aPrevInFlow) override;
#endif

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override {}

  virtual nsresult AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType) override;
  




  virtual nsIAtom* GetType() const override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGMarker"), aResult);
  }
#endif

  virtual nsContainerFrame* GetContentInsertionFrame() override {
    
    MOZ_ASSERT(GetFirstPrincipalChild() &&
               GetFirstPrincipalChild()->GetType() ==
                 nsGkAtoms::svgMarkerAnonChildFrame,
               "Where is our anonymous child?");
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  }

  
  nsresult PaintMark(gfxContext& aContext,
                     const gfxMatrix& aToMarkedFrameUserSpace,
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

  
  virtual gfxMatrix GetCanvasTM() override;

  
  
  
  
  
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
                    nsIFrame*         aPrevInFlow) override;
#endif

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override {
    return MakeFrameName(NS_LITERAL_STRING("SVGMarkerAnonChild"), aResult);
  }
#endif

  




  virtual nsIAtom* GetType() const override;

  
  virtual gfxMatrix GetCanvasTM() override
  {
    return static_cast<nsSVGMarkerFrame*>(GetParent())->GetCanvasTM();
  }
};
#endif
