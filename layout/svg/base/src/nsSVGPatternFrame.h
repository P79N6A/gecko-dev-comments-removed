





































#ifndef __NS_SVGPATTERNFRAME_H__
#define __NS_SVGPATTERNFRAME_H__

#include "nsISVGValueObserver.h"
#include "nsWeakReference.h"
#include "nsIDOMSVGAnimatedString.h"
#include "nsIDOMSVGMatrix.h"
#include "nsSVGPaintServerFrame.h"

class nsIDOMSVGAnimatedPreserveAspectRatio;
class nsIFrame;
class nsSVGLength2;
class nsSVGElement;
class gfxContext;

typedef nsSVGPaintServerFrame  nsSVGPatternFrameBase;

class nsSVGPatternFrame : public nsSVGPatternFrameBase,
                          public nsISVGValueObserver
{
public:
  friend nsIFrame* NS_NewSVGPatternFrame(nsIPresShell* aPresShell, 
                                         nsIContent*   aContent,
                                         nsStyleContext* aContext);

  nsSVGPatternFrame(nsStyleContext* aContext) : nsSVGPatternFrameBase(aContext) {}

  nsresult PaintPattern(cairo_surface_t **surface,
                        nsIDOMSVGMatrix **patternMatrix,
                        nsSVGGeometryFrame *aSource,
                        float aGraphicOpacity);

  
  virtual PRBool SetupPaintServer(gfxContext *aContext,
                                  nsSVGGeometryFrame *aSource,
                                  float aGraphicOpacity,
                                  void **aClosure);
  virtual void CleanupPaintServer(gfxContext *aContext, void *aClosure);

  
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return 1; }
  NS_IMETHOD_(nsrefcnt) Release() { return 1; }

public:
  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable, 
                                     nsISVGValue::modificationType aModType);
  NS_IMETHOD DidModifySVGObservable(nsISVGValue* observable, 
                                    nsISVGValue::modificationType aModType);
  
  
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();

  
  NS_IMETHOD DidSetStyleContext();

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGPattern"), aResult);
  }
#endif 

protected:
  nsSVGPatternFrame(nsStyleContext* aContext,
                    nsIDOMSVGURIReference *aRef);

  virtual ~nsSVGPatternFrame();

  
  PRBool checkURITarget(nsIAtom *);
  PRBool checkURITarget();
  
  nsSVGLength2 *GetX();
  nsSVGLength2 *GetY();
  nsSVGLength2 *GetWidth();
  nsSVGLength2 *GetHeight();

  PRUint16 GetPatternUnits();
  PRUint16 GetPatternContentUnits();
  nsresult GetPatternTransform(nsIDOMSVGMatrix **retval);

  NS_IMETHOD GetPreserveAspectRatio(nsIDOMSVGAnimatedPreserveAspectRatio 
                                                     **aPreserveAspectRatio);
  NS_IMETHOD GetPatternFirstChild(nsIFrame **kid);
  NS_IMETHOD GetViewBox(nsIDOMSVGRect * *aMatrix);
  nsresult   GetPatternRect(nsIDOMSVGRect **patternRect, nsIDOMSVGRect *bbox, 
                            nsSVGElement *content);
  nsresult   GetPatternMatrix(nsIDOMSVGMatrix **aCTM, 
                              nsIDOMSVGRect *bbox,
                              nsIDOMSVGRect *callerBBox,
                              nsIDOMSVGMatrix *callerCTM);
  nsresult   ConstructCTM(nsIDOMSVGMatrix **ctm, nsIDOMSVGRect *callerBBox);
  nsresult   GetCallerGeometry(nsIDOMSVGMatrix **aCTM, 
                               nsIDOMSVGRect **aBBox,
                               nsSVGElement **aContent, 
                               nsSVGGeometryFrame *aSource);

private:
  
  
  
  nsSVGGeometryFrame                     *mSource;
  nsCOMPtr<nsIDOMSVGMatrix>               mCTM;

protected:
  nsSVGPatternFrame                      *mNextPattern;
  nsCOMPtr<nsIDOMSVGAnimatedString> 	  mHref;
  PRPackedBool                            mLoopFlag;
};

#endif

