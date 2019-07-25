



#include "nsHTMLParts.h"
#include "nsContainerFrame.h"
#include "nsCSSRendering.h"
#include "nsIDocument.h"
#include "nsPageFrame.h"
#include "nsGUIEvent.h"
#include "nsIDOMEvent.h"
#include "nsStyleConsts.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsBoxFrame.h"
#include "nsStackLayout.h"
#include "nsIAnonymousContentCreator.h"
#include "nsINodeInfo.h"
#include "nsIServiceManager.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"
#include "nsContentList.h"



class nsDocElementBoxFrame : public nsBoxFrame,
                             public nsIAnonymousContentCreator
{
public:
  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  friend nsIFrame* NS_NewBoxFrame(nsIPresShell* aPresShell,
                                  nsStyleContext* aContext);

  nsDocElementBoxFrame(nsIPresShell* aShell, nsStyleContext* aContext)
    :nsBoxFrame(aShell, aContext, true) {}

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements);
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        PRUint32 aFilter);

  virtual bool IsFrameOfType(PRUint32 aFlags) const
  {
    
    if (aFlags & (nsIFrame::eReplacedContainsBlock | nsIFrame::eReplaced))
      return false;
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

NS_IMPL_FRAMEARENA_HELPERS(nsDocElementBoxFrame)

void
nsDocElementBoxFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsContentUtils::DestroyAnonymousContent(&mPopupgroupContent);
  nsContentUtils::DestroyAnonymousContent(&mTooltipContent);
  nsBoxFrame::DestroyFrom(aDestructRoot);
}

nsresult
nsDocElementBoxFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  nsIDocument* doc = mContent->GetDocument();
  if (!doc) {
    
    return NS_ERROR_FAILURE;
  }
  nsNodeInfoManager *nodeInfoManager = doc->NodeInfoManager();

  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::popupgroup,
                                          nsnull, kNameSpaceID_XUL,
                                          nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = NS_NewXULElement(getter_AddRefs(mPopupgroupContent),
                                 nodeInfo.forget());
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aElements.AppendElement(mPopupgroupContent))
    return NS_ERROR_OUT_OF_MEMORY;

  
  nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::tooltip, nsnull,
                                          kNameSpaceID_XUL,
                                          nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  rv = NS_NewXULElement(getter_AddRefs(mTooltipContent), nodeInfo.forget());
  NS_ENSURE_SUCCESS(rv, rv);

  mTooltipContent->SetAttr(nsnull, nsGkAtoms::_default,
                           NS_LITERAL_STRING("true"), false);

  if (!aElements.AppendElement(mTooltipContent))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

void
nsDocElementBoxFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                               PRUint32 aFilter)
{
  aElements.MaybeAppendElement(mPopupgroupContent);
  aElements.MaybeAppendElement(mTooltipContent);
}

NS_QUERYFRAME_HEAD(nsDocElementBoxFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsBoxFrame)

#ifdef DEBUG
NS_IMETHODIMP
nsDocElementBoxFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("DocElementBox"), aResult);
}
#endif
