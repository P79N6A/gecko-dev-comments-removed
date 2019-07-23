
























































#include "nsCOMPtr.h"
#include "nsDOMCID.h"
#include "nsDOMError.h"
#include "nsDOMString.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsHashtable.h"
#include "nsIAtom.h"
#include "nsIBaseWindow.h"
#include "nsIDOMAttr.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMLoadListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMFormListener.h"
#include "nsIDOMContextMenuListener.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMElementCSSInlineStyle.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDocument.h"
#include "nsIEventListenerManager.h"
#include "nsIEventStateManager.h"
#include "nsFocusManager.h"
#include "nsIFastLoadService.h"
#include "nsHTMLStyleSheet.h"
#include "nsINameSpaceManager.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsIPresShell.h"
#include "nsIPrincipal.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsIScriptContext.h"
#include "nsIScriptRuntime.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIServiceManager.h"
#include "nsICSSStyleRule.h"
#include "nsIStyleSheet.h"
#include "nsDOMCSSAttrDeclaration.h"
#include "nsIURL.h"
#include "nsIViewManager.h"
#include "nsIWidget.h"
#include "nsIXULDocument.h"
#include "nsIXULTemplateBuilder.h"
#include "nsIXBLService.h"
#include "nsLayoutCID.h"
#include "nsContentCID.h"
#include "nsRDFCID.h"
#include "nsStyleConsts.h"
#include "nsXPIDLString.h"
#include "nsXULControllers.h"
#include "nsIBoxObject.h"
#include "nsPIBoxObject.h"
#include "nsXULDocument.h"
#include "nsXULPopupListener.h"
#include "nsRuleWalker.h"
#include "nsIDOMViewCSS.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsCSSDeclaration.h"
#include "nsIListBoxObject.h"
#include "nsContentUtils.h"
#include "nsContentList.h"
#include "nsMutationEvent.h"
#include "nsIDOMMutationEvent.h"
#include "nsPIDOMWindow.h"
#include "nsDOMAttributeMap.h"
#include "nsDOMCSSDeclaration.h"
#include "nsStyledElement.h"
#include "nsGkAtoms.h"
#include "nsXULContentUtils.h"
#include "nsNodeUtils.h"
#include "nsFrameLoader.h"
#include "prlog.h"
#include "rdf.h"
#include "nsIDOM3EventTarget.h"
#include "nsIDOMEventGroup.h"
#include "nsIControllers.h"


#include "nsIDOMXULDocument.h"

#include "nsReadableUtils.h"
#include "nsITimelineService.h"
#include "nsIFrame.h"
#include "nsNodeInfoManager.h"
#include "nsXBLBinding.h"
#include "nsEventDispatcher.h"
#include "nsPresShellIterator.h"
#include "mozAutoDocUpdate.h"
#include "nsIDOMXULCommandEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsCCUncollectableMarker.h"


nsICSSParser* nsXULPrototypeElement::sCSSParser = nsnull;
nsIXBLService * nsXULElement::gXBLService = nsnull;




class nsScriptEventHandlerOwnerTearoff : public nsIScriptEventHandlerOwner
{
public:
    nsScriptEventHandlerOwnerTearoff(nsXULElement* aElement)
    : mElement(aElement) {}

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(nsScriptEventHandlerOwnerTearoff)

    
    virtual nsresult CompileEventHandler(nsIScriptContext* aContext,
                                         nsISupports* aTarget,
                                         nsIAtom *aName,
                                         const nsAString& aBody,
                                         const char* aURL,
                                         PRUint32 aLineNo,
                                         nsScriptObjectHolder &aHandler);
    virtual nsresult GetCompiledEventHandler(nsIAtom *aName,
                                             nsScriptObjectHolder &aHandler);

private:
    nsRefPtr<nsXULElement> mElement;
};



static NS_DEFINE_CID(kXULPopupListenerCID,        NS_XULPOPUPLISTENER_CID);



#ifdef XUL_PROTOTYPE_ATTRIBUTE_METERING
PRUint32             nsXULPrototypeAttribute::gNumElements;
PRUint32             nsXULPrototypeAttribute::gNumAttributes;
PRUint32             nsXULPrototypeAttribute::gNumEventHandlers;
PRUint32             nsXULPrototypeAttribute::gNumCacheTests;
PRUint32             nsXULPrototypeAttribute::gNumCacheHits;
PRUint32             nsXULPrototypeAttribute::gNumCacheSets;
PRUint32             nsXULPrototypeAttribute::gNumCacheFills;
#endif

class nsXULElementTearoff : public nsIDOMElementCSSInlineStyle,
                            public nsIFrameLoaderOwner
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXULElementTearoff,
                                           nsIDOMElementCSSInlineStyle)

  nsXULElementTearoff(nsXULElement *aElement)
    : mElement(aElement)
  {
  }

  NS_FORWARD_NSIDOMELEMENTCSSINLINESTYLE(static_cast<nsXULElement*>(mElement.get())->)
  NS_FORWARD_NSIFRAMELOADEROWNER(static_cast<nsXULElement*>(mElement.get())->);
private:
  nsCOMPtr<nsIDOMXULElement> mElement;
};

NS_IMPL_CYCLE_COLLECTION_1(nsXULElementTearoff, mElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXULElementTearoff)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXULElementTearoff)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXULElementTearoff)
  NS_INTERFACE_MAP_ENTRY(nsIFrameLoaderOwner)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElementCSSInlineStyle)
NS_INTERFACE_MAP_END_AGGREGATED(mElement)





nsXULElement::nsXULElement(nsINodeInfo* aNodeInfo)
    : nsGenericElement(aNodeInfo),
      mBindingParent(nsnull)
{
    XUL_PROTOTYPE_ATTRIBUTE_METER(gNumElements);
}

nsXULElement::nsXULSlots::nsXULSlots(PtrBits aFlags)
    : nsXULElement::nsDOMSlots(aFlags)
{
}

nsXULElement::nsXULSlots::~nsXULSlots()
{
    NS_IF_RELEASE(mControllers); 
    if (mFrameLoader) {
        mFrameLoader->Destroy();
    }
}

nsINode::nsSlots*
nsXULElement::CreateSlots()
{
    return new nsXULSlots(mFlagsOrSlots);
}


already_AddRefed<nsXULElement>
nsXULElement::Create(nsXULPrototypeElement* aPrototype, nsINodeInfo *aNodeInfo,
                     PRBool aIsScriptable)
{
    nsXULElement *element = new nsXULElement(aNodeInfo);
    if (element) {
        NS_ADDREF(element);

        element->mPrototype = aPrototype;
        if (aPrototype->mHasIdAttribute) {
            element->SetFlags(NODE_MAY_HAVE_ID);
        }
        if (aPrototype->mHasClassAttribute) {
            element->SetFlags(NODE_MAY_HAVE_CLASS);
        }
        if (aPrototype->mHasStyleAttribute) {
            element->SetFlags(NODE_MAY_HAVE_STYLE);
        }

        NS_ASSERTION(aPrototype->mScriptTypeID != nsIProgrammingLanguage::UNKNOWN,
                    "Need to know the language!");
        element->SetScriptTypeID(aPrototype->mScriptTypeID);

        if (aIsScriptable) {
            
            
            
            for (PRUint32 i = 0; i < aPrototype->mNumAttributes; ++i) {
                element->AddListenerFor(aPrototype->mAttributes[i].mName,
                                        PR_TRUE);
            }
        }
    }

    return element;
}

nsresult
nsXULElement::Create(nsXULPrototypeElement* aPrototype,
                     nsIDocument* aDocument,
                     PRBool aIsScriptable,
                     nsIContent** aResult)
{
    
    NS_PRECONDITION(aPrototype != nsnull, "null ptr");
    if (! aPrototype)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsCOMPtr<nsINodeInfo> nodeInfo;
    if (aDocument) {
        nsINodeInfo* ni = aPrototype->mNodeInfo;
        nodeInfo = aDocument->NodeInfoManager()->GetNodeInfo(ni->NameAtom(),
                                                             ni->GetPrefixAtom(),
                                                             ni->NamespaceID());
        NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
    }
    else {
        nodeInfo = aPrototype->mNodeInfo;
    }

    nsRefPtr<nsXULElement> element = Create(aPrototype, nodeInfo,
                                            aIsScriptable);
    if (!element) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_ADDREF(*aResult = element.get());

    return NS_OK;
}

nsresult
NS_NewXULElement(nsIContent** aResult, nsINodeInfo *aNodeInfo)
{
    NS_PRECONDITION(aNodeInfo, "need nodeinfo for non-proto Create");

    *aResult = nsnull;

    
    nsXULElement* element = new nsXULElement(aNodeInfo);
    NS_ENSURE_TRUE(element, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(*aResult = element);

    return NS_OK;
}




NS_IMPL_CYCLE_COLLECTION_CLASS(nsXULElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXULElement,
                                                  nsGenericElement)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mPrototype,
                                                    nsXULPrototypeElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsXULElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsXULElement, nsGenericElement)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsXULElement)
    NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsXULElement)
        NS_INTERFACE_TABLE_ENTRY(nsXULElement, nsIDOMNode)
        NS_INTERFACE_TABLE_ENTRY(nsXULElement, nsIDOMElement)
        NS_INTERFACE_TABLE_ENTRY(nsXULElement, nsIDOMXULElement)
    NS_OFFSET_AND_INTERFACE_TABLE_END
    NS_ELEMENT_INTERFACE_TABLE_TO_MAP_SEGUE
    NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIScriptEventHandlerOwner,
                                   new nsScriptEventHandlerOwnerTearoff(this))
    NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOMElementCSSInlineStyle,
                                   new nsXULElementTearoff(this))
    NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIFrameLoaderOwner,
                                   new nsXULElementTearoff(this))
    NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(XULElement)
NS_ELEMENT_INTERFACE_MAP_END




nsresult
nsXULElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
    *aResult = nsnull;

    
    nsRefPtr<nsXULElement> element;
    if (mPrototype) {
        element = nsXULElement::Create(mPrototype, aNodeInfo, PR_TRUE);
        NS_ASSERTION(GetScriptTypeID() == mPrototype->mScriptTypeID,
                     "Didn't get the default language from proto?");
    }
    else {
        element = new nsXULElement(aNodeInfo);
        if (element) {
        	
        	
        	element->SetScriptTypeID(GetScriptTypeID());
        }
    }

    if (!element) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    
    
    

    

    nsresult rv = CopyInnerTo(element);
    if (NS_SUCCEEDED(rv)) {
        NS_ADDREF(*aResult = element);
    }

    return rv;
}



