

















#include "nsCOMPtr.h"
#include "nsDOMCID.h"
#include "nsError.h"
#include "nsDOMString.h"
#include "nsIDOMEvent.h"
#include "nsIAtom.h"
#include "nsIBaseWindow.h"
#include "nsIDOMAttr.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMElementCSSInlineStyle.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDocument.h"
#include "nsLayoutStylesheetCache.h"
#include "mozilla/EventListenerManager.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/EventStates.h"
#include "nsFocusManager.h"
#include "nsHTMLStyleSheet.h"
#include "nsIJSRuntimeService.h"
#include "nsNameSpaceManager.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsIPresShell.h"
#include "nsIPrincipal.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsIScriptContext.h"
#include "nsIScriptError.h"
#include "nsIScriptSecurityManager.h"
#include "nsIServiceManager.h"
#include "mozilla/css/StyleRule.h"
#include "nsIStyleSheet.h"
#include "nsIURL.h"
#include "nsViewManager.h"
#include "nsIWidget.h"
#include "nsIXULDocument.h"
#include "nsIXULTemplateBuilder.h"
#include "nsLayoutCID.h"
#include "nsContentCID.h"
#include "mozilla/dom/Event.h"
#include "nsRDFCID.h"
#include "nsStyleConsts.h"
#include "nsXPIDLString.h"
#include "nsXULControllers.h"
#include "nsIBoxObject.h"
#include "nsPIBoxObject.h"
#include "XULDocument.h"
#include "nsXULPopupListener.h"
#include "nsRuleWalker.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsCSSParser.h"
#include "ListBoxObject.h"
#include "nsContentUtils.h"
#include "nsContentList.h"
#include "mozilla/InternalMutationEvent.h"
#include "mozilla/MouseEvents.h"
#include "nsIDOMMutationEvent.h"
#include "nsPIDOMWindow.h"
#include "nsJSPrincipals.h"
#include "nsDOMAttributeMap.h"
#include "nsGkAtoms.h"
#include "nsXULContentUtils.h"
#include "nsNodeUtils.h"
#include "nsFrameLoader.h"
#include "prlog.h"
#include "rdf.h"
#include "nsIControllers.h"
#include "nsAttrValueOrString.h"
#include "nsAttrValueInlines.h"
#include "mozilla/Attributes.h"
#include "nsIController.h"
#include "nsQueryObject.h"
#include <algorithm>


#include "nsIDOMXULDocument.h"

#include "nsReadableUtils.h"
#include "nsIFrame.h"
#include "nsNodeInfoManager.h"
#include "nsXBLBinding.h"
#include "mozilla/EventDispatcher.h"
#include "mozAutoDocUpdate.h"
#include "nsIDOMXULCommandEvent.h"
#include "nsCCUncollectableMarker.h"
#include "nsICSSDeclaration.h"

#include "mozilla/dom/XULElementBinding.h"
#include "mozilla/dom/BoxObject.h"

using namespace mozilla;
using namespace mozilla::dom;

#ifdef XUL_PROTOTYPE_ATTRIBUTE_METERING
uint32_t             nsXULPrototypeAttribute::gNumElements;
uint32_t             nsXULPrototypeAttribute::gNumAttributes;
uint32_t             nsXULPrototypeAttribute::gNumCacheTests;
uint32_t             nsXULPrototypeAttribute::gNumCacheHits;
uint32_t             nsXULPrototypeAttribute::gNumCacheSets;
uint32_t             nsXULPrototypeAttribute::gNumCacheFills;
#endif

class nsXULElementTearoff final : public nsIDOMElementCSSInlineStyle,
                                  public nsIFrameLoaderOwner
{
  ~nsXULElementTearoff() {}

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXULElementTearoff,
                                           nsIDOMElementCSSInlineStyle)

  explicit nsXULElementTearoff(nsXULElement* aElement)
    : mElement(aElement)
  {
  }

  NS_IMETHOD GetStyle(nsIDOMCSSStyleDeclaration** aStyle) override
  {
    nsXULElement* element = static_cast<nsXULElement*>(mElement.get());
    NS_ADDREF(*aStyle = element->Style());
    return NS_OK;
  }
  NS_FORWARD_NSIFRAMELOADEROWNER(static_cast<nsXULElement*>(mElement.get())->)
private:
  nsCOMPtr<nsIDOMXULElement> mElement;
};

NS_IMPL_CYCLE_COLLECTION(nsXULElementTearoff, mElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXULElementTearoff)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXULElementTearoff)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXULElementTearoff)
  NS_INTERFACE_MAP_ENTRY(nsIFrameLoaderOwner)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElementCSSInlineStyle)
NS_INTERFACE_MAP_END_AGGREGATED(mElement)





nsXULElement::nsXULElement(already_AddRefed<mozilla::dom::NodeInfo> aNodeInfo)
    : nsStyledElement(aNodeInfo),
      mBindingParent(nullptr)
{
    XUL_PROTOTYPE_ATTRIBUTE_METER(gNumElements);

    
    if (IsReadWriteTextElement()) {
        AddStatesSilently(NS_EVENT_STATE_MOZ_READWRITE);
        RemoveStatesSilently(NS_EVENT_STATE_MOZ_READONLY);
    }
}

nsXULElement::~nsXULElement()
{
}

nsXULElement::nsXULSlots::nsXULSlots()
    : nsXULElement::nsDOMSlots()
{
}

nsXULElement::nsXULSlots::~nsXULSlots()
{
    NS_IF_RELEASE(mControllers); 
    if (mFrameLoader) {
        mFrameLoader->Destroy();
    }
}

void
nsXULElement::nsXULSlots::Traverse(nsCycleCollectionTraversalCallback &cb)
{
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mSlots->mFrameLoader");
    cb.NoteXPCOMChild(NS_ISUPPORTS_CAST(nsIFrameLoader*, mFrameLoader));
}

nsINode::nsSlots*
nsXULElement::CreateSlots()
{
    return new nsXULSlots();
}

void
nsXULElement::MaybeUpdatePrivateLifetime()
{
    if (AttrValueIs(kNameSpaceID_None, nsGkAtoms::windowtype,
                    NS_LITERAL_STRING("navigator:browser"),
                    eCaseMatters)) {
        return;
    }

    nsPIDOMWindow* win = OwnerDoc()->GetWindow();
    nsCOMPtr<nsIDocShell> docShell = win ? win->GetDocShell() : nullptr;
    if (docShell) {
        docShell->SetAffectPrivateSessionLifetime(false);
    }
}


already_AddRefed<nsXULElement>
nsXULElement::Create(nsXULPrototypeElement* aPrototype, mozilla::dom::NodeInfo *aNodeInfo,
                     bool aIsScriptable, bool aIsRoot)
{
    nsRefPtr<mozilla::dom::NodeInfo> ni = aNodeInfo;
    nsRefPtr<nsXULElement> element = new nsXULElement(ni.forget());
    if (element) {
        if (aPrototype->mHasIdAttribute) {
            element->SetHasID();
        }
        if (aPrototype->mHasClassAttribute) {
            element->SetFlags(NODE_MAY_HAVE_CLASS);
        }
        if (aPrototype->mHasStyleAttribute) {
            element->SetMayHaveStyle();
        }

        element->MakeHeavyweight(aPrototype);
        if (aIsScriptable) {
            
            
            
            for (uint32_t i = 0; i < aPrototype->mNumAttributes; ++i) {
                element->AddListenerFor(aPrototype->mAttributes[i].mName,
                                        true);
            }
        }

        if (aIsRoot && aPrototype->mNodeInfo->Equals(nsGkAtoms::window)) {
            for (uint32_t i = 0; i < aPrototype->mNumAttributes; ++i) {
                if (aPrototype->mAttributes[i].mName.Equals(nsGkAtoms::windowtype)) {
                    element->MaybeUpdatePrivateLifetime();
                }
            }
        }
    }

    return element.forget();
}

nsresult
nsXULElement::Create(nsXULPrototypeElement* aPrototype,
                     nsIDocument* aDocument,
                     bool aIsScriptable,
                     bool aIsRoot,
                     Element** aResult)
{
    
    NS_PRECONDITION(aPrototype != nullptr, "null ptr");
    if (! aPrototype)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResult != nullptr, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
    if (aDocument) {
        mozilla::dom::NodeInfo* ni = aPrototype->mNodeInfo;
        nodeInfo = aDocument->NodeInfoManager()->
          GetNodeInfo(ni->NameAtom(), ni->GetPrefixAtom(), ni->NamespaceID(),
                      nsIDOMNode::ELEMENT_NODE);
    }
    else {
        nodeInfo = aPrototype->mNodeInfo;
    }

    nsRefPtr<nsXULElement> element = Create(aPrototype, nodeInfo,
                                            aIsScriptable, aIsRoot);
    if (!element) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    element.forget(aResult);

    return NS_OK;
}

nsresult
NS_NewXULElement(Element** aResult, already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo)
{
    nsRefPtr<mozilla::dom::NodeInfo> ni = aNodeInfo;

    NS_PRECONDITION(ni, "need nodeinfo for non-proto Create");

    nsIDocument* doc = ni->GetDocument();
    if (doc && !doc->AllowXULXBL()) {
        return NS_ERROR_NOT_AVAILABLE;
    }

    NS_ADDREF(*aResult = new nsXULElement(ni.forget()));

    return NS_OK;
}

void
NS_TrustedNewXULElement(nsIContent** aResult,
                        already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo)
{
    nsRefPtr<mozilla::dom::NodeInfo> ni = aNodeInfo;
    NS_PRECONDITION(ni, "need nodeinfo for non-proto Create");

    
    NS_ADDREF(*aResult = new nsXULElement(ni.forget()));
}




NS_IMPL_CYCLE_COLLECTION_CLASS(nsXULElement)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXULElement,
                                                  nsStyledElement)
    {
        nsXULSlots* slots = static_cast<nsXULSlots*>(tmp->GetExistingSlots());
        if (slots) {
            slots->Traverse(cb);
        }
    }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsXULElement,
                                                nsStyledElement)
    
    tmp->ClearHasID();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(nsXULElement, nsStyledElement)
NS_IMPL_RELEASE_INHERITED(nsXULElement, nsStyledElement)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsXULElement)
    NS_INTERFACE_TABLE_INHERITED(nsXULElement, nsIDOMNode, nsIDOMElement,
                                 nsIDOMXULElement)
    NS_ELEMENT_INTERFACE_TABLE_TO_MAP_SEGUE
    NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOMElementCSSInlineStyle,
                                   new nsXULElementTearoff(this))
    NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIFrameLoaderOwner,
                                   new nsXULElementTearoff(this))
NS_INTERFACE_MAP_END_INHERITING(nsStyledElement)




