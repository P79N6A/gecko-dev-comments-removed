




#include "nsAccessibilityService.h"


#include "Accessible-inl.h"
#include "ApplicationAccessibleWrap.h"
#include "ARIAGridAccessibleWrap.h"
#include "DocAccessible-inl.h"
#include "FocusManager.h"
#include "HTMLCanvasAccessible.h"
#include "HTMLElementAccessibles.h"
#include "HTMLImageMapAccessible.h"
#include "HTMLLinkAccessible.h"
#include "HTMLListAccessible.h"
#include "HTMLSelectAccessible.h"
#include "HTMLTableAccessibleWrap.h"
#include "HyperTextAccessibleWrap.h"
#include "nsAccessiblePivot.h"
#include "nsAccUtils.h"
#include "nsARIAMap.h"
#include "nsEventShell.h"
#include "nsIAccessibleProvider.h"
#include "OuterDocAccessible.h"
#include "Platform.h"
#include "Role.h"
#include "RootAccessibleWrap.h"
#include "States.h"
#include "Statistics.h"
#include "TextLeafAccessibleWrap.h"

#ifdef MOZ_ACCESSIBILITY_ATK
#include "AtkSocketAccessible.h"
#endif

#ifdef XP_WIN
#include "mozilla/a11y/Compatibility.h"
#include "HTMLWin32ObjectAccessible.h"
#endif

#ifdef A11Y_LOG
#include "Logging.h"
#endif

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

#include "nsIDOMDocument.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMXULElement.h"
#include "nsImageFrame.h"
#include "nsIObserverService.h"
#include "nsLayoutUtils.h"
#include "nsNPAPIPluginInstance.h"
#include "nsObjectFrame.h"
#include "nsSVGPathGeometryFrame.h"
#include "nsTreeBodyFrame.h"
#include "nsTreeColumns.h"
#include "nsTreeUtils.h"
#include "mozilla/dom/Element.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/Util.h"
#include "nsDeckFrame.h"

#ifdef MOZ_XUL
#include "XULAlertAccessible.h"
#include "XULColorPickerAccessible.h"
#include "XULComboboxAccessible.h"
#include "XULElementAccessibles.h"
#include "XULFormControlAccessible.h"
#include "XULListboxAccessibleWrap.h"
#include "XULMenuAccessibleWrap.h"
#include "XULSliderAccessible.h"
#include "XULTabAccessible.h"
#include "XULTreeGridAccessibleWrap.h"
#endif

using namespace mozilla;
using namespace mozilla::a11y;








static bool
MustBeAccessible(nsIContent* aContent, DocAccessible* aDocument)
{
  if (aContent->GetPrimaryFrame()->IsFocusable())
    return true;

  uint32_t attrCount = aContent->GetAttrCount();
  for (uint32_t attrIdx = 0; attrIdx < attrCount; attrIdx++) {
    const nsAttrName* attr = aContent->GetAttrNameAt(attrIdx);
    if (attr->NamespaceEquals(kNameSpaceID_None)) {
      nsIAtom* attrAtom = attr->Atom();
      nsDependentAtomString attrStr(attrAtom);
      if (!StringBeginsWith(attrStr, NS_LITERAL_STRING("aria-")))
        continue; 

      
      uint8_t attrFlags = nsAccUtils::GetAttributeCharacteristics(attrAtom);
      if ((attrFlags & ATTR_GLOBAL) && (!(attrFlags & ATTR_VALTOKEN) ||
           nsAccUtils::HasDefinedARIAToken(aContent, attrAtom))) {
        return true;
      }
    }
  }

  
  
  nsAutoString id;
  if (nsCoreUtils::GetID(aContent, id) && !id.IsEmpty())
    return aDocument->IsDependentID(id);

  return false;
}





nsAccessibilityService *nsAccessibilityService::gAccessibilityService = nullptr;
ApplicationAccessible* nsAccessibilityService::gApplicationAccessible = nullptr;
bool nsAccessibilityService::gIsShutdown = true;

nsAccessibilityService::nsAccessibilityService() :
  DocManager(), FocusManager()
{
}

nsAccessibilityService::~nsAccessibilityService()
{
  NS_ASSERTION(gIsShutdown, "Accessibility wasn't shutdown!");
  gAccessibilityService = nullptr;
}




NS_IMPL_ISUPPORTS_INHERITED4(nsAccessibilityService,
                             DocManager,
                             nsIAccessibilityService,
                             nsIAccessibleRetrieval,
                             nsIObserver,
                             nsISelectionListener) 




NS_IMETHODIMP
nsAccessibilityService::Observe(nsISupports *aSubject, const char *aTopic,
                         const PRUnichar *aData)
{
  if (!nsCRT::strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID))
    Shutdown();

  return NS_OK;
}


void
nsAccessibilityService::NotifyOfAnchorJumpTo(nsIContent* aTargetNode)
{
  nsIDocument* documentNode = aTargetNode->GetCurrentDoc();
  if (documentNode) {
    DocAccessible* document = GetDocAccessible(documentNode);
    if (document)
      document->SetAnchorJump(aTargetNode);
  }
}


void
nsAccessibilityService::FireAccessibleEvent(uint32_t aEvent,
                                            Accessible* aTarget)
{
  nsEventShell::FireEvent(aEvent, aTarget);
}




Accessible*
nsAccessibilityService::GetRootDocumentAccessible(nsIPresShell* aPresShell,
                                                  bool aCanCreate)
{
  nsIPresShell* ps = aPresShell;
  nsIDocument* documentNode = aPresShell->GetDocument();
  if (documentNode) {
    nsCOMPtr<nsISupports> container = documentNode->GetContainer();
    nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryInterface(container));
    if (treeItem) {
      nsCOMPtr<nsIDocShellTreeItem> rootTreeItem;
      treeItem->GetRootTreeItem(getter_AddRefs(rootTreeItem));
      if (treeItem != rootTreeItem) {
        nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(rootTreeItem));
        ps = docShell->GetPresShell();
      }

      return aCanCreate ? GetDocAccessible(ps) : ps->GetDocAccessible();
    }
  }
  return nullptr;
}

#ifdef XP_WIN
static StaticAutoPtr<nsTArray<nsCOMPtr<nsIContent> > > sPendingPlugins;
static StaticAutoPtr<nsTArray<nsCOMPtr<nsITimer> > > sPluginTimers;

