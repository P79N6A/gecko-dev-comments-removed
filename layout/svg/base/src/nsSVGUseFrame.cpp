



































#include "nsSVGGFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsIDOMSVGUseElement.h"
#include "nsIDOMSVGTransformable.h"
#include "nsSVGElement.h"
#include "nsSVGUseElement.h"
#include "gfxMatrix.h"

typedef nsSVGGFrame nsSVGUseFrameBase;

class nsSVGUseFrame : public nsSVGUseFrameBase,
                      public nsIAnonymousContentCreator
{
  friend nsIFrame*
  NS_NewSVGUseFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

protected:
  nsSVGUseFrame(nsStyleContext* aContext) : nsSVGUseFrameBase(aContext) {}

public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  
#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);

  virtual void Destroy();

  




  virtual nsIAtom* GetType() const;

  virtual PRBool IsLeaf() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGUse"), aResult);
  }
#endif

  
  virtual nsresult CreateAnonymousContent(nsTArray<nsIContent*>& aElements);
};




nsIFrame*
NS_NewSVGUseFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGUseFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGUseFrame)

nsIAtom *
nsSVGUseFrame::GetType() const
{
  return nsGkAtoms::svgUseFrame;
}




NS_QUERYFRAME_HEAD(nsSVGUseFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGUseFrameBase)




#ifdef DEBUG
NS_IMETHODIMP
nsSVGUseFrame::Init(nsIContent* aContent,
                    nsIFrame* aParent,
                    nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGUseElement> use = do_QueryInterface(aContent);
  NS_ASSERTION(use, "Content is not an SVG use!");

  return nsSVGUseFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

NS_IMETHODIMP
nsSVGUseFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                nsIAtom*        aAttribute,
                                PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::x ||
       aAttribute == nsGkAtoms::y)) {
    
    mCanvasTM = nsnull;
    
    nsSVGUtils::NotifyChildrenOfSVGChange(this, TRANSFORM_CHANGED);
    return NS_OK;
  }

  return nsSVGUseFrameBase::AttributeChanged(aNameSpaceID,
                                             aAttribute, aModType);
}

void
nsSVGUseFrame::Destroy()
{
  nsRefPtr<nsSVGUseElement> use = static_cast<nsSVGUseElement*>(mContent);
  nsSVGUseFrameBase::Destroy();
  use->DestroyAnonymousContent();
}

PRBool
nsSVGUseFrame::IsLeaf() const
{
  return PR_TRUE;
}





nsresult
nsSVGUseFrame::CreateAnonymousContent(nsTArray<nsIContent*>& aElements)
{
  nsSVGUseElement *use = static_cast<nsSVGUseElement*>(mContent);

  nsIContent* clone = use->CreateAnonymousContent();
  if (!clone)
    return NS_ERROR_FAILURE;
  if (!aElements.AppendElement(clone))
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}
