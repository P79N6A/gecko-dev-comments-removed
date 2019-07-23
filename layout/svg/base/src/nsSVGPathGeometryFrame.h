





































#ifndef __NS_SVGPATHGEOMETRYFRAME_H__
#define __NS_SVGPATHGEOMETRYFRAME_H__

#include "nsFrame.h"
#include "nsISVGChildFrame.h"
#include "nsWeakReference.h"
#include "nsGkAtoms.h"
#include "nsSVGGeometryFrame.h"
#include "gfxRect.h"

class nsPresContext;
class nsIDOMSVGMatrix;
class nsSVGMarkerFrame;
class nsISVGFilterFrame;
class nsSVGMarkerProperty;

typedef nsSVGGeometryFrame nsSVGPathGeometryFrameBase;

#define HITTEST_MASK_FILL 1
#define HITTEST_MASK_STROKE 2

class nsSVGPathGeometryFrame : public nsSVGPathGeometryFrameBase,
                               public nsISVGChildFrame
{
public:
  nsSVGPathGeometryFrame(nsStyleContext* aContext);

   
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef() { return 1; }
  NS_IMETHOD_(nsrefcnt) Release() { return 1; }

  
  virtual void Destroy();
  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);

  NS_IMETHOD DidSetStyleContext();

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGPathGeometry"), aResult);
  }
#endif

  
  NS_IMETHOD GetCanvasTM(nsIDOMSVGMatrix * *aCTM);
  virtual nsresult UpdateGraphic(PRBool suppressInvalidation = PR_FALSE);

protected:
  
  NS_IMETHOD PaintSVG(nsSVGRenderState *aContext, nsRect *aDirtyRect);
  NS_IMETHOD GetFrameForPointSVG(float x, float y, nsIFrame** hit);
  NS_IMETHOD_(nsRect) GetCoveredRegion();
  NS_IMETHOD UpdateCoveredRegion();
  NS_IMETHOD InitialUpdate();
  NS_IMETHOD NotifyCanvasTMChanged(PRBool suppressInvalidation);
  NS_IMETHOD NotifyRedrawSuspended();
  NS_IMETHOD NotifyRedrawUnsuspended();
  NS_IMETHOD SetMatrixPropagation(PRBool aPropagate);
  NS_IMETHOD SetOverrideCTM(nsIDOMSVGMatrix *aCTM);
  NS_IMETHOD GetBBox(nsIDOMSVGRect **_retval);
  NS_IMETHOD_(PRBool) IsDisplayContainer() { return PR_FALSE; }
  NS_IMETHOD_(PRBool) HasValidCoveredRect() { return PR_TRUE; }

protected:
  virtual PRUint16 GetHittestMask();

private:
  void Render(nsSVGRenderState *aContext);
  void GeneratePath(gfxContext *aContext);

  




  static PRBool
  IsDegeneratePath(const gfxRect& rect)
  {
    return (rect.X() == 0 && rect.Y() == 0 &&
            rect.Width() == 0 && rect.Height() == 0);
  }

  nsSVGMarkerProperty *GetMarkerProperty();
  void UpdateMarkerProperty();

  void RemovePathProperties();

  nsCOMPtr<nsIDOMSVGMatrix> mOverrideCTM;
  PRPackedBool mPropagateTransform;
};

#endif 
