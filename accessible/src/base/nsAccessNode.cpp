





































#include "nsAccessNode.h"

#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsApplicationAccessibleWrap.h"
#include "nsCoreUtils.h"
#include "nsRootAccessible.h"

#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMWindow.h"
#include "nsIFrame.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIObserverService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsFocusManager.h"
#include "nsPresContext.h"
#include "mozilla/Services.h"





nsIStringBundle *nsAccessNode::gStringBundle = 0;

bool nsAccessNode::gIsFormFillEnabled = false;

nsApplicationAccessible *nsAccessNode::gApplicationAccessible = nsnull;




 



NS_IMPL_CYCLE_COLLECTION_1(nsAccessNode, mContent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsAccessNode)
  NS_INTERFACE_MAP_ENTRY(nsAccessNode)
NS_INTERFACE_MAP_END
 
NS_IMPL_CYCLE_COLLECTING_ADDREF(nsAccessNode)
NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_DESTROY(nsAccessNode, LastRelease())




nsAccessNode::
  nsAccessNode(nsIContent* aContent, nsDocAccessible* aDoc) :
  mContent(aContent), mDoc(aDoc)
{
#ifdef DEBUG_A11Y
  mIsInitialized = false;
#endif
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




bool
nsAccessNode::IsDefunct() const
{
  return !mContent;
}

bool
nsAccessNode::Init()
{
  return true;
}


void
nsAccessNode::Shutdown()
{
  mContent = nsnull;
  mDoc = nsnull;
}

nsApplicationAccessible*
nsAccessNode::GetApplicationAccessible()
{
  NS_ASSERTION(!nsAccessibilityService::IsShutdown(),
               "Accessibility wasn't initialized!");

  if (!gApplicationAccessible) {
    nsApplicationAccessibleWrap::PreCreate();

    gApplicationAccessible = new nsApplicationAccessibleWrap();
    if (!gApplicationAccessible)
      return nsnull;

    
    NS_ADDREF(gApplicationAccessible);

    nsresult rv = gApplicationAccessible->Init();
    if (NS_FAILED(rv)) {
      gApplicationAccessible->Shutdown();
      NS_RELEASE(gApplicationAccessible);
      return nsnull;
    }
  }

  return gApplicationAccessible;
}

void nsAccessNode::InitXPAccessibility()
{
  nsCOMPtr<nsIStringBundleService> stringBundleService =
    mozilla::services::GetStringBundleService();
  if (stringBundleService) {
    
    stringBundleService->CreateBundle(ACCESSIBLE_BUNDLE_URL, 
                                      &gStringBundle);
  }

  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefBranch) {
    prefBranch->GetBoolPref("browser.formfill.enable", &gIsFormFillEnabled);
  }

  NotifyA11yInitOrShutdown(true);
}


void nsAccessNode::NotifyA11yInitOrShutdown(bool aIsInit)
{
  nsCOMPtr<nsIObserverService> obsService =
    mozilla::services::GetObserverService();
  NS_ASSERTION(obsService, "No observer service to notify of a11y init/shutdown");
  if (!obsService)
    return;

  static const PRUnichar kInitIndicator[] = { '1', 0 };
  static const PRUnichar kShutdownIndicator[] = { '0', 0 }; 
  obsService->NotifyObservers(nsnull, "a11y-init-or-shutdown",
                              aIsInit ? kInitIndicator  : kShutdownIndicator);
}

void nsAccessNode::ShutdownXPAccessibility()
{
  
  
  

  NS_IF_RELEASE(gStringBundle);

  
  
  nsApplicationAccessibleWrap::Unload();
  if (gApplicationAccessible) {
    gApplicationAccessible->Shutdown();
    NS_RELEASE(gApplicationAccessible);
  }

  NotifyA11yInitOrShutdown(false);
}


nsPresContext* nsAccessNode::GetPresContext()
{
  if (IsDefunct())
    return nsnull;

  nsIPresShell* presShell(mDoc->PresShell());

  return presShell ? presShell->GetPresContext() : nsnull;
}

nsRootAccessible*
nsAccessNode::RootAccessible() const
{
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mContent);
  NS_ASSERTION(docShellTreeItem, "No docshell tree item for mContent");
  if (!docShellTreeItem) {
    return nsnull;
  }
  nsCOMPtr<nsIDocShellTreeItem> root;
  docShellTreeItem->GetRootTreeItem(getter_AddRefs(root));
  NS_ASSERTION(root, "No root content tree item");
  if (!root) {
    return nsnull;
  }

  nsDocAccessible* docAcc = nsAccUtils::GetDocAccessibleFor(root);
  return docAcc ? docAcc->AsRoot() : nsnull;
}

nsIFrame*
nsAccessNode::GetFrame() const
{
  return mContent ? mContent->GetPrimaryFrame() : nsnull;
}

bool
nsAccessNode::IsPrimaryForNode() const
{
  return true;
}


void
nsAccessNode::ScrollTo(PRUint32 aScrollType)
{
  if (IsDefunct())
    return;

  nsIPresShell* shell = mDoc->PresShell();
  if (!shell)
    return;

  nsIFrame *frame = GetFrame();
  if (!frame)
    return;

  nsIContent* content = frame->GetContent();
  if (!content)
    return;

  PRInt16 vPercent, hPercent;
  nsCoreUtils::ConvertScrollTypeToPercents(aScrollType, &vPercent, &hPercent);
  shell->ScrollContentIntoView(content, vPercent, hPercent,
                               nsIPresShell::SCROLL_OVERFLOW_HIDDEN);
}


already_AddRefed<nsINode>
nsAccessNode::GetCurrentFocus()
{
  
  
  nsIDocument* doc = GetDocumentNode();
  NS_ENSURE_TRUE(doc, nsnull);

  nsIDOMWindow* win = doc->GetWindow();

  nsCOMPtr<nsIDOMWindow> focusedWindow;
  nsCOMPtr<nsIDOMElement> focusedElement;
  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  if (fm)
    fm->GetFocusedElementForWindow(win, true, getter_AddRefs(focusedWindow),
                                   getter_AddRefs(focusedElement));

  nsINode *focusedNode = nsnull;
  if (focusedElement) {
    CallQueryInterface(focusedElement, &focusedNode);
  }
  else if (focusedWindow) {
    nsCOMPtr<nsIDOMDocument> doc;
    focusedWindow->GetDocument(getter_AddRefs(doc));
    if (doc)
      CallQueryInterface(doc, &focusedNode);
  }

  return focusedNode;
}

void
nsAccessNode::Language(nsAString& aLanguage)
{
  aLanguage.Truncate();

  if (IsDefunct())
    return;

  nsCoreUtils::GetLanguageFor(mContent, nsnull, aLanguage);
  if (aLanguage.IsEmpty()) { 
    mContent->OwnerDoc()->GetHeaderData(nsGkAtoms::headerContentLanguage,
                                        aLanguage);
  }
}