class PluginTimerCallBack MOZ_FINAL : public nsITimerCallback
{
public:
  PluginTimerCallBack(nsIContent* aContent) : mContent(aContent) {}

  NS_DECL_ISUPPORTS

  NS_IMETHODIMP Notify(nsITimer* aTimer) MOZ_FINAL
  {
    nsIPresShell* ps = mContent->OwnerDoc()->GetShell();
    if (ps) {
      DocAccessible* doc = ps->GetDocAccessible();
      if (doc) {
        
        
        doc->RecreateAccessible(mContent);
        sPluginTimers->RemoveElement(aTimer);
        return NS_OK;
      }
    }

    
    
    sPendingPlugins->RemoveElement(mContent);
    sPluginTimers->RemoveElement(aTimer);
    return NS_OK;
  }

private:
  nsCOMPtr<nsIContent> mContent;
};

NS_IMPL_ISUPPORTS1(PluginTimerCallBack, nsITimerCallback)
#endif

already_AddRefed<Accessible>
nsAccessibilityService::CreatePluginAccessible(nsObjectFrame* aFrame,
                                               nsIContent* aContent,
                                               Accessible* aContext)
{
  
  
  if (aFrame->GetRect().IsEmpty())
    return nullptr;

#if defined(XP_WIN) || defined(MOZ_ACCESSIBILITY_ATK)
  nsRefPtr<nsNPAPIPluginInstance> pluginInstance;
  if (NS_SUCCEEDED(aFrame->GetPluginInstance(getter_AddRefs(pluginInstance))) &&
      pluginInstance) {
#ifdef XP_WIN
    if (!sPendingPlugins->Contains(aContent) &&
        (Preferences::GetBool("accessibility.delay_plugins") ||
         Compatibility::IsJAWS() || Compatibility::IsWE())) {
      nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID);
      nsRefPtr<PluginTimerCallBack> cb = new PluginTimerCallBack(aContent);
      timer->InitWithCallback(cb, Preferences::GetUint("accessibility.delay_plugin_time"),
                              nsITimer::TYPE_ONE_SHOT);
      sPluginTimers->AppendElement(timer);
      sPendingPlugins->AppendElement(aContent);
      return nullptr;
    }

    
    
    
    sPendingPlugins->RemoveElement(aContent);

    
    HWND pluginPort = nullptr;
    aFrame->GetPluginPort(&pluginPort);

    Accessible* accessible =
      new HTMLWin32ObjectOwnerAccessible(aContent, aContext->Document(),
                                         pluginPort);
    NS_ADDREF(accessible);
    return accessible;

#elif MOZ_ACCESSIBILITY_ATK
    if (!AtkSocketAccessible::gCanEmbed)
      return nullptr;

    
    
    nsCString plugId;
    nsresult rv = pluginInstance->GetValueFromPlugin(
      NPPVpluginNativeAccessibleAtkPlugId, &plugId);
    if (NS_SUCCEEDED(rv) && !plugId.IsEmpty()) {
      AtkSocketAccessible* socketAccessible =
        new AtkSocketAccessible(aContent, aContext->Document(), plugId);

      NS_ADDREF(socketAccessible);
      return socketAccessible;
    }
#endif
  }
#endif

  return nullptr;
}

void
nsAccessibilityService::DeckPanelSwitched(nsIPresShell* aPresShell,
                                          nsIContent* aDeckNode,
                                          nsIFrame* aPrevBoxFrame,
                                          nsIFrame* aCurrentBoxFrame)
{
  
  
  DocAccessible* document = GetDocAccessible(aPresShell);
  if (!document || document->HasAccessible(aDeckNode))
    return;

  if (aPrevBoxFrame) {
    nsIContent* panelNode = aPrevBoxFrame->GetContent();
#ifdef A11Y_LOG
    if (logging::IsEnabled(logging::eTree)) {
      logging::MsgBegin("TREE", "deck panel unselected");
      logging::Node("container", panelNode);
      logging::Node("content", aDeckNode);
      logging::MsgEnd();
    }
#endif

    document->ContentRemoved(aDeckNode, panelNode);
  }

  if (aCurrentBoxFrame) {
    nsIContent* panelNode = aCurrentBoxFrame->GetContent();
#ifdef A11Y_LOG
    if (logging::IsEnabled(logging::eTree)) {
      logging::MsgBegin("TREE", "deck panel selected");
      logging::Node("container", panelNode);
      logging::Node("content", aDeckNode);
      logging::MsgEnd();
    }
#endif

    document->ContentInserted(aDeckNode, panelNode, panelNode->GetNextSibling());
  }
}

void
nsAccessibilityService::ContentRangeInserted(nsIPresShell* aPresShell,
                                             nsIContent* aContainer,
                                             nsIContent* aStartChild,
                                             nsIContent* aEndChild)
{
#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eTree)) {
    logging::MsgBegin("TREE", "content inserted");
    logging::Node("container", aContainer);
    for (nsIContent* child = aStartChild; child != aEndChild;
         child = child->GetNextSibling()) {
      logging::Node("content", child);
    }
    logging::MsgEnd();
    logging::Stack();
  }
#endif

  DocAccessible* docAccessible = GetDocAccessible(aPresShell);
  if (docAccessible)
    docAccessible->ContentInserted(aContainer, aStartChild, aEndChild);
}

void
nsAccessibilityService::ContentRemoved(nsIPresShell* aPresShell,
                                       nsIContent* aContainer,
                                       nsIContent* aChild)
{
#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eTree)) {
    logging::MsgBegin("TREE", "content removed");
    logging::Node("container", aContainer);
    logging::Node("content", aChild);
    logging::MsgEnd();
    logging::Stack();
  }
#endif

  DocAccessible* docAccessible = GetDocAccessible(aPresShell);
  if (docAccessible)
    docAccessible->ContentRemoved(aContainer, aChild);
}

void
nsAccessibilityService::UpdateText(nsIPresShell* aPresShell,
                                   nsIContent* aContent)
{
  DocAccessible* document = GetDocAccessible(aPresShell);
  if (document)
    document->UpdateText(aContent);
}

