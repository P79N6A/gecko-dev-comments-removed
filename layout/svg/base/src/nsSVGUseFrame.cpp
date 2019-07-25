



































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

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  




  virtual nsIAtom* GetType() const;

  virtual bool IsLeaf() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGUse"), aResult);
  }
#endif

  
  virtual void NotifySVGChanged(PRUint32 aFlags);

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements);
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        PRUint32 aFilter);
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
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::x ||
        aAttribute == nsGkAtoms::y) {
      
      mCanvasTM = nsnull;
    
      nsSVGUtils::NotifyChildrenOfSVGChange(this, TRANSFORM_CHANGED);
    } else if (aAttribute == nsGkAtoms::width ||
               aAttribute == nsGkAtoms::height) {
      static_cast<nsSVGUseElement*>(mContent)->SyncWidthHeight(aAttribute);
    }
  } else if (aNameSpaceID == kNameSpaceID_XLink &&
             aAttribute == nsGkAtoms::href) {
    
    nsSVGUseElement *use = static_cast<nsSVGUseElement*>(mContent);
    use->mOriginal = nsnull;
    use->UnlinkSource();
    use->TriggerReclone();
  }

  return nsSVGUseFrameBase::AttributeChanged(aNameSpaceID,
                                             aAttribute, aModType);
}

void
nsSVGUseFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsRefPtr<nsSVGUseElement> use = static_cast<nsSVGUseElement*>(mContent);
  nsSVGUseFrameBase::DestroyFrom(aDestructRoot);
  use->DestroyAnonymousContent();
}

bool
nsSVGUseFrame::IsLeaf() const
{
  return true;
}





void
nsSVGUseFrame::NotifySVGChanged(PRUint32 aFlags)
{
  if (aFlags & COORD_CONTEXT_CHANGED &&
      !(aFlags & TRANSFORM_CHANGED)) {
    
    
    nsSVGUseElement *use = static_cast<nsSVGUseElement*>(mContent);
    if (use->mLengthAttributes[nsSVGUseElement::X].IsPercentage() ||
        use->mLengthAttributes[nsSVGUseElement::Y].IsPercentage()) {
      aFlags |= TRANSFORM_CHANGED;
    }
  }

  nsSVGUseFrameBase::NotifySVGChanged(aFlags);
}




nsresult
nsSVGUseFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  nsSVGUseElement *use = static_cast<nsSVGUseElement*>(mContent);

  nsIContent* clone = use->CreateAnonymousContent();
  if (!clone)
    return NS_ERROR_FAILURE;
  if (!aElements.AppendElement(clone))
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

void
nsSVGUseFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        PRUint32 aFilter)
{
  nsSVGUseElement *use = static_cast<nsSVGUseElement*>(mContent);
  nsIContent* clone = use->GetAnonymousContent();
  aElements.MaybeAppendElement(clone);
}