nsresult
nsXULElement::Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const
{
    *aResult = nullptr;

    nsRefPtr<mozilla::dom::NodeInfo> ni = aNodeInfo;
    nsRefPtr<nsXULElement> element = new nsXULElement(ni.forget());

    
    
    
    

    

    uint32_t count = mAttrsAndChildren.AttrCount();
    nsresult rv = NS_OK;
    for (uint32_t i = 0; i < count; ++i) {
        const nsAttrName* originalName = mAttrsAndChildren.AttrNameAt(i);
        const nsAttrValue* originalValue = mAttrsAndChildren.AttrAt(i);
        nsAttrValue attrValue;

        
        if (originalValue->Type() == nsAttrValue::eCSSStyleRule) {
            nsRefPtr<css::Rule> ruleClone =
                originalValue->GetCSSStyleRuleValue()->Clone();

            nsString stringValue;
            originalValue->ToString(stringValue);

            nsRefPtr<css::StyleRule> styleRule = do_QueryObject(ruleClone);
            attrValue.SetTo(styleRule, &stringValue);
        } else {
            attrValue.SetTo(*originalValue);
        }

        if (originalName->IsAtom()) {
           rv = element->mAttrsAndChildren.SetAndTakeAttr(originalName->Atom(),
                                                          attrValue);
        } else {
            rv = element->mAttrsAndChildren.SetAndTakeAttr(originalName->NodeInfo(),
                                                           attrValue);
        }
        NS_ENSURE_SUCCESS(rv, rv);
        element->AddListenerFor(*originalName, true);
        if (originalName->Equals(nsGkAtoms::id) &&
            !originalValue->IsEmptyString()) {
            element->SetHasID();
        }
        if (originalName->Equals(nsGkAtoms::_class)) {
            element->SetFlags(NODE_MAY_HAVE_CLASS);
        }
        if (originalName->Equals(nsGkAtoms::style)) {
            element->SetMayHaveStyle();
        }
    }

    element.forget(aResult);
    return rv;
}



NS_IMETHODIMP
nsXULElement::GetElementsByAttribute(const nsAString& aAttribute,
                                     const nsAString& aValue,
                                     nsIDOMNodeList** aReturn)
{
    *aReturn = GetElementsByAttribute(aAttribute, aValue).take();
    return NS_OK;
}

already_AddRefed<nsINodeList>
nsXULElement::GetElementsByAttribute(const nsAString& aAttribute,
                                     const nsAString& aValue)
{
    nsCOMPtr<nsIAtom> attrAtom(do_GetAtom(aAttribute));
    void* attrValue = new nsString(aValue);
    nsRefPtr<nsContentList> list =
        new nsContentList(this,
                          XULDocument::MatchAttribute,
                          nsContentUtils::DestroyMatchString,
                          attrValue,
                          true,
                          attrAtom,
                          kNameSpaceID_Unknown);
    return list.forget();
}

NS_IMETHODIMP
nsXULElement::GetElementsByAttributeNS(const nsAString& aNamespaceURI,
                                       const nsAString& aAttribute,
                                       const nsAString& aValue,
                                       nsIDOMNodeList** aReturn)
{
    ErrorResult rv;
    *aReturn =
        GetElementsByAttributeNS(aNamespaceURI, aAttribute, aValue, rv).take();
    return rv.StealNSResult();
}

already_AddRefed<nsINodeList>
nsXULElement::GetElementsByAttributeNS(const nsAString& aNamespaceURI,
                                       const nsAString& aAttribute,
                                       const nsAString& aValue,
                                       ErrorResult& rv)
{
    nsCOMPtr<nsIAtom> attrAtom(do_GetAtom(aAttribute));

    int32_t nameSpaceId = kNameSpaceID_Wildcard;
    if (!aNamespaceURI.EqualsLiteral("*")) {
      rv =
        nsContentUtils::NameSpaceManager()->RegisterNameSpace(aNamespaceURI,
                                                              nameSpaceId);
      if (rv.Failed()) {
          return nullptr;
      }
    }

    void* attrValue = new nsString(aValue);
    nsRefPtr<nsContentList> list =
        new nsContentList(this,
                          XULDocument::MatchAttribute,
                          nsContentUtils::DestroyMatchString,
                          attrValue,
                          true,
                          attrAtom,
                          nameSpaceId);

    return list.forget();
}

EventListenerManager*
nsXULElement::GetEventListenerManagerForAttr(nsIAtom* aAttrName, bool* aDefer)
{
    
    
    
    nsIDocument* doc = OwnerDoc();

    nsPIDOMWindow *window;
    Element *root = doc->GetRootElement();
    if ((!root || root == this) && !mNodeInfo->Equals(nsGkAtoms::overlay) &&
        (window = doc->GetInnerWindow())) {

        nsCOMPtr<EventTarget> piTarget = do_QueryInterface(window);

        *aDefer = false;
        return piTarget->GetOrCreateListenerManager();
    }

    return nsStyledElement::GetEventListenerManagerForAttr(aAttrName, aDefer);
}


static bool IsNonList(mozilla::dom::NodeInfo* aNodeInfo)
{
  return !aNodeInfo->Equals(nsGkAtoms::tree) &&
         !aNodeInfo->Equals(nsGkAtoms::listbox) &&
         !aNodeInfo->Equals(nsGkAtoms::richlistbox);
}

bool
nsXULElement::IsFocusableInternal(int32_t *aTabIndex, bool aWithMouse)
{
  



























  
  bool shouldFocus = false;

#ifdef XP_MACOSX
  
  
  
  if (aWithMouse &&
      IsNonList(mNodeInfo) && 
      !EventStateManager::IsRemoteTarget(this))
  {
    return false;
  }
#endif

  nsCOMPtr<nsIDOMXULControlElement> xulControl = do_QueryObject(this);
  if (xulControl) {
    
    bool disabled;
    xulControl->GetDisabled(&disabled);
    if (disabled) {
      if (aTabIndex)
        *aTabIndex = -1;
      return false;
    }
    shouldFocus = true;
  }

  if (aTabIndex) {
    if (xulControl) {
      if (HasAttr(kNameSpaceID_None, nsGkAtoms::tabindex)) {
        
        
        int32_t tabIndex = 0;
        xulControl->GetTabIndex(&tabIndex);
        shouldFocus = *aTabIndex >= 0 || tabIndex >= 0;
        *aTabIndex = tabIndex;
      }
      else {
        
        
        shouldFocus = *aTabIndex >= 0;
        if (shouldFocus)
          *aTabIndex = 0;
      }

      if (shouldFocus && sTabFocusModelAppliesToXUL &&
          !(sTabFocusModel & eTabFocus_formElementsMask)) {
        
        
        
        
        
        
        
        if (IsNonList(mNodeInfo))
          *aTabIndex = -1;
      }
    }
    else {
      shouldFocus = *aTabIndex >= 0;
    }
  }

  return shouldFocus;
}

void
nsXULElement::PerformAccesskey(bool aKeyCausesActivation,
                               bool aIsTrustedEvent)
{
    nsCOMPtr<nsIContent> content(this);

    if (IsXULElement(nsGkAtoms::label)) {
        nsCOMPtr<nsIDOMElement> element;

        nsAutoString control;
        GetAttr(kNameSpaceID_None, nsGkAtoms::control, control);
        if (!control.IsEmpty()) {
            
            
            nsCOMPtr<nsIDOMDocument> domDocument =
                do_QueryInterface(content->GetUncomposedDoc());
            if (domDocument)
                domDocument->GetElementById(control, getter_AddRefs(element));
        }
        
        
        content = do_QueryInterface(element);

        if (!content)
            return;
    }

    nsIFrame* frame = content->GetPrimaryFrame();
    if (!frame || !frame->IsVisibleConsideringAncestors())
        return;

    nsXULElement* elm = FromContent(content);
    if (elm) {
        
        if (!content->IsXULElement(nsGkAtoms::toolbarbutton)) {
          nsIFocusManager* fm = nsFocusManager::GetFocusManager();
          if (fm) {
            nsCOMPtr<nsIDOMElement> element;
            
            if (content->IsXULElement(nsGkAtoms::radio)) {
              nsCOMPtr<nsIDOMXULSelectControlItemElement> controlItem(do_QueryInterface(content));
              if (controlItem) {
                bool disabled;
                controlItem->GetDisabled(&disabled);
                if (!disabled) {
                  nsCOMPtr<nsIDOMXULSelectControlElement> selectControl;
                  controlItem->GetControl(getter_AddRefs(selectControl));
                  element = do_QueryInterface(selectControl);
                }
              }
            }
            else {
              element = do_QueryInterface(content);
            }
            if (element)
              fm->SetFocus(element, nsIFocusManager::FLAG_BYKEY);
          }
        }
        if (aKeyCausesActivation &&
            !content->IsAnyOfXULElements(nsGkAtoms::textbox, nsGkAtoms::menulist)) {
          elm->ClickWithInputSource(nsIDOMMouseEvent::MOZ_SOURCE_KEYBOARD);
        }
    }
    else {
        content->PerformAccesskey(aKeyCausesActivation, aIsTrustedEvent);
    }
}



void
nsXULElement::AddListenerFor(const nsAttrName& aName,
                             bool aCompileEventHandlers)
{
    
    
    
    
    if (aName.IsAtom()) {
        nsIAtom *attr = aName.Atom();
        MaybeAddPopupListener(attr);
        if (aCompileEventHandlers &&
            nsContentUtils::IsEventAttributeName(attr, EventNameType_XUL)) {
            nsAutoString value;
            GetAttr(kNameSpaceID_None, attr, value);
            SetEventHandler(attr, value, true);
        }
    }
}

void
nsXULElement::MaybeAddPopupListener(nsIAtom* aLocalName)
{
    
    
    
    if (aLocalName == nsGkAtoms::menu ||
        aLocalName == nsGkAtoms::contextmenu ||
        
        aLocalName == nsGkAtoms::popup ||
        aLocalName == nsGkAtoms::context) {
        AddPopupListener(aLocalName);
    }
}





void
nsXULElement::UpdateEditableState(bool aNotify)
{
    
    
    nsIContent *parent = GetParent();

    SetEditableFlag(parent && parent->HasFlag(NODE_IS_EDITABLE));
    UpdateState(aNotify);
}





static inline bool XULElementsRulesInMinimalXULSheet(nsIAtom* aTag)
{
  return 
         aTag == nsGkAtoms::scrollbar ||
         aTag == nsGkAtoms::scrollbarbutton ||
         aTag == nsGkAtoms::scrollcorner ||
         aTag == nsGkAtoms::slider ||
         aTag == nsGkAtoms::thumb ||
         aTag == nsGkAtoms::scale ||
         
         aTag == nsGkAtoms::resizer ||
         aTag == nsGkAtoms::label ||
         aTag == nsGkAtoms::videocontrols;
}

#ifdef DEBUG





static inline bool
IsInVideoControls(nsXULElement* aElement)
{
  nsIContent* ancestor = aElement->GetParent();
  while (ancestor) {
    if (ancestor->NodeInfo()->Equals(nsGkAtoms::videocontrols, kNameSpaceID_XUL)) {
      return true;
    }
    ancestor = ancestor->GetParent();
  }
  return false;
}








