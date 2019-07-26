






#include "nsXULLabelFrame.h"
#include "nsHTMLParts.h"
#include "nsINameSpaceManager.h"
#include "nsEventStateManager.h"

nsIFrame*
NS_NewXULLabelFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  nsXULLabelFrame* it = new (aPresShell) nsXULLabelFrame(aContext);
  
  if (it)
    it->SetFlags(NS_BLOCK_FLOAT_MGR | NS_BLOCK_MARGIN_ROOT);

  return it;
}

NS_IMPL_FRAMEARENA_HELPERS(nsXULLabelFrame)



nsresult
nsXULLabelFrame::RegUnregAccessKey(bool aDoReg)
{
  
  if (!mContent)
    return NS_ERROR_FAILURE;

  
  
  
  
  
  if (!mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::control))
    return NS_OK;

  nsAutoString accessKey;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, accessKey);

  if (accessKey.IsEmpty())
    return NS_OK;

  
  
  nsEventStateManager *esm = PresContext()->EventStateManager();

  uint32_t key = accessKey.First();
  if (aDoReg)
    esm->RegisterAccessKey(mContent, key);
  else
    esm->UnregisterAccessKey(mContent, key);

  return NS_OK;
}




void
nsXULLabelFrame::Init(nsIContent*      aContent,
                      nsIFrame*        aParent,
                      nsIFrame*        aPrevInFlow)
{
  nsBlockFrame::Init(aContent, aParent, aPrevInFlow);

  
  RegUnregAccessKey(true);
}

void
nsXULLabelFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  RegUnregAccessKey(false);
  nsBlockFrame::DestroyFrom(aDestructRoot);
} 

NS_IMETHODIMP
nsXULLabelFrame::AttributeChanged(int32_t aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  int32_t aModType)
{
  nsresult rv = nsBlockFrame::AttributeChanged(aNameSpaceID, 
                                               aAttribute, aModType);

  
  
  if (aAttribute == nsGkAtoms::accesskey || aAttribute == nsGkAtoms::control)
    RegUnregAccessKey(true);

  return rv;
}

nsIAtom*
nsXULLabelFrame::GetType() const
{
  return nsGkAtoms::XULLabelFrame;
}




#ifdef DEBUG
NS_IMETHODIMP
nsXULLabelFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("XULLabel"), aResult);
}
#endif
