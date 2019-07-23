
























































#include "jsapi.h"      
#include "nsCOMPtr.h"
#include "nsDOMCID.h"
#include "nsDOMError.h"
#include "nsDOMString.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsHashtable.h"
#include "nsIAtom.h"
#include "nsIDOMAttr.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMLoadListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMFormListener.h"
#include "nsIDOMXULListener.h"
#include "nsIDOMContextMenuListener.h"
#include "nsIDOMDragListener.h"
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
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsXULContentUtils.h"
#include "nsNodeUtils.h"

#include "prlog.h"
#include "rdf.h"

#include "nsIControllers.h"


#include "nsIDOMXULDocument.h"

#include "nsReadableUtils.h"
#include "nsITimelineService.h"
#include "nsIFrame.h"
#include "nsNodeInfoManager.h"
#include "nsXBLBinding.h"
#include "nsEventDispatcher.h"
#include "nsPresShellIterator.h"




#define XUL_ELEMENT_CHILDREN_MUST_BE_REBUILT \
  (nsXULElement::eChildrenMustBeRebuilt << XUL_ELEMENT_LAZY_STATE_OFFSET)

#define XUL_ELEMENT_TEMPLATE_CONTENTS_BUILT \
  (nsXULElement::eTemplateContentsBuilt << XUL_ELEMENT_LAZY_STATE_OFFSET)

#define XUL_ELEMENT_CONTAINER_CONTENTS_BUILT \
  (nsXULElement::eContainerContentsBuilt << XUL_ELEMENT_LAZY_STATE_OFFSET)

class nsIDocShell;


nsICSSParser* nsXULPrototypeElement::sCSSParser = nsnull;
nsIXBLService * nsXULElement::gXBLService = nsnull;
nsICSSOMFactory* nsXULElement::gCSSOMFactory = nsnull;




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
static NS_DEFINE_CID(kCSSOMFactoryCID,            NS_CSSOMFACTORY_CID);



#ifdef XUL_PROTOTYPE_ATTRIBUTE_METERING
PRUint32             nsXULPrototypeAttribute::gNumElements;
PRUint32             nsXULPrototypeAttribute::gNumAttributes;
PRUint32             nsXULPrototypeAttribute::gNumEventHandlers;
PRUint32             nsXULPrototypeAttribute::gNumCacheTests;
PRUint32             nsXULPrototypeAttribute::gNumCacheHits;
PRUint32             nsXULPrototypeAttribute::gNumCacheSets;
PRUint32             nsXULPrototypeAttribute::gNumCacheFills;
#endif

class nsXULElementTearoff : public nsIDOMElementCSSInlineStyle
{
public:
  NS_DECL_ISUPPORTS

  nsXULElementTearoff(nsXULElement *aElement)
    : mElement(aElement)
  {
  }

  NS_FORWARD_NSIDOMELEMENTCSSINLINESTYLE(mElement->)

private:
  nsRefPtr<nsXULElement> mElement;
};

NS_IMPL_ADDREF(nsXULElementTearoff)
NS_IMPL_RELEASE(nsXULElementTearoff)