bool
IsInFeedSubscribeLine(nsXULElement* aElement)
{
  nsIContent* bindingParent = aElement->GetBindingParent();
  if (bindingParent) {
    while (bindingParent->GetBindingParent()) {
      bindingParent = bindingParent->GetBindingParent();
    }
    nsIAtom* idAtom = bindingParent->GetID();
    if (idAtom && idAtom->Equals(NS_LITERAL_STRING("feedSubscribeLine"))) {
      return true;
    }
  }
  return false;
}
#endif

class XULInContentErrorReporter : public nsRunnable
{
public:
  explicit XULInContentErrorReporter(nsIDocument* aDocument) : mDocument(aDocument) {}

  NS_IMETHOD Run()
  {
    mDocument->WarnOnceAbout(nsIDocument::eImportXULIntoContent, false);
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDocument> mDocument;
};

nsresult
nsXULElement::BindToTree(nsIDocument* aDocument,
                         nsIContent* aParent,
                         nsIContent* aBindingParent,
                         bool aCompileEventHandlers)
{
  if (!aBindingParent &&
      aDocument &&
      !aDocument->IsLoadedAsInteractiveData() &&
      !aDocument->AllowXULXBL() &&
      !aDocument->HasWarnedAbout(nsIDocument::eImportXULIntoContent)) {
    nsContentUtils::AddScriptRunner(new XULInContentErrorReporter(aDocument));
  }

  nsresult rv = nsStyledElement::BindToTree(aDocument, aParent,
                                            aBindingParent,
                                            aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIDocument* doc = GetComposedDoc();
  if (doc &&
      !doc->LoadsFullXULStyleSheetUpFront() &&
      !doc->IsUnstyledDocument()) {

    
    
    
    
    
    
    
    
    

    if (!XULElementsRulesInMinimalXULSheet(NodeInfo()->NameAtom())) {
      doc->EnsureOnDemandBuiltInUASheet(nsLayoutStylesheetCache::XULSheet());
      
      
      
      
      
      
      
      
      
      
      NS_ASSERTION(IsInVideoControls(this) ||
                   IsInFeedSubscribeLine(this),
                   "Unexpected XUL element in non-XUL doc");
    }
  }

  if (aDocument) {
      NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
                   "Missing a script blocker!");
      
      LoadSrc();
  }

  return rv;
}

void
nsXULElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
    
    
    
    
    
    
    
    
    
    
    
    nsXULSlots* slots = static_cast<nsXULSlots*>(GetExistingDOMSlots());
    if (slots) {
        NS_IF_RELEASE(slots->mControllers);
        if (slots->mFrameLoader) {
            
            
            
            
            
            slots->mFrameLoader->Destroy();
            slots->mFrameLoader = nullptr;
        }
    }

    nsStyledElement::UnbindFromTree(aDeep, aNullParent);
}

void
nsXULElement::RemoveChildAt(uint32_t aIndex, bool aNotify)
{
    nsCOMPtr<nsIContent> oldKid = mAttrsAndChildren.GetSafeChildAt(aIndex);
    if (!oldKid) {
      return;
    }

    
    
    
    nsCOMPtr<nsIDOMXULMultiSelectControlElement> controlElement;
    nsCOMPtr<nsIListBoxObject> listBox;
    bool fireSelectionHandler = false;

    
    
    int32_t newCurrentIndex = -1;

    if (oldKid->NodeInfo()->Equals(nsGkAtoms::listitem, kNameSpaceID_XUL)) {
      
      
      
      
      controlElement = do_QueryObject(this);

      
      if (!controlElement)
        GetParentTree(getter_AddRefs(controlElement));
      nsCOMPtr<nsIDOMXULElement> xulElement(do_QueryInterface(controlElement));

      nsCOMPtr<nsIDOMElement> oldKidElem = do_QueryInterface(oldKid);
      if (xulElement && oldKidElem) {
        
        
        int32_t length;
        controlElement->GetSelectedCount(&length);
        for (int32_t i = 0; i < length; i++) {
          nsCOMPtr<nsIDOMXULSelectControlItemElement> node;
          controlElement->MultiGetSelectedItem(i, getter_AddRefs(node));
          
          nsCOMPtr<nsIDOMElement> selElem = do_QueryInterface(node);
          if (selElem == oldKidElem &&
              NS_SUCCEEDED(controlElement->RemoveItemFromSelection(node))) {
            length--;
            i--;
            fireSelectionHandler = true;
          }
        }

        nsCOMPtr<nsIDOMXULSelectControlItemElement> curItem;
        controlElement->GetCurrentItem(getter_AddRefs(curItem));
        nsCOMPtr<nsIContent> curNode = do_QueryInterface(curItem);
        if (curNode && nsContentUtils::ContentIsDescendantOf(curNode, oldKid)) {
            
            nsCOMPtr<nsIBoxObject> box;
            xulElement->GetBoxObject(getter_AddRefs(box));
            listBox = do_QueryInterface(box);
            if (listBox && oldKidElem) {
              listBox->GetIndexOfItem(oldKidElem, &newCurrentIndex);
            }

            
            if (newCurrentIndex == -1)
              newCurrentIndex = -2;
        }
      }
    }

    nsStyledElement::RemoveChildAt(aIndex, aNotify);
    
    if (newCurrentIndex == -2)
        controlElement->SetCurrentItem(nullptr);
    else if (newCurrentIndex > -1) {
        
        int32_t treeRows;
        listBox->GetRowCount(&treeRows);
        if (treeRows > 0) {
            newCurrentIndex = std::min((treeRows - 1), newCurrentIndex);
            nsCOMPtr<nsIDOMElement> newCurrentItem;
            listBox->GetItemAtIndex(newCurrentIndex, getter_AddRefs(newCurrentItem));
            nsCOMPtr<nsIDOMXULSelectControlItemElement> xulCurItem = do_QueryInterface(newCurrentItem);
            if (xulCurItem)
                controlElement->SetCurrentItem(xulCurItem);
        } else {
            controlElement->SetCurrentItem(nullptr);
        }
    }

    nsIDocument* doc;
    if (fireSelectionHandler && (doc = GetComposedDoc())) {
      nsContentUtils::DispatchTrustedEvent(doc,
                                           static_cast<nsIContent*>(this),
                                           NS_LITERAL_STRING("select"),
                                           false,
                                           true);
    }
}

void
nsXULElement::UnregisterAccessKey(const nsAString& aOldValue)
{
    
    
    nsIDocument* doc = GetComposedDoc();
    if (doc && !aOldValue.IsEmpty()) {
        nsIPresShell *shell = doc->GetShell();

        if (shell) {
            nsIContent *content = this;

            
            if (mNodeInfo->Equals(nsGkAtoms::label)) {
                
                
                
                content = GetBindingParent();
            }

            if (content) {
                shell->GetPresContext()->EventStateManager()->
                    UnregisterAccessKey(content, aOldValue.First());
            }
        }
    }
}

nsresult
nsXULElement::BeforeSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                            const nsAttrValueOrString* aValue, bool aNotify)
{
    if (aNamespaceID == kNameSpaceID_None && aName == nsGkAtoms::accesskey &&
        IsInDoc()) {
        nsAutoString oldValue;
        if (GetAttr(aNamespaceID, aName, oldValue)) {
            UnregisterAccessKey(oldValue);
        }
    } 
    else if (aNamespaceID == kNameSpaceID_None && (aName ==
             nsGkAtoms::command || aName == nsGkAtoms::observes) && IsInDoc()) {

        nsAutoString oldValue;
        GetAttr(kNameSpaceID_None, nsGkAtoms::observes, oldValue);
        if (oldValue.IsEmpty()) {
          GetAttr(kNameSpaceID_None, nsGkAtoms::command, oldValue);
        }

        if (!oldValue.IsEmpty()) {
          RemoveBroadcaster(oldValue);
        }
    }
    else if (aNamespaceID == kNameSpaceID_None &&
             aValue &&
             mNodeInfo->Equals(nsGkAtoms::window) &&
             aName == nsGkAtoms::chromemargin) {
      nsAttrValue attrValue;
      
      if (!attrValue.ParseIntMarginValue(aValue->String())) {
        return NS_ERROR_INVALID_ARG;
      }
    }

    return nsStyledElement::BeforeSetAttr(aNamespaceID, aName,
                                          aValue, aNotify);
}

nsresult
nsXULElement::AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                           const nsAttrValue* aValue, bool aNotify)
{
    if (aNamespaceID == kNameSpaceID_None) {
        if (aValue) {
            
            
            MaybeAddPopupListener(aName);
            if (nsContentUtils::IsEventAttributeName(aName, EventNameType_XUL)) {
                if (aValue->Type() == nsAttrValue::eString) {
                    SetEventHandler(aName, aValue->GetStringValue(), true);
                } else {
                    nsAutoString body;
                    aValue->ToString(body);
                    SetEventHandler(aName, body, true);
                }
            }
    
            nsIDocument* document = GetUncomposedDoc();

            
            if (mNodeInfo->Equals(nsGkAtoms::window)) {
                if (aName == nsGkAtoms::hidechrome) {
                    HideWindowChrome(
                      aValue->Equals(NS_LITERAL_STRING("true"), eCaseMatters));
                }
                else if (aName == nsGkAtoms::chromemargin) {
                    SetChromeMargins(aValue);
                }

                else if (aName == nsGkAtoms::windowtype &&
                         document && document->GetRootElement() == this) {
                    MaybeUpdatePrivateLifetime();
                }
            }
            
            
            if (document && document->GetRootElement() == this) {
                if (aName == nsGkAtoms::title) {
                    document->NotifyPossibleTitleChange(false);
                }
                else if ((aName == nsGkAtoms::activetitlebarcolor ||
                          aName == nsGkAtoms::inactivetitlebarcolor)) {
                    nscolor color = NS_RGBA(0, 0, 0, 0);
                    if (aValue->Type() == nsAttrValue::eColor) {
                        aValue->GetColorValue(color);
                    } else {
                        nsAutoString tmp;
                        nsAttrValue attrValue;
                        aValue->ToString(tmp);
                        attrValue.ParseColor(tmp);
                        attrValue.GetColorValue(color);
                    }
                    SetTitlebarColor(color, aName == nsGkAtoms::activetitlebarcolor);
                }
                else if (aName == nsGkAtoms::drawintitlebar) {
                    SetDrawsInTitlebar(
                        aValue->Equals(NS_LITERAL_STRING("true"), eCaseMatters));
                }
                else if (aName == nsGkAtoms::drawtitle) {
                    SetDrawsTitle(
                        aValue->Equals(NS_LITERAL_STRING("true"), eCaseMatters));
                }
                else if (aName == nsGkAtoms::localedir) {
                    
                    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(document);
                    if (xuldoc) {
                        xuldoc->ResetDocumentDirection();
                    }
                }
                else if (aName == nsGkAtoms::lwtheme ||
                         aName == nsGkAtoms::lwthemetextcolor) {
                    
                    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(document);
                    if (xuldoc) {
                        xuldoc->ResetDocumentLWTheme();
                        UpdateBrightTitlebarForeground(document);
                    }
                }
                else if (aName == nsGkAtoms::brighttitlebarforeground) {
                    UpdateBrightTitlebarForeground(document);
                }
            }
    
            if (aName == nsGkAtoms::src && document) {
                LoadSrc();
            }
        } else  {
            if (mNodeInfo->Equals(nsGkAtoms::window)) {
                if (aName == nsGkAtoms::hidechrome) {
                    HideWindowChrome(false);
                }
                else if (aName == nsGkAtoms::chromemargin) {
                    ResetChromeMargins();
                }
            }
    
            nsIDocument* doc = GetUncomposedDoc();
            if (doc && doc->GetRootElement() == this) {
                if ((aName == nsGkAtoms::activetitlebarcolor ||
                     aName == nsGkAtoms::inactivetitlebarcolor)) {
                    
                    SetTitlebarColor(NS_RGBA(0, 0, 0, 0), aName == nsGkAtoms::activetitlebarcolor);
                }
                else if (aName == nsGkAtoms::localedir) {
                    
                    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(doc);
                    if (xuldoc) {
                        xuldoc->ResetDocumentDirection();
                    }
                }
                else if ((aName == nsGkAtoms::lwtheme ||
                          aName == nsGkAtoms::lwthemetextcolor)) {
                    
                    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(doc);
                    if (xuldoc) {
                        xuldoc->ResetDocumentLWTheme();
                        UpdateBrightTitlebarForeground(doc);
                    }
                }
                else if (aName == nsGkAtoms::brighttitlebarforeground) {
                    UpdateBrightTitlebarForeground(doc);
                }
                else if (aName == nsGkAtoms::drawintitlebar) {
                    SetDrawsInTitlebar(false);
                }
                else if (aName == nsGkAtoms::drawtitle) {
                    SetDrawsTitle(false);
                }
            }
        }

        
        
    }

    return nsStyledElement::AfterSetAttr(aNamespaceID, aName,
                                         aValue, aNotify);
}

