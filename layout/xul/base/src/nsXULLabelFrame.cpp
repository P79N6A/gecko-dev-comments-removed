






































#include "nsXULLabelFrame.h"
#include "nsHTMLParts.h"
#include "nsINameSpaceManager.h"
#include "nsIEventStateManager.h"

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
nsXULLabelFrame::RegUnregAccessKey(PRBool aDoReg)
{
  
  if (!mContent)
    return NS_ERROR_FAILURE;

  
  
  
  
  
  if (!mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::control))
    return NS_OK;

  nsAutoString accessKey;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, accessKey);

  if (accessKey.IsEmpty())
    return NS_OK;

  
  
  nsIEventStateManager *esm = PresContext()->EventStateManager();
  nsresult rv;

  PRUint32 key = accessKey.First();
  if (aDoReg)
    rv = esm->RegisterAccessKey(mContent, key);
  else
    rv = esm->UnregisterAccessKey(mContent, key);

  return rv;
}




NS_IMETHODIMP
nsXULLabelFrame::Init(nsIContent*      aContent,
                      nsIFrame*        aParent,
                      nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsBlockFrame::Init(aContent, aParent, aPrevInFlow);
  if (NS_FAILED(rv))
    return rv;

  
  return RegUnregAccessKey(PR_TRUE);
}

void
nsXULLabelFrame::Destroy()
{
  
  RegUnregAccessKey(PR_FALSE);
  nsBlockFrame::Destroy();
} 

NS_IMETHODIMP
nsXULLabelFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  PRInt32 aModType)
{
  nsresult rv = nsBlockFrame::AttributeChanged(aNameSpaceID, 
                                               aAttribute, aModType);

  
  
  if (aAttribute == nsGkAtoms::accesskey || aAttribute == nsGkAtoms::control)
    RegUnregAccessKey(PR_TRUE);

  return rv;
}

nsIAtom*
nsXULLabelFrame::GetType() const
{
  return nsGkAtoms::XULLabelFrame;
}




#ifdef NS_DEBUG
NS_IMETHODIMP
nsXULLabelFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("XULLabel"), aResult);
}
#endif
