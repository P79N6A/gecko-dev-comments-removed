



































#include "nsHTMLParts.h"
#include "nsContainerFrame.h"
#include "nsCSSRendering.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsIWidget.h"
#include "nsPageFrame.h"
#include "nsIRenderingContext.h"
#include "nsGUIEvent.h"
#include "nsIDOMEvent.h"
#include "nsStyleConsts.h"
#include "nsGkAtoms.h"
#include "nsIEventStateManager.h"
#include "nsIDeviceContext.h"
#include "nsIPresShell.h"
#include "nsBoxFrame.h"
#include "nsStackLayout.h"
#include "nsIAnonymousContentCreator.h"
#include "nsINodeInfo.h"
#include "nsIServiceManager.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"



class nsDocElementBoxFrame : public nsBoxFrame,
                             public nsIAnonymousContentCreator
{
public:
  virtual void Destroy();

  friend nsIFrame* NS_NewBoxFrame(nsIPresShell* aPresShell,
                                  nsStyleContext* aContext);

  nsDocElementBoxFrame(nsIPresShell* aShell, nsStyleContext* aContext)
    :nsBoxFrame(aShell, aContext, PR_TRUE) {}

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual nsresult CreateAnonymousContent(nsTArray<nsIContent*>& aElements);

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    
    if (aFlags & (nsIFrame::eReplacedContainsBlock | nsIFrame::eReplaced))
      return PR_FALSE;
    return nsBoxFrame::IsFrameOfType(aFlags);
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
private:
  nsCOMPtr<nsIContent> mPopupgroupContent;
  nsCOMPtr<nsIContent> mTooltipContent;
};



nsIFrame*
NS_NewDocElementBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsDocElementBoxFrame (aPresShell, aContext);
}

void
nsDocElementBoxFrame::Destroy()
{
  nsContentUtils::DestroyAnonymousContent(&mPopupgroupContent);
  nsContentUtils::DestroyAnonymousContent(&mTooltipContent);
  nsBoxFrame::Destroy();
}

nsresult
nsDocElementBoxFrame::CreateAnonymousContent(nsTArray<nsIContent*>& aElements)
{
  nsIDocument* doc = mContent->GetDocument();
  if (!doc) {
    
    return NS_ERROR_FAILURE;
  }
  nsNodeInfoManager *nodeInfoManager = doc->NodeInfoManager();

  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nsresult rv = nodeInfoManager->GetNodeInfo(nsGkAtoms::popupgroup,
                                             nsnull, kNameSpaceID_XUL,
                                             getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewXULElement(getter_AddRefs(mPopupgroupContent), nodeInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aElements.AppendElement(mPopupgroupContent))
    return NS_ERROR_OUT_OF_MEMORY;

  
  rv = nodeInfoManager->GetNodeInfo(nsGkAtoms::tooltip, nsnull,
                                    kNameSpaceID_XUL, getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewXULElement(getter_AddRefs(mTooltipContent), nodeInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  mTooltipContent->SetAttr(nsnull, nsGkAtoms::_default,
                           NS_LITERAL_STRING("true"), PR_FALSE);

  if (!aElements.AppendElement(mTooltipContent))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsDocElementBoxFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt)
nsDocElementBoxFrame::Release(void)
{
  return NS_OK;
}

NS_INTERFACE_MAP_BEGIN(nsDocElementBoxFrame)
  NS_INTERFACE_MAP_ENTRY(nsIAnonymousContentCreator)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)

#ifdef DEBUG
NS_IMETHODIMP
nsDocElementBoxFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("DocElementBox"), aResult);
}
#endif