NS_IMETHODIMP
nsXULElement::GetElementsByAttribute(const nsAString& aAttribute,
                                     const nsAString& aValue,
                                     nsIDOMNodeList** aReturn)
{
    nsCOMPtr<nsIAtom> attrAtom(do_GetAtom(aAttribute));
    NS_ENSURE_TRUE(attrAtom, NS_ERROR_OUT_OF_MEMORY);
    void* attrValue = new nsString(aValue);
    NS_ENSURE_TRUE(attrValue, NS_ERROR_OUT_OF_MEMORY);
    nsContentList *list = 
        new nsContentList(this,
                          nsXULDocument::MatchAttribute,
                          nsContentUtils::DestroyMatchString,
                          attrValue,
                          PR_TRUE,
                          attrAtom,
                          kNameSpaceID_Unknown);
    NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(*aReturn = list);
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetElementsByAttributeNS(const nsAString& aNamespaceURI,
                                       const nsAString& aAttribute,
                                       const nsAString& aValue,
                                       nsIDOMNodeList** aReturn)
{
    nsCOMPtr<nsIAtom> attrAtom(do_GetAtom(aAttribute));
    NS_ENSURE_TRUE(attrAtom, NS_ERROR_OUT_OF_MEMORY);

    PRInt32 nameSpaceId = kNameSpaceID_Wildcard;
    if (!aNamespaceURI.EqualsLiteral("*")) {
      nsresult rv =
        nsContentUtils::NameSpaceManager()->RegisterNameSpace(aNamespaceURI,
                                                              nameSpaceId);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    void* attrValue = new nsString(aValue);
    NS_ENSURE_TRUE(attrValue, NS_ERROR_OUT_OF_MEMORY);
    
    nsContentList *list = 
        new nsContentList(this,
                          nsXULDocument::MatchAttribute,
                          nsContentUtils::DestroyMatchString,
                          attrValue,
                          PR_TRUE,
                          attrAtom,
                          nameSpaceId);
    NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(*aReturn = list);
    return NS_OK;
}

nsresult
nsXULElement::GetEventListenerManagerForAttr(nsIEventListenerManager** aManager,
                                             nsISupports** aTarget,
                                             PRBool* aDefer)
{
    
    
    
    nsIDocument* doc = GetOwnerDoc();
    if (!doc)
        return NS_ERROR_UNEXPECTED; 

    nsPIDOMWindow *window;
    nsIContent *root = doc->GetRootContent();
    if ((!root || root == this) && !mNodeInfo->Equals(nsGkAtoms::overlay) &&
        (window = doc->GetInnerWindow()) && window->IsInnerWindow()) {

        nsCOMPtr<nsPIDOMEventTarget> piTarget = do_QueryInterface(window);
        if (!piTarget)
            return NS_ERROR_UNEXPECTED;

        *aDefer = PR_FALSE;
        *aManager = piTarget->GetListenerManager(PR_TRUE);
        NS_ENSURE_STATE(*aManager);
        NS_ADDREF(*aManager);
        NS_ADDREF(*aTarget = window);
        return NS_OK;
    }

    return nsGenericElement::GetEventListenerManagerForAttr(aManager,
                                                            aTarget,
                                                            aDefer);
}

PRBool
nsXULElement::IsFocusable(PRInt32 *aTabIndex)
{
  



























  
  PRBool shouldFocus = PR_FALSE;

  nsCOMPtr<nsIDOMXULControlElement> xulControl = 
    do_QueryInterface(static_cast<nsIContent*>(this));
  if (xulControl) {
    
    PRBool disabled;
    xulControl->GetDisabled(&disabled);
    if (disabled) {
      if (aTabIndex)
        *aTabIndex = -1;
      return PR_FALSE;
    }
    shouldFocus = PR_TRUE;
  }

  if (aTabIndex) {
    if (xulControl) {
      if (HasAttr(kNameSpaceID_None, nsGkAtoms::tabindex)) {
        
        
        PRInt32 tabIndex = 0;
        xulControl->GetTabIndex(&tabIndex);
        shouldFocus = *aTabIndex >= 0 || tabIndex >= 0;
        *aTabIndex = tabIndex;
      }
      else {
        
        
        shouldFocus = *aTabIndex >= 0;
      }

      if (shouldFocus && sTabFocusModelAppliesToXUL &&
          !(sTabFocusModel & eTabFocus_formElementsMask)) {
        
        
        
        
        
        
        
        if (!mNodeInfo->Equals(nsGkAtoms::tree) && !mNodeInfo->Equals(nsGkAtoms::listbox))
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
nsXULElement::PerformAccesskey(PRBool aKeyCausesActivation,
                               PRBool aIsTrustedEvent)
{
    nsCOMPtr<nsIContent> content(this);

    if (Tag() == nsGkAtoms::label) {
        nsCOMPtr<nsIDOMElement> element;

        nsAutoString control;
        GetAttr(kNameSpaceID_None, nsGkAtoms::control, control);
        if (!control.IsEmpty()) {
            nsCOMPtr<nsIDOMDocument> domDocument =
                do_QueryInterface(content->GetCurrentDoc());
            if (domDocument)
                domDocument->GetElementById(control, getter_AddRefs(element));
        }
        
        
        content = do_QueryInterface(element);

        if (!content)
            return;
    }

    nsIDocument* doc = GetCurrentDoc();
    if (!doc)
        return;

    nsIPresShell *shell = doc->GetPrimaryShell();
    if (!shell)
        return;

    nsIFrame* frame = shell->GetPrimaryFrameFor(content);
    if (!frame)
        return;

    const nsStyleVisibility* vis = frame->GetStyleVisibility();

    if (vis->mVisible == NS_STYLE_VISIBILITY_COLLAPSE ||
        vis->mVisible == NS_STYLE_VISIBILITY_HIDDEN ||
        !frame->AreAncestorViewsVisible())
        return;

    nsCOMPtr<nsIDOMXULElement> elm(do_QueryInterface(content));
    if (elm) {
        
        nsIAtom *tag = content->Tag();
        if (tag != nsGkAtoms::toolbarbutton) {
          nsIFocusManager* fm = nsFocusManager::GetFocusManager();
          if (fm) {
            nsCOMPtr<nsIDOMElement> element;
            
            if (tag == nsGkAtoms::radio) {
              nsCOMPtr<nsIDOMXULSelectControlItemElement> controlItem(do_QueryInterface(elm));
              if (controlItem) {
                PRBool disabled;
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
        if (aKeyCausesActivation && tag != nsGkAtoms::textbox && tag != nsGkAtoms::menulist)
            elm->Click();
    }
    else {
        content->PerformAccesskey(aKeyCausesActivation, aIsTrustedEvent);
    }
}





NS_IMPL_CYCLE_COLLECTION_CLASS(nsScriptEventHandlerOwnerTearoff)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsScriptEventHandlerOwnerTearoff)
  tmp->mElement = nsnull;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsScriptEventHandlerOwnerTearoff)
  cb.NoteXPCOMChild(static_cast<nsIContent*>(tmp->mElement));
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsScriptEventHandlerOwnerTearoff)
  NS_INTERFACE_MAP_ENTRY(nsIScriptEventHandlerOwner)
NS_INTERFACE_MAP_END_AGGREGATED(mElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsScriptEventHandlerOwnerTearoff)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsScriptEventHandlerOwnerTearoff)

nsresult
nsScriptEventHandlerOwnerTearoff::GetCompiledEventHandler(
                                                nsIAtom *aName,
                                                nsScriptObjectHolder &aHandler)
{
    XUL_PROTOTYPE_ATTRIBUTE_METER(gNumCacheTests);
    aHandler.drop();

    nsXULPrototypeAttribute *attr =
        mElement->FindPrototypeAttribute(kNameSpaceID_None, aName);
    if (attr) {
        XUL_PROTOTYPE_ATTRIBUTE_METER(gNumCacheHits);
        aHandler.set(attr->mEventHandler);
    }

    return NS_OK;
}

nsresult
nsScriptEventHandlerOwnerTearoff::CompileEventHandler(
                                                nsIScriptContext* aContext,
                                                nsISupports* aTarget,
                                                nsIAtom *aName,
                                                const nsAString& aBody,
                                                const char* aURL,
                                                PRUint32 aLineNo,
                                                nsScriptObjectHolder &aHandler)
{
    nsresult rv;

    XUL_PROTOTYPE_ATTRIBUTE_METER(gNumCacheSets);

    
    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(mElement->GetOwnerDoc());

    nsIScriptContext *context;
    nsXULPrototypeElement *elem = mElement->mPrototype;
    if (elem && xuldoc) {
        

        
        
        
        
        
        
        
        nsCOMPtr<nsIScriptGlobalObjectOwner> globalOwner;
        rv = xuldoc->GetScriptGlobalObjectOwner(getter_AddRefs(globalOwner));
        NS_ENSURE_SUCCESS(rv, rv);
        NS_ENSURE_TRUE(globalOwner, NS_ERROR_UNEXPECTED);

        nsIScriptGlobalObject* global = globalOwner->GetScriptGlobalObject();
        NS_ENSURE_TRUE(global, NS_ERROR_UNEXPECTED);

        context = global->GetScriptContext(aContext->GetScriptTypeID());
        
        
        
        NS_ASSERTION(context,
                     "Failed to get a language context from the global!?");
        NS_ENSURE_TRUE(context, NS_ERROR_UNEXPECTED);
    }
    else {
        
        NS_ASSERTION(aTarget != nsnull, "no prototype and no target?!");
        context = aContext;
    }

    
    PRUint32 argCount;
    const char **argNames;
    nsContentUtils::GetEventArgNames(kNameSpaceID_XUL, aName, &argCount,
                                     &argNames);

    nsCxPusher pusher;
    if (!pusher.Push((JSContext*)context->GetNativeContext())) {
      return NS_ERROR_FAILURE;
    }

    rv = context->CompileEventHandler(aName, argCount, argNames,
                                      aBody, aURL, aLineNo,
                                      SCRIPTVERSION_DEFAULT,  
                                      aHandler);
    if (NS_FAILED(rv)) return rv;

    
    
    rv = aContext->BindCompiledEventHandler(aTarget, aContext->GetNativeGlobal(),
                                            aName, aHandler);
    if (NS_FAILED(rv)) return rv;

    nsXULPrototypeAttribute *attr =
        mElement->FindPrototypeAttribute(kNameSpaceID_None, aName);
    if (attr) {
        XUL_PROTOTYPE_ATTRIBUTE_METER(gNumCacheFills);
        
        if (aHandler) {
            NS_ASSERTION(!attr->mEventHandler, "Leaking handler.");

            rv = nsContentUtils::HoldScriptObject(aContext->GetScriptTypeID(),
                                                  elem,
                                                  &NS_CYCLE_COLLECTION_NAME(nsXULPrototypeNode),
                                                  aHandler,
                                                  elem->mHoldsScriptObject);
            if (NS_FAILED(rv)) return rv;

            elem->mHoldsScriptObject = PR_TRUE;
        }
        attr->mEventHandler = (void *)aHandler;
    }

    return NS_OK;
}

void
nsXULElement::AddListenerFor(const nsAttrName& aName,
                             PRBool aCompileEventHandlers)
{
    
    
    
    
    if (aName.IsAtom()) {
        nsIAtom *attr = aName.Atom();
        MaybeAddPopupListener(attr);
        if (aCompileEventHandlers &&
            nsContentUtils::IsEventAttributeName(attr, EventNameType_XUL)) {
            nsAutoString value;
            GetAttr(kNameSpaceID_None, attr, value);
            AddScriptEventListener(attr, value, PR_TRUE);
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






nsresult
nsXULElement::BindToTree(nsIDocument* aDocument,
                         nsIContent* aParent,
                         nsIContent* aBindingParent,
                         PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericElement::BindToTree(aDocument, aParent,
                                             aBindingParent,
                                             aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument) {
      NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
                   "Missing a script blocker!");
      
      LoadSrc();
  }

  return rv;
}

void
nsXULElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
    
    
    
    
    
    
    
    
    
    
    
    nsXULSlots* slots = static_cast<nsXULSlots*>(GetExistingDOMSlots());
    if (slots) {
        NS_IF_RELEASE(slots->mControllers);
        if (slots->mFrameLoader) {
            
            
            
            
            
            slots->mFrameLoader->Destroy();
            slots->mFrameLoader = nsnull;
        }
    }

    nsGenericElement::UnbindFromTree(aDeep, aNullParent);
}

nsresult
nsXULElement::RemoveChildAt(PRUint32 aIndex, PRBool aNotify, PRBool aMutationEvent)
{
    NS_ASSERTION(aMutationEvent, "Someone tried to inhibit mutations on XUL child removal.");
    nsresult rv;
    nsCOMPtr<nsIContent> oldKid = mAttrsAndChildren.GetSafeChildAt(aIndex);
    if (!oldKid) {
      return NS_OK;
    }

    
    
    
    nsCOMPtr<nsIDOMXULMultiSelectControlElement> controlElement;
    nsCOMPtr<nsIListBoxObject> listBox;
    PRBool fireSelectionHandler = PR_FALSE;

    
    
    PRInt32 newCurrentIndex = -1;

    if (oldKid->NodeInfo()->Equals(nsGkAtoms::listitem, kNameSpaceID_XUL)) {
      
      
      
      
      controlElement = do_QueryInterface(static_cast<nsIContent*>(this));

      
      if (!controlElement)
        rv = GetParentTree(getter_AddRefs(controlElement));

      nsCOMPtr<nsIDOMElement> oldKidElem = do_QueryInterface(oldKid);
      if (controlElement && oldKidElem) {
        
        
        PRInt32 length;
        controlElement->GetSelectedCount(&length);
        for (PRInt32 i = 0; i < length; i++) {
          nsCOMPtr<nsIDOMXULSelectControlItemElement> node;
          controlElement->GetSelectedItem(i, getter_AddRefs(node));
          
          nsCOMPtr<nsIDOMElement> selElem = do_QueryInterface(node);
          if (selElem == oldKidElem &&
              NS_SUCCEEDED(controlElement->RemoveItemFromSelection(node))) {
            length--;
            i--;
            fireSelectionHandler = PR_TRUE;
          }
        }

        nsCOMPtr<nsIDOMXULSelectControlItemElement> curItem;
        controlElement->GetCurrentItem(getter_AddRefs(curItem));
        nsCOMPtr<nsIContent> curNode = do_QueryInterface(curItem);
        if (curNode && nsContentUtils::ContentIsDescendantOf(curNode, oldKid)) {
            
            nsCOMPtr<nsIBoxObject> box;
            controlElement->GetBoxObject(getter_AddRefs(box));
            listBox = do_QueryInterface(box);
            if (listBox && oldKidElem) {
              listBox->GetIndexOfItem(oldKidElem, &newCurrentIndex);
            }

            
            if (newCurrentIndex == -1)
              newCurrentIndex = -2;
        }
      }
    }

    rv = nsGenericElement::RemoveChildAt(aIndex, aNotify, aMutationEvent);
    
    if (newCurrentIndex == -2)
        controlElement->SetCurrentItem(nsnull);
    else if (newCurrentIndex > -1) {
        
        PRInt32 treeRows;
        listBox->GetRowCount(&treeRows);
        if (treeRows > 0) {
            newCurrentIndex = PR_MIN((treeRows - 1), newCurrentIndex);
            nsCOMPtr<nsIDOMElement> newCurrentItem;
            listBox->GetItemAtIndex(newCurrentIndex, getter_AddRefs(newCurrentItem));
            nsCOMPtr<nsIDOMXULSelectControlItemElement> xulCurItem = do_QueryInterface(newCurrentItem);
            if (xulCurItem)
                controlElement->SetCurrentItem(xulCurItem);
        } else {
            controlElement->SetCurrentItem(nsnull);
        }
    }

    nsIDocument* doc;
    if (fireSelectionHandler && (doc = GetCurrentDoc())) {
      nsContentUtils::DispatchTrustedEvent(doc,
                                           static_cast<nsIContent*>(this),
                                           NS_LITERAL_STRING("select"),
                                           PR_FALSE,
                                           PR_TRUE);
    }

    return rv;
}

void
nsXULElement::UnregisterAccessKey(const nsAString& aOldValue)
{
    
    
    nsIDocument* doc = GetCurrentDoc();
    if (doc && !aOldValue.IsEmpty()) {
        nsIPresShell *shell = doc->GetPrimaryShell();

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
nsXULElement::BeforeSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                            const nsAString* aValue, PRBool aNotify)
{
    if (aNamespaceID == kNameSpaceID_None && aName == nsGkAtoms::accesskey &&
        IsInDoc()) {
        const nsAttrValue* attrVal = FindLocalOrProtoAttr(aNamespaceID, aName);
        if (attrVal) {
            nsAutoString oldValue;
            attrVal->ToString(oldValue);
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

    return nsGenericElement::BeforeSetAttr(aNamespaceID, aName,
                                           aValue, aNotify);
}

nsresult
nsXULElement::AfterSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                           const nsAString* aValue, PRBool aNotify)
{
    if (aNamespaceID == kNameSpaceID_None) {
        

        
        
        MaybeAddPopupListener(aName);
        if (nsContentUtils::IsEventAttributeName(aName, EventNameType_XUL) && aValue) {
            
            
            
            
            
            PRBool defer = mPrototype == nsnull ||
                           mPrototype->mScriptTypeID == GetScriptTypeID();
            AddScriptEventListener(aName, *aValue, defer);
        }

        
        if (aName == nsGkAtoms::hidechrome &&
            mNodeInfo->Equals(nsGkAtoms::window) &&
            aValue) {
            HideWindowChrome(aValue->EqualsLiteral("true"));
        }

        
        
        nsIDocument *document = GetCurrentDoc();
        if (document && document->GetRootContent() == this) {
            if (aName == nsGkAtoms::title) {
                document->NotifyPossibleTitleChange(PR_FALSE);
            }
            else if ((aName == nsGkAtoms::activetitlebarcolor ||
                      aName == nsGkAtoms::inactivetitlebarcolor)) {
                nscolor color = NS_RGBA(0, 0, 0, 0);
                nsAttrValue attrValue;
                attrValue.ParseColor(*aValue, document);
                attrValue.GetColorValue(color);
                SetTitlebarColor(color, aName == nsGkAtoms::activetitlebarcolor);
            }
            else if (aName == nsGkAtoms::drawintitlebar) {
                SetDrawsInTitlebar(aValue && aValue->EqualsLiteral("true"));
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
                }
            }
        }

        if (aName == nsGkAtoms::src && document) {
            LoadSrc();
        }

        
        
    }

    return nsGenericElement::AfterSetAttr(aNamespaceID, aName,
                                          aValue, aNotify);
}

PRBool
nsXULElement::ParseAttribute(PRInt32 aNamespaceID,
                             nsIAtom* aAttribute,
                             const nsAString& aValue,
                             nsAttrValue& aResult)
{
    

    
    
    
    if (aNamespaceID == kNameSpaceID_None) {
        if (aAttribute == nsGkAtoms::style) {
            SetFlags(NODE_MAY_HAVE_STYLE);
            nsStyledElement::ParseStyleAttribute(this, aValue, aResult, PR_FALSE);
            return PR_TRUE;
        }

        if (aAttribute == nsGkAtoms::_class) {
            SetFlags(NODE_MAY_HAVE_CLASS);
            aResult.ParseAtomArray(aValue);
            return PR_TRUE;
        }
    }

    if (!nsGenericElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                          aResult)) {
        
        aResult.ParseStringOrAtom(aValue);
    }

    return PR_TRUE;
}

const nsAttrName*
nsXULElement::InternalGetExistingAttrNameFromQName(const nsAString& aStr) const
{
    NS_ConvertUTF16toUTF8 name(aStr);
    const nsAttrName* attrName =
        mAttrsAndChildren.GetExistingAttrNameFromQName(name);
    if (attrName) {
        return attrName;
    }

    if (mPrototype) {
        PRUint32 i;
        for (i = 0; i < mPrototype->mNumAttributes; ++i) {
            attrName = &mPrototype->mAttributes[i].mName;
            if (attrName->QualifiedNameEquals(name)) {
                return attrName;
            }
        }
    }

    return nsnull;
}

PRBool
nsXULElement::GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                      nsAString& aResult) const
{
    NS_ASSERTION(nsnull != aName, "must have attribute name");
    NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown,
                 "must have a real namespace ID!");

    const nsAttrValue* val = FindLocalOrProtoAttr(aNameSpaceID, aName);

    if (!val) {
        
        
        
        aResult.Truncate();
        return PR_FALSE;
    }

    val->ToString(aResult);

    return PR_TRUE;
}

PRBool
nsXULElement::HasAttr(PRInt32 aNameSpaceID, nsIAtom* aName) const
{
    NS_ASSERTION(nsnull != aName, "must have attribute name");
    NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown,
                 "must have a real namespace ID!");

    return mAttrsAndChildren.GetAttr(aName, aNameSpaceID) ||
           FindPrototypeAttribute(aNameSpaceID, aName);
}

PRBool
nsXULElement::AttrValueIs(PRInt32 aNameSpaceID,
                          nsIAtom* aName,
                          const nsAString& aValue,
                          nsCaseTreatment aCaseSensitive) const
{
  NS_ASSERTION(aName, "Must have attr name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown, "Must have namespace");

  const nsAttrValue* val = FindLocalOrProtoAttr(aNameSpaceID, aName);
  return val && val->Equals(aValue, aCaseSensitive);
}

PRBool
nsXULElement::AttrValueIs(PRInt32 aNameSpaceID,
                          nsIAtom* aName,
                          nsIAtom* aValue,
                          nsCaseTreatment aCaseSensitive) const
{
  NS_ASSERTION(aName, "Must have attr name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown, "Must have namespace");
  NS_ASSERTION(aValue, "Null value atom");

  const nsAttrValue* val = FindLocalOrProtoAttr(aNameSpaceID, aName);
  return val && val->Equals(aValue, aCaseSensitive);
}

PRInt32
nsXULElement::FindAttrValueIn(PRInt32 aNameSpaceID,
                              nsIAtom* aName,
                              AttrValuesArray* aValues,
                              nsCaseTreatment aCaseSensitive) const
{
  NS_ASSERTION(aName, "Must have attr name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown, "Must have namespace");
  NS_ASSERTION(aValues, "Null value array");
  
  const nsAttrValue* val = FindLocalOrProtoAttr(aNameSpaceID, aName);
  if (val) {
    for (PRInt32 i = 0; aValues[i]; ++i) {
      if (val->Equals(*aValues[i], aCaseSensitive)) {
        return i;
      }
    }
    return ATTR_VALUE_NO_MATCH;
  }
  return ATTR_MISSING;
}

nsresult
nsXULElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, PRBool aNotify)
{
    
    
    NS_ASSERTION(nsnull != aName, "must have attribute name");
    nsresult rv;

    
    
    
    
    
    
    
    

    nsXULPrototypeAttribute *protoattr =
        FindPrototypeAttribute(aNameSpaceID, aName);
    if (protoattr) {
        
        
        rv = MakeHeavyweight();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    PRInt32 index = mAttrsAndChildren.IndexOfAttr(aName, aNameSpaceID);
    if (index < 0) {
        NS_ASSERTION(!protoattr, "we used to have a protoattr, we should now "
                                 "have a normal one");

        return NS_OK;
    }

    nsAutoString oldValue;
    GetAttr(aNameSpaceID, aName, oldValue);

    nsIDocument* doc = GetCurrentDoc();
    mozAutoDocUpdate updateBatch(doc, UPDATE_CONTENT_MODEL, aNotify);

    
    
    PRUint32 stateMask;
    if (aNotify) {
        stateMask = PRUint32(IntrinsicState());
 
        nsNodeUtils::AttributeWillChange(this, aNameSpaceID, aName,
                                         nsIDOMMutationEvent::REMOVAL);
    }

    PRBool hasMutationListeners = aNotify &&
        nsContentUtils::HasMutationListeners(this,
            NS_EVENT_BITS_MUTATION_ATTRMODIFIED, this);

    nsCOMPtr<nsIDOMAttr> attrNode;
    if (hasMutationListeners) {
        nsAutoString attrName;
        aName->ToString(attrName);
        nsAutoString ns;
        nsContentUtils::NameSpaceManager()->GetNameSpaceURI(aNameSpaceID, ns);
        GetAttributeNodeNS(ns, attrName, getter_AddRefs(attrNode));
    }

    nsDOMSlots *slots = GetExistingDOMSlots();
    if (slots && slots->mAttributeMap) {
      slots->mAttributeMap->DropAttribute(aNameSpaceID, aName);
    }

    nsAttrValue ignored;
    rv = mAttrsAndChildren.RemoveAttrAt(index, ignored);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    

    
    
    

    if (aNameSpaceID == kNameSpaceID_None) {
        if (aName == nsGkAtoms::hidechrome &&
            mNodeInfo->Equals(nsGkAtoms::window)) {
            HideWindowChrome(PR_FALSE);
        }

        if (doc && doc->GetRootContent() == this) {
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
                }
            }
            else if (aName == nsGkAtoms::drawintitlebar) {
                SetDrawsInTitlebar(PR_FALSE);
            }
        }

        
        
        if (aName == nsGkAtoms::accesskey || aName == nsGkAtoms::control) {
            UnregisterAccessKey(oldValue);
        }

        
        
        if (doc && (aName == nsGkAtoms::observes ||
                          aName == nsGkAtoms::command)) {
            RemoveBroadcaster(oldValue);
        }
    }

    if (doc) {
        nsRefPtr<nsXBLBinding> binding =
            doc->BindingManager()->GetBinding(this);
        if (binding)
            binding->AttributeChanged(aName, aNameSpaceID, PR_TRUE, aNotify);

    }

    if (aNotify) {
        stateMask = stateMask ^ PRUint32(IntrinsicState());
        if (stateMask && doc) {
            MOZ_AUTO_DOC_UPDATE(doc, UPDATE_CONTENT_STATE, aNotify);
            doc->ContentStatesChanged(this, nsnull, stateMask);
        }
        nsNodeUtils::AttributeChanged(this, aNameSpaceID, aName,
                                      nsIDOMMutationEvent::REMOVAL,
                                      stateMask);
    }

    if (hasMutationListeners) {
        mozAutoRemovableBlockerRemover blockerRemover;

        nsMutationEvent mutation(PR_TRUE, NS_MUTATION_ATTRMODIFIED);

        mutation.mRelatedNode = attrNode;
        mutation.mAttrName = aName;

        if (!oldValue.IsEmpty())
          mutation.mPrevAttrValue = do_GetAtom(oldValue);
        mutation.mAttrChange = nsIDOMMutationEvent::REMOVAL;

        mozAutoSubtreeModified subtree(GetOwnerDoc(), this);
        nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this),
                                    nsnull, &mutation);
    }

    return NS_OK;
}

void
nsXULElement::RemoveBroadcaster(const nsAString & broadcasterId)
{
    nsCOMPtr<nsIDOMXULDocument> xuldoc = do_QueryInterface(GetOwnerDoc());
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

const nsAttrName*
nsXULElement::GetAttrNameAt(PRUint32 aIndex) const
{
    PRUint32 localCount = mAttrsAndChildren.AttrCount();
    PRUint32 protoCount = mPrototype ? mPrototype->mNumAttributes : 0;

    if (localCount > protoCount) {
        

        
        if (aIndex < localCount) {
            return mAttrsAndChildren.AttrNameAt(aIndex);
        }

        aIndex -= localCount;

        
        
        for (PRUint32 i = 0; i < protoCount; i++) {
            const nsAttrName* name = &mPrototype->mAttributes[i].mName;
            if (mAttrsAndChildren.GetAttr(name->LocalName(), name->NamespaceID())) {
                aIndex++;
            }
            if (i == aIndex) {
                return name;
            }
        }
    }
    else {
        

        
        if (aIndex < protoCount) {
            return &mPrototype->mAttributes[aIndex].mName;
        }

        aIndex -= protoCount;

        
        
        for (PRUint32 i = 0; i < localCount; i++) {
            const nsAttrName* name = mAttrsAndChildren.AttrNameAt(i);

            for (PRUint32 j = 0; j < protoCount; j++) {
                if (mPrototype->mAttributes[j].mName.Equals(*name)) {
                    aIndex++;
                    break;
                }
            }
            if (i == aIndex) {
                return name;
            }
        }
    }

    return nsnull;
}

PRUint32
nsXULElement::GetAttrCount() const
{
    PRUint32 localCount = mAttrsAndChildren.AttrCount();
    PRUint32 protoCount = mPrototype ? mPrototype->mNumAttributes : 0;

    if (localCount > protoCount) {
        
        PRUint32 count = localCount;

        for (PRUint32 i = 0; i < protoCount; i++) {
            const nsAttrName* name = &mPrototype->mAttributes[i].mName;
            if (!mAttrsAndChildren.GetAttr(name->LocalName(), name->NamespaceID())) {
                count++;
            }
        }

        return count;
    }

    
    PRUint32 count = protoCount;

    for (PRUint32 i = 0; i < localCount; i++) {
        const nsAttrName* name = mAttrsAndChildren.AttrNameAt(i);

        count++;
        for (PRUint32 j = 0; j < protoCount; j++) {
            if (mPrototype->mAttributes[j].mName.Equals(*name)) {
                count--;
                break;
            }
        }
    }

    return count;
}

void
nsXULElement::DestroyContent()
{
    nsXULSlots* slots = static_cast<nsXULSlots*>(GetExistingDOMSlots());
    if (slots) {
        NS_IF_RELEASE(slots->mControllers);
        if (slots->mFrameLoader) {
            slots->mFrameLoader->Destroy();
            slots->mFrameLoader = nsnull;
        }
    }

    nsGenericElement::DestroyContent();
}

#ifdef DEBUG
void
nsXULElement::List(FILE* out, PRInt32 aIndent) const
{
    nsCString prefix("XUL");
    if (HasSlots()) {
      prefix.Append('*');
    }
    prefix.Append(' ');

    nsGenericElement::List(out, aIndent, prefix);
}
#endif

nsresult
nsXULElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
    aVisitor.mForceContentDispatch = PR_TRUE; 
    nsIAtom* tag = Tag();
    if (IsRootOfNativeAnonymousSubtree() &&
        (tag == nsGkAtoms::scrollbar || tag == nsGkAtoms::scrollcorner) &&
        (aVisitor.mEvent->message == NS_MOUSE_CLICK ||
         aVisitor.mEvent->message == NS_MOUSE_DOUBLECLICK ||
         aVisitor.mEvent->message == NS_XUL_COMMAND ||
         aVisitor.mEvent->message == NS_CONTEXTMENU ||
         aVisitor.mEvent->message == NS_DRAGDROP_START ||
         aVisitor.mEvent->message == NS_DRAGDROP_GESTURE)) {
        
        aVisitor.mCanHandle = PR_TRUE;
        aVisitor.mParentTarget = nsnull;
        return NS_OK;
    }
    if (aVisitor.mEvent->message == NS_XUL_COMMAND &&
        aVisitor.mEvent->eventStructType == NS_INPUT_EVENT &&
        aVisitor.mEvent->originalTarget == static_cast<nsIContent*>(this) &&
        tag != nsGkAtoms::command) {
        
        
        nsCOMPtr<nsIDOMXULCommandEvent> xulEvent =
            do_QueryInterface(aVisitor.mDOMEvent);
        
        
        nsAutoString command;
        if (xulEvent && GetAttr(kNameSpaceID_None, nsGkAtoms::command, command) &&
            !command.IsEmpty()) {
            
            
            aVisitor.mCanHandle = PR_FALSE;

            
            nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(GetCurrentDoc()));
            NS_ENSURE_STATE(domDoc);
            nsCOMPtr<nsIDOMElement> commandElt;
            domDoc->GetElementById(command, getter_AddRefs(commandElt));
            nsCOMPtr<nsIContent> commandContent(do_QueryInterface(commandElt));
            if (commandContent) {
                
                
                
                
                nsCOMPtr<nsIDOMNSEvent> nsevent =
                    do_QueryInterface(aVisitor.mDOMEvent);
                while (nsevent) {
                    nsCOMPtr<nsIDOMEventTarget> oTarget;
                    nsevent->GetOriginalTarget(getter_AddRefs(oTarget));
                    NS_ENSURE_STATE(!SameCOMIdentity(oTarget, commandContent));
                    nsCOMPtr<nsIDOMEvent> tmp;
                    nsCOMPtr<nsIDOMXULCommandEvent> commandEvent =
                        do_QueryInterface(nsevent);
                    if (commandEvent) {
                        commandEvent->GetSourceEvent(getter_AddRefs(tmp));
                    }
                    nsevent = do_QueryInterface(tmp);
                }

                nsInputEvent* orig =
                    static_cast<nsInputEvent*>(aVisitor.mEvent);
                nsContentUtils::DispatchXULCommand(
                  commandContent,
                  NS_IS_TRUSTED_EVENT(aVisitor.mEvent),
                  aVisitor.mDOMEvent,
                  nsnull,
                  orig->isControl,
                  orig->isAlt,
                  orig->isShift,
                  orig->isMeta);
            } else {
                NS_WARNING("A XUL element is attached to a command that doesn't exist!\n");
            }
            return NS_OK;
        }
    }

    return nsGenericElement::PreHandleEvent(aVisitor);
}


NS_IMETHODIMP
nsXULElement::GetResource(nsIRDFResource** aResource)
{
    nsAutoString id;
    GetAttr(kNameSpaceID_None, nsGkAtoms::ref, id);
    if (id.IsEmpty()) {
        GetAttr(kNameSpaceID_None, nsGkAtoms::id, id);
    }

    if (!id.IsEmpty()) {
        return nsXULContentUtils::RDFService()->
            GetUnicodeResource(id, aResource);
    }
    *aResource = nsnull;

    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetDatabase(nsIRDFCompositeDataSource** aDatabase)
{
    nsCOMPtr<nsIXULTemplateBuilder> builder;
    GetBuilder(getter_AddRefs(builder));

    if (builder)
        builder->GetDatabase(aDatabase);
    else
        *aDatabase = nsnull;

    return NS_OK;
}


NS_IMETHODIMP
nsXULElement::GetBuilder(nsIXULTemplateBuilder** aBuilder)
{
    *aBuilder = nsnull;

    
    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(GetCurrentDoc());
    if (xuldoc)
        xuldoc->GetTemplateBuilderFor(this, aBuilder);

    return NS_OK;
}






nsIAtom*
nsXULElement::GetID() const
{
    if (!HasFlag(NODE_MAY_HAVE_ID)) {
        return nsnull;
    }

    const nsAttrValue* attrVal = FindLocalOrProtoAttr(kNameSpaceID_None, nsGkAtoms::id);

    NS_ASSERTION(!attrVal ||
                 attrVal->Type() == nsAttrValue::eAtom ||
                 (attrVal->Type() == nsAttrValue::eString &&
                  attrVal->GetStringValue().IsEmpty()),
                 "unexpected attribute type");

    if (attrVal && attrVal->Type() == nsAttrValue::eAtom) {
        return attrVal->GetAtomValue();
    }
    return nsnull;
}

const nsAttrValue*
nsXULElement::DoGetClasses() const
{
    NS_ASSERTION(HasFlag(NODE_MAY_HAVE_CLASS), "Unexpected call");
    return FindLocalOrProtoAttr(kNameSpaceID_None, nsGkAtoms::_class);
}

NS_IMETHODIMP
nsXULElement::WalkContentStyleRules(nsRuleWalker* aRuleWalker)
{
    return NS_OK;
}

nsICSSStyleRule*
nsXULElement::GetInlineStyleRule()
{
    if (!HasFlag(NODE_MAY_HAVE_STYLE)) {
        return nsnull;
    }
    
    const nsAttrValue* attrVal = FindLocalOrProtoAttr(kNameSpaceID_None, nsGkAtoms::style);

    if (attrVal && attrVal->Type() == nsAttrValue::eCSSStyleRule) {
        return attrVal->GetCSSStyleRuleValue();
    }

    return nsnull;
}

NS_IMETHODIMP
nsXULElement::SetInlineStyleRule(nsICSSStyleRule* aStyleRule, PRBool aNotify)
{
  SetFlags(NODE_MAY_HAVE_STYLE);
  PRBool modification = PR_FALSE;
  nsAutoString oldValueStr;

  PRBool hasListeners = aNotify &&
    nsContentUtils::HasMutationListeners(this,
                                         NS_EVENT_BITS_MUTATION_ATTRMODIFIED,
                                         this);

  
  
  
  
  if (hasListeners) {
    
    
    
    modification = GetAttr(kNameSpaceID_None, nsGkAtoms::style,
                           oldValueStr);
  }
  else if (aNotify && IsInDoc()) {
    modification = !!mAttrsAndChildren.GetAttr(nsGkAtoms::style);
  }

  nsAttrValue attrValue(aStyleRule);

  return SetAttrAndNotify(kNameSpaceID_None, nsGkAtoms::style, nsnull, oldValueStr,
                          attrValue, modification, hasListeners, aNotify, nsnull);
}

nsChangeHint
nsXULElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                     PRInt32 aModType) const
{
    nsChangeHint retval(NS_STYLE_HINT_NONE);

    if (aAttribute == nsGkAtoms::value &&
        (aModType == nsIDOMMutationEvent::REMOVAL ||
         aModType == nsIDOMMutationEvent::ADDITION)) {
      nsIAtom *tag = Tag();
      if (tag == nsGkAtoms::label || tag == nsGkAtoms::description)
        
        
        
        
        
        retval = NS_STYLE_HINT_FRAMECHANGE;
    } else {
        
        
        if (nsGkAtoms::left == aAttribute || nsGkAtoms::top == aAttribute ||
            nsGkAtoms::right == aAttribute || nsGkAtoms::bottom == aAttribute)
            retval = NS_STYLE_HINT_REFLOW;
    }

    return retval;
}

NS_IMETHODIMP_(PRBool)
nsXULElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
    return PR_FALSE;
}

nsIAtom *
nsXULElement::GetIDAttributeName() const
{
    return nsGkAtoms::id;
}

nsIAtom *
nsXULElement::GetClassAttributeName() const
{
    return nsGkAtoms::_class;
}


NS_IMETHODIMP
nsXULElement::GetControllers(nsIControllers** aResult)
{
    if (! Controllers()) {
        nsDOMSlots* slots = GetDOMSlots();
        if (!slots)
          return NS_ERROR_OUT_OF_MEMORY;

        nsresult rv;
        rv = NS_NewXULControllers(nsnull, NS_GET_IID(nsIControllers),
                                  reinterpret_cast<void**>(&slots->mControllers));

        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create a controllers");
        if (NS_FAILED(rv)) return rv;
    }

    *aResult = Controllers();
    NS_IF_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::GetBoxObject(nsIBoxObject** aResult)
{
  *aResult = nsnull;

  
  nsIDocument* nsDoc = GetOwnerDoc();

  return nsDoc ? nsDoc->GetBoxObjectFor(this, aResult) : NS_ERROR_FAILURE;
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
                   PR_TRUE);                                        \
  }

#define NS_IMPL_XUL_BOOL_ATTR(_method, _atom)                       \
  NS_IMETHODIMP                                                     \
  nsXULElement::Get##_method(PRBool* aResult)                       \
  {                                                                 \
    *aResult = BoolAttrIsTrue(nsGkAtoms::_atom);                   \
                                                                    \
    return NS_OK;                                                   \
  }                                                                 \
  NS_IMETHODIMP                                                     \
  nsXULElement::Set##_method(PRBool aValue)                         \
  {                                                                 \
    if (aValue)                                                     \
      SetAttr(kNameSpaceID_None, nsGkAtoms::_atom,                 \
              NS_LITERAL_STRING("true"), PR_TRUE);                  \
    else                                                            \
      UnsetAttr(kNameSpaceID_None, nsGkAtoms::_atom, PR_TRUE);     \
                                                                    \
    return NS_OK;                                                   \
  }


NS_IMPL_XUL_STRING_ATTR(Id, id)
NS_IMPL_XUL_STRING_ATTR(ClassName, _class)
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
nsXULElement::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
    nsresult rv;

    
    if (mPrototype &&
        !mAttrsAndChildren.GetAttr(nsGkAtoms::style, kNameSpaceID_None)) {

        nsXULPrototypeAttribute *protoattr =
                  FindPrototypeAttribute(kNameSpaceID_None, nsGkAtoms::style);
        if (protoattr && protoattr->mValue.Type() == nsAttrValue::eCSSStyleRule) {
            nsCOMPtr<nsICSSRule> ruleClone;
            rv = protoattr->mValue.GetCSSStyleRuleValue()->Clone(*getter_AddRefs(ruleClone));
            NS_ENSURE_SUCCESS(rv, rv);

            nsAttrValue value;
            nsCOMPtr<nsICSSStyleRule> styleRule = do_QueryInterface(ruleClone);
            value.SetTo(styleRule);

            rv = mAttrsAndChildren.SetAndTakeAttr(nsGkAtoms::style, value);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    nsDOMSlots* slots = GetDOMSlots();
    NS_ENSURE_TRUE(slots, NS_ERROR_OUT_OF_MEMORY);

    if (!slots->mStyle) {
        slots->mStyle = new nsDOMCSSAttributeDeclaration(this
#ifdef MOZ_SMIL
                                                         , PR_FALSE
#endif 
                                                         );
        NS_ENSURE_TRUE(slots->mStyle, NS_ERROR_OUT_OF_MEMORY);
        SetFlags(NODE_MAY_HAVE_STYLE);
    }

    NS_IF_ADDREF(*aStyle = slots->mStyle);

    return NS_OK;
}

nsresult
nsXULElement::LoadSrc()
{
    
    
    nsIAtom* tag = Tag();
    if (tag != nsGkAtoms::browser &&
        tag != nsGkAtoms::editor &&
        tag != nsGkAtoms::iframe) {
        return NS_OK;
    }
    if (!IsInDoc() ||
        !GetOwnerDoc()->GetRootContent() ||
        GetOwnerDoc()->GetRootContent()->
            NodeInfo()->Equals(nsGkAtoms::overlay, kNameSpaceID_XUL)) {
        return NS_OK;
    }
    nsXULSlots* slots = static_cast<nsXULSlots*>(GetSlots());
    NS_ENSURE_TRUE(slots, NS_ERROR_OUT_OF_MEMORY);
    if (!slots->mFrameLoader) {
        slots->mFrameLoader = nsFrameLoader::Create(this);
        NS_ENSURE_TRUE(slots->mFrameLoader, NS_OK);
    }

    return slots->mFrameLoader->LoadFrame();
}

nsresult
nsXULElement::GetFrameLoader(nsIFrameLoader **aFrameLoader)
{
    *aFrameLoader = GetFrameLoader().get();
    return NS_OK;
}

already_AddRefed<nsFrameLoader>
nsXULElement::GetFrameLoader()
{
    nsXULSlots* slots = static_cast<nsXULSlots*>(GetExistingSlots());
    if (!slots)
        return nsnull;

    nsFrameLoader* loader = slots->mFrameLoader;
    NS_IF_ADDREF(loader);
    return loader;
}

nsresult
nsXULElement::SwapFrameLoaders(nsIFrameLoaderOwner* aOtherOwner)
{
    nsCOMPtr<nsIContent> otherContent(do_QueryInterface(aOtherOwner));
    NS_ENSURE_TRUE(otherContent, NS_ERROR_NOT_IMPLEMENTED);

    nsXULElement* otherEl = FromContent(otherContent);
    NS_ENSURE_TRUE(otherEl, NS_ERROR_NOT_IMPLEMENTED);

    if (otherEl == this) {
        
        return NS_OK;
    }

    nsXULSlots *ourSlots = static_cast<nsXULSlots*>(GetExistingDOMSlots());
    nsXULSlots *otherSlots =
        static_cast<nsXULSlots*>(otherEl->GetExistingDOMSlots());
    if (!ourSlots || !ourSlots->mFrameLoader ||
        !otherSlots || !otherSlots->mFrameLoader) {
        
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    return
        ourSlots->mFrameLoader->SwapWithOtherLoader(otherSlots->mFrameLoader,
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
    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(static_cast<nsIContent*>(this));
    return fm ? fm->SetFocus(this, 0) : NS_OK;
}

NS_IMETHODIMP
nsXULElement::Blur()
{
    if (!ShouldBlur(this))
      return NS_OK;

    nsIDocument* doc = GetCurrentDoc();
    if (!doc)
      return NS_OK;

    nsIDOMWindow* win = doc->GetWindow();
    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    if (win && fm)
      return fm->ClearFocus(win);
    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::Click()
{
    if (BoolAttrIsTrue(nsGkAtoms::disabled))
        return NS_OK;

    nsCOMPtr<nsIDocument> doc = GetCurrentDoc(); 
    if (doc) {
        nsPresShellIterator iter(doc);
        nsCOMPtr<nsIPresShell> shell;
        while ((shell = iter.GetNextShell())) {
            
            nsCOMPtr<nsPresContext> context = shell->GetPresContext();

            PRBool isCallerChrome = nsContentUtils::IsCallerChrome();

            nsMouseEvent eventDown(isCallerChrome, NS_MOUSE_BUTTON_DOWN,
                                   nsnull, nsMouseEvent::eReal);
            nsMouseEvent eventUp(isCallerChrome, NS_MOUSE_BUTTON_UP,
                                 nsnull, nsMouseEvent::eReal);
            nsMouseEvent eventClick(isCallerChrome, NS_MOUSE_CLICK, nsnull,
                                    nsMouseEvent::eReal);

            
            nsEventStatus status = nsEventStatus_eIgnore;
            nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this),
                                        context, &eventDown,  nsnull, &status);

            
            status = nsEventStatus_eIgnore;  
            nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this),
                                        context, &eventUp, nsnull, &status);

            
            status = nsEventStatus_eIgnore;  
            nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this),
                                        context, &eventClick, nsnull, &status);
        }
    }

    
    return DoCommand();
}