bool
nsXULElement::ParseAttribute(int32_t aNamespaceID,
                             nsIAtom* aAttribute,
                             const nsAString& aValue,
                             nsAttrValue& aResult)
{
    
    if (!nsStyledElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                         aResult)) {
        
        aResult.ParseStringOrAtom(aValue);
    }

    return true;
}

void
nsXULElement::RemoveBroadcaster(const nsAString & broadcasterId)
{
    nsCOMPtr<nsIDOMXULDocument> xuldoc = do_QueryInterface(OwnerDoc());
    if (xuldoc) {
        nsCOMPtr<nsIDOMElement> broadcaster;
        nsCOMPtr<nsIDOMDocument> domDoc (do_QueryInterface(xuldoc));
        domDoc->GetElementById(broadcasterId, getter_AddRefs(broadcaster));
        if (broadcaster) {
            xuldoc->RemoveBroadcastListenerFor(broadcaster, this,
              NS_LITERAL_STRING("*"));
        }
    }
}

void
nsXULElement::DestroyContent()
{
    nsXULSlots* slots = static_cast<nsXULSlots*>(GetExistingDOMSlots());
    if (slots) {
        NS_IF_RELEASE(slots->mControllers);
        if (slots->mFrameLoader) {
            slots->mFrameLoader->Destroy();
            slots->mFrameLoader = nullptr;
        }
    }

    nsStyledElement::DestroyContent();
}

#ifdef DEBUG
void
nsXULElement::List(FILE* out, int32_t aIndent) const
{
    nsCString prefix("XUL");
    if (HasSlots()) {
      prefix.Append('*');
    }
    prefix.Append(' ');

    nsStyledElement::List(out, aIndent, prefix);
}
#endif

nsresult
nsXULElement::PreHandleEvent(EventChainPreVisitor& aVisitor)
{
    aVisitor.mForceContentDispatch = true; 
    if (IsRootOfNativeAnonymousSubtree() &&
        (IsAnyOfXULElements(nsGkAtoms::scrollbar, nsGkAtoms::scrollcorner)) &&
        (aVisitor.mEvent->message == NS_MOUSE_CLICK ||
         aVisitor.mEvent->message == NS_MOUSE_DOUBLECLICK ||
         aVisitor.mEvent->message == NS_XUL_COMMAND ||
         aVisitor.mEvent->message == NS_CONTEXTMENU ||
         aVisitor.mEvent->message == NS_DRAGDROP_START ||
         aVisitor.mEvent->message == NS_DRAGDROP_GESTURE)) {
        
        aVisitor.mCanHandle = true;
        aVisitor.mParentTarget = nullptr;
        return NS_OK;
    }
    if (aVisitor.mEvent->message == NS_XUL_COMMAND &&
        aVisitor.mEvent->mClass == eInputEventClass &&
        aVisitor.mEvent->originalTarget == static_cast<nsIContent*>(this) &&
        !IsXULElement(nsGkAtoms::command)) {
        
        
        nsCOMPtr<nsIDOMXULCommandEvent> xulEvent =
            do_QueryInterface(aVisitor.mDOMEvent);
        
        
        nsAutoString command;
        if (xulEvent && GetAttr(kNameSpaceID_None, nsGkAtoms::command, command) &&
            !command.IsEmpty()) {
            
            
            aVisitor.mCanHandle = false;
            aVisitor.mAutomaticChromeDispatch = false;

            
            nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(GetUncomposedDoc()));
            NS_ENSURE_STATE(domDoc);
            nsCOMPtr<nsIDOMElement> commandElt;
            domDoc->GetElementById(command, getter_AddRefs(commandElt));
            nsCOMPtr<nsIContent> commandContent(do_QueryInterface(commandElt));
            if (commandContent) {
                
                
                
                
                nsCOMPtr<nsIDOMEvent> domEvent = aVisitor.mDOMEvent;
                while (domEvent) {
                    Event* event = domEvent->InternalDOMEvent();
                    NS_ENSURE_STATE(!SameCOMIdentity(event->GetOriginalTarget(),
                                                     commandContent));
                    nsCOMPtr<nsIDOMXULCommandEvent> commandEvent =
                        do_QueryInterface(domEvent);
                    if (commandEvent) {
                        commandEvent->GetSourceEvent(getter_AddRefs(domEvent));
                    } else {
                        domEvent = nullptr;
                    }
                }

                WidgetInputEvent* orig = aVisitor.mEvent->AsInputEvent();
                nsContentUtils::DispatchXULCommand(
                  commandContent,
                  aVisitor.mEvent->mFlags.mIsTrusted,
                  aVisitor.mDOMEvent,
                  nullptr,
                  orig->IsControl(),
                  orig->IsAlt(),
                  orig->IsShift(),
                  orig->IsMeta());
            } else {
                NS_WARNING("A XUL element is attached to a command that doesn't exist!\n");
            }
            return NS_OK;
        }
    }

    return nsStyledElement::PreHandleEvent(aVisitor);
}


NS_IMETHODIMP
nsXULElement::GetResource(nsIRDFResource** aResource)
{
    ErrorResult rv;
    *aResource = GetResource(rv).take();
    return rv.StealNSResult();
}

already_AddRefed<nsIRDFResource>
nsXULElement::GetResource(ErrorResult& rv)
{
    nsAutoString id;
    GetAttr(kNameSpaceID_None, nsGkAtoms::ref, id);
    if (id.IsEmpty()) {
        GetAttr(kNameSpaceID_None, nsGkAtoms::id, id);
    }

    if (id.IsEmpty()) {
        return nullptr;
    }

    nsCOMPtr<nsIRDFResource> resource;
    rv = nsXULContentUtils::RDFService()->
        GetUnicodeResource(id, getter_AddRefs(resource));
    return resource.forget();
}

NS_IMETHODIMP
nsXULElement::GetDatabase(nsIRDFCompositeDataSource** aDatabase)
{
    *aDatabase = GetDatabase().take();
    return NS_OK;
}

already_AddRefed<nsIRDFCompositeDataSource>
nsXULElement::GetDatabase()
{
    nsCOMPtr<nsIXULTemplateBuilder> builder = GetBuilder();
    if (!builder) {
        return nullptr;
    }

    nsCOMPtr<nsIRDFCompositeDataSource> database;
    builder->GetDatabase(getter_AddRefs(database));
    return database.forget();
}


NS_IMETHODIMP
nsXULElement::GetBuilder(nsIXULTemplateBuilder** aBuilder)
{
    *aBuilder = GetBuilder().take();
    return NS_OK;
}

already_AddRefed<nsIXULTemplateBuilder>
nsXULElement::GetBuilder()
{
    
    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(GetUncomposedDoc());
    if (!xuldoc) {
        return nullptr;
    }

    nsCOMPtr<nsIXULTemplateBuilder> builder;
    xuldoc->GetTemplateBuilderFor(this, getter_AddRefs(builder));
    return builder.forget();
}




NS_IMETHODIMP
nsXULElement::WalkContentStyleRules(nsRuleWalker* aRuleWalker)
{
    return NS_OK;
}

nsChangeHint
nsXULElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                     int32_t aModType) const
{
    nsChangeHint retval(NS_STYLE_HINT_NONE);

    if (aAttribute == nsGkAtoms::value &&
        (aModType == nsIDOMMutationEvent::REMOVAL ||
         aModType == nsIDOMMutationEvent::ADDITION)) {
      if (IsAnyOfXULElements(nsGkAtoms::label, nsGkAtoms::description))
        
        
        
        
        
        retval = NS_STYLE_HINT_FRAMECHANGE;
    } else {
        
        
        if (nsGkAtoms::left == aAttribute || nsGkAtoms::top == aAttribute ||
            nsGkAtoms::right == aAttribute || nsGkAtoms::bottom == aAttribute ||
            nsGkAtoms::start == aAttribute || nsGkAtoms::end == aAttribute)
            retval = NS_STYLE_HINT_REFLOW;
    }

    return retval;
}

NS_IMETHODIMP_(bool)
nsXULElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
    return false;
}


NS_IMETHODIMP
nsXULElement::GetControllers(nsIControllers** aResult)
{
    ErrorResult rv;
    NS_IF_ADDREF(*aResult = GetControllers(rv));
    return rv.StealNSResult();
}

nsIControllers*
nsXULElement::GetControllers(ErrorResult& rv)
{
    if (! Controllers()) {
        nsDOMSlots* slots = DOMSlots();

        rv = NS_NewXULControllers(nullptr, NS_GET_IID(nsIControllers),
                                  reinterpret_cast<void**>(&slots->mControllers));

        NS_ASSERTION(NS_SUCCEEDED(rv.ErrorCode()),
                     "unable to create a controllers");
        if (rv.Failed()) {
            return nullptr;
        }
    }

    return Controllers();
}

