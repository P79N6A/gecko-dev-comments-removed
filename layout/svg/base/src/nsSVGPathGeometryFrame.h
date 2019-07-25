





































#ifndef __NS_SVGPATHGEOMETRYFRAME_H__
#define __NS_SVGPATHGEOMETRYFRAME_H__

#include "gfxMatrix.h"
#include "gfxRect.h"
#include "nsFrame.h"
#include "nsISVGChildFrame.h"
#include "nsLiteralString.h"
#include "nsQueryFrame.h"
#include "nsRect.h"
#include "nsSVGGeometryFrame.h"
#include "nsSVGUtils.h"

class gfxContext;
class nsIAtom;
class nsIFrame;
class nsIPresShell;
class nsRenderingContext;
class nsStyleContext;
class nsSVGMarkerFrame;
class nsSVGMarkerProperty;

struct nsPoint;

typedef nsSVGGeometryFrame nsSVGPathGeometryFrameBase;

class nsSVGPathGeometryFrame : public nsSVGPathGeometryFrameBase,
                               public nsISVGChildFrame
{
  friend nsIFrame*
  NS_NewSVGPathGeometryFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGPathGeometryFrame(nsStyleContext* aContext) :
    nsSVGPathGeometryFrameBase(aContext) {}

public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGPathGeometry"), aResult);
  }
#endif

  
  gfxMatrix GetCanvasTM();

protected:
  
  NS_IMETHOD PaintSVG(nsRenderingContext *aContext,
                      const nsIntRect *aDirtyRect);
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint);
  NS_IMETHOD_(nsRect) GetCoveredRegion();
  virtual void UpdateBounds();
  virtual void NotifySVGChanged(PRUint32 aFlags);
  virtual gfxRect GetBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                      PRUint32 aFlags);
  NS_IMETHOD_(bool) IsDisplayContainer() { return false; }
  NS_IMETHOD_(bool) HasValidCoveredRect() {
    return !(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD);
  }

protected:
  void GeneratePath(gfxContext *aContext,
                    const gfxMatrix *aOverrideTransform = nsnull);

private:
  void Render(nsRenderingContext *aContext);

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
