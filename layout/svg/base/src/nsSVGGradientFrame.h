





































#ifndef __NS_SVGGRADIENTFRAME_H__
#define __NS_SVGGRADIENTFRAME_H__

#include "nsSVGPaintServerFrame.h"
#include "nsWeakReference.h"
#include "nsSVGElement.h"
#include "gfxPattern.h"

class nsIDOMSVGStopElement;

typedef nsSVGPaintServerFrame nsSVGGradientFrameBase;





class nsSVGGradientFrame : public nsSVGGradientFrameBase
{
protected:
  nsSVGGradientFrame(nsStyleContext* aContext);

public:
  
  virtual PRBool SetupPaintServer(gfxContext *aContext,
                                  nsSVGGeometryFrame *aSource,
                                  float aGraphicOpacity);

  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGGradient"), aResult);
  }
#endif 

private:

  
  
  
  nsSVGGradientFrame* GetReferencedGradient();

  
  
  
  nsSVGGradientElement* GetGradientWithAttr(nsIAtom *aAttrName, nsIContent *aDefault);

  
  
  
  nsSVGGradientElement* GetGradientWithAttr(nsIAtom *aAttrName, nsIAtom *aGradType,
                                            nsIContent *aDefault);

  
  PRInt32 GetStopFrame(PRInt32 aIndex, nsIFrame * *aStopFrame);

  PRUint16 GetSpreadMethod();
  PRUint32 GetStopCount();
  void GetStopInformation(PRInt32 aIndex,
                          float *aOffset, nscolor *aColor, float *aStopOpacity);

  
  gfxMatrix GetGradientTransform(nsSVGGeometryFrame *aSource);

protected:
  virtual already_AddRefed<gfxPattern> CreateGradient() = 0;

  
  nsSVGLinearGradientElement* GetLinearGradientWithAttr(nsIAtom *aAttrName, nsIContent *aDefault)
  {
    return static_cast<nsSVGLinearGradientElement*>(
            GetGradientWithAttr(aAttrName, nsGkAtoms::svgLinearGradientFrame, aDefault));
  }
  nsSVGRadialGradientElement* GetRadialGradientWithAttr(nsIAtom *aAttrName, nsIContent *aDefault)
  {
    return static_cast<nsSVGRadialGradientElement*>(
            GetGradientWithAttr(aAttrName, nsGkAtoms::svgRadialGradientFrame, aDefault));
  }

  
  PRUint16 GetGradientUnits();

  
  nsRefPtr<nsSVGElement>                 mSourceContent;

private:
  
  
  
  PRPackedBool                           mLoopFlag;
  
  
  PRPackedBool                           mNoHRefURI;
};






typedef nsSVGGradientFrame nsSVGLinearGradientFrameBase;

class nsSVGLinearGradientFrame : public nsSVGLinearGradientFrameBase
{
  friend nsIFrame* NS_NewSVGLinearGradientFrame(nsIPresShell* aPresShell,
                                                nsStyleContext* aContext);
protected:
  nsSVGLinearGradientFrame(nsStyleContext* aContext) :
    nsSVGLinearGradientFrameBase(aContext) {}

public:
  
#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  virtual nsIAtom* GetType() const;  

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGLinearGradient"), aResult);
  }
#endif 

protected:
  float GradientLookupAttribute(nsIAtom *aAtomName, PRUint16 aEnumName);
  virtual already_AddRefed<gfxPattern> CreateGradient();
};





typedef nsSVGGradientFrame nsSVGRadialGradientFrameBase;

class nsSVGRadialGradientFrame : public nsSVGRadialGradientFrameBase
{
  friend nsIFrame* NS_NewSVGRadialGradientFrame(nsIPresShell* aPresShell,
                                                nsStyleContext* aContext);
protected:
  nsSVGRadialGradientFrame(nsStyleContext* aContext) :
    nsSVGRadialGradientFrameBase(aContext) {}

public:
  
#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  virtual nsIAtom* GetType() const;  

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGRadialGradient"), aResult);
  }
#endif 

protected:
  float GradientLookupAttribute(nsIAtom *aAtomName, PRUint16 aEnumName,
                                nsSVGRadialGradientElement *aElement = nsnull);
  virtual already_AddRefed<gfxPattern> CreateGradient();
};

#endif 