NS_IMETHODIMP
nsXULElement::GetBoxObject(nsIBoxObject** aResult)
{
    ErrorResult rv;
    *aResult = GetBoxObject(rv).take();
    return rv.StealNSResult();
}

already_AddRefed<BoxObject>
nsXULElement::GetBoxObject(ErrorResult& rv)
{
    
    return OwnerDoc()->GetBoxObjectFor(this, rv);
}


#define NS_IMPL_XUL_STRING_ATTR(_method, _atom)                     \
  NS_IMETHODIMP                                                     \
  nsXULElement::Get##_method(nsAString& aReturn)                    \
  {                                                                 \
    GetAttr(kNameSpaceID_None, nsGkAtoms::_atom, aReturn);         \
    return NS_OK;                                                   \
  }                                                                 \
  NS_IMETHODIMP                                                     \
  nsXULElement::Set##_method(const nsAString& aValue)               \
  {                                                                 \
    return SetAttr(kNameSpaceID_None, nsGkAtoms::_atom, aValue,    \
                   true);                                        \
  }

#define NS_IMPL_XUL_BOOL_ATTR(_method, _atom)                       \
  NS_IMETHODIMP                                                     \
  nsXULElement::Get##_method(bool* aResult)                         \
  {                                                                 \
    *aResult = _method();                                           \
    return NS_OK;                                                   \
  }                                                                 \
  NS_IMETHODIMP                                                     \
  nsXULElement::Set##_method(bool aValue)                           \
  {                                                                 \
      SetXULBoolAttr(nsGkAtoms::_atom, aValue);                     \
      return NS_OK;                                                 \
  }


NS_IMPL_XUL_STRING_ATTR(Align, align)
NS_IMPL_XUL_STRING_ATTR(Dir, dir)
NS_IMPL_XUL_STRING_ATTR(Flex, flex)
NS_IMPL_XUL_STRING_ATTR(FlexGroup, flexgroup)
NS_IMPL_XUL_STRING_ATTR(Ordinal, ordinal)
NS_IMPL_XUL_STRING_ATTR(Orient, orient)
NS_IMPL_XUL_STRING_ATTR(Pack, pack)
NS_IMPL_XUL_BOOL_ATTR(Hidden, hidden)
NS_IMPL_XUL_BOOL_ATTR(Collapsed, collapsed)
NS_IMPL_XUL_BOOL_ATTR(AllowEvents, allowevents)
NS_IMPL_XUL_STRING_ATTR(Observes, observes)
NS_IMPL_XUL_STRING_ATTR(Menu, menu)
NS_IMPL_XUL_STRING_ATTR(ContextMenu, contextmenu)
NS_IMPL_XUL_STRING_ATTR(Tooltip, tooltip)
NS_IMPL_XUL_STRING_ATTR(Width, width)
NS_IMPL_XUL_STRING_ATTR(Height, height)
NS_IMPL_XUL_STRING_ATTR(MinWidth, minwidth)
NS_IMPL_XUL_STRING_ATTR(MinHeight, minheight)
NS_IMPL_XUL_STRING_ATTR(MaxWidth, maxwidth)
NS_IMPL_XUL_STRING_ATTR(MaxHeight, maxheight)
NS_IMPL_XUL_STRING_ATTR(Persist, persist)
NS_IMPL_XUL_STRING_ATTR(Left, left)
NS_IMPL_XUL_STRING_ATTR(Top, top)
NS_IMPL_XUL_STRING_ATTR(Datasources, datasources)
NS_IMPL_XUL_STRING_ATTR(Ref, ref)
NS_IMPL_XUL_STRING_ATTR(TooltipText, tooltiptext)
NS_IMPL_XUL_STRING_ATTR(StatusText, statustext)

nsresult
nsXULElement::LoadSrc()
{
    
    
    if (!IsAnyOfXULElements(nsGkAtoms::browser, nsGkAtoms::editor,
                            nsGkAtoms::iframe)) {
        return NS_OK;
    }
    if (!IsInDoc() ||
        !OwnerDoc()->GetRootElement() ||
        OwnerDoc()->GetRootElement()->
            NodeInfo()->Equals(nsGkAtoms::overlay, kNameSpaceID_XUL)) {
        return NS_OK;
    }
    nsXULSlots* slots = static_cast<nsXULSlots*>(Slots());
    if (!slots->mFrameLoader) {
        
        
        
        
        slots->mFrameLoader = nsFrameLoader::Create(this, false);
        if (AttrValueIs(kNameSpaceID_None, nsGkAtoms::prerendered,
                        NS_LITERAL_STRING("true"), eIgnoreCase)) {
            nsresult rv = slots->mFrameLoader->SetIsPrerendered();
            NS_ENSURE_SUCCESS(rv,rv);
        }

        NS_ENSURE_TRUE(slots->mFrameLoader, NS_OK);
    }

    return slots->mFrameLoader->LoadFrame();
}

nsresult
nsXULElement::GetFrameLoader(nsIFrameLoader **aFrameLoader)
{
    *aFrameLoader = GetFrameLoader().take();
    return NS_OK;
}

already_AddRefed<nsFrameLoader>
nsXULElement::GetFrameLoader()
{
    nsXULSlots* slots = static_cast<nsXULSlots*>(GetExistingSlots());
    if (!slots)
        return nullptr;

    nsRefPtr<nsFrameLoader> loader = slots->mFrameLoader;
    return loader.forget();
}

nsresult
nsXULElement::SetIsPrerendered()
{
  return SetAttr(kNameSpaceID_None, nsGkAtoms::prerendered, nullptr,
                 NS_LITERAL_STRING("true"), true);
}

nsresult
nsXULElement::SwapFrameLoaders(nsIFrameLoaderOwner* aOtherOwner)
{
    nsCOMPtr<nsIContent> otherContent(do_QueryInterface(aOtherOwner));
    NS_ENSURE_TRUE(otherContent, NS_ERROR_NOT_IMPLEMENTED);

    nsXULElement* otherEl = FromContent(otherContent);
    NS_ENSURE_TRUE(otherEl, NS_ERROR_NOT_IMPLEMENTED);

    ErrorResult rv;
    SwapFrameLoaders(*otherEl, rv);
    return rv.StealNSResult();
}

void
nsXULElement::SwapFrameLoaders(nsXULElement& aOtherElement, ErrorResult& rv)
{
    if (&aOtherElement == this) {
        
        return;
    }

    nsXULSlots *ourSlots = static_cast<nsXULSlots*>(GetExistingDOMSlots());
    nsXULSlots *otherSlots =
        static_cast<nsXULSlots*>(aOtherElement.GetExistingDOMSlots());
    if (!ourSlots || !ourSlots->mFrameLoader ||
        !otherSlots || !otherSlots->mFrameLoader) {
        
        rv.Throw(NS_ERROR_NOT_IMPLEMENTED);
        return;
    }

    rv = ourSlots->mFrameLoader->SwapWithOtherLoader(otherSlots->mFrameLoader,
                                                     ourSlots->mFrameLoader,
                                                     otherSlots->mFrameLoader);
}

