





































#include "nsDocAccessible.h"

#include "nsIAccessible.h"

#include "nsAccCache.h"
#include "nsAccessibilityAtoms.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"

#include "nsHashtable.h"
#include "nsAccessibilityService.h"
#include "nsApplicationAccessibleWrap.h"
#include "nsIAccessibleDocument.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocument.h"
#include "nsIDOMCSSPrimitiveValue.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIFrame.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsRootAccessible.h"
#include "nsFocusManager.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"





nsIStringBundle *nsAccessNode::gStringBundle = 0;
nsIStringBundle *nsAccessNode::gKeyStringBundle = 0;
nsINode *nsAccessNode::gLastFocusedNode = nsnull;

PRBool nsAccessNode::gIsFormFillEnabled = PR_FALSE;

nsApplicationAccessible *nsAccessNode::gApplicationAccessible = nsnull;




 



NS_IMPL_CYCLE_COLLECTION_1(nsAccessNode, mContent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsAccessNode)
  NS_INTERFACE_MAP_ENTRY(nsIAccessNode)
  NS_INTERFACE_MAP_ENTRY(nsAccessNode)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIAccessNode)
NS_INTERFACE_MAP_END
 
NS_IMPL_CYCLE_COLLECTING_ADDREF(nsAccessNode)
NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_DESTROY(nsAccessNode, LastRelease())




nsAccessNode::
  nsAccessNode(nsIContent *aContent, nsIWeakReference *aShell) :
  mContent(aContent), mWeakShell(aShell)
{
#ifdef DEBUG_A11Y
  mIsInitialized = PR_FALSE;
#endif
}

nsAccessNode::~nsAccessNode()
{
  NS_ASSERTION(!mWeakShell, "LastRelease was never called!?!");
}

void nsAccessNode::LastRelease()
{
  
  if (mWeakShell) {
    Shutdown();
    NS_ASSERTION(!mWeakShell, "A Shutdown() impl forgot to call its parent's Shutdown?");
  }
  
  delete this;
}




bool
nsAccessNode::IsDefunct() const
{
  return !mContent;
}

PRBool
nsAccessNode::Init()
{
  return PR_TRUE;
}


void
nsAccessNode::Shutdown()
{
  mContent = nsnull;
  mWeakShell = nsnull;
}


NS_IMETHODIMP
nsAccessNode::GetUniqueID(void **aUniqueID)
{
  NS_ENSURE_ARG_POINTER(aUniqueID);

  *aUniqueID = UniqueID();
  return NS_OK;
}


NS_IMETHODIMP
nsAccessNode::GetOwnerWindow(void **aWindow)
{
  NS_ENSURE_ARG_POINTER(aWindow);
  *aWindow = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  return GetDocAccessible()->GetWindowHandle(aWindow);
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
    stringBundleService->CreateBundle(PLATFORM_KEYS_BUNDLE_URL, 
                                      &gKeyStringBundle);
  }

  nsAccessibilityAtoms::AddRefAtoms();

  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefBranch) {
    prefBranch->GetBoolPref("browser.formfill.enable", &gIsFormFillEnabled);
  }

  NotifyA11yInitOrShutdown(PR_TRUE);
}


void nsAccessNode::NotifyA11yInitOrShutdown(PRBool aIsInit)
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
  NS_IF_RELEASE(gKeyStringBundle);
  NS_IF_RELEASE(gLastFocusedNode);

  
  
  nsApplicationAccessibleWrap::Unload();
  if (gApplicationAccessible) {
    gApplicationAccessible->Shutdown();
    NS_RELEASE(gApplicationAccessible);
  }

  NotifyA11yInitOrShutdown(PR_FALSE);
}

already_AddRefed<nsIPresShell>
nsAccessNode::GetPresShell()
{
  nsIPresShell* presShell = nsnull;
  if (mWeakShell)
    CallQueryReferent(mWeakShell.get(), &presShell);

  return presShell;
}


nsPresContext* nsAccessNode::GetPresContext()
{
  nsCOMPtr<nsIPresShell> presShell(GetPresShell());
  if (!presShell) {
    return nsnull;
  }
  return presShell->GetPresContext();
}

