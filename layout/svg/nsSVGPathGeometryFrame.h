




#ifndef __NS_SVGPATHGEOMETRYFRAME_H__
#define __NS_SVGPATHGEOMETRYFRAME_H__

#include "mozilla/Attributes.h"
#include "gfxMatrix.h"
#include "gfxRect.h"
#include "nsFrame.h"
#include "nsISVGChildFrame.h"
#include "nsLiteralString.h"
#include "nsQueryFrame.h"
#include "nsSVGUtils.h"

namespace mozilla {
namespace gfx {
class DrawTarget;
}
}

class gfxContext;
class nsDisplaySVGPathGeometry;
class nsIAtom;
class nsIFrame;
class nsIPresShell;
class nsStyleContext;
class nsSVGMarkerFrame;
class nsSVGMarkerProperty;

struct nsPoint;
struct nsRect;

typedef nsFrame nsSVGPathGeometryFrameBase;

class nsSVGPathGeometryFrame : public nsSVGPathGeometryFrameBase,
                               public nsISVGChildFrame
{
  typedef mozilla::gfx::DrawTarget DrawTarget;

  friend nsIFrame*
  NS_NewSVGPathGeometryFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  friend class nsDisplaySVGPathGeometry;

protected:
  explicit nsSVGPathGeometryFrame(nsStyleContext* aContext)
    : nsSVGPathGeometryFrameBase(aContext)
  {
     AddStateBits(NS_FRAME_SVG_LAYOUT | NS_FRAME_MAY_BE_TRANSFORMED);
  }

public:
  NS_DECL_QUERYFRAME_TARGET(nsSVGPathGeometryFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsSVGPathGeometryFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG | nsIFrame::eSVGGeometry));
  }

  virtual nsresult  AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType) override;

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) override;

  




  virtual nsIAtom* GetType() const override;

  virtual bool IsSVGTransformed(Matrix *aOwnTransforms = nullptr,
                                Matrix *aFromParentTransforms = nullptr) const override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGPathGeometry"), aResult);
  }
#endif

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  
  gfxMatrix GetCanvasTM();
protected:
  
  virtual nsresult PaintSVG(gfxContext& aContext,
                            const gfxMatrix& aTransform,
                            const nsIntRect* aDirtyRect = nullptr) override;
  virtual nsIFrame* GetFrameForPoint(const gfxPoint& aPoint) override;
  virtual nsRect GetCoveredRegion() override;
  virtual void ReflowSVG() override;
  virtual void NotifySVGChanged(uint32_t aFlags) override;
  virtual SVGBBox GetBBoxContribution(const Matrix &aToBBoxUserspace,
                                      uint32_t aFlags) override;
  virtual bool IsDisplayContainer() override { return false; }

  





  virtual uint16_t GetHitTestFlags();
private:
  enum { eRenderFill = 1, eRenderStroke = 2 };
  void Render(gfxContext* aContext, uint32_t aRenderComponents,
              const gfxMatrix& aTransform);

  



  void PaintMarkers(gfxContext& aContext, const gfxMatrix& aMatrix);

  struct MarkerProperties {
    nsSVGMarkerProperty* mMarkerStart;
    nsSVGMarkerProperty* mMarkerMid;
    nsSVGMarkerProperty* mMarkerEnd;

    bool MarkersExist() const {
      return mMarkerStart || mMarkerMid || mMarkerEnd;
    }

    nsSVGMarkerFrame *GetMarkerStartFrame();
    nsSVGMarkerFrame *GetMarkerMidFrame();
    nsSVGMarkerFrame *GetMarkerEndFrame();
  };

  


  static MarkerProperties GetMarkerProperties(nsSVGPathGeometryFrame *aFrame);
};

#endif 