NS_IMETHODIMP
nsXULElement::GetParentTree(nsIDOMXULMultiSelectControlElement** aTreeElement)
{
    for (nsIContent* current = GetParent(); current;
         current = current->GetParent()) {
        if (current->NodeInfo()->Equals(nsGkAtoms::listbox,
                                        kNameSpaceID_XUL)) {
            CallQueryInterface(current, aTreeElement);
            
            

            return NS_OK;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::Focus()
{
    ErrorResult rv;
    Focus(rv);
    return rv.StealNSResult();
}

void
nsXULElement::Focus(ErrorResult& rv)
{
    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    nsCOMPtr<nsIDOMElement> elem = do_QueryObject(this);
    if (fm) {
        rv = fm->SetFocus(this, 0);
    }
}

NS_IMETHODIMP
nsXULElement::Blur()
{
    ErrorResult rv;
    Blur(rv);
    return rv.StealNSResult();
}

void
nsXULElement::Blur(ErrorResult& rv)
{
    if (!ShouldBlur(this))
      return;

    nsIDocument* doc = GetComposedDoc();
    if (!doc)
      return;

    nsIDOMWindow* win = doc->GetWindow();
    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    if (win && fm) {
      rv = fm->ClearFocus(win);
    }
}

NS_IMETHODIMP
nsXULElement::Click()
{
  return ClickWithInputSource(nsIDOMMouseEvent::MOZ_SOURCE_UNKNOWN);
}

void
nsXULElement::Click(ErrorResult& rv)
{
  rv = Click();
}

nsresult
nsXULElement::ClickWithInputSource(uint16_t aInputSource)
{
    if (BoolAttrIsTrue(nsGkAtoms::disabled))
        return NS_OK;

    nsCOMPtr<nsIDocument> doc = GetComposedDoc(); 
    if (doc) {
        nsCOMPtr<nsIPresShell> shell = doc->GetShell();
        if (shell) {
            
            nsRefPtr<nsPresContext> context = shell->GetPresContext();

            bool isCallerChrome = nsContentUtils::IsCallerChrome();

            WidgetMouseEvent eventDown(isCallerChrome, NS_MOUSE_BUTTON_DOWN,
                                       nullptr, WidgetMouseEvent::eReal);
            WidgetMouseEvent eventUp(isCallerChrome, NS_MOUSE_BUTTON_UP,
                                     nullptr, WidgetMouseEvent::eReal);
            WidgetMouseEvent eventClick(isCallerChrome, NS_MOUSE_CLICK, nullptr,
                                        WidgetMouseEvent::eReal);
            eventDown.inputSource = eventUp.inputSource = eventClick.inputSource 
                                  = aInputSource;

            
            nsEventStatus status = nsEventStatus_eIgnore;
            EventDispatcher::Dispatch(static_cast<nsIContent*>(this),
                                      context, &eventDown,  nullptr, &status);

            
            status = nsEventStatus_eIgnore;  
            EventDispatcher::Dispatch(static_cast<nsIContent*>(this),
                                      context, &eventUp, nullptr, &status);

            
            status = nsEventStatus_eIgnore;  
            EventDispatcher::Dispatch(static_cast<nsIContent*>(this),
                                      context, &eventClick, nullptr, &status);
        }
    }

    
    return DoCommand();
}

NS_IMETHODIMP
nsXULElement::DoCommand()
{
    nsCOMPtr<nsIDocument> doc = GetComposedDoc(); 
    if (doc) {
        nsContentUtils::DispatchXULCommand(this, true);
    }

    return NS_OK;
}

nsIContent *
nsXULElement::GetBindingParent() const
{
    return mBindingParent;
}

bool
nsXULElement::IsNodeOfType(uint32_t aFlags) const
{
    return !(aFlags & ~eCONTENT);
}

nsresult
nsXULElement::AddPopupListener(nsIAtom* aName)
{
    
    bool isContext = (aName == nsGkAtoms::context ||
                        aName == nsGkAtoms::contextmenu);
    uint32_t listenerFlag = isContext ?
                            XUL_ELEMENT_HAS_CONTENTMENU_LISTENER :
                            XUL_ELEMENT_HAS_POPUP_LISTENER;

    if (HasFlag(listenerFlag)) {
        return NS_OK;
    }

    nsCOMPtr<nsIDOMEventListener> listener =
      new nsXULPopupListener(this, isContext);

    
    EventListenerManager* manager = GetOrCreateListenerManager();
    SetFlags(listenerFlag);

    if (isContext) {
      manager->AddEventListenerByType(listener,
                                      NS_LITERAL_STRING("contextmenu"),
                                      TrustedEventsAtSystemGroupBubble());
    } else {
      manager->AddEventListenerByType(listener,
                                      NS_LITERAL_STRING("mousedown"),
                                      TrustedEventsAtSystemGroupBubble());
    }
    return NS_OK;
}

EventStates
nsXULElement::IntrinsicState() const
{
    EventStates state = nsStyledElement::IntrinsicState();

    if (IsReadWriteTextElement()) {
        state |= NS_EVENT_STATE_MOZ_READWRITE;
        state &= ~NS_EVENT_STATE_MOZ_READONLY;
    }

    return state;
}



nsresult
nsXULElement::MakeHeavyweight(nsXULPrototypeElement* aPrototype)
{
    if (!aPrototype) {
        return NS_OK;
    }

    uint32_t i;
    nsresult rv;
    for (i = 0; i < aPrototype->mNumAttributes; ++i) {
        nsXULPrototypeAttribute* protoattr = &aPrototype->mAttributes[i];
        nsAttrValue attrValue;
        
        
        if (protoattr->mValue.Type() == nsAttrValue::eCSSStyleRule) {
            nsRefPtr<css::Rule> ruleClone =
                protoattr->mValue.GetCSSStyleRuleValue()->Clone();

            nsString stringValue;
            protoattr->mValue.ToString(stringValue);

            nsRefPtr<css::StyleRule> styleRule = do_QueryObject(ruleClone);
            attrValue.SetTo(styleRule, &stringValue);
        }
        else {
            attrValue.SetTo(protoattr->mValue);
        }

        
        if (protoattr->mName.IsAtom()) {
            rv = mAttrsAndChildren.SetAndTakeAttr(protoattr->mName.Atom(), attrValue);
        }
        else {
            rv = mAttrsAndChildren.SetAndTakeAttr(protoattr->mName.NodeInfo(),
                                                  attrValue);
        }
        NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
}

nsresult
nsXULElement::HideWindowChrome(bool aShouldHide)
{
    nsIDocument* doc = GetUncomposedDoc();
    if (!doc || doc->GetRootElement() != this)
      return NS_ERROR_UNEXPECTED;

    
    if (!doc->IsRootDisplayDocument())
      return NS_OK;

    nsIPresShell *shell = doc->GetShell();

    if (shell) {
        nsIFrame* frame = GetPrimaryFrame();

        nsPresContext *presContext = shell->GetPresContext();

        if (frame && presContext && presContext->IsChrome()) {
            nsView* view = frame->GetClosestView();

            if (view) {
                nsIWidget* w = view->GetWidget();
                NS_ENSURE_STATE(w);
                w->HideWindowChrome(aShouldHide);
            }
        }
    }

    return NS_OK;
}

nsIWidget*
nsXULElement::GetWindowWidget()
{
    nsIDocument* doc = GetComposedDoc();

    
    if (doc && doc->IsRootDisplayDocument()) {
        nsCOMPtr<nsISupports> container = doc->GetContainer();
        nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(container);
        if (baseWindow) {
            nsCOMPtr<nsIWidget> mainWidget;
            baseWindow->GetMainWidget(getter_AddRefs(mainWidget));
            return mainWidget;
        }
    }
    return nullptr;
}

void
nsXULElement::SetTitlebarColor(nscolor aColor, bool aActive)
{
    nsIWidget* mainWidget = GetWindowWidget();
    if (mainWidget) {
        mainWidget->SetWindowTitlebarColor(aColor, aActive);
    }
}

class SetDrawInTitleBarEvent : public nsRunnable
{
public:
  SetDrawInTitleBarEvent(nsIWidget* aWidget, bool aState)
    : mWidget(aWidget)
    , mState(aState)
  {}

  NS_IMETHOD Run() {
    NS_ASSERTION(mWidget, "You shouldn't call this runnable with a null widget!");

    mWidget->SetDrawsInTitlebar(mState);
    return NS_OK;
  }

private:
  nsCOMPtr<nsIWidget> mWidget;
  bool mState;
};

void
nsXULElement::SetDrawsInTitlebar(bool aState)
{
    nsIWidget* mainWidget = GetWindowWidget();
    if (mainWidget) {
        nsContentUtils::AddScriptRunner(new SetDrawInTitleBarEvent(mainWidget, aState));
    }
}

void
nsXULElement::SetDrawsTitle(bool aState)
{
    nsIWidget* mainWidget = GetWindowWidget();
    if (mainWidget) {
        
        
        mainWidget->SetDrawsTitle(aState);
    }
}

void
nsXULElement::UpdateBrightTitlebarForeground(nsIDocument* aDoc)
{
    nsIWidget* mainWidget = GetWindowWidget();
    if (mainWidget) {
        
        
        mainWidget->SetUseBrightTitlebarForeground(
          aDoc->GetDocumentLWTheme() == nsIDocument::Doc_Theme_Bright ||
          aDoc->GetRootElement()->AttrValueIs(kNameSpaceID_None,
                                              nsGkAtoms::brighttitlebarforeground,
                                              NS_LITERAL_STRING("true"),
                                              eCaseMatters));
    }
}

class MarginSetter : public nsRunnable
{
public:
    explicit MarginSetter(nsIWidget* aWidget) :
        mWidget(aWidget), mMargin(-1, -1, -1, -1)
    {}
    MarginSetter(nsIWidget *aWidget, const nsIntMargin& aMargin) :
        mWidget(aWidget), mMargin(aMargin)
    {}

    NS_IMETHOD Run()
    {
        
        
        mWidget->SetNonClientMargins(mMargin);
        return NS_OK;
    }

private:
    nsCOMPtr<nsIWidget> mWidget;
    nsIntMargin mMargin;
};

void
nsXULElement::SetChromeMargins(const nsAttrValue* aValue)
{
    if (!aValue)
        return;

    nsIWidget* mainWidget = GetWindowWidget();
    if (!mainWidget)
        return;

    
    nsIntMargin margins;
    bool gotMargins = false;

    if (aValue->Type() == nsAttrValue::eIntMarginValue) {
        gotMargins = aValue->GetIntMarginValue(margins);
    } else {
        nsAutoString tmp;
        aValue->ToString(tmp);
        gotMargins = nsContentUtils::ParseIntMarginValue(tmp, margins);
    }
    if (gotMargins) {
        nsContentUtils::AddScriptRunner(new MarginSetter(mainWidget, margins));
    }
}

void
nsXULElement::ResetChromeMargins()
{
    nsIWidget* mainWidget = GetWindowWidget();
    if (!mainWidget)
        return;
    
    nsContentUtils::AddScriptRunner(new MarginSetter(mainWidget));
}

bool
nsXULElement::BoolAttrIsTrue(nsIAtom* aName) const
{
    const nsAttrValue* attr =
        GetAttrInfo(kNameSpaceID_None, aName).mValue;

    return attr && attr->Type() == nsAttrValue::eAtom &&
           attr->GetAtomValue() == nsGkAtoms::_true;
}

void
nsXULElement::RecompileScriptEventListeners()
{
    int32_t i, count = mAttrsAndChildren.AttrCount();
    for (i = 0; i < count; ++i) {
        const nsAttrName *name = mAttrsAndChildren.AttrNameAt(i);

        
        if (!name->IsAtom()) {
            continue;
        }

        nsIAtom *attr = name->Atom();
        if (!nsContentUtils::IsEventAttributeName(attr, EventNameType_XUL)) {
            continue;
        }

        nsAutoString value;
        GetAttr(kNameSpaceID_None, attr, value);
        SetEventHandler(attr, value, true);
    }
}

bool
nsXULElement::IsEventAttributeName(nsIAtom *aName)
{
  return nsContentUtils::IsEventAttributeName(aName, EventNameType_XUL);
}

JSObject*
nsXULElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto)
{
    return dom::XULElementBinding::Wrap(aCx, this, aGivenProto);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXULPrototypeNode)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXULPrototypeNode)
    if (tmp->mType == nsXULPrototypeNode::eType_Element) {
        static_cast<nsXULPrototypeElement*>(tmp)->Unlink();
    }
    else if (tmp->mType == nsXULPrototypeNode::eType_Script) {
        static_cast<nsXULPrototypeScript*>(tmp)->UnlinkJSObjects();
    }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXULPrototypeNode)
    if (tmp->mType == nsXULPrototypeNode::eType_Element) {
        nsXULPrototypeElement *elem =
            static_cast<nsXULPrototypeElement*>(tmp);
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mNodeInfo");
        cb.NoteNativeChild(elem->mNodeInfo,
                           NS_CYCLE_COLLECTION_PARTICIPANT(NodeInfo));
        uint32_t i;
        for (i = 0; i < elem->mNumAttributes; ++i) {
            const nsAttrName& name = elem->mAttributes[i].mName;
            if (!name.IsAtom()) {
                NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb,
                    "mAttributes[i].mName.NodeInfo()");
                cb.NoteNativeChild(name.NodeInfo(), 
                                   NS_CYCLE_COLLECTION_PARTICIPANT(NodeInfo));
            }
        }
        ImplCycleCollectionTraverse(cb, elem->mChildren, "mChildren");
    }
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsXULPrototypeNode)
    if (tmp->mType == nsXULPrototypeNode::eType_Script) {
        nsXULPrototypeScript *script =
            static_cast<nsXULPrototypeScript*>(tmp);
        script->Trace(aCallbacks, aClosure);
    }
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(nsXULPrototypeNode, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsXULPrototypeNode, Release)






nsXULPrototypeAttribute::~nsXULPrototypeAttribute()
{
    MOZ_COUNT_DTOR(nsXULPrototypeAttribute);
}