NS_INTERFACE_MAP_BEGIN(nsXULElementTearoff)
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
    nsresult rv;
    if (aDocument) {
        nsINodeInfo* ni = aPrototype->mNodeInfo;
        rv = aDocument->NodeInfoManager()->GetNodeInfo(ni->NameAtom(),
                                                       ni->GetPrefixAtom(),
                                                       ni->NamespaceID(),
                                                       getter_AddRefs(nodeInfo));
        NS_ENSURE_SUCCESS(rv, rv);
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

NS_IMETHODIMP
nsXULElement::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    NS_ENSURE_ARG_POINTER(aInstancePtr);
    *aInstancePtr = nsnull;

    if ( aIID.Equals(NS_GET_IID(nsXPCOMCycleCollectionParticipant)) ) {
      *aInstancePtr = &NS_CYCLE_COLLECTION_NAME(nsXULElement);
      return NS_OK;
    }

    nsresult rv = nsGenericElement::QueryInterface(aIID, aInstancePtr);
    if (NS_SUCCEEDED(rv))
        return rv;

    nsISupports *inst = nsnull;

    if (aIID.Equals(NS_GET_IID(nsIDOMNode))) {
        inst = NS_STATIC_CAST(nsIDOMNode *, this);
    } else if (aIID.Equals(NS_GET_IID(nsIDOMElement))) {
        inst = NS_STATIC_CAST(nsIDOMElement *, this);
    } else if (aIID.Equals(NS_GET_IID(nsIDOMXULElement))) {
        inst = NS_STATIC_CAST(nsIDOMXULElement *, this);
    } else if (aIID.Equals(NS_GET_IID(nsIScriptEventHandlerOwner))) {
        inst = NS_STATIC_CAST(nsIScriptEventHandlerOwner*,
                              new nsScriptEventHandlerOwnerTearoff(this));
        NS_ENSURE_TRUE(inst, NS_ERROR_OUT_OF_MEMORY);
    } else if (aIID.Equals(NS_GET_IID(nsIDOMElementCSSInlineStyle))) {
        inst = NS_STATIC_CAST(nsIDOMElementCSSInlineStyle *,
                              new nsXULElementTearoff(this));
        NS_ENSURE_TRUE(inst, NS_ERROR_OUT_OF_MEMORY);
    } else if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {
        inst = NS_GetDOMClassInfoInstance(eDOMClassInfo_XULElement_id);
        NS_ENSURE_TRUE(inst, NS_ERROR_OUT_OF_MEMORY);
    } else {
        return PostQueryInterface(aIID, aInstancePtr);
    }

    NS_ADDREF(inst);
 
    *aInstancePtr = inst;
    return NS_OK;
}




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

    nsIContent *root = doc->GetRootContent();
    if ((!root || root == this) && !mNodeInfo->Equals(nsGkAtoms::overlay)) {
        nsPIDOMWindow *window = doc->GetInnerWindow();

        nsCOMPtr<nsPIDOMEventTarget> piTarget = do_QueryInterface(window);
        if (!piTarget)
            return NS_ERROR_UNEXPECTED;

        nsresult rv = piTarget->GetListenerManager(PR_TRUE, aManager);
        if (NS_SUCCEEDED(rv)) {
            NS_ADDREF(*aTarget = window);
        }
        *aDefer = PR_FALSE;
        return rv;
    }

    return nsGenericElement::GetEventListenerManagerForAttr(aManager,
                                                            aTarget,
                                                            aDefer);
}

PRBool
nsXULElement::IsFocusable(PRInt32 *aTabIndex)
{
  
  PRInt32 tabIndex = aTabIndex? *aTabIndex : -1;
  PRBool disabled = tabIndex < 0;
  nsCOMPtr<nsIDOMXULControlElement> xulControl = 
    do_QueryInterface(NS_STATIC_CAST(nsIContent*, this));
  if (xulControl) {
    xulControl->GetDisabled(&disabled);
    if (disabled) {
      tabIndex = -1;  
    }
    else if (HasAttr(kNameSpaceID_None, nsGkAtoms::tabindex)) {
      
      xulControl->GetTabIndex(&tabIndex);
    }
    if (tabIndex != -1 && sTabFocusModelAppliesToXUL &&
        !(sTabFocusModel & eTabFocus_formElementsMask)) {
      
      
      
      
      
      if (!mNodeInfo->Equals(nsGkAtoms::tree) && !mNodeInfo->Equals(nsGkAtoms::listbox))
        tabIndex = -1; 
    }
  }

  if (aTabIndex) {
    *aTabIndex = tabIndex;
  }

  return tabIndex >= 0 || (!disabled && HasAttr(kNameSpaceID_None, nsGkAtoms::tabindex));
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

    
    nsIAtom *tag = content->Tag();
    if (tag == nsGkAtoms::textbox || tag == nsGkAtoms::menulist) {
        
        elm->Focus();
    } else if (tag == nsGkAtoms::toolbarbutton) {
        
        elm->Click();
    } else {
        
        elm->Focus();
        elm->Click();
    }
}





