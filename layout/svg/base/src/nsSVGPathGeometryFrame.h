





































#ifndef __NS_SVGPATHGEOMETRYFRAME_H__
#define __NS_SVGPATHGEOMETRYFRAME_H__

#include "nsFrame.h"
#include "nsISVGChildFrame.h"
#include "nsWeakReference.h"
#include "nsGkAtoms.h"
#include "nsSVGGeometryFrame.h"
#include "gfxRect.h"
#include "gfxMatrix.h"

class nsSVGMarkerFrame;
class nsSVGMarkerProperty;

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
  
  NS_IMETHOD PaintSVG(nsSVGRenderState *aContext,
                      const nsIntRect *aDirtyRect);
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint);
  NS_IMETHOD_(nsRect) GetCoveredRegion();
  NS_IMETHOD UpdateCoveredRegion();
  NS_IMETHOD InitialUpdate();
  virtual void NotifySVGChanged(PRUint32 aFlags);
  NS_IMETHOD NotifyRedrawSuspended();
  NS_IMETHOD NotifyRedrawUnsuspended();
  virtual gfxRect GetBBoxContribution(const gfxMatrix &aToBBoxUserspace);
  NS_IMETHOD_(PRBool) IsDisplayContainer() { return PR_FALSE; }
  NS_IMETHOD_(PRBool) HasValidCoveredRect() { return PR_TRUE; }

protected:
  void GeneratePath(gfxContext *aContext,
                    const gfxMatrix *aOverrideTransform = nsnull);

private:
  void Render(nsSVGRenderState *aContext);

  struct MarkerProperties {
    nsSVGMarkerProperty* mMarkerStart;
    nsSVGMarkerProperty* mMarkerMid;
    nsSVGMarkerProperty* mMarkerEnd;

    PRBool MarkersExist() const {
      return mMarkerStart || mMarkerMid || mMarkerEnd;
    }

    nsSVGMarkerFrame *GetMarkerStartFrame();
    nsSVGMarkerFrame *GetMarkerMidFrame();
    nsSVGMarkerFrame *GetMarkerEndFrame();
  };

  


  static MarkerProperties GetMarkerProperties(nsSVGPathGeometryFrame *aFrame);
};

#endif 
