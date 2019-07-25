






































#include "nsAccessibilityAtoms.h"
#include "nsAccessibilityService.h"
#include "nsCoreUtils.h"
#include "nsAccUtils.h"
#include "nsApplicationAccessibleWrap.h"
#include "nsARIAGridAccessibleWrap.h"
#include "nsARIAMap.h"
#include "nsIContentViewer.h"
#include "nsCURILoader.h"
#include "nsDocAccessible.h"
#include "nsHTMLImageMapAccessible.h"
#include "nsHTMLLinkAccessible.h"
#include "nsHTMLSelectAccessible.h"
#include "nsHTMLTableAccessibleWrap.h"
#include "nsHTMLTextAccessible.h"
#include "nsHyperTextAccessibleWrap.h"
#include "nsIAccessibilityService.h"
#include "nsIAccessibleProvider.h"

#include "nsIDOMDocument.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLLegendElement.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLOptGroupElement.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIDOMXULElement.h"
#include "nsIHTMLDocument.h"
#include "nsIImageFrame.h"
#include "nsILink.h"
#include "nsIObserverService.h"
#include "nsIPluginInstance.h"
#include "nsISupportsUtils.h"
#include "nsObjectFrame.h"
#include "nsOuterDocAccessible.h"
#include "nsRootAccessibleWrap.h"
#include "nsTextFragment.h"
#include "mozilla/Services.h"

#ifdef MOZ_XUL
#include "nsXULAlertAccessible.h"
#include "nsXULColorPickerAccessible.h"
#include "nsXULComboboxAccessible.h"
#include "nsXULFormControlAccessible.h"
#include "nsXULListboxAccessibleWrap.h"
#include "nsXULMenuAccessibleWrap.h"
#include "nsXULSliderAccessible.h"
#include "nsXULTabAccessible.h"
#include "nsXULTextAccessible.h"
#include "nsXULTreeGridAccessibleWrap.h"
#endif


#ifdef XP_WIN
#include "nsHTMLWin32ObjectAccessible.h"
#endif

#ifndef DISABLE_XFORMS_HOOKS
#include "nsXFormsFormControlsAccessible.h"
#include "nsXFormsWidgetsAccessible.h"
#endif

#include "mozilla/FunctionTimer.h"





nsAccessibilityService *nsAccessibilityService::gAccessibilityService = nsnull;
PRBool nsAccessibilityService::gIsShutdown = PR_TRUE;

nsAccessibilityService::nsAccessibilityService() : nsAccDocManager()
{
  NS_TIME_FUNCTION;
}

nsAccessibilityService::~nsAccessibilityService()
{
  NS_ASSERTION(gIsShutdown, "Accessibility wasn't shutdown!");
  gAccessibilityService = nsnull;
}




NS_IMPL_ISUPPORTS_INHERITED3(nsAccessibilityService,
                             nsAccDocManager,
                             nsIAccessibilityService,
                             nsIAccessibleRetrieval,
                             nsIObserver)




NS_IMETHODIMP
nsAccessibilityService::Observe(nsISupports *aSubject, const char *aTopic,
                         const PRUnichar *aData)
{
  if (!nsCRT::strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID))
    Shutdown();

  return NS_OK;
}


void
nsAccessibilityService::NotifyOfAnchorJumpTo(nsIContent *aTarget)
{
  nsIDocument *document = aTarget->GetCurrentDoc();
  if (!document)
    return;

  nsINode *targetNode = aTarget;
  nsAccessible *targetAcc = GetAccessible(targetNode);

  
  
  nsDocAccessible *docAccessible = GetDocAccessible(document);
  if (!docAccessible)
    return;

  
  
  if (!targetAcc) {
    targetAcc = GetContainerAccessible(targetNode, PR_TRUE);
    targetNode = targetAcc->GetNode();
  }

  NS_ASSERTION(targetNode,
      "No accessible in parent chain!? Expect at least a document accessible.");
  if (!targetNode)
    return;

  
  
  docAccessible->
    FireDelayedAccessibleEvent(nsIAccessibleEvent::EVENT_SCROLLING_START,
                               targetNode);
}


nsresult
nsAccessibilityService::FireAccessibleEvent(PRUint32 aEvent,
                                            nsIAccessible *aTarget)
{
  nsRefPtr<nsAccessible> accessible(do_QueryObject(aTarget));
  nsEventShell::FireEvent(aEvent, accessible);
  return NS_OK;
}


nsresult
nsAccessibilityService::GetInfo(nsIFrame *aFrame, nsIWeakReference **aShell,
                                nsIContent **aContent)
{
  NS_ASSERTION(aFrame,"Error -- 1st argument (aFrame) is null!!");
  if (!aFrame) {
    return NS_ERROR_FAILURE;
  }
  nsIContent* content = aFrame->GetContent();
  if (!content)
    return NS_ERROR_FAILURE;

  nsIDocument* document = content->GetOwnerDoc();
  if (!document)
    return NS_ERROR_FAILURE;

  NS_ASSERTION(document->GetShell(),
               "Error: aFrame's document doesn't have a PresShell!");

  
  nsCOMPtr<nsIWeakReference> weakShell =
    do_GetWeakReference(document->GetShell());

  weakShell.forget(aShell);
  NS_IF_ADDREF(*aContent = content);

  return NS_OK;
}




nsresult
nsAccessibilityService::CreateOuterDocAccessible(nsIDOMNode* aDOMNode, 
                                                 nsIAccessible **aOuterDocAccessible)
{
  NS_ENSURE_ARG_POINTER(aDOMNode);
  
  *aOuterDocAccessible = nsnull;

  nsCOMPtr<nsIContent> content(do_QueryInterface(aDOMNode));
  nsCOMPtr<nsIWeakReference> outerWeakShell =
    nsCoreUtils::GetWeakShellFor(content);
  NS_ENSURE_TRUE(outerWeakShell, NS_ERROR_FAILURE);

  nsOuterDocAccessible *outerDocAccessible =
    new nsOuterDocAccessible(content, outerWeakShell);
  NS_ENSURE_TRUE(outerDocAccessible, NS_ERROR_FAILURE);

  NS_ADDREF(*aOuterDocAccessible = outerDocAccessible);

  return NS_OK;
}

 