nsDocAccessible *
nsAccessNode::GetDocAccessible() const
{
  return mContent ?
    GetAccService()->GetDocAccessible(mContent->GetOwnerDoc()) : nsnull;
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




NS_IMETHODIMP
nsAccessNode::GetDOMNode(nsIDOMNode **aDOMNode)
{
  NS_ENSURE_ARG_POINTER(aDOMNode);
  *aDOMNode = nsnull;

  nsINode *node = GetNode();
  if (node)
    CallQueryInterface(node, aDOMNode);

  return NS_OK;
}

NS_IMETHODIMP
nsAccessNode::GetDocument(nsIAccessibleDocument **aDocument)
{
  NS_ENSURE_ARG_POINTER(aDocument);

  NS_IF_ADDREF(*aDocument = GetDocAccessible());
  return NS_OK;
}

NS_IMETHODIMP
nsAccessNode::GetRootDocument(nsIAccessibleDocument **aRootDocument)
{
  NS_ENSURE_ARG_POINTER(aRootDocument);

  nsRootAccessible* rootDocument = RootAccessible();
  NS_IF_ADDREF(*aRootDocument = rootDocument);
  return NS_OK;
}

NS_IMETHODIMP
nsAccessNode::GetInnerHTML(nsAString& aInnerHTML)
{
  aInnerHTML.Truncate();

  nsCOMPtr<nsIDOMNSHTMLElement> domNSElement(do_QueryInterface(mContent));
  NS_ENSURE_TRUE(domNSElement, NS_ERROR_NULL_POINTER);

  return domNSElement->GetInnerHTML(aInnerHTML);
}

NS_IMETHODIMP
nsAccessNode::ScrollTo(PRUint32 aScrollType)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresShell> shell(GetPresShell());
  NS_ENSURE_TRUE(shell, NS_ERROR_FAILURE);

  nsIFrame *frame = GetFrame();
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  nsCOMPtr<nsIContent> content = frame->GetContent();
  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

  PRInt16 vPercent, hPercent;
  nsCoreUtils::ConvertScrollTypeToPercents(aScrollType, &vPercent, &hPercent);
  return shell->ScrollContentIntoView(content, vPercent, hPercent,
                                      nsIPresShell::SCROLL_OVERFLOW_HIDDEN);
}

NS_IMETHODIMP
nsAccessNode::ScrollToPoint(PRUint32 aCoordinateType, PRInt32 aX, PRInt32 aY)
{
  nsIFrame *frame = GetFrame();
  if (!frame)
    return NS_ERROR_FAILURE;

  nsIntPoint coords;
  nsresult rv = nsAccUtils::ConvertToScreenCoords(aX, aY, aCoordinateType,
                                                  this, &coords);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIFrame *parentFrame = frame;
  while ((parentFrame = parentFrame->GetParent()))
    nsCoreUtils::ScrollFrameToPoint(parentFrame, frame, coords);

  return NS_OK;
}

NS_IMETHODIMP
nsAccessNode::GetComputedStyleValue(const nsAString& aPseudoElt,
                                    const nsAString& aPropertyName,
                                    nsAString& aValue)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMCSSStyleDeclaration> styleDecl =
    nsCoreUtils::GetComputedStyleDeclaration(aPseudoElt, mContent);
  NS_ENSURE_TRUE(styleDecl, NS_ERROR_FAILURE);

  return styleDecl->GetPropertyValue(aPropertyName, aValue);
}

NS_IMETHODIMP
nsAccessNode::GetComputedStyleCSSValue(const nsAString& aPseudoElt,
                                       const nsAString& aPropertyName,
                                       nsIDOMCSSPrimitiveValue **aCSSValue)
{
  NS_ENSURE_ARG_POINTER(aCSSValue);
  *aCSSValue = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMCSSStyleDeclaration> styleDecl =
    nsCoreUtils::GetComputedStyleDeclaration(aPseudoElt, mContent);
  NS_ENSURE_STATE(styleDecl);

  nsCOMPtr<nsIDOMCSSValue> cssValue;
  styleDecl->GetPropertyCSSValue(aPropertyName, getter_AddRefs(cssValue));
  NS_ENSURE_TRUE(cssValue, NS_ERROR_FAILURE);

  return CallQueryInterface(cssValue, aCSSValue);
}


already_AddRefed<nsINode>
nsAccessNode::GetCurrentFocus()
{
  
  
  nsCOMPtr<nsIPresShell> shell = GetPresShell();
  NS_ENSURE_TRUE(shell, nsnull);
  nsIDocument *doc = shell->GetDocument();
  NS_ENSURE_TRUE(doc, nsnull);

  nsIDOMWindow* win = doc->GetWindow();

  nsCOMPtr<nsIDOMWindow> focusedWindow;
  nsCOMPtr<nsIDOMElement> focusedElement;
  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  if (fm)
    fm->GetFocusedElementForWindow(win, PR_TRUE, getter_AddRefs(focusedWindow),
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

NS_IMETHODIMP
nsAccessNode::GetLanguage(nsAString& aLanguage)
{
  aLanguage.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCoreUtils::GetLanguageFor(mContent, nsnull, aLanguage);

  if (aLanguage.IsEmpty()) { 
    nsIDocument *doc = mContent->GetOwnerDoc();
    if (doc) {
      doc->GetHeaderData(nsAccessibilityAtoms::headerContentLanguage, aLanguage);
    }
  }
 
  return NS_OK;
}