nsresult
nsXULPrototypeElement::Serialize(nsIObjectOutputStream* aStream,
                                 nsXULPrototypeDocument* aProtoDoc,
                                 const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos)
{
    nsresult rv;

    
    rv = aStream->Write32(mType);

    
    int32_t index = aNodeInfos->IndexOf(mNodeInfo);
    NS_ASSERTION(index >= 0, "unknown mozilla::dom::NodeInfo index");
    nsresult tmp = aStream->Write32(index);
    if (NS_FAILED(tmp)) {
      rv = tmp;
    }

    
    tmp = aStream->Write32(mNumAttributes);
    if (NS_FAILED(tmp)) {
      rv = tmp;
    }

    nsAutoString attributeValue;
    uint32_t i;
    for (i = 0; i < mNumAttributes; ++i) {
        nsRefPtr<mozilla::dom::NodeInfo> ni;
        if (mAttributes[i].mName.IsAtom()) {
            ni = mNodeInfo->NodeInfoManager()->
                GetNodeInfo(mAttributes[i].mName.Atom(), nullptr,
                            kNameSpaceID_None,
                            nsIDOMNode::ATTRIBUTE_NODE);
            NS_ASSERTION(ni, "the nodeinfo should already exist");
        }
        else {
            ni = mAttributes[i].mName.NodeInfo();
        }

        index = aNodeInfos->IndexOf(ni);
        NS_ASSERTION(index >= 0, "unknown mozilla::dom::NodeInfo index");
        tmp = aStream->Write32(index);
        if (NS_FAILED(tmp)) {
          rv = tmp;
        }

        mAttributes[i].mValue.ToString(attributeValue);
        tmp = aStream->WriteWStringZ(attributeValue.get());
        if (NS_FAILED(tmp)) {
          rv = tmp;
        }
    }

    
    tmp = aStream->Write32(uint32_t(mChildren.Length()));
    if (NS_FAILED(tmp)) {
      rv = tmp;
    }
    for (i = 0; i < mChildren.Length(); i++) {
        nsXULPrototypeNode* child = mChildren[i].get();
        switch (child->mType) {
        case eType_Element:
        case eType_Text:
        case eType_PI:
            tmp = child->Serialize(aStream, aProtoDoc, aNodeInfos);
            if (NS_FAILED(tmp)) {
              rv = tmp;
            }
            break;
        case eType_Script:
            tmp = aStream->Write32(child->mType);
            if (NS_FAILED(tmp)) {
              rv = tmp;
            }
            nsXULPrototypeScript* script = static_cast<nsXULPrototypeScript*>(child);

            tmp = aStream->Write8(script->mOutOfLine);
            if (NS_FAILED(tmp)) {
              rv = tmp;
            }
            if (! script->mOutOfLine) {
                tmp = script->Serialize(aStream, aProtoDoc, aNodeInfos);
                if (NS_FAILED(tmp)) {
                  rv = tmp;
                }
            } else {
                tmp = aStream->WriteCompoundObject(script->mSrcURI,
                                                   NS_GET_IID(nsIURI),
                                                   true);
                if (NS_FAILED(tmp)) {
                  rv = tmp;
                }

                if (script->GetScriptObject()) {
                    
                    
                    
                    
                    
                    tmp = script->SerializeOutOfLine(aStream, aProtoDoc);
                    if (NS_FAILED(tmp)) {
                      rv = tmp;
                    }
                }
            }
            break;
        }
    }

    return rv;
}

nsresult
nsXULPrototypeElement::Deserialize(nsIObjectInputStream* aStream,
                                   nsXULPrototypeDocument* aProtoDoc,
                                   nsIURI* aDocumentURI,
                                   const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos)
{
    NS_PRECONDITION(aNodeInfos, "missing nodeinfo array");

    
    uint32_t number;
    nsresult rv = aStream->Read32(&number);
    mNodeInfo = aNodeInfos->ElementAt(number);
    if (!mNodeInfo)
        return NS_ERROR_UNEXPECTED;

    
    nsresult tmp = aStream->Read32(&number);
    if (NS_FAILED(tmp)) {
      rv = tmp;
    }
    mNumAttributes = int32_t(number);

    uint32_t i;
    if (mNumAttributes > 0) {
        mAttributes = new nsXULPrototypeAttribute[mNumAttributes];
        if (! mAttributes)
            return NS_ERROR_OUT_OF_MEMORY;

        nsAutoString attributeValue;
        for (i = 0; i < mNumAttributes; ++i) {
            tmp = aStream->Read32(&number);
            if (NS_FAILED(tmp)) {
              rv = tmp;
            }
            mozilla::dom::NodeInfo* ni = aNodeInfos->ElementAt(number);
            if (!ni)
                return NS_ERROR_UNEXPECTED;

            mAttributes[i].mName.SetTo(ni);

            tmp = aStream->ReadString(attributeValue);
            if (NS_FAILED(tmp)) {
              rv = tmp;
            }
            tmp = SetAttrAt(i, attributeValue, aDocumentURI);
            if (NS_FAILED(tmp)) {
              rv = tmp;
            }
        }
    }

    tmp = aStream->Read32(&number);
    if (NS_FAILED(tmp)) {
      rv = tmp;
    }
    uint32_t numChildren = int32_t(number);

    if (numChildren > 0) {
        mChildren.SetCapacity(numChildren);

        for (i = 0; i < numChildren; i++) {
            tmp = aStream->Read32(&number);
            if (NS_FAILED(tmp)) {
              rv = tmp;
            }
            Type childType = (Type)number;

            nsRefPtr<nsXULPrototypeNode> child;

            switch (childType) {
            case eType_Element:
                child = new nsXULPrototypeElement();
                if (! child)
                    return NS_ERROR_OUT_OF_MEMORY;
                child->mType = childType;

                tmp = child->Deserialize(aStream, aProtoDoc, aDocumentURI,
                                         aNodeInfos);
                if (NS_FAILED(tmp)) {
                  rv = tmp;
                }
                break;
            case eType_Text:
                child = new nsXULPrototypeText();
                if (! child)
                    return NS_ERROR_OUT_OF_MEMORY;
                child->mType = childType;

                tmp = child->Deserialize(aStream, aProtoDoc, aDocumentURI,
                                         aNodeInfos);
                if (NS_FAILED(tmp)) {
                  rv = tmp;
                }
                break;
            case eType_PI:
                child = new nsXULPrototypePI();
                if (! child)
                    return NS_ERROR_OUT_OF_MEMORY;
                child->mType = childType;

                tmp = child->Deserialize(aStream, aProtoDoc, aDocumentURI,
                                         aNodeInfos);
                if (NS_FAILED(tmp)) {
                  rv = tmp;
                }
                break;
            case eType_Script: {
                
                nsXULPrototypeScript* script = new nsXULPrototypeScript(0, 0);
                if (! script)
                    return NS_ERROR_OUT_OF_MEMORY;
                child = script;
                child->mType = childType;

                tmp = aStream->ReadBoolean(&script->mOutOfLine);
                if (NS_FAILED(tmp)) {
                  rv = tmp;
                }
                if (! script->mOutOfLine) {
                    tmp = script->Deserialize(aStream, aProtoDoc, aDocumentURI,
                                              aNodeInfos);
                    if (NS_FAILED(tmp)) {
                      rv = tmp;
                    }
                } else {
                    nsCOMPtr<nsISupports> supports;
                    tmp = aStream->ReadObject(true, getter_AddRefs(supports));
                    script->mSrcURI = do_QueryInterface(supports);
                    if (NS_FAILED(tmp)) {
                      rv = tmp;
                    }

                    tmp = script->DeserializeOutOfLine(aStream, aProtoDoc);
                    if (NS_FAILED(tmp)) {
                      rv = tmp;
                    }
                }
                
                break;
            }
            default:
                NS_NOTREACHED("Unexpected child type!");
                rv = NS_ERROR_UNEXPECTED;
            }

            mChildren.AppendElement(child);

            
            
            
            
            
            
            
            
            if (NS_FAILED(rv))
                return rv;
        }
    }

    return rv;
}

nsresult
nsXULPrototypeElement::SetAttrAt(uint32_t aPos, const nsAString& aValue,
                                 nsIURI* aDocumentURI)
{
    NS_PRECONDITION(aPos < mNumAttributes, "out-of-bounds");

    
    
    

    if (!mNodeInfo->NamespaceEquals(kNameSpaceID_XUL)) {
        mAttributes[aPos].mValue.ParseStringOrAtom(aValue);

        return NS_OK;
    }

    if (mAttributes[aPos].mName.Equals(nsGkAtoms::id) &&
        !aValue.IsEmpty()) {
        mHasIdAttribute = true;
        
        
        
        mAttributes[aPos].mValue.ParseAtom(aValue);

        return NS_OK;
    }
    else if (mAttributes[aPos].mName.Equals(nsGkAtoms::_class)) {
        mHasClassAttribute = true;
        
        mAttributes[aPos].mValue.ParseAtomArray(aValue);

        return NS_OK;
    }
    else if (mAttributes[aPos].mName.Equals(nsGkAtoms::style)) {
        mHasStyleAttribute = true;
        
        nsRefPtr<css::StyleRule> rule;

        nsCSSParser parser;

        
        
        
        
        parser.ParseStyleAttribute(aValue, aDocumentURI, aDocumentURI,
                                   
                                   
                                   mNodeInfo->NodeInfoManager()->
                                     DocumentPrincipal(),
                                   getter_AddRefs(rule));
        if (rule) {
            mAttributes[aPos].mValue.SetTo(rule, &aValue);

            return NS_OK;
        }
        
    }

    mAttributes[aPos].mValue.ParseStringOrAtom(aValue);

    return NS_OK;
}

void
nsXULPrototypeElement::Unlink()
{
    mNumAttributes = 0;
    delete[] mAttributes;
    mAttributes = nullptr;
    mChildren.Clear();
}

void
nsXULPrototypeElement::TraceAllScripts(JSTracer* aTrc)
{
    for (uint32_t i = 0; i < mChildren.Length(); ++i) {
        nsXULPrototypeNode* child = mChildren[i];
        if (child->mType == nsXULPrototypeNode::eType_Element) {
            static_cast<nsXULPrototypeElement*>(child)->TraceAllScripts(aTrc);
        } else if (child->mType == nsXULPrototypeNode::eType_Script) {
            static_cast<nsXULPrototypeScript*>(child)->TraceScriptObject(aTrc);
        }
    }
}






nsXULPrototypeScript::nsXULPrototypeScript(uint32_t aLineNo, uint32_t aVersion)
    : nsXULPrototypeNode(eType_Script),
      mLineNo(aLineNo),
      mSrcLoading(false),
      mOutOfLine(true),
      mSrcLoadWaiters(nullptr),
      mLangVersion(aVersion),
      mScriptObject(nullptr)
{
}


nsXULPrototypeScript::~nsXULPrototypeScript()
{
    UnlinkJSObjects();
}