nsresult
nsAccessibilityService::CreateHTML4ButtonAccessible(nsIFrame *aFrame,
                                                    nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  *aAccessible = new nsHTML4ButtonAccessible(content, weakShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLButtonAccessible(nsIFrame *aFrame,
                                                   nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  *aAccessible = new nsHTMLButtonAccessible(content, weakShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}


already_AddRefed<nsAccessible>
nsAccessibilityService::CreateHTMLAccessibleByMarkup(nsIFrame *aFrame,
                                                     nsIWeakReference *aWeakShell,
                                                     nsINode *aNode)
{
  
  nsRefPtr<nsAccessible> accessible;

  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
  nsIAtom *tag = content->Tag();
  if (tag == nsAccessibilityAtoms::legend) {
    accessible = new nsHTMLLegendAccessible(content, aWeakShell);
  }
  else if (tag == nsAccessibilityAtoms::option) {
    accessible = new nsHTMLSelectOptionAccessible(content, aWeakShell);
  }
  else if (tag == nsAccessibilityAtoms::optgroup) {
    accessible = new nsHTMLSelectOptGroupAccessible(content, aWeakShell);
  }
  else if (tag == nsAccessibilityAtoms::ul || tag == nsAccessibilityAtoms::ol ||
           tag == nsAccessibilityAtoms::dl) {
    accessible = new nsHTMLListAccessible(content, aWeakShell);
  }
  else if (tag == nsAccessibilityAtoms::a) {

    
    
    nsRoleMapEntry *roleMapEntry = nsAccUtils::GetRoleMapEntry(aNode);
    if (roleMapEntry && roleMapEntry->role != nsIAccessibleRole::ROLE_NOTHING
        && roleMapEntry->role != nsIAccessibleRole::ROLE_LINK) {

      accessible = new nsHyperTextAccessibleWrap(content, aWeakShell);
    } else {
      accessible = new nsHTMLLinkAccessible(content, aWeakShell);
    }
  }
  else if (tag == nsAccessibilityAtoms::dt ||
           (tag == nsAccessibilityAtoms::li && 
            aFrame->GetType() != nsAccessibilityAtoms::blockFrame)) {
    
    
    
    accessible = new nsHTMLLIAccessible(content, aWeakShell, EmptyString());
  }
  else if (tag == nsAccessibilityAtoms::abbr ||
           tag == nsAccessibilityAtoms::acronym ||
           tag == nsAccessibilityAtoms::blockquote ||
           tag == nsAccessibilityAtoms::dd ||
           tag == nsAccessibilityAtoms::form ||
           tag == nsAccessibilityAtoms::h1 ||
           tag == nsAccessibilityAtoms::h2 ||
           tag == nsAccessibilityAtoms::h3 ||
           tag == nsAccessibilityAtoms::h4 ||
           tag == nsAccessibilityAtoms::h5 ||
           tag == nsAccessibilityAtoms::h6 ||
           tag == nsAccessibilityAtoms::q) {

    accessible = new nsHyperTextAccessibleWrap(content, aWeakShell);
  }
  else if (tag == nsAccessibilityAtoms::tr) {
    accessible = new nsEnumRoleAccessible(content, aWeakShell,
                                          nsIAccessibleRole::ROLE_ROW);
  }
  else if (nsCoreUtils::IsHTMLTableHeader(content)) {
    accessible = new nsHTMLTableHeaderCellAccessibleWrap(content, aWeakShell);
  }

  return accessible.forget();
}

nsresult
nsAccessibilityService::CreateHTMLLIAccessible(nsIFrame *aFrame, 
                                               nsIFrame *aBulletFrame,
                                               const nsAString& aBulletText,
                                               nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  *aAccessible = new nsHTMLLIAccessible(content, weakShell, aBulletText);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHyperTextAccessible(nsIFrame *aFrame,
                                                  nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  *aAccessible = new nsHyperTextAccessibleWrap(content, weakShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLCheckboxAccessible(nsIFrame *aFrame,
                                                     nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  *aAccessible = new nsHTMLCheckboxAccessible(content, weakShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLComboboxAccessible(nsIDOMNode *aDOMNode,
                                                     nsIWeakReference *aPresShell,
                                                     nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(aDOMNode));
  *aAccessible = new nsHTMLComboboxAccessible(content, aPresShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLImageAccessible(nsIFrame *aFrame,
                                                  nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIHTMLDocument> htmlDoc =
    do_QueryInterface(content->GetCurrentDoc());

  nsCOMPtr<nsIDOMHTMLMapElement> mapElm;
  if (htmlDoc) {
    nsAutoString mapElmName;
    content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::usemap,
                     mapElmName);

    if (!mapElmName.IsEmpty()) {
      if (mapElmName.CharAt(0) == '#')
        mapElmName.Cut(0,1);
      mapElm = htmlDoc->GetImageMap(mapElmName);
    }
  }

  if (mapElm)
    *aAccessible = new nsHTMLImageMapAccessible(content, weakShell, mapElm);
  else
    *aAccessible = new nsHTMLImageAccessibleWrap(content, weakShell);

  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLGenericAccessible(nsIFrame *aFrame, nsIAccessible **aAccessible)
{
  return CreateHyperTextAccessible(aFrame, aAccessible);
}

nsresult
nsAccessibilityService::CreateHTMLGroupboxAccessible(nsIFrame *aFrame,
                                                     nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  *aAccessible = new nsHTMLGroupboxAccessible(content, weakShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLListboxAccessible(nsIDOMNode* aDOMNode,
                                                    nsIWeakReference *aPresShell,
                                                    nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(aDOMNode));
  *aAccessible = new nsHTMLSelectListAccessible(content, aPresShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLMediaAccessible(nsIFrame *aFrame,
                                                  nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  NS_ENSURE_SUCCESS(rv, rv);

  *aAccessible = new nsEnumRoleAccessible(content, weakShell,
                                          nsIAccessibleRole::ROLE_GROUPING);
  NS_ENSURE_TRUE(*aAccessible, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aAccessible);
  return NS_OK;
}










nsresult
nsAccessibilityService::CreateHTMLObjectFrameAccessible(nsObjectFrame *aFrame,
                                                        nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  GetInfo(aFrame, getter_AddRefs(weakShell), getter_AddRefs(content));

  if (aFrame->GetRect().IsEmpty()) {
    return NS_ERROR_FAILURE;
  }
  
  nsCOMPtr<nsIDOMHTMLObjectElement> obj(do_QueryInterface(content));
  if (obj) {
    nsCOMPtr<nsIDOMDocument> domDoc;
    obj->GetContentDocument(getter_AddRefs(domDoc));
    if (domDoc) {
      nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(content));
      return CreateOuterDocAccessible(DOMNode, aAccessible);
    }
  }

#ifdef XP_WIN
  
  nsCOMPtr<nsIPluginInstance> pluginInstance ;
  aFrame->GetPluginInstance(*getter_AddRefs(pluginInstance));
  if (pluginInstance) {
    
    HWND pluginPort = nsnull;
    aFrame->GetPluginPort(&pluginPort);

    *aAccessible =
      new nsHTMLWin32ObjectOwnerAccessible(content, weakShell, pluginPort);
    NS_ENSURE_TRUE(*aAccessible, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(*aAccessible);
    return NS_OK;
  }
#endif

  
  
  nsIFrame *frame = aFrame->GetFirstChild(nsnull);
  if (frame)
    return frame->GetAccessible(aAccessible);

  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLRadioButtonAccessible(nsIFrame *aFrame,
                                                        nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  *aAccessible = new nsHTMLRadioButtonAccessible(content, weakShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLSelectOptionAccessible(nsIDOMNode *aDOMNode,
                                                         nsIAccessible *aParent, 
                                                         nsIWeakReference *aPresShell,
                                                         nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(aDOMNode));
  *aAccessible = new nsHTMLSelectOptionAccessible(content, aPresShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLTableAccessible(nsIFrame *aFrame,
                                                  nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  *aAccessible = new nsHTMLTableAccessibleWrap(content, weakShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLTableCellAccessible(nsIFrame *aFrame,
                                                      nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  *aAccessible = new nsHTMLTableCellAccessibleWrap(content, weakShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLTextAccessible(nsIFrame *aFrame,
                                                 nsIAccessible **aAccessible)
{
  *aAccessible = nsnull;

  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  
  *aAccessible = new nsHTMLTextAccessible(content, weakShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLTextFieldAccessible(nsIFrame *aFrame,
                                                      nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  *aAccessible = new nsHTMLTextFieldAccessible(content, weakShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLLabelAccessible(nsIFrame *aFrame,
                                                  nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  *aAccessible = new nsHTMLLabelAccessible(content, weakShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLHRAccessible(nsIFrame *aFrame,
                                               nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  *aAccessible = new nsHTMLHRAccessible(content, weakShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLBRAccessible(nsIFrame *aFrame,
                                               nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  *aAccessible = new nsHTMLBRAccessible(content, weakShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

nsresult
nsAccessibilityService::CreateHTMLCaptionAccessible(nsIFrame *aFrame,
                                                    nsIAccessible **aAccessible)
{
  nsCOMPtr<nsIContent> content;
  nsCOMPtr<nsIWeakReference> weakShell;
  nsresult rv = GetInfo(aFrame, getter_AddRefs(weakShell),
                        getter_AddRefs(content));
  if (NS_FAILED(rv))
    return rv;

  *aAccessible = new nsHTMLCaptionAccessible(content, weakShell);
  if (!*aAccessible)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aAccessible);
  return NS_OK;
}

void
nsAccessibilityService::PresShellDestroyed(nsIPresShell *aPresShell)
{
  
  
  
  
  
  
  
  
  nsIDocument* doc = aPresShell->GetDocument();
  if (!doc)
    return;

  NS_LOG_ACCDOCDESTROY("presshell destroyed", doc)
  ShutdownDocAccessible(doc);
}


nsAccessible *
nsAccessibilityService::GetCachedAccessible(nsINode *aNode,
                                            nsIWeakReference *aWeakShell)
{
  nsDocAccessible *docAccessible = GetDocAccessible(aNode->GetOwnerDoc());
  return docAccessible ?
    docAccessible->GetCachedAccessible(static_cast<void*>(aNode)) : nsnull;
}




NS_IMETHODIMP
nsAccessibilityService::GetApplicationAccessible(nsIAccessible **aAccessibleApplication)
{
  NS_ENSURE_ARG_POINTER(aAccessibleApplication);

  NS_IF_ADDREF(*aAccessibleApplication = nsAccessNode::GetApplicationAccessible());
  return NS_OK;
}

NS_IMETHODIMP
nsAccessibilityService::GetAccessibleFor(nsIDOMNode *aNode,
                                         nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);

  nsCOMPtr<nsINode> node(do_QueryInterface(aNode));
  NS_IF_ADDREF(*aAccessible = GetAccessible(node));
  return NS_OK;
}

NS_IMETHODIMP
nsAccessibilityService::GetAttachedAccessibleFor(nsIDOMNode *aDOMNode,
                                                 nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG(aDOMNode);
  NS_ENSURE_ARG_POINTER(aAccessible);

  nsCOMPtr<nsINode> node(do_QueryInterface(aDOMNode));
  NS_IF_ADDREF(*aAccessible = GetAttachedAccessibleFor(node));
  return NS_OK;
}

NS_IMETHODIMP
nsAccessibilityService::GetRelevantContentNodeFor(nsIDOMNode *aNode,
                                                  nsIDOMNode **aRelevantNode)
{
  NS_ENSURE_ARG(aNode);
  NS_ENSURE_ARG_POINTER(aRelevantNode);

  nsCOMPtr<nsINode> node(do_QueryInterface(aNode));
  nsINode *relevantNode = GetRelevantContentNodeFor(node);
  CallQueryInterface(relevantNode, aRelevantNode);
  return NS_OK;
}

NS_IMETHODIMP
nsAccessibilityService::GetStringRole(PRUint32 aRole, nsAString& aString)
{
  if ( aRole >= NS_ARRAY_LENGTH(kRoleNames)) {
    aString.AssignLiteral("unknown");
    return NS_OK;
  }

  CopyUTF8toUTF16(kRoleNames[aRole], aString);
  return NS_OK;
}

NS_IMETHODIMP
nsAccessibilityService::GetStringStates(PRUint32 aStates, PRUint32 aExtraStates,
                                        nsIDOMDOMStringList **aStringStates)
{
  nsAccessibleDOMStringList *stringStates = new nsAccessibleDOMStringList();
  NS_ENSURE_TRUE(stringStates, NS_ERROR_OUT_OF_MEMORY);

  
  if (aStates & nsIAccessibleStates::STATE_UNAVAILABLE)
    stringStates->Add(NS_LITERAL_STRING("unavailable"));
  if (aStates & nsIAccessibleStates::STATE_SELECTED)
    stringStates->Add(NS_LITERAL_STRING("selected"));
  if (aStates & nsIAccessibleStates::STATE_FOCUSED)
    stringStates->Add(NS_LITERAL_STRING("focused"));
  if (aStates & nsIAccessibleStates::STATE_PRESSED)
    stringStates->Add(NS_LITERAL_STRING("pressed"));
  if (aStates & nsIAccessibleStates::STATE_CHECKED)
    stringStates->Add(NS_LITERAL_STRING("checked"));
  if (aStates & nsIAccessibleStates::STATE_MIXED)
    stringStates->Add(NS_LITERAL_STRING("mixed"));
  if (aStates & nsIAccessibleStates::STATE_READONLY)
    stringStates->Add(NS_LITERAL_STRING("readonly"));
  if (aStates & nsIAccessibleStates::STATE_HOTTRACKED)
    stringStates->Add(NS_LITERAL_STRING("hottracked"));
  if (aStates & nsIAccessibleStates::STATE_DEFAULT)
    stringStates->Add(NS_LITERAL_STRING("default"));
  if (aStates & nsIAccessibleStates::STATE_EXPANDED)
    stringStates->Add(NS_LITERAL_STRING("expanded"));
  if (aStates & nsIAccessibleStates::STATE_COLLAPSED)
    stringStates->Add(NS_LITERAL_STRING("collapsed"));
  if (aStates & nsIAccessibleStates::STATE_BUSY)
    stringStates->Add(NS_LITERAL_STRING("busy"));
  if (aStates & nsIAccessibleStates::STATE_FLOATING)
    stringStates->Add(NS_LITERAL_STRING("floating"));
  if (aStates & nsIAccessibleStates::STATE_ANIMATED)
    stringStates->Add(NS_LITERAL_STRING("animated"));
  if (aStates & nsIAccessibleStates::STATE_INVISIBLE)
    stringStates->Add(NS_LITERAL_STRING("invisible"));
  if (aStates & nsIAccessibleStates::STATE_OFFSCREEN)
    stringStates->Add(NS_LITERAL_STRING("offscreen"));
  if (aStates & nsIAccessibleStates::STATE_SIZEABLE)
    stringStates->Add(NS_LITERAL_STRING("sizeable"));
  if (aStates & nsIAccessibleStates::STATE_MOVEABLE)
    stringStates->Add(NS_LITERAL_STRING("moveable"));
  if (aStates & nsIAccessibleStates::STATE_SELFVOICING)
    stringStates->Add(NS_LITERAL_STRING("selfvoicing"));
  if (aStates & nsIAccessibleStates::STATE_FOCUSABLE)
    stringStates->Add(NS_LITERAL_STRING("focusable"));
  if (aStates & nsIAccessibleStates::STATE_SELECTABLE)
    stringStates->Add(NS_LITERAL_STRING("selectable"));
  if (aStates & nsIAccessibleStates::STATE_LINKED)
    stringStates->Add(NS_LITERAL_STRING("linked"));
  if (aStates & nsIAccessibleStates::STATE_TRAVERSED)
    stringStates->Add(NS_LITERAL_STRING("traversed"));
  if (aStates & nsIAccessibleStates::STATE_MULTISELECTABLE)
    stringStates->Add(NS_LITERAL_STRING("multiselectable"));
  if (aStates & nsIAccessibleStates::STATE_EXTSELECTABLE)
    stringStates->Add(NS_LITERAL_STRING("extselectable"));
  if (aStates & nsIAccessibleStates::STATE_PROTECTED)
    stringStates->Add(NS_LITERAL_STRING("protected"));
  if (aStates & nsIAccessibleStates::STATE_HASPOPUP)
    stringStates->Add(NS_LITERAL_STRING("haspopup"));
  if (aStates & nsIAccessibleStates::STATE_REQUIRED)
    stringStates->Add(NS_LITERAL_STRING("required"));
  if (aStates & nsIAccessibleStates::STATE_IMPORTANT)
    stringStates->Add(NS_LITERAL_STRING("important"));
  if (aStates & nsIAccessibleStates::STATE_INVALID)
    stringStates->Add(NS_LITERAL_STRING("invalid"));
  if (aStates & nsIAccessibleStates::STATE_CHECKABLE)
    stringStates->Add(NS_LITERAL_STRING("checkable"));

  
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_SUPPORTS_AUTOCOMPLETION)
    stringStates->Add(NS_LITERAL_STRING("autocompletion"));
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_DEFUNCT)
    stringStates->Add(NS_LITERAL_STRING("defunct"));
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_SELECTABLE_TEXT)
    stringStates->Add(NS_LITERAL_STRING("selectable text"));
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_EDITABLE)
    stringStates->Add(NS_LITERAL_STRING("editable"));
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_ACTIVE)
    stringStates->Add(NS_LITERAL_STRING("active"));
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_MODAL)
    stringStates->Add(NS_LITERAL_STRING("modal"));
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_MULTI_LINE)
    stringStates->Add(NS_LITERAL_STRING("multi line"));
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_HORIZONTAL)
    stringStates->Add(NS_LITERAL_STRING("horizontal"));
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_OPAQUE)
    stringStates->Add(NS_LITERAL_STRING("opaque"));
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_SINGLE_LINE)
    stringStates->Add(NS_LITERAL_STRING("single line"));
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_TRANSIENT)
    stringStates->Add(NS_LITERAL_STRING("transient"));
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_VERTICAL)
    stringStates->Add(NS_LITERAL_STRING("vertical"));
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_STALE)
    stringStates->Add(NS_LITERAL_STRING("stale"));
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_ENABLED)
    stringStates->Add(NS_LITERAL_STRING("enabled"));
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_SENSITIVE)
    stringStates->Add(NS_LITERAL_STRING("sensitive"));
  if (aExtraStates & nsIAccessibleStates::EXT_STATE_EXPANDABLE)
    stringStates->Add(NS_LITERAL_STRING("expandable"));

  
  PRUint32 stringStatesLength = 0;

  stringStates->GetLength(&stringStatesLength);
  if (!stringStatesLength)
    stringStates->Add(NS_LITERAL_STRING("unknown"));

  NS_ADDREF(*aStringStates = stringStates);
  return NS_OK;
}


NS_IMETHODIMP
nsAccessibilityService::GetStringEventType(PRUint32 aEventType,
                                           nsAString& aString)
{
  NS_ASSERTION(nsIAccessibleEvent::EVENT_LAST_ENTRY == NS_ARRAY_LENGTH(kEventTypeNames),
               "nsIAccessibleEvent constants are out of sync to kEventTypeNames");

  if (aEventType >= NS_ARRAY_LENGTH(kEventTypeNames)) {
    aString.AssignLiteral("unknown");
    return NS_OK;
  }

  CopyUTF8toUTF16(kEventTypeNames[aEventType], aString);
  return NS_OK;
}


NS_IMETHODIMP
nsAccessibilityService::GetStringRelationType(PRUint32 aRelationType,
                                              nsAString& aString)
{
  if (aRelationType >= NS_ARRAY_LENGTH(kRelationTypeNames)) {
    aString.AssignLiteral("unknown");
    return NS_OK;
  }

  CopyUTF8toUTF16(kRelationTypeNames[aRelationType], aString);
  return NS_OK;
}

NS_IMETHODIMP
nsAccessibilityService::GetAccessibleFromCache(nsIDOMNode* aNode,
                                               nsIAccessible** aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);

  
  
  
  
  
  
  nsCOMPtr<nsINode> node(do_QueryInterface(aNode));
  nsAccessible* accessible = FindAccessibleInCache(static_cast<void*>(node));
  if (!accessible) {
    nsCOMPtr<nsIDocument> document(do_QueryInterface(node));
    if (document)
      accessible = GetDocAccessibleFromCache(document);
  }

  NS_IF_ADDREF(*aAccessible = accessible);
  return NS_OK;
}


nsAccessible*
nsAccessibilityService::GetAccessibleInShell(nsIDOMNode *aNode, 
                                             nsIPresShell *aPresShell) 
{
  if (!aNode || !aPresShell)
    return nsnull;

  nsCOMPtr<nsINode> node(do_QueryInterface(aNode));
  nsCOMPtr<nsIWeakReference> weakShell(do_GetWeakReference(aPresShell));
  nsRefPtr<nsAccessible> accessible =
    GetAccessible(node, aPresShell, weakShell);
  return accessible;
}




nsAccessible *
nsAccessibilityService::GetAccessible(nsINode *aNode)
{
  if (!aNode)
    return nsnull;

  nsIPresShell *presShell = nsCoreUtils::GetPresShellFor(aNode);
  if (!presShell)
    return nsnull;

  nsCOMPtr<nsIWeakReference> weakShell(do_GetWeakReference(presShell));
  nsRefPtr<nsAccessible> accessible = GetAccessible(aNode, presShell,
                                                    weakShell);
  return accessible;
}

nsAccessible *
nsAccessibilityService::GetAccessibleInWeakShell(nsINode *aNode,
                                                 nsIWeakReference *aWeakShell) 
{
  if (!aNode || !aWeakShell)
    return nsnull;

  nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(aWeakShell));
  nsRefPtr<nsAccessible> accessible = GetAccessible(aNode, presShell,
                                                    aWeakShell);
  return accessible;
}

nsAccessible *
nsAccessibilityService::GetContainerAccessible(nsINode *aNode,
                                               PRBool aCanCreate)
{
  if (!aNode)
    return nsnull;

  nsIDocument *document = aNode->GetCurrentDoc();
  if (!document)
    return nsnull;

  nsIPresShell *presShell = document->GetShell();
  if (!presShell)
    return nsnull;

  nsINode *currNode = aNode;
  nsCOMPtr<nsIWeakReference> weakShell(do_GetWeakReference(presShell));

  nsAccessible *accessible = nsnull;
  while (!accessible && (currNode = currNode->GetNodeParent())) {
    currNode = GetAccService()->GetRelevantContentNodeFor(currNode);

    if (aCanCreate) {
      accessible = GetAccService()->GetAccessibleInWeakShell(currNode,
                                                             weakShell);

    } else {
      
      accessible = GetCachedAccessible(currNode, weakShell);
    }
  }

  return accessible;
}

nsAccessible *
nsAccessibilityService::GetAttachedAccessibleFor(nsINode *aNode)
{
  nsINode *relevantNode = GetRelevantContentNodeFor(aNode);
  if (relevantNode != aNode)
    return nsnull;

  return GetAccessible(relevantNode);
}

PRBool
nsAccessibilityService::InitAccessible(nsAccessible *aAccessible,
                                       nsRoleMapEntry *aRoleMapEntry)
{
  if (!aAccessible)
    return PR_FALSE;

  
  if (!aAccessible->Init()) {
    NS_ERROR("Failed to initialize an accessible!");

    aAccessible->Shutdown();
    return PR_FALSE;
  }

  NS_ASSERTION(aAccessible->IsInCache(),
               "Initialized accessible not in the cache!");

  aAccessible->SetRoleMapEntry(aRoleMapEntry);
  return PR_TRUE;
}

static PRBool HasRelatedContent(nsIContent *aContent)
{
  nsAutoString id;
  if (!aContent || !nsCoreUtils::GetID(aContent, id) || id.IsEmpty()) {
    return PR_FALSE;
  }

  nsIAtom *relationAttrs[] = {nsAccessibilityAtoms::aria_labelledby,
                              nsAccessibilityAtoms::aria_describedby,
                              nsAccessibilityAtoms::aria_owns,
                              nsAccessibilityAtoms::aria_controls,
                              nsAccessibilityAtoms::aria_flowto};
  if (nsCoreUtils::FindNeighbourPointingToNode(aContent, relationAttrs,
                                               NS_ARRAY_LENGTH(relationAttrs))) {
    return PR_TRUE;
  }

  nsIContent *ancestorContent = aContent;
  while ((ancestorContent = ancestorContent->GetParent()) != nsnull) {
    if (ancestorContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_activedescendant)) {
        
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

already_AddRefed<nsAccessible>
nsAccessibilityService::GetAccessible(nsINode *aNode,
                                      nsIPresShell *aPresShell,
                                      nsIWeakReference *aWeakShell,
                                      PRBool *aIsHidden)
{
  if (!aPresShell || !aWeakShell || !aNode || gIsShutdown)
    return nsnull;

  if (aIsHidden)
    *aIsHidden = PR_FALSE;

  
  nsAccessible *cachedAccessible = GetCachedAccessible(aNode, aWeakShell);
  if (cachedAccessible) {
    NS_ADDREF(cachedAccessible);
    return cachedAccessible;
  }

  

  if (aNode->IsNodeOfType(nsINode::eDOCUMENT)) {
    
    
    nsCOMPtr<nsIDocument> document(do_QueryInterface(aNode));
    nsAccessible *accessible = GetDocAccessible(document);
    NS_IF_ADDREF(accessible);
    return accessible;
  }

  
  if (!aNode->IsInDoc()) {
    NS_WARNING("Creating accessible for node with no document");
    return nsnull;
  }

  if (aNode->GetOwnerDoc() != aPresShell->GetDocument()) {
    NS_ERROR("Creating accessible for wrong pres shell");
    return nsnull;
  }

  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
  if (!content)
    return nsnull;

  
  
  
  
  nsWeakFrame weakFrame = content->GetPrimaryFrame();

  
  if (!weakFrame.GetFrame() ||
      !weakFrame.GetFrame()->GetStyleVisibility()->IsVisible()) {
    if (aIsHidden)
      *aIsHidden = PR_TRUE;

    return nsnull;
  }

  if (weakFrame.GetFrame()->GetContent() != content) {
    
    
    
    
    
    return GetAreaAccessible(weakFrame.GetFrame(), aNode, aWeakShell);
  }

  
  nsRefPtr<nsAccessible> newAcc;
  if (content->IsNodeOfType(nsINode::eTEXT)) {
    
    nsIFrame* f = weakFrame.GetFrame();
    if (f && f->IsEmpty()) {
      nsAutoString renderedWhitespace;
      f->GetRenderedText(&renderedWhitespace, nsnull, nsnull, 0, 1);
      if (renderedWhitespace.IsEmpty()) {
        
        if (aIsHidden)
          *aIsHidden = PR_TRUE;

        return nsnull;
      }
    }
    if (weakFrame.IsAlive()) {
      nsCOMPtr<nsIAccessible> newAccessible;
      weakFrame.GetFrame()->GetAccessible(getter_AddRefs(newAccessible));
      if (newAccessible) {
        newAcc = do_QueryObject(newAccessible);
        if (InitAccessible(newAcc, nsnull))
          return newAcc.forget();
        return nsnull;
      }
    }

    return nsnull;
  }

  PRBool isHTML = content->IsHTML();
  if (isHTML && content->Tag() == nsAccessibilityAtoms::map) {
    
    
    
    
    
    
    
    
    nsAutoString name;
    content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::name, name);
    if (!name.IsEmpty()) {
      if (aIsHidden)
        *aIsHidden = PR_TRUE;

      return nsnull;
    }

    newAcc = new nsHyperTextAccessibleWrap(content, aWeakShell);
    if (InitAccessible(newAcc, nsAccUtils::GetRoleMapEntry(aNode)))
      return newAcc.forget();
    return nsnull;
  }

  nsRoleMapEntry *roleMapEntry = nsAccUtils::GetRoleMapEntry(aNode);
  if (roleMapEntry && !nsCRT::strcmp(roleMapEntry->roleString, "presentation") &&
      !content->IsFocusable()) { 
    
    
    
    return nsnull;
  }

  if (weakFrame.IsAlive() && !newAcc && isHTML) {  
    PRBool tryTagNameOrFrame = PR_TRUE;

    nsIAtom *frameType = weakFrame.GetFrame()->GetType();

    PRBool partOfHTMLTable =
      frameType == nsAccessibilityAtoms::tableCaptionFrame ||
      frameType == nsAccessibilityAtoms::tableCellFrame ||
      frameType == nsAccessibilityAtoms::tableRowGroupFrame ||
      frameType == nsAccessibilityAtoms::tableRowFrame;

    if (partOfHTMLTable) {
      
      
      
      nsIContent *tableContent = content;
      while ((tableContent = tableContent->GetParent()) != nsnull) {
        nsIFrame *tableFrame = tableContent->GetPrimaryFrame();
        if (!tableFrame)
          continue;

        if (tableFrame->GetType() == nsAccessibilityAtoms::tableOuterFrame) {
          nsAccessible *tableAccessible =
            GetAccessibleInWeakShell(tableContent, aWeakShell);

          if (tableAccessible) {
            if (!roleMapEntry) {
              PRUint32 role = nsAccUtils::Role(tableAccessible);
              if (role != nsIAccessibleRole::ROLE_TABLE &&
                  role != nsIAccessibleRole::ROLE_TREE_TABLE) {
                
                
                roleMapEntry = &nsARIAMap::gEmptyRoleMap;
              }
            }

            break;
          }

#ifdef DEBUG
          nsRoleMapEntry *tableRoleMapEntry =
            nsAccUtils::GetRoleMapEntry(tableContent);
          NS_ASSERTION(tableRoleMapEntry &&
                       !nsCRT::strcmp(tableRoleMapEntry->roleString, "presentation"),
                       "No accessible for parent table and it didn't have role of presentation");
#endif

          if (!roleMapEntry && !content->IsFocusable()) {
            
            
            
            
            return nsnull;
          }

          
          tryTagNameOrFrame = PR_FALSE;
          break;
        }

        if (tableContent->Tag() == nsAccessibilityAtoms::table) {
          
          
          tryTagNameOrFrame = PR_FALSE;
          break;
        }
      }

      if (!tableContent)
        tryTagNameOrFrame = PR_FALSE;
    }

    if (roleMapEntry) {
      
      
      if ((!partOfHTMLTable || !tryTagNameOrFrame) &&
          frameType != nsAccessibilityAtoms::tableOuterFrame) {

        if (roleMapEntry->role == nsIAccessibleRole::ROLE_TABLE ||
            roleMapEntry->role == nsIAccessibleRole::ROLE_TREE_TABLE) {
          newAcc = new nsARIAGridAccessibleWrap(content, aWeakShell);

        } else if (roleMapEntry->role == nsIAccessibleRole::ROLE_GRID_CELL ||
            roleMapEntry->role == nsIAccessibleRole::ROLE_ROWHEADER ||
            roleMapEntry->role == nsIAccessibleRole::ROLE_COLUMNHEADER) {
          newAcc = new nsARIAGridCellAccessibleWrap(content, aWeakShell);
        }
      }
    }

    if (!newAcc && tryTagNameOrFrame) {
      
      
      
      
      
      newAcc = CreateHTMLAccessibleByMarkup(weakFrame.GetFrame(), aWeakShell,
                                            aNode);

      if (!newAcc) {
        
        
        
        
        
        
        nsIFrame* f = weakFrame.GetFrame();
        if (!f) {
          f = aPresShell->GetRealPrimaryFrameFor(content);
        }
        if (f->GetType() == nsAccessibilityAtoms::tableCaptionFrame &&
           f->GetRect().IsEmpty()) {
          
          
          if (aIsHidden)
            *aIsHidden = PR_TRUE;

          return nsnull;
        }

        
        nsCOMPtr<nsIAccessible> newAccessible;
        f->GetAccessible(getter_AddRefs(newAccessible));
        newAcc = do_QueryObject(newAccessible);
      }
    }
  }

  if (!newAcc) {
    
    
    newAcc = CreateAccessibleByType(content, aWeakShell);
  }

  if (!newAcc) {
    
    if (content->GetNameSpaceID() == kNameSpaceID_SVG &&
        content->Tag() == nsAccessibilityAtoms::svg) {
      newAcc = new nsEnumRoleAccessible(content, aWeakShell,
                                        nsIAccessibleRole::ROLE_DIAGRAM);
    }
    else if (content->GetNameSpaceID() == kNameSpaceID_MathML &&
             content->Tag() == nsAccessibilityAtoms::math) {
      newAcc = new nsEnumRoleAccessible(content, aWeakShell,
                                        nsIAccessibleRole::ROLE_EQUATION);
    }
  }

  if (!newAcc) {
    newAcc = CreateAccessibleForDeckChild(weakFrame.GetFrame(), content,
                                          aWeakShell);
  }

  
  
  
  
  if (!newAcc && content->Tag() != nsAccessibilityAtoms::body && content->GetParent() && 
      ((weakFrame.GetFrame() && weakFrame.GetFrame()->IsFocusable()) ||
       (isHTML && nsCoreUtils::HasClickListener(content)) ||
       HasUniversalAriaProperty(content) || roleMapEntry ||
       HasRelatedContent(content) || nsCoreUtils::IsXLink(content))) {
    
    
    
    if (isHTML) {
      
      newAcc = new nsHyperTextAccessibleWrap(content, aWeakShell);
    }
    else {  
      
      newAcc = new nsAccessibleWrap(content, aWeakShell);
    }
  }

  if (InitAccessible(newAcc, roleMapEntry))
    return newAcc.forget();
  return nsnull;
}




PRBool
nsAccessibilityService::Init()
{
  
  if (!nsAccDocManager::Init())
    return PR_FALSE;

  
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService)
    return PR_FALSE;

  observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);

  
  nsAccessNodeWrap::InitAccessibility();

  gIsShutdown = PR_FALSE;
  return PR_TRUE;
}

void
nsAccessibilityService::Shutdown()
{
  
  nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
  if (observerService)
    observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);

  
  nsAccDocManager::Shutdown();

  
  
  
  

  NS_ASSERTION(!gIsShutdown, "Accessibility was shutdown already");

  gIsShutdown = PR_TRUE;

  nsAccessNodeWrap::ShutdownAccessibility();
}

PRBool
nsAccessibilityService::HasUniversalAriaProperty(nsIContent *aContent)
{
  
  
  return nsAccUtils::HasDefinedARIAToken(aContent, nsAccessibilityAtoms::aria_atomic) ||
         nsAccUtils::HasDefinedARIAToken(aContent, nsAccessibilityAtoms::aria_busy) ||
         aContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_controls) ||
         aContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_describedby) ||
         aContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_disabled) ||
         nsAccUtils::HasDefinedARIAToken(aContent, nsAccessibilityAtoms::aria_dropeffect) ||
         aContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_flowto) ||
         nsAccUtils::HasDefinedARIAToken(aContent, nsAccessibilityAtoms::aria_grabbed) ||
         nsAccUtils::HasDefinedARIAToken(aContent, nsAccessibilityAtoms::aria_haspopup) ||
         
         nsAccUtils::HasDefinedARIAToken(aContent, nsAccessibilityAtoms::aria_invalid) ||
         aContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_label) ||
         aContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::aria_labelledby) ||
         nsAccUtils::HasDefinedARIAToken(aContent, nsAccessibilityAtoms::aria_live) ||
         nsAccUtils::HasDefinedARIAToken(aContent, nsAccessibilityAtoms::aria_owns) ||
         nsAccUtils::HasDefinedARIAToken(aContent, nsAccessibilityAtoms::aria_relevant);
}

nsINode *
nsAccessibilityService::GetRelevantContentNodeFor(nsINode *aNode)
{
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  

  if (!aNode)
    return nsnull;

  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
  if (content) {
    
    nsIContent *bindingParent;
    nsCOMArray<nsIContent> bindingsStack;

    for (bindingParent = content->GetBindingParent(); bindingParent != nsnull &&
         bindingParent != bindingParent->GetBindingParent();
         bindingParent = bindingParent->GetBindingParent()) {
      bindingsStack.AppendObject(bindingParent);
    }

    PRInt32 bindingsCount = bindingsStack.Count();
    for (PRInt32 index = bindingsCount - 1; index >= 0 ; index--) {
      bindingParent = bindingsStack[index];

      
      
      nsCOMPtr<nsIWeakReference> weakShell =
        nsCoreUtils::GetWeakShellFor(bindingParent);

      
      
      nsRefPtr<nsAccessible> accessible =
        CreateAccessibleByType(bindingParent, weakShell);

      if (accessible) {
        if (!accessible->GetAllowsAnonChildAccessibles())
          return bindingParent;
      }
    }
  }

  return aNode;
}

already_AddRefed<nsAccessible>
nsAccessibilityService::GetAreaAccessible(nsIFrame *aImageFrame,
                                          nsINode *aAreaNode,
                                          nsIWeakReference *aWeakShell)
{
  
  nsIImageFrame *imageFrame = do_QueryFrame(aImageFrame);
  if (!imageFrame)
    return nsnull;

  nsCOMPtr<nsIDOMHTMLAreaElement> areaElmt = do_QueryInterface(aAreaNode);
  if (!areaElmt)
    return nsnull;

  
  
  nsRefPtr<nsAccessible> imageAcc =
    GetCachedAccessible(aImageFrame->GetContent(), aWeakShell);
  if (!imageAcc) {
    nsCOMPtr<nsIAccessible> imageAccessible;
    CreateHTMLImageAccessible(aImageFrame,
                              getter_AddRefs(imageAccessible));

    imageAcc = do_QueryObject(imageAccessible);
    if (!InitAccessible(imageAcc, nsnull))
      return nsnull;
  }

  
  
  imageAcc->EnsureChildren();

  nsAccessible *cachedAreaAcc = GetCachedAccessible(aAreaNode, aWeakShell);
  NS_IF_ADDREF(cachedAreaAcc);
  return cachedAreaAcc;
}

already_AddRefed<nsAccessible>
nsAccessibilityService::CreateAccessibleByType(nsIContent *aContent,
                                               nsIWeakReference *aWeakShell)
{
  nsCOMPtr<nsIAccessibleProvider> accessibleProvider(do_QueryInterface(aContent));
  if (!accessibleProvider)
    return nsnull;

  PRInt32 type;
  nsresult rv = accessibleProvider->GetAccessibleType(&type);
  if (NS_FAILED(rv))
    return nsnull;

  nsRefPtr<nsAccessible> accessible;
  if (type == nsIAccessibleProvider::OuterDoc) {
    accessible = new nsOuterDocAccessible(aContent, aWeakShell);
    return accessible.forget();
  }

  switch (type)
  {
#ifdef MOZ_XUL
    case nsIAccessibleProvider::NoAccessible:
      return nsnull;

    
    case nsIAccessibleProvider::XULAlert:
      accessible = new nsXULAlertAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULButton:
      accessible = new nsXULButtonAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULCheckbox:
      accessible = new nsXULCheckboxAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULColorPicker:
      accessible = new nsXULColorPickerAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULColorPickerTile:
      accessible = new nsXULColorPickerTileAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULCombobox:
      accessible = new nsXULComboboxAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULDropmarker:
      accessible = new nsXULDropmarkerAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULGroupbox:
      accessible = new nsXULGroupboxAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULImage:
    {
      
      if (!aContent->HasAttr(kNameSpaceID_None,
                             nsAccessibilityAtoms::tooltiptext))
        return nsnull;

      accessible = new nsHTMLImageAccessibleWrap(aContent, aWeakShell);
      break;
    }
    case nsIAccessibleProvider::XULLink:
      accessible = new nsXULLinkAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULListbox:
      accessible = new nsXULListboxAccessibleWrap(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULListCell:
      accessible = new nsXULListCellAccessibleWrap(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULListHead:
      accessible = new nsXULColumnsAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULListHeader:
      accessible = new nsXULColumnItemAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULListitem:
      accessible = new nsXULListitemAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULMenubar:
      accessible = new nsXULMenubarAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULMenuitem:
      accessible = new nsXULMenuitemAccessibleWrap(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULMenupopup:
    {
#ifdef MOZ_ACCESSIBILITY_ATK
      
      
      
      
      
      nsIContent *parent = aContent->GetParent();
      if (parent && parent->NodeInfo()->Equals(nsAccessibilityAtoms::menu,
                                               kNameSpaceID_XUL))
        return nsnull;
#endif
      accessible = new nsXULMenupopupAccessible(aContent, aWeakShell);
      break;
    }
    case nsIAccessibleProvider::XULMenuSeparator:
      accessible = new nsXULMenuSeparatorAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULPane:
      accessible = new nsEnumRoleAccessible(aContent, aWeakShell,
                                            nsIAccessibleRole::ROLE_PANE);
      break;
    case nsIAccessibleProvider::XULProgressMeter:
      accessible = new nsXULProgressMeterAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULStatusBar:
      accessible = new nsXULStatusBarAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULScale:
      accessible = new nsXULSliderAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULRadioButton:
      accessible = new nsXULRadioButtonAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULRadioGroup:
      accessible = new nsXULRadioGroupAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULTab:
      accessible = new nsXULTabAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULTabs:
      accessible = new nsXULTabsAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULTabpanels:
      accessible = new nsXULTabpanelsAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULText:
      accessible = new nsXULTextAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULTextBox:
      accessible = new nsXULTextFieldAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULThumb:
      accessible = new nsXULThumbAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULTree:
      return CreateAccessibleForXULTree(aContent, aWeakShell);

    case nsIAccessibleProvider::XULTreeColumns:
      accessible = new nsXULTreeColumnsAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULTreeColumnItem:
      accessible = new nsXULColumnItemAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULToolbar:
      accessible = new nsXULToolbarAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULToolbarSeparator:
      accessible = new nsXULToolbarSeparatorAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULTooltip:
      accessible = new nsXULTooltipAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XULToolbarButton:
      accessible = new nsXULToolbarButtonAccessible(aContent, aWeakShell);
      break;
#endif 

#ifndef DISABLE_XFORMS_HOOKS
    
    case nsIAccessibleProvider::XFormsContainer:
      accessible = new nsXFormsContainerAccessible(aContent, aWeakShell);
      break;

    case nsIAccessibleProvider::XFormsLabel:
      accessible = new nsXFormsLabelAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsOutput:
      accessible = new nsXFormsOutputAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsTrigger:
      accessible = new nsXFormsTriggerAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsInput:
      accessible = new nsXFormsInputAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsInputBoolean:
      accessible = new nsXFormsInputBooleanAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsInputDate:
      accessible = new nsXFormsInputDateAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsSecret:
      accessible = new nsXFormsSecretAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsSliderRange:
      accessible = new nsXFormsRangeAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsSelect:
      accessible = new nsXFormsSelectAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsChoices:
      accessible = new nsXFormsChoicesAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsSelectFull:
      accessible = new nsXFormsSelectFullAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsItemCheckgroup:
      accessible = new nsXFormsItemCheckgroupAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsItemRadiogroup:
      accessible = new nsXFormsItemRadiogroupAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsSelectCombobox:
      accessible = new nsXFormsSelectComboboxAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsItemCombobox:
      accessible = new nsXFormsItemComboboxAccessible(aContent, aWeakShell);
      break;

    case nsIAccessibleProvider::XFormsDropmarkerWidget:
      accessible = new nsXFormsDropmarkerWidgetAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsCalendarWidget:
      accessible = new nsXFormsCalendarWidgetAccessible(aContent, aWeakShell);
      break;
    case nsIAccessibleProvider::XFormsComboboxPopupWidget:
      accessible = new nsXFormsComboboxPopupWidgetAccessible(aContent, aWeakShell);
      break;
#endif

    default:
      return nsnull;
  }

  return accessible.forget();
}




nsAccessible*
nsAccessibilityService::AddNativeRootAccessible(void* aAtkAccessible)
 {
#ifdef MOZ_ACCESSIBILITY_ATK
  nsApplicationAccessible* applicationAcc =
    nsAccessNode::GetApplicationAccessible();
  if (!applicationAcc)
    return nsnull;

  nsNativeRootAccessibleWrap* nativeRootAcc =
     new nsNativeRootAccessibleWrap((AtkObject*)aAtkAccessible);
  if (!nativeRootAcc)
    return nsnull;

  if (applicationAcc->AppendChild(nativeRootAcc))
    return nativeRootAcc;
#endif

  return nsnull;
 }

void
nsAccessibilityService::RemoveNativeRootAccessible(nsAccessible* aAccessible)
{
#ifdef MOZ_ACCESSIBILITY_ATK
  nsApplicationAccessible* applicationAcc =
    nsAccessNode::GetApplicationAccessible();

  if (applicationAcc)
    applicationAcc->RemoveChild(aAccessible);
#endif
}


nsresult
nsAccessibilityService::InvalidateSubtreeFor(nsIPresShell *aShell,
                                             nsIContent *aChangeContent,
                                             PRUint32 aChangeType)
{
  NS_ASSERTION(aChangeType == nsIAccessibilityService::FRAME_SIGNIFICANT_CHANGE ||
               aChangeType == nsIAccessibilityService::FRAME_SHOW ||
               aChangeType == nsIAccessibilityService::FRAME_HIDE ||
               aChangeType == nsIAccessibilityService::NODE_SIGNIFICANT_CHANGE ||
               aChangeType == nsIAccessibilityService::NODE_APPEND ||
               aChangeType == nsIAccessibilityService::NODE_REMOVE,
               "Incorrect aEvent passed in");

  NS_ENSURE_ARG_POINTER(aShell);

  nsDocAccessible *docAccessible = GetDocAccessible(aShell->GetDocument());
  if (docAccessible)
    docAccessible->InvalidateCacheSubtree(aChangeContent, aChangeType);

  return NS_OK;
}








nsresult
NS_GetAccessibilityService(nsIAccessibilityService** aResult)
{
  NS_ENSURE_TRUE(aResult, NS_ERROR_NULL_POINTER);
  *aResult = nsnull;
 
  if (nsAccessibilityService::gAccessibilityService) {
    NS_ADDREF(*aResult = nsAccessibilityService::gAccessibilityService);
    return NS_OK;
  }

  nsRefPtr<nsAccessibilityService> service = new nsAccessibilityService();
  NS_ENSURE_TRUE(service, NS_ERROR_OUT_OF_MEMORY);

  if (!service->Init()) {
    service->Shutdown();
    return NS_ERROR_FAILURE;
  }

  nsAccessibilityService::gAccessibilityService = service;
  NS_ADDREF(*aResult = service);

  return NS_OK;
}




already_AddRefed<nsAccessible>
nsAccessibilityService::CreateAccessibleForDeckChild(nsIFrame* aFrame,
                                                     nsIContent *aContent,
                                                     nsIWeakReference *aWeakShell)
{
  nsRefPtr<nsAccessible> accessible;

  if (aFrame->GetType() == nsAccessibilityAtoms::boxFrame ||
      aFrame->GetType() == nsAccessibilityAtoms::scrollFrame) {

    nsIFrame* parentFrame = aFrame->GetParent();
    if (parentFrame && parentFrame->GetType() == nsAccessibilityAtoms::deckFrame) {
      
      
      nsCOMPtr<nsIContent> parentContent = parentFrame->GetContent();
#ifdef MOZ_XUL
      if (parentContent->NodeInfo()->Equals(nsAccessibilityAtoms::tabpanels,
                                            kNameSpaceID_XUL)) {
        accessible = new nsXULTabpanelAccessible(aContent, aWeakShell);
      } else
#endif
        accessible =
          new nsEnumRoleAccessible(aContent, aWeakShell,
                                   nsIAccessibleRole::ROLE_PROPERTYPAGE);
    }
  }

  return accessible.forget();
}

#ifdef MOZ_XUL
already_AddRefed<nsAccessible>
nsAccessibilityService::CreateAccessibleForXULTree(nsIContent *aContent,
                                                   nsIWeakReference *aWeakShell)
{
  nsCOMPtr<nsITreeBoxObject> treeBoxObj = nsCoreUtils::GetTreeBoxObject(aContent);
  if (!treeBoxObj)
    return nsnull;

  nsCOMPtr<nsITreeColumns> treeColumns;
  treeBoxObj->GetColumns(getter_AddRefs(treeColumns));
  if (!treeColumns)
    return nsnull;

  nsRefPtr<nsAccessible> accessible;

  PRInt32 count = 0;
  treeColumns->GetCount(&count);
  if (count == 1) 
    accessible = new nsXULTreeAccessible(aContent, aWeakShell);
  else 
    accessible = new nsXULTreeGridAccessibleWrap(aContent, aWeakShell);

  return accessible.forget();
}
#endif