void
nsAccessibilityService::TreeViewChanged(nsIPresShell* aPresShell,
                                        nsIContent* aContent,
                                        nsITreeView* aView)
{
  DocAccessible* document = GetDocAccessible(aPresShell);
  if (document) {
    Accessible* accessible = document->GetAccessible(aContent);
    if (accessible) {
      XULTreeAccessible* treeAcc = accessible->AsXULTree();
      if (treeAcc) 
        treeAcc->TreeViewChanged(aView);
    }
  }
}

void
nsAccessibilityService::UpdateListBullet(nsIPresShell* aPresShell,
                                         nsIContent* aHTMLListItemContent,
                                         bool aHasBullet)
{
  DocAccessible* document = GetDocAccessible(aPresShell);
  if (document) {
    Accessible* accessible = document->GetAccessible(aHTMLListItemContent);
    if (accessible) {
      HTMLLIAccessible* listItem = accessible->AsHTMLListItem();
      if (listItem)
        listItem->UpdateBullet(aHasBullet);
    }
  }
}

void
nsAccessibilityService::UpdateImageMap(nsImageFrame* aImageFrame)
{
  nsIPresShell* presShell = aImageFrame->PresContext()->PresShell();
  DocAccessible* document = GetDocAccessible(presShell);
  if (document) {
    Accessible* accessible =
      document->GetAccessible(aImageFrame->GetContent());
    if (accessible) {
      HTMLImageMapAccessible* imageMap = accessible->AsImageMap();
      if (imageMap) {
        imageMap->UpdateChildAreas();
        return;
      }

      
      
      RecreateAccessible(presShell, aImageFrame->GetContent());
    }
  }
}

void
nsAccessibilityService::UpdateLabelValue(nsIPresShell* aPresShell,
                                         nsIContent* aLabelElm,
                                         const nsString& aNewValue)
{
  DocAccessible* document = GetDocAccessible(aPresShell);
  if (document) {
    Accessible* accessible = document->GetAccessible(aLabelElm);
    if (accessible) {
      XULLabelAccessible* xulLabel = accessible->AsXULLabel();
      NS_ASSERTION(xulLabel,
                   "UpdateLabelValue was called for wrong accessible!");
      if (xulLabel)
        xulLabel->UpdateLabelValue(aNewValue);
    }
  }
}

void
nsAccessibilityService::PresShellActivated(nsIPresShell* aPresShell)
{
  DocAccessible* document = aPresShell->GetDocAccessible();
  if (document) {
    RootAccessible* rootDocument = document->RootAccessible();
    NS_ASSERTION(rootDocument, "Entirely broken tree: no root document!");
    if (rootDocument)
      rootDocument->DocumentActivated(document);
  }
}

void
nsAccessibilityService::RecreateAccessible(nsIPresShell* aPresShell,
                                           nsIContent* aContent)
{
  DocAccessible* document = GetDocAccessible(aPresShell);
  if (document)
    document->RecreateAccessible(aContent);
}




NS_IMETHODIMP
nsAccessibilityService::GetApplicationAccessible(nsIAccessible** aAccessibleApplication)
{
  NS_ENSURE_ARG_POINTER(aAccessibleApplication);

  NS_IF_ADDREF(*aAccessibleApplication = ApplicationAcc());

  return NS_OK;
}

NS_IMETHODIMP
nsAccessibilityService::GetAccessibleFor(nsIDOMNode *aNode,
                                         nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nullptr;
  if (!aNode)
    return NS_OK;

  nsCOMPtr<nsINode> node(do_QueryInterface(aNode));
  if (!node)
    return NS_ERROR_INVALID_ARG;

  DocAccessible* document = GetDocAccessible(node->OwnerDoc());
  if (document)
    NS_IF_ADDREF(*aAccessible = document->GetAccessible(node));

  return NS_OK;
}

NS_IMETHODIMP
nsAccessibilityService::GetStringRole(uint32_t aRole, nsAString& aString)
{
#define ROLE(geckoRole, stringRole, atkRole, \
             macRole, msaaRole, ia2Role, nameRule) \
  case roles::geckoRole: \
    CopyUTF8toUTF16(stringRole, aString); \
    return NS_OK;

  switch (aRole) {
#include "RoleMap.h"
    default:
      aString.AssignLiteral("unknown");
      return NS_OK;
  }

#undef ROLE
}