NS_IMETHODIMP
nsXULElement::DoCommand()
{
    nsCOMPtr<nsIDocument> doc = GetCurrentDoc(); 
    if (doc) {
        nsContentUtils::DispatchXULCommand(this, PR_TRUE);
    }

    return NS_OK;
}

nsIContent *
nsXULElement::GetBindingParent() const
{
    return mBindingParent;
}

PRBool
nsXULElement::IsNodeOfType(PRUint32 aFlags) const
{
    return !(aFlags & ~(eCONTENT | eELEMENT));
}

static void
PopupListenerPropertyDtor(void* aObject, nsIAtom* aPropertyName,
                          void* aPropertyValue, void* aData)
{
  nsIDOMEventListener* listener =
    static_cast<nsIDOMEventListener*>(aPropertyValue);
  if (!listener) {
    return;
  }
  nsCOMPtr<nsIDOM3EventTarget> target =
    do_QueryInterface(static_cast<nsINode*>(aObject));
  if (target) {
    nsCOMPtr<nsIDOMEventGroup> systemGroup;
    static_cast<nsPIDOMEventTarget*>(aObject)->
      GetSystemEventGroup(getter_AddRefs(systemGroup));
    if (systemGroup) {
      target->RemoveGroupedEventListener(NS_LITERAL_STRING("mousedown"),
                                         listener, PR_FALSE, systemGroup);

      target->RemoveGroupedEventListener(NS_LITERAL_STRING("contextmenu"),
                                         listener, PR_FALSE, systemGroup);
    }
  }
  NS_RELEASE(listener);
}