NS_IMPL_CYCLE_COLLECTION_CLASS(nsScriptEventHandlerOwnerTearoff)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsScriptEventHandlerOwnerTearoff)
  tmp->mElement = nsnull;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsScriptEventHandlerOwnerTearoff)
  cb.NoteXPCOMChild(NS_STATIC_CAST(nsIContent*, tmp->mElement));
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
    if (mElement->mPrototype && xuldoc) {
        

        
        
        
        
        
        
        
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
    rv = context->CompileEventHandler(aName, argCount, argNames,
                                      aBody, aURL, aLineNo, aHandler);
    if (NS_FAILED(rv)) return rv;

    
    
    rv = aContext->BindCompiledEventHandler(aTarget, aContext->GetNativeGlobal(),
                                            aName, aHandler);
    if (NS_FAILED(rv)) return rv;

    nsXULPrototypeAttribute *attr =
        mElement->FindPrototypeAttribute(kNameSpaceID_None, aName);
    if (attr) {
        XUL_PROTOTYPE_ATTRIBUTE_METER(gNumCacheFills);
        
        if (aHandler) {
            rv = nsContentUtils::HoldScriptObject(aContext->GetScriptTypeID(),
                                                  aHandler);
            if (NS_FAILED(rv)) return rv;
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






void
nsXULElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
    
    
    
    
    
    
    
    
    
    
    
    nsDOMSlots* slots = GetExistingDOMSlots();
    if (slots) {
        NS_IF_RELEASE(slots->mControllers);
    }

    nsGenericElement::UnbindFromTree(aDeep, aNullParent);
}

void
nsXULElement::SetNativeAnonymous(PRBool aAnonymous)
{
    
    if (NodeInfo()->Equals(nsGkAtoms::popupgroup)) {
        nsGenericElement::SetNativeAnonymous(aAnonymous);
    } else {
      
      if (aAnonymous) {
        SetFlags(NODE_IS_ANONYMOUS_FOR_EVENTS);
      } else {
        UnsetFlags(NODE_IS_ANONYMOUS_FOR_EVENTS);
      }
    }
}

PRUint32
nsXULElement::GetChildCount() const
{
    if (NS_FAILED(EnsureContentsGenerated())) {
        return 0;
    }

    return PeekChildCount();
}

nsIContent *
nsXULElement::GetChildAt(PRUint32 aIndex) const
{
    if (NS_FAILED(EnsureContentsGenerated())) {
        return nsnull;
    }

    return mAttrsAndChildren.GetSafeChildAt(aIndex);
}

PRInt32
nsXULElement::IndexOf(nsINode* aPossibleChild) const
{
    if (NS_FAILED(EnsureContentsGenerated())) {
        return -1;
    }

    return mAttrsAndChildren.IndexOfChild(aPossibleChild);
}

nsresult
nsXULElement::RemoveChildAt(PRUint32 aIndex, PRBool aNotify)
{
    nsresult rv = EnsureContentsGenerated();
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIContent> oldKid = mAttrsAndChildren.GetSafeChildAt(aIndex);
    if (!oldKid) {
      return NS_OK;
    }

    
    
    
    nsCOMPtr<nsIDOMXULMultiSelectControlElement> controlElement;
    nsCOMPtr<nsIListBoxObject> listBox;
    PRBool fireSelectionHandler = PR_FALSE;

    
    
    PRInt32 newCurrentIndex = -1;

    if (oldKid->NodeInfo()->Equals(nsGkAtoms::listitem, kNameSpaceID_XUL)) {
      
      
      
      
      controlElement = do_QueryInterface(NS_STATIC_CAST(nsIContent*, this));

      
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

    rv = nsGenericElement::RemoveChildAt(aIndex, aNotify);
    
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
                                           NS_STATIC_CAST(nsIContent*, this),
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
            mNodeInfo->Equals(nsGkAtoms::window)) {
            HideWindowChrome(aValue && NS_LITERAL_STRING("true").Equals(*aValue));
        }

        
        nsIDocument *document = GetCurrentDoc();
        if (aName == nsGkAtoms::readonly && document) {
            mozAutoDocUpdate upd(document, UPDATE_CONTENT_STATE, PR_TRUE);
            document->ContentStatesChanged(this, nsnull,
                                           NS_EVENT_STATE_MOZ_READONLY |
                                           NS_EVENT_STATE_MOZ_READWRITE);
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
            nsGenericHTMLElement::ParseStyleAttribute(this, PR_TRUE, aValue,
                                                      aResult);
            return PR_TRUE;
        }

        if (aAttribute == nsGkAtoms::_class) {
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
    if (aNotify && doc) {
        doc->AttributeWillChange(this, aNameSpaceID, aName);
    }

    PRBool hasMutationListeners = aNotify &&
        nsContentUtils::HasMutationListeners(this,
            NS_EVENT_BITS_MUTATION_ATTRMODIFIED);

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

        
        
        if (aName == nsGkAtoms::accesskey || aName == nsGkAtoms::control) {
            UnregisterAccessKey(oldValue);
        }

        
        
        if (doc && (aName == nsGkAtoms::observes ||
                          aName == nsGkAtoms::command)) {
            nsCOMPtr<nsIDOMXULDocument> xuldoc = do_QueryInterface(doc);
            if (xuldoc) {
                
                nsCOMPtr<nsIDOMElement> broadcaster;
                nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc);
                domDoc->GetElementById(oldValue, getter_AddRefs(broadcaster));
                if (broadcaster) {
                    xuldoc->RemoveBroadcastListenerFor(broadcaster, this,
                                                       NS_LITERAL_STRING("*"));
                }
            }
        }
    }

    if (hasMutationListeners) {
        nsMutationEvent mutation(PR_TRUE, NS_MUTATION_ATTRMODIFIED);

        mutation.mRelatedNode = attrNode;
        mutation.mAttrName = aName;

        if (!oldValue.IsEmpty())
          mutation.mPrevAttrValue = do_GetAtom(oldValue);
        mutation.mAttrChange = nsIDOMMutationEvent::REMOVAL;

        mozAutoSubtreeModified subtree(GetOwnerDoc(), this);
        nsEventDispatcher::Dispatch(NS_STATIC_CAST(nsIContent*, this),
                                    nsnull, &mutation);
    }

    if (doc) {
        nsXBLBinding *binding = doc->BindingManager()->GetBinding(this);
        if (binding)
            binding->AttributeChanged(aName, aNameSpaceID, PR_TRUE, aNotify);

    }

    if (aNotify) {
        nsNodeUtils::AttributeChanged(this, aNameSpaceID, aName,
                                      nsIDOMMutationEvent::REMOVAL);
    }

    return NS_OK;
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


#ifdef DEBUG
void
nsXULElement::List(FILE* out, PRInt32 aIndent) const
{
    nsCString prefix("<XUL");
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
    if (aVisitor.mEvent->message == NS_XUL_COMMAND &&
        aVisitor.mEvent->originalTarget == NS_STATIC_CAST(nsIContent*, this) &&
        tag != nsGkAtoms::command) {
        
        
        nsAutoString command;
        GetAttr(kNameSpaceID_None, nsGkAtoms::command, command);
        if (!command.IsEmpty()) {
            
            
            aVisitor.mCanHandle = PR_FALSE;

            
            nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(GetCurrentDoc()));
            NS_ENSURE_STATE(domDoc);
            nsCOMPtr<nsIDOMElement> commandElt;
            domDoc->GetElementById(command, getter_AddRefs(commandElt));
            nsCOMPtr<nsIContent> commandContent(do_QueryInterface(commandElt));
            if (commandContent) {
                
                
                
                

                nsXULCommandEvent event(NS_IS_TRUSTED_EVENT(aVisitor.mEvent),
                                        NS_XUL_COMMAND, nsnull);
                if (aVisitor.mEvent->eventStructType == NS_XUL_COMMAND_EVENT) {
                    nsXULCommandEvent *orig =
                        NS_STATIC_CAST(nsXULCommandEvent*, aVisitor.mEvent);

                    event.isShift = orig->isShift;
                    event.isControl = orig->isControl;
                    event.isAlt = orig->isAlt;
                    event.isMeta = orig->isMeta;
                } else {
                    NS_WARNING("Incorrect eventStructType for command event");
                }

                if (!aVisitor.mDOMEvent) {
                    
                    nsEventDispatcher::CreateEvent(aVisitor.mPresContext,
                                                   aVisitor.mEvent,
                                                   EmptyString(),
                                                   &aVisitor.mDOMEvent);
                }
                event.sourceEvent = aVisitor.mDOMEvent;

                nsEventStatus status = nsEventStatus_eIgnore;
                nsEventDispatcher::Dispatch(commandContent,
                                            aVisitor.mPresContext,
                                            &event, nsnull, &status);
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





nsresult
nsXULElement::EnsureContentsGenerated(void) const
{
    if (GetFlags() & XUL_ELEMENT_CHILDREN_MUST_BE_REBUILT) {
        
        
        
        NS_PRECONDITION(IsInDoc(), "element not in tree");
        if (!IsInDoc())
            return NS_ERROR_NOT_INITIALIZED;

        
        nsXULElement* unconstThis = NS_CONST_CAST(nsXULElement*, this);

        
        
        unconstThis->ClearLazyState(eChildrenMustBeRebuilt);

        
        
        nsIContent* element = unconstThis;

        do {
            nsCOMPtr<nsIDOMXULElement> xulele = do_QueryInterface(element);
            if (xulele) {
                nsCOMPtr<nsIXULTemplateBuilder> builder;
                xulele->GetBuilder(getter_AddRefs(builder));
                if (builder) {
                    if (HasAttr(kNameSpaceID_None, nsGkAtoms::xulcontentsgenerated)) {
                        unconstThis->ClearLazyState(eChildrenMustBeRebuilt);
                        return NS_OK;
                    }

                    return builder->CreateContents(unconstThis);
                }
            }

            element = element->GetParent();
        } while (element);

        NS_ERROR("lazy state set with no XUL content builder in ancestor chain");
        return NS_ERROR_UNEXPECTED;
    }

    return NS_OK;
}



nsresult
nsXULElement::InsertChildAt(nsIContent* aKid, PRUint32 aIndex, PRBool aNotify)
{
  nsresult rv = EnsureContentsGenerated();
  NS_ENSURE_SUCCESS(rv, rv);

  return nsGenericElement::InsertChildAt(aKid, aIndex, aNotify);
}



nsIAtom*
nsXULElement::GetID() const
{
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
nsXULElement::GetClasses() const
{
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
    
    const nsAttrValue* attrVal = FindLocalOrProtoAttr(kNameSpaceID_None, nsGkAtoms::style);

    if (attrVal && attrVal->Type() == nsAttrValue::eCSSStyleRule) {
        return attrVal->GetCSSStyleRuleValue();
    }

    return nsnull;
}

NS_IMETHODIMP
nsXULElement::SetInlineStyleRule(nsICSSStyleRule* aStyleRule, PRBool aNotify)
{
  PRBool modification = PR_FALSE;
  nsAutoString oldValueStr;

  PRBool hasListeners = aNotify &&
    nsContentUtils::HasMutationListeners(this,
                                         NS_EVENT_BITS_MUTATION_ATTRMODIFIED);

  
  
  
  
  if (hasListeners) {
    
    
    
    modification = GetAttr(kNameSpaceID_None, nsGkAtoms::style,
                           oldValueStr);
  }
  else if (aNotify && IsInDoc()) {
    modification = !!mAttrsAndChildren.GetAttr(nsGkAtoms::style);
  }

  nsAttrValue attrValue(aStyleRule);

  return SetAttrAndNotify(kNameSpaceID_None, nsGkAtoms::style, nsnull, oldValueStr,
                          attrValue, modification, hasListeners, aNotify);
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
        
        
        
        if (nsGkAtoms::left == aAttribute || nsGkAtoms::top == aAttribute)
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
                                  NS_REINTERPRET_CAST(void**, &slots->mControllers));

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

  
  nsIDocument* doc = HasFlag(NODE_FORCE_XBL_BINDINGS) ?
    GetOwnerDoc() : GetCurrentDoc();
  nsCOMPtr<nsIDOMNSDocument> nsDoc(do_QueryInterface(doc));

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
        if (!gCSSOMFactory) {
            rv = CallGetService(kCSSOMFactoryCID, &gCSSOMFactory);
            NS_ENSURE_SUCCESS(rv, rv);
        }

        rv = gCSSOMFactory->CreateDOMCSSAttributeDeclaration(this,
                getter_AddRefs(slots->mStyle));
        NS_ENSURE_SUCCESS(rv, rv);
    }

    NS_IF_ADDREF(*aStyle = slots->mStyle);

    return NS_OK;
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
    if (!nsGenericElement::ShouldFocus(this)) {
        return NS_OK;
    }

    nsIDocument* doc = GetCurrentDoc();
    
    if (!doc)
        return NS_OK;

    

    nsIPresShell *shell = doc->GetPrimaryShell();
    if (!shell)
        return NS_OK;

    
    nsCOMPtr<nsPresContext> context = shell->GetPresContext();
    SetFocus(context);

    return NS_OK;
}

NS_IMETHODIMP
nsXULElement::Blur()
{
    nsIDocument* doc = GetCurrentDoc();
    
    if (!doc)
        return NS_OK;

    
    nsIPresShell *shell = doc->GetPrimaryShell();
    if (!shell)
        return NS_OK;

    
    nsCOMPtr<nsPresContext> context = shell->GetPresContext();
    if (ShouldBlur(this))
      RemoveFocus(context);

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
            nsEventDispatcher::Dispatch(NS_STATIC_CAST(nsIContent*, this),
                                        context, &eventDown,  nsnull, &status);

            
            status = nsEventStatus_eIgnore;  
            nsEventDispatcher::Dispatch(NS_STATIC_CAST(nsIContent*, this),
                                        context, &eventUp, nsnull, &status);

            
            status = nsEventStatus_eIgnore;  
            nsEventDispatcher::Dispatch(NS_STATIC_CAST(nsIContent*, this),
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
        nsPresShellIterator iter(doc);
        nsCOMPtr<nsIPresShell> shell;
        while ((shell = iter.GetNextShell())) {
            nsCOMPtr<nsPresContext> context = shell->GetPresContext();
            nsEventStatus status = nsEventStatus_eIgnore;
            nsXULCommandEvent event(PR_TRUE, NS_XUL_COMMAND, nsnull);
            nsEventDispatcher::Dispatch(NS_STATIC_CAST(nsIContent*, this),
                                        context, &event, nsnull, &status);
        }
    }

    return NS_OK;
}


void
nsXULElement::SetFocus(nsPresContext* aPresContext)
{
    if (BoolAttrIsTrue(nsGkAtoms::disabled))
        return;

    aPresContext->EventStateManager()->SetContentState(this,
                                                       NS_EVENT_STATE_FOCUS);
}

void
nsXULElement::RemoveFocus(nsPresContext* aPresContext)
{
  if (!aPresContext) 
    return;
  
  if (IsInDoc()) {
    aPresContext->EventStateManager()->SetContentState(nsnull,
                                                       NS_EVENT_STATE_FOCUS);
  }
}

nsIContent *
nsXULElement::GetBindingParent() const
{
    return mBindingParent;
}

PRBool
nsXULElement::IsNodeOfType(PRUint32 aFlags) const
{
    return !(aFlags & ~(eCONTENT | eELEMENT | eXUL));
}

static void
PopupListenerPropertyDtor(void* aObject, nsIAtom* aPropertyName,
                          void* aPropertyValue, void* aData)
{
  nsIDOMEventListener* listener =
    NS_STATIC_CAST(nsIDOMEventListener*, aPropertyValue);
  if (!listener) {
    return;
  }
  nsCOMPtr<nsIDOMEventTarget> target =
    do_QueryInterface(NS_STATIC_CAST(nsINode*, aObject));
  if (target) {
    target->RemoveEventListener(NS_LITERAL_STRING("mousedown"), listener,
                                PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING("contextmenu"), listener,
                                PR_FALSE);
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
        NS_STATIC_CAST(nsIDOMEventListener*, GetProperty(listenerAtom));
    if (popupListener) {
        
        return NS_OK;
    }

    nsresult rv = NS_NewXULPopupListener(this, isContext,
                                         getter_AddRefs(popupListener));
    if (NS_FAILED(rv))
        return rv;

    
    nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(NS_STATIC_CAST(nsIContent *, this)));
    NS_ENSURE_TRUE(target, NS_ERROR_FAILURE);
    rv = SetProperty(listenerAtom, popupListener, PopupListenerPropertyDtor,
                     PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsIDOMEventListener* listener = nsnull;
    popupListener.swap(listener);
    if (isContext)
      target->AddEventListener(NS_LITERAL_STRING("contextmenu"), listener, PR_FALSE);
    else
      target->AddEventListener(NS_LITERAL_STRING("mousedown"), listener, PR_FALSE);
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

    nsIPresShell *shell = doc->GetPrimaryShell();

    if (shell) {
        nsIContent* content = NS_STATIC_CAST(nsIContent*, this);
        nsIFrame* frame = shell->GetPrimaryFrameFor(content);

        nsPresContext *presContext = shell->GetPresContext();

        if (frame && presContext) {
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

NS_IMPL_CYCLE_COLLECTION_NATIVE_CLASS(nsXULPrototypeNode)
NS_IMPL_CYCLE_COLLECTION_UNLINK_NATIVE_0(nsXULPrototypeNode)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_BEGIN(nsXULPrototypeNode)
    if (tmp->mType == nsXULPrototypeNode::eType_Element) {
        nsXULPrototypeElement *elem =
            NS_STATIC_CAST(nsXULPrototypeElement*, tmp);
        PRUint32 i;
        for (i = 0; i < elem->mNumAttributes; ++i) {
            cb.NoteScriptChild(elem->mScriptTypeID,
                               elem->mAttributes[i].mEventHandler);
        }
        for (i = 0; i < elem->mNumChildren; ++i) {
            NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_PTR(elem->mChildren[i],
                                                         nsXULPrototypeNode)
        }
    }
    else if (tmp->mType == nsXULPrototypeNode::eType_Script) {
        NS_STATIC_CAST(nsXULPrototypeScript*, tmp)->mScriptObject.traverse(cb);
    }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(nsXULPrototypeNode, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsXULPrototypeNode, Release)






nsXULPrototypeAttribute::~nsXULPrototypeAttribute()
{
    MOZ_COUNT_DTOR(nsXULPrototypeAttribute);
    NS_ASSERTION(!mEventHandler, "Finalize not called - language object leak!");
}

void
nsXULPrototypeAttribute::Finalize(PRUint32 aLangID)
{
    if (mEventHandler) {
        if (NS_FAILED(nsContentUtils::DropScriptObject(aLangID, mEventHandler)))
            NS_ERROR("Failed to drop script object");
        mEventHandler = nsnull;
    }
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
            mNodeInfo->NodeInfoManager()->
                GetNodeInfo(mAttributes[i].mName.Atom(), nsnull,
                            kNameSpaceID_None, getter_AddRefs(ni));
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

    
    rv |= aStream->Write32(PRUint32(mNumChildren));
    for (i = 0; i < mNumChildren; i++) {
        nsXULPrototypeNode* child = mChildren[i];
        switch (child->mType) {
        case eType_Element:
        case eType_Text:
        case eType_PI:
            rv |= child->Serialize(aStream, aGlobal, aNodeInfos);
            break;
        case eType_Script:
            rv |= aStream->Write32(child->mType);
            nsXULPrototypeScript* script = NS_STATIC_CAST(nsXULPrototypeScript*, child);

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

    
    rv = aStream->Read32(&mScriptTypeID);
    
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
    mNumChildren = PRInt32(number);

    if (mNumChildren > 0) {
        mChildren = new nsXULPrototypeNode*[mNumChildren];
        if (! mChildren)
            return NS_ERROR_OUT_OF_MEMORY;

        memset(mChildren, 0, sizeof(nsXULPrototypeNode*) * mNumChildren);

        for (i = 0; i < mNumChildren; i++) {
            rv |= aStream->Read32(&number);
            Type childType = (Type)number;

            nsXULPrototypeNode* child = nsnull;

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

            mChildren[i] = child;

            
            
            
            
            
            
            
            
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
        
        
        
        mAttributes[aPos].mValue.ParseAtom(aValue);

        return NS_OK;
    }
    else if (mAttributes[aPos].mName.Equals(nsGkAtoms::_class)) {
        
        mAttributes[aPos].mValue.ParseAtomArray(aValue);
        
        return NS_OK;
    }
    else if (mAttributes[aPos].mName.Equals(nsGkAtoms::style)) {
        
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






nsXULPrototypeScript::nsXULPrototypeScript(PRUint32 aLangID, PRUint32 aLineNo, PRUint32 aVersion)
    : nsXULPrototypeNode(eType_Script),
      mLineNo(aLineNo),
      mSrcLoading(PR_FALSE),
      mOutOfLine(PR_TRUE),
      mSrcLoadWaiters(nsnull),
      mLangVersion(aVersion),
      mScriptObject(aLangID)
{
    NS_LOG_ADDREF(this, 1, ClassName(), ClassSize());
    NS_ASSERTION(aLangID != nsIProgrammingLanguage::UNKNOWN,
                 "The language ID must be known and constant");
}


nsXULPrototypeScript::~nsXULPrototypeScript()
{
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
    mScriptObject.set(newScriptObject);
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
                }
                mScriptObject.set(newScriptObject);
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
    aURI->GetSpec(urlspec);

    

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

    mScriptObject.set(newScriptObject);
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