NS_IMETHODIMP
nsAccessibilityService::GetStringStates(uint32_t aState, uint32_t aExtraState,
                                        nsIDOMDOMStringList **aStringStates)
{
  nsAccessibleDOMStringList* stringStates = new nsAccessibleDOMStringList();
  NS_ENSURE_TRUE(stringStates, NS_ERROR_OUT_OF_MEMORY);

  uint64_t state = nsAccUtils::To64State(aState, aExtraState);

  
  if (state & states::UNAVAILABLE)
    stringStates->Add(NS_LITERAL_STRING("unavailable"));
  if (state & states::SELECTED)
    stringStates->Add(NS_LITERAL_STRING("selected"));
  if (state & states::FOCUSED)
    stringStates->Add(NS_LITERAL_STRING("focused"));
  if (state & states::PRESSED)
    stringStates->Add(NS_LITERAL_STRING("pressed"));
  if (state & states::CHECKED)
    stringStates->Add(NS_LITERAL_STRING("checked"));
  if (state & states::MIXED)
    stringStates->Add(NS_LITERAL_STRING("mixed"));
  if (state & states::READONLY)
    stringStates->Add(NS_LITERAL_STRING("readonly"));
  if (state & states::HOTTRACKED)
    stringStates->Add(NS_LITERAL_STRING("hottracked"));
  if (state & states::DEFAULT)
    stringStates->Add(NS_LITERAL_STRING("default"));
  if (state & states::EXPANDED)
    stringStates->Add(NS_LITERAL_STRING("expanded"));
  if (state & states::COLLAPSED)
    stringStates->Add(NS_LITERAL_STRING("collapsed"));
  if (state & states::BUSY)
    stringStates->Add(NS_LITERAL_STRING("busy"));
  if (state & states::FLOATING)
    stringStates->Add(NS_LITERAL_STRING("floating"));
  if (state & states::ANIMATED)
    stringStates->Add(NS_LITERAL_STRING("animated"));
  if (state & states::INVISIBLE)
    stringStates->Add(NS_LITERAL_STRING("invisible"));
  if (state & states::OFFSCREEN)
    stringStates->Add(NS_LITERAL_STRING("offscreen"));
  if (state & states::SIZEABLE)
    stringStates->Add(NS_LITERAL_STRING("sizeable"));
  if (state & states::MOVEABLE)
    stringStates->Add(NS_LITERAL_STRING("moveable"));
  if (state & states::SELFVOICING)
    stringStates->Add(NS_LITERAL_STRING("selfvoicing"));
  if (state & states::FOCUSABLE)
    stringStates->Add(NS_LITERAL_STRING("focusable"));
  if (state & states::SELECTABLE)
    stringStates->Add(NS_LITERAL_STRING("selectable"));
  if (state & states::LINKED)
    stringStates->Add(NS_LITERAL_STRING("linked"));
  if (state & states::TRAVERSED)
    stringStates->Add(NS_LITERAL_STRING("traversed"));
  if (state & states::MULTISELECTABLE)
    stringStates->Add(NS_LITERAL_STRING("multiselectable"));
  if (state & states::EXTSELECTABLE)
    stringStates->Add(NS_LITERAL_STRING("extselectable"));
  if (state & states::PROTECTED)
    stringStates->Add(NS_LITERAL_STRING("protected"));
  if (state & states::HASPOPUP)
    stringStates->Add(NS_LITERAL_STRING("haspopup"));
  if (state & states::REQUIRED)
    stringStates->Add(NS_LITERAL_STRING("required"));
  if (state & states::ALERT)
    stringStates->Add(NS_LITERAL_STRING("alert"));
  if (state & states::INVALID)
    stringStates->Add(NS_LITERAL_STRING("invalid"));
  if (state & states::CHECKABLE)
    stringStates->Add(NS_LITERAL_STRING("checkable"));

  
  if (state & states::SUPPORTS_AUTOCOMPLETION)
    stringStates->Add(NS_LITERAL_STRING("autocompletion"));
  if (state & states::DEFUNCT)
    stringStates->Add(NS_LITERAL_STRING("defunct"));
  if (state & states::SELECTABLE_TEXT)
    stringStates->Add(NS_LITERAL_STRING("selectable text"));
  if (state & states::EDITABLE)
    stringStates->Add(NS_LITERAL_STRING("editable"));
  if (state & states::ACTIVE)
    stringStates->Add(NS_LITERAL_STRING("active"));
  if (state & states::MODAL)
    stringStates->Add(NS_LITERAL_STRING("modal"));
  if (state & states::MULTI_LINE)
    stringStates->Add(NS_LITERAL_STRING("multi line"));
  if (state & states::HORIZONTAL)
    stringStates->Add(NS_LITERAL_STRING("horizontal"));
  if (state & states::OPAQUE1)
    stringStates->Add(NS_LITERAL_STRING("opaque"));
  if (state & states::SINGLE_LINE)
    stringStates->Add(NS_LITERAL_STRING("single line"));
  if (state & states::TRANSIENT)
    stringStates->Add(NS_LITERAL_STRING("transient"));
  if (state & states::VERTICAL)
    stringStates->Add(NS_LITERAL_STRING("vertical"));
  if (state & states::STALE)
    stringStates->Add(NS_LITERAL_STRING("stale"));
  if (state & states::ENABLED)
    stringStates->Add(NS_LITERAL_STRING("enabled"));
  if (state & states::SENSITIVE)
    stringStates->Add(NS_LITERAL_STRING("sensitive"));
  if (state & states::EXPANDABLE)
    stringStates->Add(NS_LITERAL_STRING("expandable"));

  
  uint32_t stringStatesLength = 0;
  stringStates->GetLength(&stringStatesLength);
  if (!stringStatesLength)
    stringStates->Add(NS_LITERAL_STRING("unknown"));

  NS_ADDREF(*aStringStates = stringStates);
  return NS_OK;
}


NS_IMETHODIMP
nsAccessibilityService::GetStringEventType(uint32_t aEventType,
                                           nsAString& aString)
{
  NS_ASSERTION(nsIAccessibleEvent::EVENT_LAST_ENTRY == ArrayLength(kEventTypeNames),
               "nsIAccessibleEvent constants are out of sync to kEventTypeNames");

  if (aEventType >= ArrayLength(kEventTypeNames)) {
    aString.AssignLiteral("unknown");
    return NS_OK;
  }

  CopyUTF8toUTF16(kEventTypeNames[aEventType], aString);
  return NS_OK;
}


