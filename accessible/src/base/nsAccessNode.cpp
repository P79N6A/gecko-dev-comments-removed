




#include "nsAccessNode.h"

#include "ApplicationAccessibleWrap.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "RootAccessible.h"

#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMWindow.h"
#include "nsIFrame.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsFocusManager.h"
#include "nsPresContext.h"
#include "mozilla/Services.h"

using namespace mozilla::a11y;





ApplicationAccessible* nsAccessNode::gApplicationAccessible = nullptr;




 



NS_IMPL_CYCLE_COLLECTION_1(nsAccessNode, mContent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsAccessNode)
  NS_INTERFACE_MAP_ENTRY(nsAccessNode)
NS_INTERFACE_MAP_END
 
NS_IMPL_CYCLE_COLLECTING_ADDREF(nsAccessNode)
NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_DESTROY(nsAccessNode, LastRelease())




nsAccessNode::
  nsAccessNode(nsIContent* aContent, DocAccessible* aDoc) :
  mContent(aContent), mDoc(aDoc)
{
}

nsAccessNode::~nsAccessNode()
{
  NS_ASSERTION(!mDoc, "LastRelease was never called!?!");
}

void nsAccessNode::LastRelease()
{
  
  if (mDoc) {
    Shutdown();
    NS_ASSERTION(!mDoc, "A Shutdown() impl forgot to call its parent's Shutdown?");
  }
  
  delete this;
}





void
nsAccessNode::Shutdown()
{
  mContent = nullptr;
  mDoc = nullptr;
}

ApplicationAccessible*
nsAccessNode::GetApplicationAccessible()
{
  NS_ASSERTION(!nsAccessibilityService::IsShutdown(),
               "Accessibility wasn't initialized!");

  if (!gApplicationAccessible) {
    ApplicationAccessibleWrap::PreCreate();

    gApplicationAccessible = new ApplicationAccessibleWrap();

    
    NS_ADDREF(gApplicationAccessible);

    gApplicationAccessible->Init();
  }

  return gApplicationAccessible;
}

void nsAccessNode::ShutdownXPAccessibility()
{
  
  
  

  
  
  ApplicationAccessibleWrap::Unload();
  if (gApplicationAccessible) {
    gApplicationAccessible->Shutdown();
    NS_RELEASE(gApplicationAccessible);
  }
}

RootAccessible*
nsAccessNode::RootAccessible() const
{
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mContent);
  NS_ASSERTION(docShellTreeItem, "No docshell tree item for mContent");
  if (!docShellTreeItem) {
    return nullptr;
  }
  nsCOMPtr<nsIDocShellTreeItem> root;
  docShellTreeItem->GetRootTreeItem(getter_AddRefs(root));
  NS_ASSERTION(root, "No root content tree item");
  if (!root) {
    return nullptr;
  }

  DocAccessible* docAcc = nsAccUtils::GetDocAccessibleFor(root);
  return docAcc ? docAcc->AsRoot() : nullptr;
}

nsIFrame*
nsAccessNode::GetFrame() const
{
  return mContent ? mContent->GetPrimaryFrame() : nullptr;
}

nsINode*
nsAccessNode::GetNode() const
{
  return mContent;
}

nsIDocument*
nsAccessNode::GetDocumentNode() const
{
  return mContent ? mContent->OwnerDoc() : nullptr;
}

void
nsAccessNode::Language(nsAString& aLanguage)
{
  aLanguage.Truncate();

  if (!mDoc)
    return;

  nsCoreUtils::GetLanguageFor(mContent, nullptr, aLanguage);
  if (aLanguage.IsEmpty()) { 
    mContent->OwnerDoc()->GetHeaderData(nsGkAtoms::headerContentLanguage,
                                        aLanguage);
  }
}

