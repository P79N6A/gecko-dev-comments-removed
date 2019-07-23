






































#include "nsAreaFrame.h"
#include "nsBlockBandData.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsINodeInfo.h"
#include "nsGkAtoms.h"
#include "nsHTMLParts.h"

#ifdef MOZ_XUL
#include "nsINameSpaceManager.h"
#include "nsIEventStateManager.h"
#endif

#undef NOISY_MAX_ELEMENT_SIZE
#undef NOISY_FINAL_SIZE

nsIFrame*
NS_NewAreaFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUint32 aFlags)
{
  nsAreaFrame* it = new (aPresShell) nsAreaFrame(aContext);
  
  if (it != nsnull)
    it->SetFlags(aFlags);

  return it;
}

#ifdef MOZ_XUL



nsresult
nsAreaFrame::RegUnregAccessKey(PRBool aDoReg)
{
  
  if (!mContent)
    return NS_ERROR_FAILURE;

  
  if (!mContent->NodeInfo()->Equals(nsGkAtoms::label, kNameSpaceID_XUL))
    return NS_OK;

  
  
  
  
  
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
#endif




#ifdef MOZ_XUL
NS_IMETHODIMP
nsAreaFrame::Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsBlockFrame::Init(aContent, aParent, aPrevInFlow);
  if (NS_FAILED(rv))
    return rv;

  
  return RegUnregAccessKey(PR_TRUE);
}

void
nsAreaFrame::Destroy()
{
  
  RegUnregAccessKey(PR_FALSE);
  nsBlockFrame::Destroy();
} 

NS_IMETHODIMP
nsAreaFrame::AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType)
{
  nsresult rv = nsBlockFrame::AttributeChanged(aNameSpaceID, 
                                               aAttribute, aModType);

  
  
  if (aAttribute == nsGkAtoms::accesskey || aAttribute == nsGkAtoms::control)
    RegUnregAccessKey(PR_TRUE);

  return rv;
}
#endif

nsIAtom*
nsAreaFrame::GetType() const
{
  return nsGkAtoms::areaFrame;
}




#ifdef NS_DEBUG
NS_IMETHODIMP
nsAreaFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Area"), aResult);
}
#endif