NS_IMETHODIMP
nsAccessibilityService::GetStringRelationType(uint32_t aRelationType,
                                              nsAString& aString)
{
  if (aRelationType >= ArrayLength(kRelationTypeNames)) {
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
  *aAccessible = nullptr;
  if (!aNode)
    return NS_OK;

  nsCOMPtr<nsINode> node(do_QueryInterface(aNode));
  if (!node)
    return NS_ERROR_INVALID_ARG;

  
  
  
  
  
  
  Accessible* accessible = FindAccessibleInCache(node);
  if (!accessible) {
    nsCOMPtr<nsIDocument> document(do_QueryInterface(node));
    if (document)
      accessible = GetExistingDocAccessible(document);
  }

  NS_IF_ADDREF(*aAccessible = accessible);
  return NS_OK;
}

NS_IMETHODIMP
nsAccessibilityService::CreateAccessiblePivot(nsIAccessible* aRoot,
                                              nsIAccessiblePivot** aPivot)
{
  NS_ENSURE_ARG_POINTER(aPivot);
  NS_ENSURE_ARG(aRoot);
  *aPivot = nullptr;

  nsRefPtr<Accessible> accessibleRoot(do_QueryObject(aRoot));
  NS_ENSURE_TRUE(accessibleRoot, NS_ERROR_INVALID_ARG);

  nsAccessiblePivot* pivot = new nsAccessiblePivot(accessibleRoot);
  NS_ADDREF(*aPivot = pivot);

  return NS_OK;
}

NS_IMETHODIMP
nsAccessibilityService::SetLogging(const nsACString& aModules)
{
#ifdef A11Y_LOG
  logging::Enable(PromiseFlatCString(aModules));
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsAccessibilityService::IsLogged(const nsAString& aModule, bool* aIsLogged)
{
  NS_ENSURE_ARG_POINTER(aIsLogged);
  *aIsLogged = false;

#ifdef A11Y_LOG
  *aIsLogged = logging::IsEnabled(aModule);
#endif

  return NS_OK;
}




Accessible*
nsAccessibilityService::GetOrCreateAccessible(nsINode* aNode,
                                              Accessible* aContext,
                                              bool* aIsSubtreeHidden)
{
  NS_PRECONDITION(aContext && aNode && !gIsShutdown,
                  "Maybe let'd do a crash? Oh, yes, baby!");

  if (aIsSubtreeHidden)
    *aIsSubtreeHidden = false;

  DocAccessible* document = aContext->Document();

  
  
  
  Accessible* cachedAccessible = document->GetAccessible(aNode);
  if (cachedAccessible)
    return cachedAccessible;

  

  if (aNode->IsNodeOfType(nsINode::eDOCUMENT)) {
    
    
    nsCOMPtr<nsIDocument> document(do_QueryInterface(aNode));
    return GetDocAccessible(document);
  }

  
  if (!aNode->IsInDoc()) {
    NS_WARNING("Creating accessible for node with no document");
    return nullptr;
  }

  if (aNode->OwnerDoc() != document->DocumentNode()) {
    NS_ERROR("Creating accessible for wrong document");
    return nullptr;
  }

  if (!aNode->IsContent())
    return nullptr;

  nsIContent* content = aNode->AsContent();
  nsIFrame* frame = content->GetPrimaryFrame();

  
  
  if (!frame || !frame->StyleVisibility()->IsVisible()) {
    if (aIsSubtreeHidden && !frame)
      *aIsSubtreeHidden = true;

    return nullptr;
  }

  if (frame->GetContent() != content) {
    
    
    
    
    
    

#ifdef DEBUG
  nsImageFrame* imageFrame = do_QueryFrame(frame);
  NS_ASSERTION(imageFrame && content->IsHTML() && content->Tag() == nsGkAtoms::area,
               "Unknown case of not main content for the frame!");
#endif
    return nullptr;
  }

#ifdef DEBUG
  nsImageFrame* imageFrame = do_QueryFrame(frame);
  NS_ASSERTION(!imageFrame || !content->IsHTML() || content->Tag() != nsGkAtoms::area,
               "Image map manages the area accessible creation!");
#endif

  
  nsRefPtr<Accessible> newAcc;

  
  if (content->IsNodeOfType(nsINode::eTEXT)) {
    nsAutoString text;
    frame->GetRenderedText(&text, nullptr, nullptr, 0, UINT32_MAX);
    if (text.IsEmpty()) {
      if (aIsSubtreeHidden)
        *aIsSubtreeHidden = true;

      return nullptr;
    }

    newAcc = CreateAccessibleByFrameType(frame, content, aContext);
    if (document->BindToDocument(newAcc, nullptr)) {
      newAcc->AsTextLeaf()->SetText(text);
      return newAcc;
    }

    return nullptr;
  }

  bool isHTML = content->IsHTML();
  if (isHTML && content->Tag() == nsGkAtoms::map) {
    
    
    
    
    
    
    
    
    if (nsLayoutUtils::GetAllInFlowRectsUnion(frame,
                                              frame->GetParent()).IsEmpty()) {
      if (aIsSubtreeHidden)
        *aIsSubtreeHidden = true;

      return nullptr;
    }

    newAcc = new HyperTextAccessibleWrap(content, document);
    if (document->BindToDocument(newAcc, aria::GetRoleMap(aNode)))
      return newAcc;
    return nullptr;
  }

  nsRoleMapEntry* roleMapEntry = aria::GetRoleMap(aNode);

  
  
  
  if (roleMapEntry && roleMapEntry->Is(nsGkAtoms::presentation)) {
    if (!MustBeAccessible(content, document))
      return nullptr;

    roleMapEntry = nullptr;
  }

  if (!newAcc && isHTML) {  
    if (roleMapEntry) {
      
      
      if ((roleMapEntry->accTypes & eTableCell)) {
        if (aContext->IsTableRow() &&
            (frame->AccessibleType() != eHTMLTableCellType ||
             aContext->GetContent() != content->GetParent())) {
          newAcc = new ARIAGridCellAccessibleWrap(content, document);
        }

      } else if ((roleMapEntry->IsOfType(eTable)) &&
                 frame->AccessibleType() != eHTMLTableType) {
        newAcc = new ARIAGridAccessibleWrap(content, document);
      }
    }

    if (!newAcc) {
      
      
      newAcc = CreateHTMLAccessibleByMarkup(frame, content, aContext);

      
      if (!newAcc)
        newAcc = CreateAccessibleByFrameType(frame, content, aContext);

      
      
      if (!roleMapEntry && newAcc) {
        if (frame->AccessibleType() == eHTMLTableRowType) {
          nsRoleMapEntry* contextRoleMap = aContext->ARIARoleMap();
          if (contextRoleMap && !(contextRoleMap->IsOfType(eTable)))
            roleMapEntry = &nsARIAMap::gEmptyRoleMap;

        } else if (frame->AccessibleType() == eHTMLTableCellType &&
                   aContext->ARIARoleMap() == &nsARIAMap::gEmptyRoleMap) {
          roleMapEntry = &nsARIAMap::gEmptyRoleMap;

        } else if (content->Tag() == nsGkAtoms::dt ||
                   content->Tag() == nsGkAtoms::li ||
                   content->Tag() == nsGkAtoms::dd ||
                   frame->AccessibleType() == eHTMLLiType) {
          nsRoleMapEntry* contextRoleMap = aContext->ARIARoleMap();
          if (contextRoleMap && !(contextRoleMap->IsOfType(eList)))
            roleMapEntry = &nsARIAMap::gEmptyRoleMap;
        }
      }
    }
  }

  
  if (!newAcc && content->IsXUL()) {
    
    if (!aContext->IsXULTabpanels()) {
      nsDeckFrame* deckFrame = do_QueryFrame(frame->GetParent());
      if (deckFrame && deckFrame->GetSelectedBox() != frame) {
        if (aIsSubtreeHidden)
          *aIsSubtreeHidden = true;

        return nullptr;
      }
    }

    
    
    newAcc = CreateAccessibleByType(content, document);

    
    
    if (!newAcc && aContext->IsXULTabpanels() &&
        content->GetParent() == aContext->GetContent()) {
      nsIAtom* frameType = frame->GetType();
      if (frameType == nsGkAtoms::boxFrame ||
          frameType == nsGkAtoms::scrollFrame) {
        newAcc = new XULTabpanelAccessible(content, document);
      }
    }
  }

  if (!newAcc) {
    if (content->IsSVG()) {
      nsSVGPathGeometryFrame* pathGeometryFrame = do_QueryFrame(frame);
      if (pathGeometryFrame) {
        
        
        
        newAcc = new EnumRoleAccessible(content, document, roles::GRAPHIC);
      } else if (content->Tag() == nsGkAtoms::svg) {
        newAcc = new EnumRoleAccessible(content, document, roles::DIAGRAM);
      }
    } else if (content->IsMathML(nsGkAtoms::math)) {
      newAcc = new EnumRoleAccessible(content, document, roles::EQUATION);
    }
  }

  
  
  
  
  if (!newAcc && content->Tag() != nsGkAtoms::body && content->GetParent() &&
      (roleMapEntry || MustBeAccessible(content, document) ||
       (isHTML && nsCoreUtils::HasClickListener(content)))) {
    
    
    
    if (isHTML) {
      
      newAcc = new HyperTextAccessibleWrap(content, document);
    } else {  
      
      newAcc = new AccessibleWrap(content, document);
    }
  }

  return document->BindToDocument(newAcc, roleMapEntry) ? newAcc : nullptr;
}




bool
nsAccessibilityService::Init()
{
  
  if (!DocManager::Init())
    return false;

  
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService)
    return false;

  observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);

  static const PRUnichar kInitIndicator[] = { '1', 0 };
  observerService->NotifyObservers(nullptr, "a11y-init-or-shutdown", kInitIndicator);

#ifdef A11Y_LOG
  logging::CheckEnv();
#endif

  gApplicationAccessible = new ApplicationAccessibleWrap();
  NS_ADDREF(gApplicationAccessible); 

#ifdef MOZ_CRASHREPORTER
  CrashReporter::
    AnnotateCrashReport(NS_LITERAL_CSTRING("Accessibility"),
                        NS_LITERAL_CSTRING("Active"));
#endif

#ifdef XP_WIN
  sPendingPlugins = new nsTArray<nsCOMPtr<nsIContent> >;
  sPluginTimers = new nsTArray<nsCOMPtr<nsITimer> >;
#endif

  gIsShutdown = false;

  
  PlatformInit();

  return true;
}