nsresult
nsXULElement::AddPopupListener(nsIAtom* aName)
{
    
    PRBool isContext = (aName == nsGkAtoms::context ||
                        aName == nsGkAtoms::contextmenu);
    nsIAtom* listenerAtom = isContext ?
                            nsGkAtoms::contextmenulistener :
                            nsGkAtoms::popuplistener;

    nsCOMPtr<nsIDOMEventListener> popupListener =
        static_cast<nsIDOMEventListener*>(GetProperty(listenerAtom));
    if (popupListener) {
        
        return NS_OK;
    }

    nsCOMPtr<nsIDOMEventGroup> systemGroup;
    GetSystemEventGroup(getter_AddRefs(systemGroup));
    NS_ENSURE_STATE(systemGroup);

    nsresult rv = NS_NewXULPopupListener(this, isContext,
                                         getter_AddRefs(popupListener));
    if (NS_FAILED(rv))
        return rv;

    
    nsCOMPtr<nsIDOM3EventTarget> target(do_QueryInterface(static_cast<nsIContent *>(this)));
    NS_ENSURE_TRUE(target, NS_ERROR_FAILURE);
    rv = SetProperty(listenerAtom, popupListener, PopupListenerPropertyDtor,
                     PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsIDOMEventListener* listener = nsnull;
    popupListener.swap(listener);

    if (isContext) {
      target->AddGroupedEventListener(NS_LITERAL_STRING("contextmenu"),
                                      listener, PR_FALSE, systemGroup);
    } else {
      target->AddGroupedEventListener(NS_LITERAL_STRING("mousedown"),
                                      listener, PR_FALSE, systemGroup);
    }
    return NS_OK;
}

PRInt32
nsXULElement::IntrinsicState() const
{
    PRInt32 state = nsGenericElement::IntrinsicState();

    const nsIAtom* tag = Tag();
    if (GetNameSpaceID() == kNameSpaceID_XUL &&
        (tag == nsGkAtoms::textbox || tag == nsGkAtoms::textarea) &&
        !HasAttr(kNameSpaceID_None, nsGkAtoms::readonly)) {
        state |= NS_EVENT_STATE_MOZ_READWRITE;
        state &= ~NS_EVENT_STATE_MOZ_READONLY;
    }

    return state;
}



nsGenericElement::nsAttrInfo
nsXULElement::GetAttrInfo(PRInt32 aNamespaceID, nsIAtom *aName) const
{

    nsAttrInfo info(nsGenericElement::GetAttrInfo(aNamespaceID, aName));
    if (!info.mValue) {
        nsXULPrototypeAttribute *protoattr =
            FindPrototypeAttribute(aNamespaceID, aName);
        if (protoattr) {
            return nsAttrInfo(&protoattr->mName, &protoattr->mValue);
        }
    }

    return info;
}


nsXULPrototypeAttribute *
nsXULElement::FindPrototypeAttribute(PRInt32 aNamespaceID,
                                     nsIAtom* aLocalName) const
{
    if (!mPrototype) {
        return nsnull;
    }

    PRUint32 i, count = mPrototype->mNumAttributes;
    if (aNamespaceID == kNameSpaceID_None) {
        
        for (i = 0; i < count; ++i) {
            nsXULPrototypeAttribute *protoattr = &mPrototype->mAttributes[i];
            if (protoattr->mName.Equals(aLocalName)) {
                return protoattr;
            }
        }
    }
    else {
        for (i = 0; i < count; ++i) {
            nsXULPrototypeAttribute *protoattr = &mPrototype->mAttributes[i];
            if (protoattr->mName.Equals(aLocalName, aNamespaceID)) {
                return protoattr;
            }
        }
    }

    return nsnull;
}

nsresult nsXULElement::MakeHeavyweight()
{
    if (!mPrototype)
        return NS_OK;           

    nsRefPtr<nsXULPrototypeElement> proto;
    proto.swap(mPrototype);

    PRBool hadAttributes = mAttrsAndChildren.AttrCount() > 0;

    PRUint32 i;
    nsresult rv;
    for (i = 0; i < proto->mNumAttributes; ++i) {
        nsXULPrototypeAttribute* protoattr = &proto->mAttributes[i];

        
        
        if (hadAttributes &&
            mAttrsAndChildren.GetAttr(protoattr->mName.LocalName(),
                                      protoattr->mName.NamespaceID())) {
            continue;
        }

        
        nsAttrValue attrValue(protoattr->mValue);
        
        
        if (attrValue.Type() == nsAttrValue::eCSSStyleRule) {
            nsCOMPtr<nsICSSRule> ruleClone;
            rv = attrValue.GetCSSStyleRuleValue()->Clone(*getter_AddRefs(ruleClone));
            NS_ENSURE_SUCCESS(rv, rv);

            nsCOMPtr<nsICSSStyleRule> styleRule = do_QueryInterface(ruleClone);
            attrValue.SetTo(styleRule);
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
nsXULElement::HideWindowChrome(PRBool aShouldHide)
{
    nsIDocument* doc = GetCurrentDoc();
    if (!doc || doc->GetRootContent() != this)
      return NS_ERROR_UNEXPECTED;

    
    if (!doc->IsRootDisplayDocument())
      return NS_OK;

    nsIPresShell *shell = doc->GetPrimaryShell();

    if (shell) {
        nsIContent* content = static_cast<nsIContent*>(this);
        nsIFrame* frame = shell->GetPrimaryFrameFor(content);

        nsPresContext *presContext = shell->GetPresContext();

        if (frame && presContext && presContext->IsChrome()) {
            nsIView* view = frame->GetClosestView();

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
    nsIDocument* doc = GetCurrentDoc();

    
    if (doc->IsRootDisplayDocument()) {
        nsCOMPtr<nsISupports> container = doc->GetContainer();
        nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(container);
        if (baseWindow) {
            nsCOMPtr<nsIWidget> mainWidget;
            baseWindow->GetMainWidget(getter_AddRefs(mainWidget));
            return mainWidget;
        }
    }
    return nsnull;
}

void
nsXULElement::SetTitlebarColor(nscolor aColor, PRBool aActive)
{
    nsIWidget* mainWidget = GetWindowWidget();
    if (mainWidget) {
        mainWidget->SetWindowTitlebarColor(aColor, aActive);
    }
}

void
nsXULElement::SetDrawsInTitlebar(PRBool aState)
{
    nsIWidget* mainWidget = GetWindowWidget();
    if (mainWidget) {
        mainWidget->SetDrawsInTitlebar(aState);
    }
}

PRBool
nsXULElement::BoolAttrIsTrue(nsIAtom* aName)
{
    const nsAttrValue* attr =
        FindLocalOrProtoAttr(kNameSpaceID_None, aName);

    return attr && attr->Type() == nsAttrValue::eAtom &&
           attr->GetAtomValue() == nsGkAtoms::_true;
}

void
nsXULElement::RecompileScriptEventListeners()
{
    PRInt32 i, count = mAttrsAndChildren.AttrCount();
    PRBool haveLocalAttributes = (count > 0);
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
        AddScriptEventListener(attr, value, PR_TRUE);
    }

    if (mPrototype) {
        
        
        
        NS_ASSERTION(mPrototype->mScriptTypeID == GetScriptTypeID(),
                     "Prototype and node confused about default language?");

        count = mPrototype->mNumAttributes;
        for (i = 0; i < count; ++i) {
            const nsAttrName &name = mPrototype->mAttributes[i].mName;

            
            if (!name.IsAtom()) {
                continue;
            }

            nsIAtom *attr = name.Atom();

            
            if (haveLocalAttributes && mAttrsAndChildren.GetAttr(attr)) {
                continue;
            }

            if (!nsContentUtils::IsEventAttributeName(attr, EventNameType_XUL)) {
                continue;
            }

            nsAutoString value;
            GetAttr(kNameSpaceID_None, attr, value);
            AddScriptEventListener(attr, value, PR_TRUE);
        }
    }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXULPrototypeNode)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_NATIVE(nsXULPrototypeNode)
    if (tmp->mType == nsXULPrototypeNode::eType_Element) {
        static_cast<nsXULPrototypeElement*>(tmp)->Unlink();
    }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_BEGIN(nsXULPrototypeNode)
    if (tmp->mType == nsXULPrototypeNode::eType_Element) {
        nsXULPrototypeElement *elem =
            static_cast<nsXULPrototypeElement*>(tmp);
        cb.NoteXPCOMChild(elem->mNodeInfo);
        PRUint32 i;
        for (i = 0; i < elem->mNumAttributes; ++i) {
            const nsAttrName& name = elem->mAttributes[i].mName;
            if (!name.IsAtom())
                cb.NoteXPCOMChild(name.NodeInfo());
        }
        for (i = 0; i < elem->mChildren.Length(); ++i) {
            NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_PTR(elem->mChildren[i].get(),
                                                         nsXULPrototypeNode,
                                                         "mChildren[i]")
        }
    }
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_NATIVE_BEGIN(nsXULPrototypeNode)
    if (tmp->mType == nsXULPrototypeNode::eType_Element) {
        nsXULPrototypeElement *elem =
            static_cast<nsXULPrototypeElement*>(tmp);
        if (elem->mHoldsScriptObject) {
            PRUint32 i;
            for (i = 0; i < elem->mNumAttributes; ++i) {
                void *handler = elem->mAttributes[i].mEventHandler;
                NS_IMPL_CYCLE_COLLECTION_TRACE_CALLBACK(elem->mScriptTypeID,
                                                        handler)
            }
        }
    }
    else if (tmp->mType == nsXULPrototypeNode::eType_Script) {
        nsXULPrototypeScript *script =
            static_cast<nsXULPrototypeScript*>(tmp);
        NS_IMPL_CYCLE_COLLECTION_TRACE_CALLBACK(script->mScriptObject.mLangID,
                                                script->mScriptObject.mObject)
    }
NS_IMPL_CYCLE_COLLECTION_TRACE_END
NS_IMPL_CYCLE_COLLECTION_ROOT_BEGIN_NATIVE(nsXULPrototypeNode, AddRef)
    if (tmp->mType == nsXULPrototypeNode::eType_Element) {
        static_cast<nsXULPrototypeElement*>(tmp)->UnlinkJSObjects();
    }
    else if (tmp->mType == nsXULPrototypeNode::eType_Script) {
        static_cast<nsXULPrototypeScript*>(tmp)->UnlinkJSObjects();
    }
NS_IMPL_CYCLE_COLLECTION_ROOT_END

NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsXULPrototypeNode, Release)






nsXULPrototypeAttribute::~nsXULPrototypeAttribute()
{
    MOZ_COUNT_DTOR(nsXULPrototypeAttribute);
}







nsresult
nsXULPrototypeElement::Serialize(nsIObjectOutputStream* aStream,
                                 nsIScriptGlobalObject* aGlobal,
                                 const nsCOMArray<nsINodeInfo> *aNodeInfos)
{
    nsresult rv;

    
    rv = aStream->Write32(mType);

    
    rv |= aStream->Write32(mScriptTypeID);

    
    PRInt32 index = aNodeInfos->IndexOf(mNodeInfo);
    NS_ASSERTION(index >= 0, "unknown nsINodeInfo index");
    rv |= aStream->Write32(index);

    
    rv |= aStream->Write32(mNumAttributes);

    nsAutoString attributeValue;
    PRUint32 i;
    for (i = 0; i < mNumAttributes; ++i) {
        nsCOMPtr<nsINodeInfo> ni;
        if (mAttributes[i].mName.IsAtom()) {
            ni = mNodeInfo->NodeInfoManager()->
                GetNodeInfo(mAttributes[i].mName.Atom(), nsnull,
                            kNameSpaceID_None);
            NS_ASSERTION(ni, "the nodeinfo should already exist");
        }
        else {
            ni = mAttributes[i].mName.NodeInfo();
        }

        index = aNodeInfos->IndexOf(ni);
        NS_ASSERTION(index >= 0, "unknown nsINodeInfo index");
        rv |= aStream->Write32(index);

        mAttributes[i].mValue.ToString(attributeValue);
        rv |= aStream->WriteWStringZ(attributeValue.get());
    }

    
    rv |= aStream->Write32(PRUint32(mChildren.Length()));
    for (i = 0; i < mChildren.Length(); i++) {
        nsXULPrototypeNode* child = mChildren[i].get();
        switch (child->mType) {
        case eType_Element:
        case eType_Text:
        case eType_PI:
            rv |= child->Serialize(aStream, aGlobal, aNodeInfos);
            break;
        case eType_Script:
            rv |= aStream->Write32(child->mType);
            nsXULPrototypeScript* script = static_cast<nsXULPrototypeScript*>(child);

            rv |= aStream->Write32(script->mScriptObject.mLangID);

            rv |= aStream->Write8(script->mOutOfLine);
            if (! script->mOutOfLine) {
                rv |= script->Serialize(aStream, aGlobal, aNodeInfos);
            } else {
                rv |= aStream->WriteCompoundObject(script->mSrcURI,
                                                   NS_GET_IID(nsIURI),
                                                   PR_TRUE);

                if (script->mScriptObject.mObject) {
                    
                    
                    
                    
                    
                    rv |= script->SerializeOutOfLine(aStream, aGlobal);
                }
            }
            break;
        }
    }

    return rv;
}

nsresult
nsXULPrototypeElement::Deserialize(nsIObjectInputStream* aStream,
                                   nsIScriptGlobalObject* aGlobal,
                                   nsIURI* aDocumentURI,
                                   const nsCOMArray<nsINodeInfo> *aNodeInfos)
{
    NS_PRECONDITION(aNodeInfos, "missing nodeinfo array");
    nsresult rv;

    
    PRUint32 scriptId = 0;
    rv = aStream->Read32(&scriptId);
    mScriptTypeID = scriptId;

    
    PRUint32 number;
    rv |= aStream->Read32(&number);
    mNodeInfo = aNodeInfos->SafeObjectAt(number);
    if (!mNodeInfo)
        return NS_ERROR_UNEXPECTED;

    
    rv |= aStream->Read32(&number);
    mNumAttributes = PRInt32(number);

    PRUint32 i;
    if (mNumAttributes > 0) {
        mAttributes = new nsXULPrototypeAttribute[mNumAttributes];
        if (! mAttributes)
            return NS_ERROR_OUT_OF_MEMORY;

        nsAutoString attributeValue;
        for (i = 0; i < mNumAttributes; ++i) {
            rv |= aStream->Read32(&number);
            nsINodeInfo* ni = aNodeInfos->SafeObjectAt(number);
            if (!ni)
                return NS_ERROR_UNEXPECTED;

            mAttributes[i].mName.SetTo(ni);

            rv |= aStream->ReadString(attributeValue);
            rv |= SetAttrAt(i, attributeValue, aDocumentURI);
        }
    }

    rv |= aStream->Read32(&number);
    PRUint32 numChildren = PRInt32(number);

    if (numChildren > 0) {
        if (!mChildren.SetCapacity(numChildren))
            return NS_ERROR_OUT_OF_MEMORY;

        for (i = 0; i < numChildren; i++) {
            rv |= aStream->Read32(&number);
            Type childType = (Type)number;

            nsRefPtr<nsXULPrototypeNode> child;

            switch (childType) {
            case eType_Element:
                child = new nsXULPrototypeElement();
                if (! child)
                    return NS_ERROR_OUT_OF_MEMORY;
                child->mType = childType;

                rv |= child->Deserialize(aStream, aGlobal, aDocumentURI,
                                         aNodeInfos);
                break;
            case eType_Text:
                child = new nsXULPrototypeText();
                if (! child)
                    return NS_ERROR_OUT_OF_MEMORY;
                child->mType = childType;

                rv |= child->Deserialize(aStream, aGlobal, aDocumentURI,
                                         aNodeInfos);
                break;
            case eType_PI:
                child = new nsXULPrototypePI();
                if (! child)
                    return NS_ERROR_OUT_OF_MEMORY;
                child->mType = childType;

                rv |= child->Deserialize(aStream, aGlobal, aDocumentURI,
                                         aNodeInfos);
                break;
            case eType_Script: {
                PRUint32 langID = nsIProgrammingLanguage::UNKNOWN;
                rv |= aStream->Read32(&langID);

                
                nsXULPrototypeScript* script = new nsXULPrototypeScript(langID, 0, 0);
                if (! script)
                    return NS_ERROR_OUT_OF_MEMORY;
                child = script;
                child->mType = childType;

                rv |= aStream->Read8(&script->mOutOfLine);
                if (! script->mOutOfLine) {
                    rv |= script->Deserialize(aStream, aGlobal, aDocumentURI,
                                              aNodeInfos);
                } else {
                    rv |= aStream->ReadObject(PR_TRUE, getter_AddRefs(script->mSrcURI));

                    rv |= script->DeserializeOutOfLine(aStream, aGlobal);
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
nsXULPrototypeElement::SetAttrAt(PRUint32 aPos, const nsAString& aValue,
                                 nsIURI* aDocumentURI)
{
    NS_PRECONDITION(aPos < mNumAttributes, "out-of-bounds");

    
    
    

    if (!mNodeInfo->NamespaceEquals(kNameSpaceID_XUL)) {
        mAttributes[aPos].mValue.ParseStringOrAtom(aValue);

        return NS_OK;
    }

    if (mAttributes[aPos].mName.Equals(nsGkAtoms::id) &&
        !aValue.IsEmpty()) {
        mHasIdAttribute = PR_TRUE;
        
        
        
        mAttributes[aPos].mValue.ParseAtom(aValue);

        return NS_OK;
    }
    else if (mAttributes[aPos].mName.Equals(nsGkAtoms::_class)) {
        mHasClassAttribute = PR_TRUE;
        
        mAttributes[aPos].mValue.ParseAtomArray(aValue);
        
        return NS_OK;
    }
    else if (mAttributes[aPos].mName.Equals(nsGkAtoms::style)) {
        mHasStyleAttribute = PR_TRUE;
        
        nsCOMPtr<nsICSSStyleRule> rule;
        nsICSSParser* parser = GetCSSParser();
        NS_ENSURE_TRUE(parser, NS_ERROR_OUT_OF_MEMORY);

        
        parser->ParseStyleAttribute(aValue, aDocumentURI, aDocumentURI,
                                    
                                    
                                    mNodeInfo->NodeInfoManager()->
                                      DocumentPrincipal(),
                                    getter_AddRefs(rule));
        if (rule) {
            mAttributes[aPos].mValue.SetTo(rule);

            return NS_OK;
        }
        
    }

    mAttributes[aPos].mValue.ParseStringOrAtom(aValue);

    return NS_OK;
}

void
nsXULPrototypeElement::UnlinkJSObjects()
{
    if (mHoldsScriptObject) {
        nsContentUtils::DropScriptObjects(mScriptTypeID, this,
                                          &NS_CYCLE_COLLECTION_NAME(nsXULPrototypeNode));
        mHoldsScriptObject = PR_FALSE;
    }
}

void
nsXULPrototypeElement::Unlink()
{
    mNumAttributes = 0;
    delete[] mAttributes;
    mAttributes = nsnull;
}






nsXULPrototypeScript::nsXULPrototypeScript(PRUint32 aLangID, PRUint32 aLineNo, PRUint32 aVersion)
    : nsXULPrototypeNode(eType_Script),
      mLineNo(aLineNo),
      mSrcLoading(PR_FALSE),
      mOutOfLine(PR_TRUE),
      mSrcLoadWaiters(nsnull),
      mLangVersion(aVersion),
      mScriptObject(aLangID)
{
    NS_ASSERTION(aLangID != nsIProgrammingLanguage::UNKNOWN,
                 "The language ID must be known and constant");
}


nsXULPrototypeScript::~nsXULPrototypeScript()
{
    UnlinkJSObjects();
}

nsresult
nsXULPrototypeScript::Serialize(nsIObjectOutputStream* aStream,
                                nsIScriptGlobalObject* aGlobal,
                                const nsCOMArray<nsINodeInfo> *aNodeInfos)
{
    nsIScriptContext *context = aGlobal->GetScriptContext(
                                        mScriptObject.mLangID);
    NS_ASSERTION(!mSrcLoading || mSrcLoadWaiters != nsnull ||
                 !mScriptObject.mObject,
                 "script source still loading when serializing?!");
    if (!mScriptObject.mObject)
        return NS_ERROR_FAILURE;

    
    nsresult rv;
    rv = aStream->Write32(mLineNo);
    if (NS_FAILED(rv)) return rv;
    rv = aStream->Write32(mLangVersion);
    if (NS_FAILED(rv)) return rv;
    
    rv = context->Serialize(aStream, mScriptObject.mObject);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsXULPrototypeScript::SerializeOutOfLine(nsIObjectOutputStream* aStream,
                                         nsIScriptGlobalObject* aGlobal)
{
    nsXULPrototypeCache* cache = nsXULPrototypeCache::GetInstance();
    if (!cache)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ASSERTION(cache->IsEnabled(),
                 "writing to the FastLoad file, but the XUL cache is off?");

    nsIFastLoadService* fastLoadService = cache->GetFastLoadService();
    if (!fastLoadService)
        return NS_ERROR_NOT_AVAILABLE;

    nsCAutoString urispec;
    nsresult rv = mSrcURI->GetAsciiSpec(urispec);
    if (NS_FAILED(rv))
        return rv;

    PRBool exists = PR_FALSE;
    fastLoadService->HasMuxedDocument(urispec.get(), &exists);
    




    if (exists)
        return NS_OK;

    
    
    
    nsCOMPtr<nsIObjectOutputStream> objectOutput = aStream;
    if (! objectOutput) {
        fastLoadService->GetOutputStream(getter_AddRefs(objectOutput));
        if (! objectOutput)
            return NS_ERROR_NOT_AVAILABLE;
    }

    rv = fastLoadService->
         StartMuxedDocument(mSrcURI, urispec.get(),
                            nsIFastLoadService::NS_FASTLOAD_WRITE);
    NS_ASSERTION(rv != NS_ERROR_NOT_AVAILABLE, "reading FastLoad?!");

    nsCOMPtr<nsIURI> oldURI;
    rv |= fastLoadService->SelectMuxedDocument(mSrcURI, getter_AddRefs(oldURI));
    rv |= Serialize(objectOutput, aGlobal, nsnull);
    rv |= fastLoadService->EndMuxedDocument(mSrcURI);

    if (oldURI) {
        nsCOMPtr<nsIURI> tempURI;
        rv |= fastLoadService->
              SelectMuxedDocument(oldURI, getter_AddRefs(tempURI));
    }

    if (NS_FAILED(rv))
        cache->AbortFastLoads();
    return rv;
}


nsresult
nsXULPrototypeScript::Deserialize(nsIObjectInputStream* aStream,
                                  nsIScriptGlobalObject* aGlobal,
                                  nsIURI* aDocumentURI,
                                  const nsCOMArray<nsINodeInfo> *aNodeInfos)
{
    NS_TIMELINE_MARK_FUNCTION("chrome script deserialize");
    nsresult rv;

    NS_ASSERTION(!mSrcLoading || mSrcLoadWaiters != nsnull ||
                 !mScriptObject.mObject,
                 "prototype script not well-initialized when deserializing?!");

    
    aStream->Read32(&mLineNo);
    aStream->Read32(&mLangVersion);

    nsIScriptContext *context = aGlobal->GetScriptContext(
                                            mScriptObject.mLangID);
    NS_ASSERTION(context != nsnull, "Have no context for deserialization");
    NS_ENSURE_TRUE(context, NS_ERROR_UNEXPECTED);
    nsScriptObjectHolder newScriptObject(context);
    rv = context->Deserialize(aStream, newScriptObject);
    if (NS_FAILED(rv)) {
        NS_WARNING("Language deseralization failed");
        return rv;
    }
    Set(newScriptObject);
    return NS_OK;
}


nsresult
nsXULPrototypeScript::DeserializeOutOfLine(nsIObjectInputStream* aInput,
                                           nsIScriptGlobalObject* aGlobal)
{
    
    
    nsresult rv = NS_OK;

    nsXULPrototypeCache* cache = nsXULPrototypeCache::GetInstance();
    nsIFastLoadService* fastLoadService = cache->GetFastLoadService();

    
    
    
    nsCOMPtr<nsIObjectInputStream> objectInput = aInput;
    if (! objectInput && fastLoadService)
        fastLoadService->GetInputStream(getter_AddRefs(objectInput));

    if (objectInput) {
        PRBool useXULCache = PR_TRUE;
        if (mSrcURI) {
            
            
            
            
            
            
            
            
            
            
            useXULCache = cache->IsEnabled();

            if (useXULCache) {
                PRUint32 newLangID = nsIProgrammingLanguage::UNKNOWN;
                void *newScriptObject = cache->GetScript(mSrcURI, &newLangID);
                if (newScriptObject) {
                    
                    
                    
                    
                    
                    
                    
                    if (mScriptObject.mLangID != newLangID) {
                        NS_ERROR("XUL cache gave different language?");
                        return NS_ERROR_UNEXPECTED;
                    }
                    Set(newScriptObject);
                }
            }
        }

        if (! mScriptObject.mObject) {
            nsCOMPtr<nsIURI> oldURI;

            if (mSrcURI) {
                nsCAutoString spec;
                mSrcURI->GetAsciiSpec(spec);
                rv = fastLoadService->StartMuxedDocument(mSrcURI, spec.get(),
                                                         nsIFastLoadService::NS_FASTLOAD_READ);
                if (NS_SUCCEEDED(rv))
                    rv = fastLoadService->SelectMuxedDocument(mSrcURI, getter_AddRefs(oldURI));
            } else {
                
                
                
                
                PRInt32 direction;
                fastLoadService->GetDirection(&direction);
                if (direction != nsIFastLoadService::NS_FASTLOAD_READ)
                    rv = NS_ERROR_NOT_AVAILABLE;
            }

            
            
            
            
            
            
            if (NS_SUCCEEDED(rv))
                rv = Deserialize(objectInput, aGlobal, nsnull, nsnull);

            if (NS_SUCCEEDED(rv) && mSrcURI) {
                rv = fastLoadService->EndMuxedDocument(mSrcURI);

                if (NS_SUCCEEDED(rv) && oldURI) {
                    nsCOMPtr<nsIURI> tempURI;
                    rv = fastLoadService->SelectMuxedDocument(oldURI, getter_AddRefs(tempURI));

                    NS_ASSERTION(NS_SUCCEEDED(rv) && (!tempURI || tempURI == mSrcURI),
                                 "not currently deserializing into the script we thought we were!");
                }
            }

            if (NS_SUCCEEDED(rv)) {
                if (useXULCache && mSrcURI) {
                    PRBool isChrome = PR_FALSE;
                    mSrcURI->SchemeIs("chrome", &isChrome);
                    if (isChrome) {
                        cache->PutScript(mSrcURI,
                                         mScriptObject.mLangID,
                                         mScriptObject.mObject);
                    }
                }
            } else {
                
                
                
                
                if (rv != NS_ERROR_NOT_AVAILABLE)
                    cache->AbortFastLoads();
            }
        }
    }

    return rv;
}

nsresult
nsXULPrototypeScript::Compile(const PRUnichar* aText,
                              PRInt32 aTextLength,
                              nsIURI* aURI,
                              PRUint32 aLineNo,
                              nsIDocument* aDocument,
                              nsIScriptGlobalObjectOwner* aGlobalOwner)
{
    
    
    
    
    
    
    
    
    
    
    nsresult rv;

    
    nsIScriptContext *context;

    {
        nsIScriptGlobalObject* global = aGlobalOwner->GetScriptGlobalObject();
        NS_ASSERTION(global != nsnull, "prototype doc has no script global");
        if (! global)
            return NS_ERROR_UNEXPECTED;

        context = global->GetScriptContext(mScriptObject.mLangID);
        NS_ASSERTION(context != nsnull, "no context for script global");
        if (! context)
            return NS_ERROR_UNEXPECTED;
    }

    nsCAutoString urlspec;
    nsContentUtils::GetWrapperSafeScriptFilename(aDocument, aURI, urlspec);

    

    nsScriptObjectHolder newScriptObject(context);
    rv = context->CompileScript(aText,
                                aTextLength,
                                nsnull,
                                
                                
                                
                                
                                
                                
                                aDocument->NodePrincipal(),
                                urlspec.get(),
                                aLineNo,
                                mLangVersion,
                                newScriptObject);
    if (NS_FAILED(rv))
        return rv;

    Set(newScriptObject);
    return rv;
}






nsresult
nsXULPrototypeText::Serialize(nsIObjectOutputStream* aStream,
                              nsIScriptGlobalObject* aGlobal,
                              const nsCOMArray<nsINodeInfo> *aNodeInfos)
{
    nsresult rv;

    
    rv = aStream->Write32(mType);

    rv |= aStream->WriteWStringZ(mValue.get());

    return rv;
}

nsresult
nsXULPrototypeText::Deserialize(nsIObjectInputStream* aStream,
                                nsIScriptGlobalObject* aGlobal,
                                nsIURI* aDocumentURI,
                                const nsCOMArray<nsINodeInfo> *aNodeInfos)
{
    nsresult rv;

    rv = aStream->ReadString(mValue);

    return rv;
}






nsresult
nsXULPrototypePI::Serialize(nsIObjectOutputStream* aStream,
                            nsIScriptGlobalObject* aGlobal,
                            const nsCOMArray<nsINodeInfo> *aNodeInfos)
{
    nsresult rv;

    
    rv = aStream->Write32(mType);

    rv |= aStream->WriteWStringZ(mTarget.get());
    rv |= aStream->WriteWStringZ(mData.get());

    return rv;
}

nsresult
nsXULPrototypePI::Deserialize(nsIObjectInputStream* aStream,
                              nsIScriptGlobalObject* aGlobal,
                              nsIURI* aDocumentURI,
                              const nsCOMArray<nsINodeInfo> *aNodeInfos)
{
    nsresult rv;

    rv = aStream->ReadString(mTarget);
    rv |= aStream->ReadString(mData);

    return rv;
}