nsresult
nsXULPrototypeScript::Serialize(nsIObjectOutputStream* aStream,
                                nsXULPrototypeDocument* aProtoDoc,
                                const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos)
{
    NS_ENSURE_TRUE(aProtoDoc, NS_ERROR_UNEXPECTED);
    AutoSafeJSContext cx;
    JS::Rooted<JSObject*> global(cx, xpc::CompilationScope());
    NS_ENSURE_TRUE(global, NS_ERROR_UNEXPECTED);
    JSAutoCompartment ac(cx, global);

    NS_ASSERTION(!mSrcLoading || mSrcLoadWaiters != nullptr ||
                 !mScriptObject,
                 "script source still loading when serializing?!");
    if (!mScriptObject)
        return NS_ERROR_FAILURE;

    
    nsresult rv;
    rv = aStream->Write32(mLineNo);
    if (NS_FAILED(rv)) return rv;
    rv = aStream->Write32(mLangVersion);
    if (NS_FAILED(rv)) return rv;

    
    
    
    JS::Handle<JSScript*> script =
        JS::Handle<JSScript*>::fromMarkedLocation(mScriptObject.address());
    MOZ_ASSERT(xpc::CompilationScope() == JS::CurrentGlobalOrNull(cx));
    return nsContentUtils::XPConnect()->WriteScript(aStream, cx,
                                                    xpc_UnmarkGrayScript(script));
}

nsresult
nsXULPrototypeScript::SerializeOutOfLine(nsIObjectOutputStream* aStream,
                                         nsXULPrototypeDocument* aProtoDoc)
{
    nsresult rv = NS_ERROR_NOT_IMPLEMENTED;

    bool isChrome = false;
    if (NS_FAILED(mSrcURI->SchemeIs("chrome", &isChrome)) || !isChrome)
       
       return rv;

    nsXULPrototypeCache* cache = nsXULPrototypeCache::GetInstance();
    if (!cache)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ASSERTION(cache->IsEnabled(),
                 "writing to the cache file, but the XUL cache is off?");
    bool exists;
    cache->HasData(mSrcURI, &exists);

    




    if (exists)
        return NS_OK;

    nsCOMPtr<nsIObjectOutputStream> oos;
    rv = cache->GetOutputStream(mSrcURI, getter_AddRefs(oos));
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsresult tmp = Serialize(oos, aProtoDoc, nullptr);
    if (NS_FAILED(tmp)) {
      rv = tmp;
    }
    tmp = cache->FinishOutputStream(mSrcURI);
    if (NS_FAILED(tmp)) {
      rv = tmp;
    }

    if (NS_FAILED(rv))
        cache->AbortCaching();
    return rv;
}


nsresult
nsXULPrototypeScript::Deserialize(nsIObjectInputStream* aStream,
                                  nsXULPrototypeDocument* aProtoDoc,
                                  nsIURI* aDocumentURI,
                                  const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos)
{
    NS_ASSERTION(!mSrcLoading || mSrcLoadWaiters != nullptr ||
                 !mScriptObject,
                 "prototype script not well-initialized when deserializing?!");

    
    aStream->Read32(&mLineNo);
    aStream->Read32(&mLangVersion);

    AutoSafeJSContext cx;
    JS::Rooted<JSObject*> global(cx, xpc::CompilationScope());
    NS_ENSURE_TRUE(global, NS_ERROR_UNEXPECTED);
    JSAutoCompartment ac(cx, global);

    JS::Rooted<JSScript*> newScriptObject(cx);
    nsresult rv = nsContentUtils::XPConnect()->ReadScript(aStream, cx,
                                                          newScriptObject.address());
    NS_ENSURE_SUCCESS(rv, rv);
    Set(newScriptObject);
    return NS_OK;
}


nsresult
nsXULPrototypeScript::DeserializeOutOfLine(nsIObjectInputStream* aInput,
                                           nsXULPrototypeDocument* aProtoDoc)
{
    
    
    nsresult rv = NS_OK;
    nsXULPrototypeCache* cache = nsXULPrototypeCache::GetInstance();
  
    nsCOMPtr<nsIObjectInputStream> objectInput = aInput;
    if (cache) {
        bool useXULCache = true;
        if (mSrcURI) {
            
            
            
            
            
            
            
            
            
            useXULCache = cache->IsEnabled();

            if (useXULCache) {
                JSScript* newScriptObject =
                    cache->GetScript(mSrcURI);
                if (newScriptObject)
                    Set(newScriptObject);
            }
        }

        if (!mScriptObject) {
            if (mSrcURI) {
                rv = cache->GetInputStream(mSrcURI, getter_AddRefs(objectInput));
            } 
            
            
 
            
            
            
            
            
            
            if (NS_SUCCEEDED(rv))
                rv = Deserialize(objectInput, aProtoDoc, nullptr, nullptr);

            if (NS_SUCCEEDED(rv)) {
                if (useXULCache && mSrcURI) {
                    bool isChrome = false;
                    mSrcURI->SchemeIs("chrome", &isChrome);
                    if (isChrome)
                        cache->PutScript(mSrcURI, GetScriptObject());
                }
                cache->FinishInputStream(mSrcURI);
            } else {
                
                
                
                
                if (rv != NS_ERROR_NOT_AVAILABLE)
                    cache->AbortCaching();
            }
        }
    }
    return rv;
}

class NotifyOffThreadScriptCompletedRunnable : public nsRunnable
{
    nsRefPtr<nsIOffThreadScriptReceiver> mReceiver;
    void *mToken;

public:
    NotifyOffThreadScriptCompletedRunnable(already_AddRefed<nsIOffThreadScriptReceiver> aReceiver,
                                           void *aToken)
      : mReceiver(aReceiver), mToken(aToken)
    {}

    NS_DECL_NSIRUNNABLE
};

NS_IMETHODIMP
NotifyOffThreadScriptCompletedRunnable::Run()
{
    MOZ_ASSERT(NS_IsMainThread());

    
    
    
    nsCOMPtr<nsIJSRuntimeService> svc = do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
    NS_ENSURE_TRUE(svc, NS_ERROR_FAILURE);

    JSScript *script;
    {
        AutoSafeJSContext cx;
        JSAutoCompartment ac(cx, xpc::CompilationScope());
        script = JS::FinishOffThreadScript(cx, JS_GetRuntime(cx), mToken);
    }

    return mReceiver->OnScriptCompileComplete(script, script ? NS_OK : NS_ERROR_FAILURE);
}

static void
OffThreadScriptReceiverCallback(void *aToken, void *aCallbackData)
{
    
    
    nsIOffThreadScriptReceiver* aReceiver = static_cast<nsIOffThreadScriptReceiver*>(aCallbackData);
    nsRefPtr<NotifyOffThreadScriptCompletedRunnable> notify =
        new NotifyOffThreadScriptCompletedRunnable(
            already_AddRefed<nsIOffThreadScriptReceiver>(aReceiver), aToken);
    NS_DispatchToMainThread(notify);
}

nsresult
nsXULPrototypeScript::Compile(JS::SourceBufferHolder& aSrcBuf,
                              nsIURI* aURI, uint32_t aLineNo,
                              nsIDocument* aDocument,
                              nsIOffThreadScriptReceiver *aOffThreadReceiver )
{
    
    AutoSafeJSContext cx;
    JSAutoCompartment ac(cx, xpc::CompilationScope());

    nsAutoCString urlspec;
    nsContentUtils::GetWrapperSafeScriptFilename(aDocument, aURI, urlspec);

    
    NS_ENSURE_TRUE(JSVersion(mLangVersion) != JSVERSION_UNKNOWN, NS_OK);
    JS::CompileOptions options(cx);
    options.setIntroductionType("scriptElement")
           .setFileAndLine(urlspec.get(), aLineNo)
           .setVersion(JSVersion(mLangVersion));
    
    
    
    options.setSourceIsLazy(mOutOfLine);
    JS::Rooted<JSObject*> scope(cx, JS::CurrentGlobalOrNull(cx));
    if (scope) {
      JS::ExposeObjectToActiveJS(scope);
    }

    if (aOffThreadReceiver && JS::CanCompileOffThread(cx, options, aSrcBuf.length())) {
        if (!JS::CompileOffThread(cx, options,
                                  aSrcBuf.get(), aSrcBuf.length(),
                                  OffThreadScriptReceiverCallback,
                                  static_cast<void*>(aOffThreadReceiver))) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        
        NS_ADDREF(aOffThreadReceiver);
    } else {
        JS::Rooted<JSScript*> script(cx);
        if (!JS::Compile(cx, options, aSrcBuf, &script))
            return NS_ERROR_OUT_OF_MEMORY;
        Set(script);
    }
    return NS_OK;
}

nsresult
nsXULPrototypeScript::Compile(const char16_t* aText,
                              int32_t aTextLength,
                              nsIURI* aURI,
                              uint32_t aLineNo,
                              nsIDocument* aDocument,
                              nsIOffThreadScriptReceiver *aOffThreadReceiver )
{
  JS::SourceBufferHolder srcBuf(aText, aTextLength,
                                JS::SourceBufferHolder::NoOwnership);
  return Compile(srcBuf, aURI, aLineNo, aDocument, aOffThreadReceiver);
}

void
nsXULPrototypeScript::UnlinkJSObjects()
{
    if (mScriptObject) {
        mScriptObject = nullptr;
        mozilla::DropJSObjects(this);
    }
}

void
nsXULPrototypeScript::Set(JSScript* aObject)
{
    MOZ_ASSERT(!mScriptObject, "Leaking script object.");
    if (!aObject) {
        mScriptObject = nullptr;
        return;
    }

    mScriptObject = aObject;
    mozilla::HoldJSObjects(this);
}






nsresult
nsXULPrototypeText::Serialize(nsIObjectOutputStream* aStream,
                              nsXULPrototypeDocument* aProtoDoc,
                              const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos)
{
    nsresult rv;

    
    rv = aStream->Write32(mType);

    nsresult tmp = aStream->WriteWStringZ(mValue.get());
    if (NS_FAILED(tmp)) {
      rv = tmp;
    }

    return rv;
}

nsresult
nsXULPrototypeText::Deserialize(nsIObjectInputStream* aStream,
                                nsXULPrototypeDocument* aProtoDoc,
                                nsIURI* aDocumentURI,
                                const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos)
{
    nsresult rv;

    rv = aStream->ReadString(mValue);

    return rv;
}






nsresult
nsXULPrototypePI::Serialize(nsIObjectOutputStream* aStream,
                            nsXULPrototypeDocument* aProtoDoc,
                            const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos)
{
    nsresult rv;

    
    rv = aStream->Write32(mType);

    nsresult tmp = aStream->WriteWStringZ(mTarget.get());
    if (NS_FAILED(tmp)) {
      rv = tmp;
    }
    tmp = aStream->WriteWStringZ(mData.get());
    if (NS_FAILED(tmp)) {
      rv = tmp;
    }

    return rv;
}

nsresult
nsXULPrototypePI::Deserialize(nsIObjectInputStream* aStream,
                              nsXULPrototypeDocument* aProtoDoc,
                              nsIURI* aDocumentURI,
                              const nsTArray<nsRefPtr<mozilla::dom::NodeInfo>> *aNodeInfos)
{
    nsresult rv;

    rv = aStream->ReadString(mTarget);
    nsresult tmp = aStream->ReadString(mData);
    if (NS_FAILED(tmp)) {
      rv = tmp;
    }

    return rv;
}