void
nsAccessibilityService::Shutdown()
{
  
  nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
  if (observerService) {
    observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);

    static const PRUnichar kShutdownIndicator[] = { '0', 0 };
    observerService->NotifyObservers(nullptr, "a11y-init-or-shutdown", kShutdownIndicator);
  }

  
  DocManager::Shutdown();

  SelectionManager::Shutdown();

#ifdef XP_WIN
  sPendingPlugins = nullptr;

  uint32_t timerCount = sPluginTimers->Length();
  for (uint32_t i = 0; i < timerCount; i++)
    sPluginTimers->ElementAt(i)->Cancel();

  sPluginTimers = nullptr;
#endif

  
  
  
  

  NS_ASSERTION(!gIsShutdown, "Accessibility was shutdown already");

  gIsShutdown = true;

  PlatformShutdown();
  gApplicationAccessible->Shutdown();
  NS_RELEASE(gApplicationAccessible);
  gApplicationAccessible = nullptr;
}

already_AddRefed<Accessible>
nsAccessibilityService::CreateAccessibleByType(nsIContent* aContent,
                                               DocAccessible* aDoc)
{
  nsCOMPtr<nsIAccessibleProvider> accessibleProvider(do_QueryInterface(aContent));
  if (!accessibleProvider)
    return nullptr;

  int32_t type;
  nsresult rv = accessibleProvider->GetAccessibleType(&type);
  if (NS_FAILED(rv))
    return nullptr;

  if (type == nsIAccessibleProvider::OuterDoc) {
    Accessible* accessible = new OuterDocAccessible(aContent, aDoc);
    NS_ADDREF(accessible);
    return accessible;
  }

  Accessible* accessible = nullptr;
  switch (type)
  {
#ifdef MOZ_XUL
    case nsIAccessibleProvider::NoAccessible:
      return nullptr;

    
    case nsIAccessibleProvider::XULAlert:
      accessible = new XULAlertAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULButton:
      accessible = new XULButtonAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULCheckbox:
      accessible = new XULCheckboxAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULColorPicker:
      accessible = new XULColorPickerAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULColorPickerTile:
      accessible = new XULColorPickerTileAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULCombobox:
      accessible = new XULComboboxAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULTabpanels:
      accessible = new XULTabpanelsAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULDropmarker:
      accessible = new XULDropmarkerAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULGroupbox:
      accessible = new XULGroupboxAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULImage:
    {
      
      if (!aContent->HasAttr(kNameSpaceID_None,
                             nsGkAtoms::tooltiptext))
        return nullptr;

      accessible = new ImageAccessibleWrap(aContent, aDoc);
      break;

    }
    case nsIAccessibleProvider::XULLink:
      accessible = new XULLinkAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULListbox:
      accessible = new XULListboxAccessibleWrap(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULListCell:
      accessible = new XULListCellAccessibleWrap(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULListHead:
      accessible = new XULColumAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULListHeader:
      accessible = new XULColumnItemAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULListitem:
      accessible = new XULListitemAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULMenubar:
      accessible = new XULMenubarAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULMenuitem:
      accessible = new XULMenuitemAccessibleWrap(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULMenupopup:
    {
#ifdef MOZ_ACCESSIBILITY_ATK
      
      
      
      
      
      nsIContent *parent = aContent->GetParent();
      if (parent && parent->NodeInfo()->Equals(nsGkAtoms::menu,
                                               kNameSpaceID_XUL))
        return nullptr;
#endif
      accessible = new XULMenupopupAccessible(aContent, aDoc);
      break;

    }
    case nsIAccessibleProvider::XULMenuSeparator:
      accessible = new XULMenuSeparatorAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULPane:
      accessible = new EnumRoleAccessible(aContent, aDoc, roles::PANE);
      break;

    case nsIAccessibleProvider::XULProgressMeter:
      accessible = new XULProgressMeterAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULStatusBar:
      accessible = new XULStatusBarAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULScale:
      accessible = new XULSliderAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULRadioButton:
      accessible = new XULRadioButtonAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULRadioGroup:
      accessible = new XULRadioGroupAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULTab:
      accessible = new XULTabAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULTabs:
      accessible = new XULTabsAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULText:
      accessible = new XULLabelAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULTextBox:
      accessible = new XULTextFieldAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULThumb:
      accessible = new XULThumbAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULTree:
      return CreateAccessibleForXULTree(aContent, aDoc);

    case nsIAccessibleProvider::XULTreeColumns:
      accessible = new XULTreeColumAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULTreeColumnItem:
      accessible = new XULColumnItemAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULToolbar:
      accessible = new XULToolbarAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULToolbarSeparator:
      accessible = new XULToolbarSeparatorAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULTooltip:
      accessible = new XULTooltipAccessible(aContent, aDoc);
      break;

    case nsIAccessibleProvider::XULToolbarButton:
      accessible = new XULToolbarButtonAccessible(aContent, aDoc);
      break;

#endif 

    default:
      return nullptr;
  }

  NS_IF_ADDREF(accessible);
  return accessible;
}

already_AddRefed<Accessible>
nsAccessibilityService::CreateHTMLAccessibleByMarkup(nsIFrame* aFrame,
                                                     nsIContent* aContent,
                                                     Accessible* aContext)
{
  DocAccessible* document = aContext->Document();
  if (aContext->IsTableRow()) {
    if (nsCoreUtils::IsHTMLTableHeader(aContent) &&
        aContext->GetContent() == aContent->GetParent()) {
      Accessible* accessible = new HTMLTableHeaderCellAccessibleWrap(aContent,
                                                                     document);
      NS_ADDREF(accessible);
      return accessible;
    }

    return nullptr;
  }

  
  nsIAtom* tag = aContent->Tag();
  if (tag == nsGkAtoms::figcaption) {
    Accessible* accessible = new HTMLFigcaptionAccessible(aContent, document);
    NS_ADDREF(accessible);
    return accessible;
  }

  if (tag == nsGkAtoms::figure) {
    Accessible* accessible = new HTMLFigureAccessible(aContent, document);
    NS_ADDREF(accessible);
    return accessible;
  }

  if (tag == nsGkAtoms::legend) {
    Accessible* accessible = new HTMLLegendAccessible(aContent, document);
    NS_ADDREF(accessible);
    return accessible;
  }

  if (tag == nsGkAtoms::option) {
    Accessible* accessible = new HTMLSelectOptionAccessible(aContent, document);
    NS_ADDREF(accessible);
    return accessible;
  }

  if (tag == nsGkAtoms::optgroup) {
    Accessible* accessible =
      new HTMLSelectOptGroupAccessible(aContent, document);
    NS_ADDREF(accessible);
    return accessible;
  }

  if (tag == nsGkAtoms::ul || tag == nsGkAtoms::ol ||
      tag == nsGkAtoms::dl) {
    Accessible* accessible = new HTMLListAccessible(aContent, document);
    NS_ADDREF(accessible);
    return accessible;
  }

  if (tag == nsGkAtoms::a) {
    
    
    nsRoleMapEntry* roleMapEntry = aria::GetRoleMap(aContent);
    if (roleMapEntry && roleMapEntry->role != roles::NOTHING &&
        roleMapEntry->role != roles::LINK) {
      Accessible* accessible = new HyperTextAccessibleWrap(aContent, document);
      NS_ADDREF(accessible);
      return accessible;
    }

    Accessible* accessible = new HTMLLinkAccessible(aContent, document);
    NS_ADDREF(accessible);
    return accessible;
  }

  if (aContext->IsList()) {
    
    
    
    if (aContext->GetContent() == aContent->GetParent()) {
      if (tag == nsGkAtoms::dt || tag == nsGkAtoms::li) {
        Accessible* accessible = new HTMLLIAccessible(aContent, document);
        NS_ADDREF(accessible);
        return accessible;
      }

      if (tag == nsGkAtoms::dd) {
        Accessible* accessible = new HyperTextAccessibleWrap(aContent, document);
        NS_ADDREF(accessible);
        return accessible;
      }
    }

    return nullptr;
  }

  if (tag == nsGkAtoms::abbr ||
      tag == nsGkAtoms::acronym ||
      tag == nsGkAtoms::blockquote ||
      tag == nsGkAtoms::form ||
      tag == nsGkAtoms::h1 ||
      tag == nsGkAtoms::h2 ||
      tag == nsGkAtoms::h3 ||
      tag == nsGkAtoms::h4 ||
      tag == nsGkAtoms::h5 ||
      tag == nsGkAtoms::h6 ||
      tag == nsGkAtoms::q) {
    Accessible* accessible = new HyperTextAccessibleWrap(aContent, document);
    NS_ADDREF(accessible);
    return accessible;
  }

  if (tag == nsGkAtoms::output) {
    Accessible* accessible = new HTMLOutputAccessible(aContent, document);
    NS_ADDREF(accessible);
    return accessible;
  }

  if (tag == nsGkAtoms::progress) {
    Accessible* accessible =
      new HTMLProgressMeterAccessible(aContent, document);
    NS_ADDREF(accessible);
    return accessible;
  }

  return nullptr;
 }

already_AddRefed<Accessible>
nsAccessibilityService::CreateAccessibleByFrameType(nsIFrame* aFrame,
                                                    nsIContent* aContent,
                                                    Accessible* aContext)
{
  DocAccessible* document = aContext->Document();

  nsRefPtr<Accessible> newAcc;
  switch (aFrame->AccessibleType()) {
    case eNoType:
      return nullptr;
    case eHTMLBRType:
      newAcc = new HTMLBRAccessible(aContent, document);
      break;
    case eHTMLButtonType:
      newAcc = new HTMLButtonAccessible(aContent, document);
      break;
    case eHTMLCanvasType:
      newAcc = new HTMLCanvasAccessible(aContent, document);
      break;
    case eHTMLCaptionType:
      if (aContext->IsTable() &&
          aContext->GetContent() == aContent->GetParent()) {
        newAcc = new HTMLCaptionAccessible(aContent, document);
      }
      break;
    case eHTMLCheckboxType:
      newAcc = new HTMLCheckboxAccessible(aContent, document);
      break;
    case eHTMLComboboxType:
      newAcc = new HTMLComboboxAccessible(aContent, document);
      break;
    case eHTMLFileInputType:
      newAcc = new HTMLFileInputAccessible(aContent, document);
      break;
    case eHTMLGroupboxType:
      newAcc = new HTMLGroupboxAccessible(aContent, document);
      break;
    case eHTMLHRType:
      newAcc = new HTMLHRAccessible(aContent, document);
      break;
    case eHTMLImageMapType:
      newAcc = new HTMLImageMapAccessible(aContent, document);
      break;
    case eHTMLLabelType:
      newAcc = new HTMLLabelAccessible(aContent, document);
      break;
    case eHTMLLiType:
      if (aContext->IsList() &&
          aContext->GetContent() == aContent->GetParent()) {
        newAcc = new HTMLLIAccessible(aContent, document);
      }
      break;
    case eHTMLSelectListType:
      newAcc = new HTMLSelectListAccessible(aContent, document);
      break;
    case eHTMLMediaType:
      newAcc = new EnumRoleAccessible(aContent, document, roles::GROUPING);
      break;
    case eHTMLRadioButtonType:
      newAcc = new HTMLRadioButtonAccessible(aContent, document);
      break;
    case eHTMLTableType:
      newAcc = new HTMLTableAccessibleWrap(aContent, document);
      break;
    case eHTMLTableCellType:
      
      
      
      if (aContext->IsHTMLTableRow() || aContext->IsHTMLTable())
        newAcc = new HTMLTableCellAccessibleWrap(aContent, document);
      break;

    case eHTMLTableRowType: {
      
      
      if (aContext->IsTable()) {
        nsIContent* parentContent = aContent->GetParent();
        nsIFrame* parentFrame = parentContent->GetPrimaryFrame();
        if (parentFrame->GetType() == nsGkAtoms::tableRowGroupFrame) {
          parentContent = parentContent->GetParent();
          parentFrame = parentContent->GetPrimaryFrame();
        }

        if (parentFrame->GetType() == nsGkAtoms::tableOuterFrame &&
            aContext->GetContent() == parentContent) {
          newAcc = new HTMLTableRowAccessible(aContent, document);
        }
      }
      break;
    }
    case eHTMLTextFieldType:
      newAcc = new HTMLTextFieldAccessible(aContent, document);
      break;
    case eHyperTextType:
      if (aContent->Tag() != nsGkAtoms::dt && aContent->Tag() != nsGkAtoms::dd)
        newAcc = new HyperTextAccessibleWrap(aContent, document);
      break;

    case eImageType:
      newAcc = new ImageAccessibleWrap(aContent, document);
      break;
    case eOuterDocType:
      newAcc = new OuterDocAccessible(aContent, document);
      break;
    case ePluginType: {
      nsObjectFrame* objectFrame = do_QueryFrame(aFrame);
      newAcc = CreatePluginAccessible(objectFrame, aContent, aContext);
      break;
    }
    case eTextLeafType:
      newAcc = new TextLeafAccessibleWrap(aContent, document);
      break;
    default:
      MOZ_ASSERT(false);
      break;
  }

  return newAcc.forget();
}




Accessible*
nsAccessibilityService::AddNativeRootAccessible(void* aAtkAccessible)
{
#ifdef MOZ_ACCESSIBILITY_ATK
  ApplicationAccessible* applicationAcc = ApplicationAcc();
  if (!applicationAcc)
    return nullptr;

  GtkWindowAccessible* nativeWnd =
    new GtkWindowAccessible(static_cast<AtkObject*>(aAtkAccessible));

  if (applicationAcc->AppendChild(nativeWnd))
    return nativeWnd;
#endif

  return nullptr;
}

void
nsAccessibilityService::RemoveNativeRootAccessible(Accessible* aAccessible)
{
#ifdef MOZ_ACCESSIBILITY_ATK
  ApplicationAccessible* applicationAcc = ApplicationAcc();

  if (applicationAcc)
    applicationAcc->RemoveChild(aAccessible);
#endif
}








nsresult
NS_GetAccessibilityService(nsIAccessibilityService** aResult)
{
  NS_ENSURE_TRUE(aResult, NS_ERROR_NULL_POINTER);
  *aResult = nullptr;
 
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

  statistics::A11yInitialized();

  nsAccessibilityService::gAccessibilityService = service;
  NS_ADDREF(*aResult = service);

  return NS_OK;
}




#ifdef MOZ_XUL
already_AddRefed<Accessible>
nsAccessibilityService::CreateAccessibleForXULTree(nsIContent* aContent,
                                                   DocAccessible* aDoc)
{
  nsIContent* child = nsTreeUtils::GetDescendantChild(aContent,
                                                      nsGkAtoms::treechildren);
  if (!child)
    return nullptr;

  nsTreeBodyFrame* treeFrame = do_QueryFrame(child->GetPrimaryFrame());
  if (!treeFrame)
    return nullptr;

  nsRefPtr<nsTreeColumns> treeCols = treeFrame->Columns();
  int32_t count = 0;
  treeCols->GetCount(&count);

  
  if (count == 1) {
    Accessible* accessible = new XULTreeAccessible(aContent, aDoc, treeFrame);
    NS_ADDREF(accessible);
    return accessible;
  }

  
  Accessible* accessible = new XULTreeGridAccessibleWrap(aContent, aDoc, treeFrame);
  NS_ADDREF(accessible);
  return accessible;
}
#endif





namespace mozilla {
namespace a11y {

FocusManager*
FocusMgr()
{
  return nsAccessibilityService::gAccessibilityService;
}

SelectionManager*
SelectionMgr()
{
  return nsAccessibilityService::gAccessibilityService;
}

ApplicationAccessible*
ApplicationAcc()
{
  return nsAccessibilityService::gApplicationAccessible;
}

EPlatformDisabledState
PlatformDisabledState()
{
  static int disabledState = 0xff;

  if (disabledState == 0xff) {
    disabledState = Preferences::GetInt("accessibility.force_disabled", 0);
    if (disabledState < ePlatformIsForceEnabled)
      disabledState = ePlatformIsForceEnabled;
    else if (disabledState > ePlatformIsDisabled)
      disabledState = ePlatformIsDisabled;
  }

  return (EPlatformDisabledState)disabledState;
}

}
}
