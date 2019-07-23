





































#include "nsIDOMSVGStopElement.h"
#include "nsStyleContext.h"
#include "nsFrame.h"
#include "nsGkAtoms.h"
#include "nsISVGValue.h"





typedef nsFrame  nsSVGStopFrameBase;

class nsSVGStopFrame : public nsSVGStopFrameBase
{
public:
  nsSVGStopFrame(nsStyleContext* aContext) : nsSVGStopFrameBase(aContext) {}

  
  NS_IMETHOD DidSetStyleContext();

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  




  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsSVGStopFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG));
  }

#ifdef DEBUG
  
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGStop"), aResult);
  }
#endif

  friend nsIFrame* NS_NewSVGStopFrame(nsIPresShell*   aPresShell,
                                      nsIContent*     aContent,
                                      nsIFrame*       aParentFrame,
                                      nsStyleContext* aContext);
};







NS_IMETHODIMP
nsSVGStopFrame::DidSetStyleContext()
{
  
  if (mParent)
    mParent->DidSetStyleContext();
  return NS_OK;
}

nsIAtom *
nsSVGStopFrame::GetType() const
{
  return nsGkAtoms::svgStopFrame;
}

NS_IMETHODIMP
nsSVGStopFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                 nsIAtom*        aAttribute,
                                 PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::offset) {

    
    
    
    
    if (mParent) {
      nsISVGValue *svgParent;
      CallQueryInterface(mParent, &svgParent);
      if (svgParent) {
        svgParent->BeginBatchUpdate();
        svgParent->EndBatchUpdate();
      }
    }
    return NS_OK;
  } 

  return nsSVGStopFrameBase::AttributeChanged(aNameSpaceID,
                                              aAttribute, aModType);
}





nsIFrame* NS_NewSVGStopFrame(nsIPresShell*   aPresShell,
                             nsIContent*     aContent,
                             nsIFrame*       aParentFrame,
                             nsStyleContext* aContext)
{
  nsCOMPtr<nsIDOMSVGStopElement> grad = do_QueryInterface(aContent);
  NS_ASSERTION(grad, "NS_NewSVGStopFrame -- Content doesn't support nsIDOMSVGStopElement");
  if (!grad)
    return nsnull;

  return new (aPresShell) nsSVGStopFrame(aContext);
}
