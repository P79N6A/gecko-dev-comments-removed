





































#ifndef __NS_SVGGRADIENTFRAME_H__
#define __NS_SVGGRADIENTFRAME_H__

#include "nsSVGPaintServerFrame.h"
#include "nsISVGValueObserver.h"
#include "nsWeakReference.h"
#include "nsIDOMSVGAnimatedString.h"
#include "nsSVGElement.h"
#include "cairo.h"

class nsIDOMSVGStopElement;

typedef nsSVGPaintServerFrame  nsSVGGradientFrameBase;

class nsSVGGradientFrame : public nsSVGGradientFrameBase,
                           public nsISVGValueObserver
{
public:
  
  virtual nsresult SetupPaintServer(gfxContext *aContext,
                                    nsSVGGeometryFrame *aSource,
                                    float aOpacity,
                                    void **aClosure);
  virtual void CleanupPaintServer(gfxContext *aContext, void *aClosure);

  
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable, 
                                     nsISVGValue::modificationType aModType);
  NS_IMETHOD DidModifySVGObservable(nsISVGValue* observable, 
                                    nsISVGValue::modificationType aModType);

  
  NS_IMETHOD DidSetStyleContext();
  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  virtual nsIAtom* GetType() const;  

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

#ifdef DEBUG
  
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGGradient"), aResult);
  }
#endif 

  
  NS_IMETHOD PaintSVG(gfxContext* aContext)
  {
    return NS_OK;  
  }
  
private:

  
  
  
  
  
  

  
  void GetRefedGradientFromHref();

  
  
  nsIContent* GetGradientWithAttr(nsIAtom *aAttrName);

  
  
  nsIContent* GetGradientWithAttr(nsIAtom *aAttrName, nsIAtom *aGradType);

  
  PRInt32 GetStopFrame(PRInt32 aIndex, nsIFrame * *aStopFrame);

  PRUint16 GetSpreadMethod();
  PRUint32 GetStopCount();
  void GetStopInformation(PRInt32 aIndex,
                          float *aOffset, nscolor *aColor, float *aOpacity);
  nsresult GetGradientTransform(nsIDOMSVGMatrix **retval,
                                nsSVGGeometryFrame *aSource);

protected:

  virtual cairo_pattern_t *CreateGradient() = 0;

  
  nsIContent* GetLinearGradientWithAttr(nsIAtom *aAttrName)
  {
    return GetGradientWithAttr(aAttrName, nsGkAtoms::svgLinearGradientFrame);
  }
  nsIContent* GetRadialGradientWithAttr(nsIAtom *aAttrName)
  {
    return GetGradientWithAttr(aAttrName, nsGkAtoms::svgRadialGradientFrame);
  }

  
  
  void WillModify(modificationType aModType = mod_other)
  {
    mLoopFlag = PR_TRUE;
    nsSVGValue::WillModify(aModType);
    mLoopFlag = PR_FALSE;
  }
  void DidModify(modificationType aModType = mod_other)
  {
    mLoopFlag = PR_TRUE;
    nsSVGValue::DidModify(aModType);
    mLoopFlag = PR_FALSE;
  }

  
  PRUint16 GetGradientUnits();

  nsSVGGradientFrame(nsStyleContext* aContext,
                     nsIDOMSVGURIReference *aRef);

  virtual ~nsSVGGradientFrame();

  
  nsRefPtr<nsSVGElement>                 mSourceContent;

private:

  
  nsCOMPtr<nsIDOMSVGAnimatedString>      mHref;

  
  
  nsSVGGradientFrame                    *mNextGrad;

  
  
  
  PRPackedBool                           mLoopFlag;

  
  
  
  
  PRPackedBool                           mInitialized;
};






typedef nsSVGGradientFrame nsSVGLinearGradientFrameBase;

class nsSVGLinearGradientFrame : public nsSVGLinearGradientFrameBase
{
public:
  friend nsIFrame* NS_NewSVGLinearGradientFrame(nsIPresShell* aPresShell, 
                                                nsIContent*   aContent,
                                                nsStyleContext* aContext);

  
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
  nsSVGLinearGradientFrame(nsStyleContext* aContext,
                           nsIDOMSVGURIReference *aRef) :
    nsSVGLinearGradientFrameBase(aContext, aRef) {}

  float GradientLookupAttribute(nsIAtom *aAtomName, PRUint16 aEnumName);
  virtual cairo_pattern_t *CreateGradient();
};





typedef nsSVGGradientFrame nsSVGRadialGradientFrameBase;

class nsSVGRadialGradientFrame : public nsSVGRadialGradientFrameBase
{
public:
  friend nsIFrame* NS_NewSVGRadialGradientFrame(nsIPresShell* aPresShell, 
                                                nsIContent*   aContent,
                                                nsStyleContext* aContext);

  
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
  nsSVGRadialGradientFrame(nsStyleContext* aContext,
                           nsIDOMSVGURIReference *aRef) :
    nsSVGRadialGradientFrameBase(aContext, aRef) {}

  float GradientLookupAttribute(nsIAtom *aAtomName, PRUint16 aEnumName,
                                nsIContent *aElement = nsnull);
  virtual cairo_pattern_t *CreateGradient();
};

#endif 

