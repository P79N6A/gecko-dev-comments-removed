





































#include "nsSVGTSpanFrame.h"
#include "nsISVGGlyphFragmentNode.h"
#include "nsSVGGraphicElement.h"
#include "nsSVGMatrix.h"
#include "nsSVGAElement.h"
#include "nsSVGUtils.h"
#include "gfxMatrix.h"






typedef nsSVGTSpanFrame nsSVGAFrameBase;

class nsSVGAFrame : public nsSVGAFrameBase
{
  friend nsIFrame*
  NS_NewSVGAFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGAFrame(nsStyleContext* aContext) :
    nsSVGAFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  
  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGA"), aResult);
  }
#endif
  
  virtual void NotifySVGChanged(PRUint32 aFlags);
  
  
  virtual gfxMatrix GetCanvasTM();
  
private:
  nsCOMPtr<nsIDOMSVGMatrix> mCanvasTM;
};




nsIFrame*
NS_NewSVGAFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGAFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGAFrame)



#ifdef DEBUG
NS_IMETHODIMP
nsSVGAFrame::Init(nsIContent* aContent,
                  nsIFrame* aParent,
                  nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGAElement> elem = do_QueryInterface(aContent);
  NS_ASSERTION(elem,
               "Trying to construct an SVGAFrame for a "
               "content element that doesn't support the right interfaces");

  return nsSVGAFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

NS_IMETHODIMP
nsSVGAFrame::AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType)
{
  if (aNameSpaceID != kNameSpaceID_None)
    return NS_OK;

  if (aAttribute == nsGkAtoms::transform) {
    

    
    mCanvasTM = nsnull;
    
    nsSVGUtils::NotifyChildrenOfSVGChange(this, TRANSFORM_CHANGED);
  }

 return NS_OK;
}

nsIAtom *
nsSVGAFrame::GetType() const
{
  return nsGkAtoms::svgAFrame;
}




void
nsSVGAFrame::NotifySVGChanged(PRUint32 aFlags)
{
  if (aFlags & TRANSFORM_CHANGED) {
    
    mCanvasTM = nsnull;
  }

  nsSVGAFrameBase::NotifySVGChanged(aFlags);
}




gfxMatrix
nsSVGAFrame::GetCanvasTM()
{
  if (!mCanvasTM) {
    NS_ASSERTION(mParent, "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
    nsSVGAElement *content = static_cast<nsSVGAElement*>(mContent);

    gfxMatrix tm = content->PrependLocalTransformTo(parent->GetCanvasTM());

    mCanvasTM = NS_NewSVGMatrix(tm);
  }

  return nsSVGUtils::ConvertSVGMatrixToThebes(mCanvasTM);
}
