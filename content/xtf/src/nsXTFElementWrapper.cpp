




#include "nsXTFElementWrapper.h"
#include "nsIXTFElement.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsXTFInterfaceAggregator.h"
#include "nsIClassInfo.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocument.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsEventStateManager.h"
#include "nsEventListenerManager.h"
#include "nsIDOMEvent.h"
#include "nsGUIEvent.h"
#include "nsContentUtils.h"
#include "nsIXTFService.h"
#include "nsIDOMAttr.h"
#include "nsIAttribute.h"
#include "nsDOMAttributeMap.h"
#include "nsUnicharUtils.h"
#include "nsEventDispatcher.h"
#include "nsIProgrammingLanguage.h"
#include "nsIXPConnect.h"
#include "nsXTFWeakTearoff.h"
#include "mozAutoDocUpdate.h"
#include "nsFocusManager.h"

using namespace mozilla;

nsXTFElementWrapper::nsXTFElementWrapper(already_AddRefed<nsINodeInfo> aNodeInfo,
                                         nsIXTFElement* aXTFElement)
    : nsXTFElementWrapperBase(aNodeInfo),
      mXTFElement(aXTFElement),
      mNotificationMask(0),
      mIntrinsicState(0),
      mTmpAttrName(nsGkAtoms::_asterix) 
                                            
{
  
  SetFlags(NODE_MAY_HAVE_CLASS);
}

nsXTFElementWrapper::~nsXTFElementWrapper()
{
  mXTFElement->OnDestroyed();
  mXTFElement = nullptr;
  if (mClassInfo) {
    mClassInfo->Disconnect();
    mClassInfo = nullptr;
  }
}

nsresult
nsXTFElementWrapper::Init()
{
  
  
  nsISupports* weakWrapper = nullptr;
  nsresult rv = NS_NewXTFWeakTearoff(NS_GET_IID(nsIXTFElementWrapper),
                                     (nsIXTFElementWrapper*)this,
                                     &weakWrapper);
  NS_ENSURE_SUCCESS(rv, rv);

  mXTFElement->OnCreated(static_cast<nsIXTFElementWrapper*>(weakWrapper));
  weakWrapper->Release();

  bool innerHandlesAttribs = false;
  GetXTFElement()->GetIsAttributeHandler(&innerHandlesAttribs);
  if (innerHandlesAttribs)
    mAttributeHandler = do_QueryInterface(GetXTFElement());
  return NS_OK;
}




NS_IMPL_ADDREF_INHERITED(nsXTFElementWrapper, nsXTFElementWrapperBase)
NS_IMPL_RELEASE_INHERITED(nsXTFElementWrapper, nsXTFElementWrapperBase)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXTFElementWrapper)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXTFElementWrapper,
                                                  nsXTFElementWrapperBase)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mXTFElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mAttributeHandler)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMETHODIMP
nsXTFElementWrapper::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null out param");

  NS_IMPL_QUERY_CYCLE_COLLECTION(nsXTFElementWrapper)
  if (aIID.Equals(NS_GET_IID(nsIClassInfo)) ||
      aIID.Equals(NS_GET_IID(nsXPCClassInfo))) {
    if (!mClassInfo) {
      mClassInfo = new nsXTFClassInfo(this);
    }
    NS_ADDREF(mClassInfo);
    *aInstancePtr = static_cast<nsIClassInfo*>(mClassInfo);
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIXPCScriptable))) {
    if (!mClassInfo) {
      mClassInfo = new nsXTFClassInfo(this);
    }
    NS_ADDREF(mClassInfo);
    *aInstancePtr = static_cast<nsIXPCScriptable*>(mClassInfo);
    return NS_OK;
  }
  if (aIID.Equals(NS_GET_IID(nsIXTFElementWrapper))) {
    *aInstancePtr = static_cast<nsIXTFElementWrapper*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  nsresult rv = nsXTFElementWrapperBase::QueryInterface(aIID, aInstancePtr);
  if (NS_SUCCEEDED(rv)) {
    return rv;
  }

  
  nsCOMPtr<nsISupports> inner;
  QueryInterfaceInner(aIID, getter_AddRefs(inner));

  if (inner) {
    rv = NS_NewXTFInterfaceAggregator(aIID, inner,
                                      static_cast<nsIContent*>(this),
                                      aInstancePtr);

    return rv;
  }

  return NS_ERROR_NO_INTERFACE;
}

nsXPCClassInfo*
nsXTFElementWrapper::GetClassInfo()
{
  if (!mClassInfo) {
    mClassInfo = new nsXTFClassInfo(this);
  }
  return mClassInfo;
}




nsresult
nsXTFElementWrapper::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                nsIContent* aBindingParent,
                                bool aCompileEventHandlers)
{
  
  
  nsCOMPtr<nsIDOMElement> domParent;
  if (aParent != GetParent()) {
    domParent = do_QueryInterface(aParent);
  }

  nsCOMPtr<nsIDOMDocument> domDocument;
  if (aDocument &&
      (mNotificationMask & (nsIXTFElement::NOTIFY_WILL_CHANGE_DOCUMENT |
                            nsIXTFElement::NOTIFY_DOCUMENT_CHANGED))) {
    domDocument = do_QueryInterface(aDocument);
  }

  if (domDocument &&
      (mNotificationMask & (nsIXTFElement::NOTIFY_WILL_CHANGE_DOCUMENT))) {
    GetXTFElement()->WillChangeDocument(domDocument);
  }

  if (domParent &&
      (mNotificationMask & (nsIXTFElement::NOTIFY_WILL_CHANGE_PARENT))) {
    GetXTFElement()->WillChangeParent(domParent);
  }

  nsresult rv = nsXTFElementWrapperBase::BindToTree(aDocument, aParent,
                                                    aBindingParent,
                                                    aCompileEventHandlers);

  NS_ENSURE_SUCCESS(rv, rv);

  if (mNotificationMask & nsIXTFElement::NOTIFY_PERFORM_ACCESSKEY)
    RegUnregAccessKey(true);

  if (domDocument &&
      (mNotificationMask & (nsIXTFElement::NOTIFY_DOCUMENT_CHANGED))) {
    GetXTFElement()->DocumentChanged(domDocument);
  }

  if (domParent &&
      (mNotificationMask & (nsIXTFElement::NOTIFY_PARENT_CHANGED))) {
    GetXTFElement()->ParentChanged(domParent);
  }

  return rv;  
}

void
nsXTFElementWrapper::UnbindFromTree(bool aDeep, bool aNullParent)
{
  
  

  bool inDoc = IsInDoc();
  if (inDoc &&
      (mNotificationMask & nsIXTFElement::NOTIFY_WILL_CHANGE_DOCUMENT)) {
    GetXTFElement()->WillChangeDocument(nullptr);
  }

  bool parentChanged = aNullParent && GetParent();

  if (parentChanged &&
      (mNotificationMask & nsIXTFElement::NOTIFY_WILL_CHANGE_PARENT)) {
    GetXTFElement()->WillChangeParent(nullptr);
  }

  if (mNotificationMask & nsIXTFElement::NOTIFY_PERFORM_ACCESSKEY)
    RegUnregAccessKey(false);

  nsXTFElementWrapperBase::UnbindFromTree(aDeep, aNullParent);

  if (parentChanged &&
      (mNotificationMask & nsIXTFElement::NOTIFY_PARENT_CHANGED)) {
    GetXTFElement()->ParentChanged(nullptr);
  }

  if (inDoc &&
      (mNotificationMask & nsIXTFElement::NOTIFY_DOCUMENT_CHANGED)) {
    GetXTFElement()->DocumentChanged(nullptr);
  }
}

nsresult
nsXTFElementWrapper::InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                   bool aNotify)
{
  nsresult rv;

  nsCOMPtr<nsIDOMNode> domKid;
  if (mNotificationMask & (nsIXTFElement::NOTIFY_WILL_INSERT_CHILD |
                           nsIXTFElement::NOTIFY_CHILD_INSERTED))
    domKid = do_QueryInterface(aKid);
  
  if (mNotificationMask & nsIXTFElement::NOTIFY_WILL_INSERT_CHILD)
    GetXTFElement()->WillInsertChild(domKid, aIndex);
  rv = nsXTFElementWrapperBase::InsertChildAt(aKid, aIndex, aNotify);
  if (mNotificationMask & nsIXTFElement::NOTIFY_CHILD_INSERTED)
    GetXTFElement()->ChildInserted(domKid, aIndex);
  
  return rv;
}

void
nsXTFElementWrapper::RemoveChildAt(uint32_t aIndex, bool aNotify)
{
  if (mNotificationMask & nsIXTFElement::NOTIFY_WILL_REMOVE_CHILD)
    GetXTFElement()->WillRemoveChild(aIndex);
  nsXTFElementWrapperBase::RemoveChildAt(aIndex, aNotify);
  if (mNotificationMask & nsIXTFElement::NOTIFY_CHILD_REMOVED)
    GetXTFElement()->ChildRemoved(aIndex);
}

nsIAtom *
nsXTFElementWrapper::GetIDAttributeName() const
{
  
  return nsGkAtoms::id;
}

nsresult
nsXTFElementWrapper::SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                             nsIAtom* aPrefix, const nsAString& aValue,
                             bool aNotify)
{
  nsresult rv;

  if (aNameSpaceID == kNameSpaceID_None &&
      (mNotificationMask & nsIXTFElement::NOTIFY_WILL_SET_ATTRIBUTE))
    GetXTFElement()->WillSetAttribute(aName, aValue);

  if (aNameSpaceID==kNameSpaceID_None && HandledByInner(aName)) {
    rv = mAttributeHandler->SetAttribute(aName, aValue);
    
  }
  else { 
    rv = nsXTFElementWrapperBase::SetAttr(aNameSpaceID, aName, aPrefix, aValue, aNotify);
  }
  
  if (aNameSpaceID == kNameSpaceID_None &&
      (mNotificationMask & nsIXTFElement::NOTIFY_ATTRIBUTE_SET))
    GetXTFElement()->AttributeSet(aName, aValue);

  if (mNotificationMask & nsIXTFElement::NOTIFY_PERFORM_ACCESSKEY) {
    nsCOMPtr<nsIDOMAttr> accesskey;
    GetXTFElement()->GetAccesskeyNode(getter_AddRefs(accesskey));
    nsCOMPtr<nsIAttribute> attr(do_QueryInterface(accesskey));
    if (attr && attr->NodeInfo()->Equals(aName, aNameSpaceID))
      RegUnregAccessKey(true);
  }

  return rv;
}

bool
nsXTFElementWrapper::GetAttr(int32_t aNameSpaceID, nsIAtom* aName, 
                             nsAString& aResult) const
{
  if (aNameSpaceID==kNameSpaceID_None && HandledByInner(aName)) {
    
    nsresult rv = mAttributeHandler->GetAttribute(aName, aResult);
    return NS_SUCCEEDED(rv) && !aResult.IsVoid();
  }
  else { 
    return nsXTFElementWrapperBase::GetAttr(aNameSpaceID, aName, aResult);
  }
}

bool
nsXTFElementWrapper::HasAttr(int32_t aNameSpaceID, nsIAtom* aName) const
{
  if (aNameSpaceID==kNameSpaceID_None && HandledByInner(aName)) {
    bool rval = false;
    mAttributeHandler->HasAttribute(aName, &rval);
    return rval;
  }
  else { 
    return nsXTFElementWrapperBase::HasAttr(aNameSpaceID, aName);
  }
}

bool
nsXTFElementWrapper::AttrValueIs(int32_t aNameSpaceID,
                                 nsIAtom* aName,
                                 const nsAString& aValue,
                                 nsCaseTreatment aCaseSensitive) const
{
  NS_ASSERTION(aName, "Must have attr name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown, "Must have namespace");

  if (aNameSpaceID == kNameSpaceID_None && HandledByInner(aName)) {
    nsAutoString ourVal;
    if (!GetAttr(aNameSpaceID, aName, ourVal)) {
      return false;
    }
    return aCaseSensitive == eCaseMatters ?
      aValue.Equals(ourVal) :
      aValue.Equals(ourVal, nsCaseInsensitiveStringComparator());
  }

  return nsXTFElementWrapperBase::AttrValueIs(aNameSpaceID, aName, aValue,
                                              aCaseSensitive);
}

bool
nsXTFElementWrapper::AttrValueIs(int32_t aNameSpaceID,
                                 nsIAtom* aName,
                                 nsIAtom* aValue,
                                 nsCaseTreatment aCaseSensitive) const
{
  NS_ASSERTION(aName, "Must have attr name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown, "Must have namespace");
  NS_ASSERTION(aValue, "Null value atom");

  if (aNameSpaceID == kNameSpaceID_None && HandledByInner(aName)) {
    nsAutoString ourVal;
    if (!GetAttr(aNameSpaceID, aName, ourVal)) {
      return false;
    }
    if (aCaseSensitive == eCaseMatters) {
      return aValue->Equals(ourVal);
    }
    nsAutoString val;
    aValue->ToString(val);
    return val.Equals(ourVal, nsCaseInsensitiveStringComparator());
  }

  return nsXTFElementWrapperBase::AttrValueIs(aNameSpaceID, aName, aValue,
                                              aCaseSensitive);
}

int32_t
nsXTFElementWrapper::FindAttrValueIn(int32_t aNameSpaceID,
                                     nsIAtom* aName,
                                     AttrValuesArray* aValues,
                                     nsCaseTreatment aCaseSensitive) const
{
  NS_ASSERTION(aName, "Must have attr name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown, "Must have namespace");
  NS_ASSERTION(aValues, "Null value array");
  
  if (aNameSpaceID == kNameSpaceID_None && HandledByInner(aName)) {
    nsAutoString ourVal;
    if (!GetAttr(aNameSpaceID, aName, ourVal)) {
      return ATTR_MISSING;
    }
    
    for (int32_t i = 0; aValues[i]; ++i) {
      if (aCaseSensitive == eCaseMatters) {
        if ((*aValues[i])->Equals(ourVal)) {
          return i;
        }
      } else {
        nsAutoString val;
        (*aValues[i])->ToString(val);
        if (val.Equals(ourVal, nsCaseInsensitiveStringComparator())) {
          return i;
        }
      }
    }
    return ATTR_VALUE_NO_MATCH;
  }

  return nsXTFElementWrapperBase::FindAttrValueIn(aNameSpaceID, aName, aValues,
                                                  aCaseSensitive);
}

nsresult
nsXTFElementWrapper::UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttr, 
                               bool aNotify)
{
  nsresult rv;

  if (aNameSpaceID == kNameSpaceID_None &&
      (mNotificationMask & nsIXTFElement::NOTIFY_WILL_REMOVE_ATTRIBUTE))
    GetXTFElement()->WillRemoveAttribute(aAttr);

  if (mNotificationMask & nsIXTFElement::NOTIFY_PERFORM_ACCESSKEY) {
    nsCOMPtr<nsIDOMAttr> accesskey;
    GetXTFElement()->GetAccesskeyNode(getter_AddRefs(accesskey));
    nsCOMPtr<nsIAttribute> attr(do_QueryInterface(accesskey));
    if (attr && attr->NodeInfo()->Equals(aAttr, aNameSpaceID))
      RegUnregAccessKey(false);
  }

  if (aNameSpaceID==kNameSpaceID_None && HandledByInner(aAttr)) {
    nsDOMSlots *slots = GetExistingDOMSlots();
    if (slots && slots->mAttributeMap) {
      slots->mAttributeMap->DropAttribute(aNameSpaceID, aAttr);
    }
    rv = mAttributeHandler->RemoveAttribute(aAttr);

    
    
    
    

    
  }
  else { 
    rv = nsXTFElementWrapperBase::UnsetAttr(aNameSpaceID, aAttr, aNotify);
  }

  if (aNameSpaceID == kNameSpaceID_None &&
      (mNotificationMask & nsIXTFElement::NOTIFY_ATTRIBUTE_REMOVED))
    GetXTFElement()->AttributeRemoved(aAttr);

  return rv;
}

const nsAttrName*
nsXTFElementWrapper::GetAttrNameAt(uint32_t aIndex) const
{
  uint32_t innerCount=0;
  if (mAttributeHandler) {
    mAttributeHandler->GetAttributeCount(&innerCount);
  }
  
  if (aIndex < innerCount) {
    nsCOMPtr<nsIAtom> localName;
    nsresult rv = mAttributeHandler->GetAttributeNameAt(aIndex, getter_AddRefs(localName));
    NS_ENSURE_SUCCESS(rv, nullptr);

    const_cast<nsXTFElementWrapper*>(this)->mTmpAttrName.SetTo(localName);
    return &mTmpAttrName;
  }
  else { 
    return nsXTFElementWrapperBase::GetAttrNameAt(aIndex - innerCount);
  }
}

uint32_t
nsXTFElementWrapper::GetAttrCount() const
{
  uint32_t innerCount = 0;
  if (mAttributeHandler) {
    mAttributeHandler->GetAttributeCount(&innerCount);
  }
  
  return innerCount + nsXTFElementWrapperBase::GetAttrCount();
}

void
nsXTFElementWrapper::BeginAddingChildren()
{
  if (mNotificationMask & nsIXTFElement::NOTIFY_BEGIN_ADDING_CHILDREN)
    GetXTFElement()->BeginAddingChildren();
}

void
nsXTFElementWrapper::DoneAddingChildren(bool aHaveNotified)
{
  if (mNotificationMask & nsIXTFElement::NOTIFY_DONE_ADDING_CHILDREN)
    GetXTFElement()->DoneAddingChildren();
}

already_AddRefed<nsINodeInfo>
nsXTFElementWrapper::GetExistingAttrNameFromQName(const nsAString& aStr) const
{
  nsINodeInfo* nodeInfo = nsXTFElementWrapperBase::GetExistingAttrNameFromQName(aStr).get();

  
  if (!nodeInfo) {
    nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(aStr);
    if (HandledByInner(nameAtom)) 
      nodeInfo = mNodeInfo->NodeInfoManager()->
        GetNodeInfo(nameAtom, nullptr, kNameSpaceID_None,
                    nsIDOMNode::ATTRIBUTE_NODE).get();
  }
  
  return nodeInfo;
}

nsEventStates
nsXTFElementWrapper::IntrinsicState() const
{
  nsEventStates retState = nsXTFElementWrapperBase::IntrinsicState();
  if (mIntrinsicState.HasState(NS_EVENT_STATE_MOZ_READONLY)) {
    retState &= ~NS_EVENT_STATE_MOZ_READWRITE;
  } else if (mIntrinsicState.HasState(NS_EVENT_STATE_MOZ_READWRITE)) {
    retState &= ~NS_EVENT_STATE_MOZ_READONLY;
  }

  return  retState | mIntrinsicState;
}

void
nsXTFElementWrapper::PerformAccesskey(bool aKeyCausesActivation,
                                      bool aIsTrustedEvent)
{
  if (mNotificationMask & nsIXTFElement::NOTIFY_PERFORM_ACCESSKEY) {
    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    if (fm)
      fm->SetFocus(this, nsIFocusManager::FLAG_BYKEY);

    if (aKeyCausesActivation)
      GetXTFElement()->PerformAccesskey();
  }
}

nsresult
nsXTFElementWrapper::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nullptr;
  nsCOMPtr<nsIContent> it;
  nsContentUtils::GetXTFService()->CreateElement(getter_AddRefs(it),
                                                 aNodeInfo);
  if (!it)
    return NS_ERROR_OUT_OF_MEMORY;

  nsXTFElementWrapper* wrapper =
    static_cast<nsXTFElementWrapper*>(it.get());
  nsresult rv = const_cast<nsXTFElementWrapper*>(this)->CopyInnerTo(wrapper);

  if (NS_SUCCEEDED(rv)) {
    if (mAttributeHandler) {
      uint32_t innerCount = 0;
      mAttributeHandler->GetAttributeCount(&innerCount);
      for (uint32_t i = 0; i < innerCount; ++i) {
        nsCOMPtr<nsIAtom> attrName;
        mAttributeHandler->GetAttributeNameAt(i, getter_AddRefs(attrName));
        if (attrName) {
          nsAutoString value;
          if (NS_SUCCEEDED(mAttributeHandler->GetAttribute(attrName, value)))
            it->SetAttr(kNameSpaceID_None, attrName, value, true);
        }
      }
    }
    NS_ADDREF(*aResult = it);
  }

  
  wrapper->CloneState(const_cast<nsXTFElementWrapper*>(this));
  return rv;
}




void
nsXTFElementWrapper::GetAttribute(const nsAString& aName, nsString& aReturn)
{
  const nsAttrName* name = InternalGetExistingAttrNameFromQName(aName);
  if (name) {
    GetAttr(name->NamespaceID(), name->LocalName(), aReturn);
    return;
  }

  
  if (mAttributeHandler) {
    nsresult rv = nsContentUtils::CheckQName(aName, false);
    if (NS_FAILED(rv)) {
      return; 
    }
    nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(aName);
    if (HandledByInner(nameAtom)) {
      GetAttr(kNameSpaceID_None, nameAtom, aReturn);
      return;
    }
  }
  
  SetDOMStringToNull(aReturn);
}

void
nsXTFElementWrapper::RemoveAttribute(const nsAString& aName,
                                     ErrorResult& aError)
{
  const nsAttrName* name = InternalGetExistingAttrNameFromQName(aName);

  if (name) {
    nsAttrName tmp(*name);
    aError = UnsetAttr(name->NamespaceID(), name->LocalName(), true);
    return;
  }

  
  if (mAttributeHandler) {
    nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(aName);
    aError = UnsetAttr(kNameSpaceID_None, nameAtom, true);
  }
}

bool
nsXTFElementWrapper::HasAttribute(const nsAString& aName) const
{
  const nsAttrName* name = InternalGetExistingAttrNameFromQName(aName);
  if (name) {
    return true;
  }
  
  
  if (mAttributeHandler) {
    nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(aName);
    return HasAttr(kNameSpaceID_None, nameAtom);
  }

  return false;
}






NS_IMETHODIMP 
nsXTFElementWrapper::GetInterfaces(uint32_t* aCount, nsIID*** aArray)
{
  *aArray = nullptr;
  *aCount = 0;
  uint32_t baseCount = 0;
  nsIID** baseArray = nullptr;
  uint32_t xtfCount = 0;
  nsIID** xtfArray = nullptr;

  nsCOMPtr<nsIClassInfo> baseCi = GetBaseXPCClassInfo();
  if (baseCi) {
    baseCi->GetInterfaces(&baseCount, &baseArray);
  }

  GetXTFElement()->GetScriptingInterfaces(&xtfCount, &xtfArray);
  if (!xtfCount) {
    *aCount = baseCount;
    *aArray = baseArray;
    return NS_OK;
  } else if (!baseCount) {
    *aCount = xtfCount;
    *aArray = xtfArray;
    return NS_OK;
  }

  uint32_t count = baseCount + xtfCount;
  nsIID** iids = static_cast<nsIID**>
                            (nsMemory::Alloc(count * sizeof(nsIID*)));
  NS_ENSURE_TRUE(iids, NS_ERROR_OUT_OF_MEMORY);

  uint32_t i = 0;
  for (; i < baseCount; ++i) {
    iids[i] = static_cast<nsIID*>
                         (nsMemory::Clone(baseArray[i], sizeof(nsIID)));
    if (!iids[i]) {
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(baseCount, baseArray);
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(xtfCount, xtfArray);
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(i, iids);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  for (; i < count; ++i) {
    iids[i] = static_cast<nsIID*>
                         (nsMemory::Clone(xtfArray[i - baseCount], sizeof(nsIID)));
    if (!iids[i]) {
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(baseCount, baseArray);
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(xtfCount, xtfArray);
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(i, iids);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(baseCount, baseArray);
  NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(xtfCount, xtfArray);
  *aArray = iids;
  *aCount = count;

  return NS_OK;
}


NS_IMETHODIMP 
nsXTFElementWrapper::GetHelperForLanguage(uint32_t language,
                                          nsISupports** aHelper)
{
  *aHelper = nullptr;
  nsCOMPtr<nsIClassInfo> ci = GetBaseXPCClassInfo();
  return
    ci ? ci->GetHelperForLanguage(language, aHelper) : NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP 
nsXTFElementWrapper::GetContractID(char * *aContractID)
{
  *aContractID = nullptr;
  return NS_OK;
}


NS_IMETHODIMP 
nsXTFElementWrapper::GetClassDescription(char * *aClassDescription)
{
  *aClassDescription = nullptr;
  return NS_OK;
}


NS_IMETHODIMP 
nsXTFElementWrapper::GetClassID(nsCID * *aClassID)
{
  *aClassID = nullptr;
  return NS_OK;
}


NS_IMETHODIMP 
nsXTFElementWrapper::GetImplementationLanguage(uint32_t *aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::UNKNOWN;
  return NS_OK;
}


NS_IMETHODIMP 
nsXTFElementWrapper::GetFlags(uint32_t *aFlags)
{
  *aFlags = nsIClassInfo::DOM_OBJECT;
  return NS_OK;
}


NS_IMETHODIMP 
nsXTFElementWrapper::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
  return NS_ERROR_NOT_AVAILABLE;
}





NS_IMETHODIMP
nsXTFElementWrapper::GetElementNode(nsIDOMElement * *aElementNode)
{
  *aElementNode = (nsIDOMElement*)this;
  NS_ADDREF(*aElementNode);
  return NS_OK;
}


NS_IMETHODIMP
nsXTFElementWrapper::GetDocumentFrameElement(nsIDOMElement * *aDocumentFrameElement)
{
  *aDocumentFrameElement = nullptr;
  
  nsIDocument *doc = GetCurrentDoc();
  if (!doc) {
    NS_WARNING("no document");
    return NS_OK;
  }
  nsCOMPtr<nsISupports> container = doc->GetContainer();
  if (!container) {
    NS_ERROR("no docshell");
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsPIDOMWindow> pidomwin = do_GetInterface(container);
  if (!pidomwin) {
    NS_ERROR("no nsPIDOMWindow interface on docshell");
    return NS_ERROR_FAILURE;
  }
  *aDocumentFrameElement = pidomwin->GetFrameElementInternal();
  NS_IF_ADDREF(*aDocumentFrameElement);
  return NS_OK;
}


NS_IMETHODIMP
nsXTFElementWrapper::GetNotificationMask(uint32_t *aNotificationMask)
{
  *aNotificationMask = mNotificationMask;
  return NS_OK;
}
NS_IMETHODIMP
nsXTFElementWrapper::SetNotificationMask(uint32_t aNotificationMask)
{
  mNotificationMask = aNotificationMask;
  return NS_OK;
}



bool
nsXTFElementWrapper::QueryInterfaceInner(REFNSIID aIID, void** result)
{
  
  
  if (aIID.Equals(NS_GET_IID(nsIXPConnectWrappedJS))) return false;

  GetXTFElement()->QueryInterface(aIID, result);
  return (*result!=nullptr);
}

bool
nsXTFElementWrapper::HandledByInner(nsIAtom *attr) const
{
  bool retval = false;
  if (mAttributeHandler)
    mAttributeHandler->HandlesAttribute(attr, &retval);
  return retval;
}




uint32_t
nsXTFElementWrapper::GetScriptableFlags()
{
  return GetBaseXPCClassInfo() ? GetBaseXPCClassInfo()->GetScriptableFlags()
                               : 0;
}

nsresult
nsXTFElementWrapper::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  nsresult rv = NS_OK;
  if (aVisitor.mEventStatus == nsEventStatus_eConsumeNoDefault ||
      !(mNotificationMask & nsIXTFElement::NOTIFY_HANDLE_DEFAULT)) {
    return rv;
  }

  if (!aVisitor.mDOMEvent) {
    
    if (NS_FAILED(rv = nsEventDispatcher::CreateEvent(aVisitor.mPresContext,
                                                      aVisitor.mEvent,
                                                      EmptyString(),
                                                      &aVisitor.mDOMEvent)))
      return rv;
  }
  if (!aVisitor.mDOMEvent)
    return NS_ERROR_FAILURE;
  
  bool defaultHandled = false;
  nsIXTFElement* xtfElement = GetXTFElement();
  if (xtfElement)
    rv = xtfElement->HandleDefault(aVisitor.mDOMEvent, &defaultHandled);
  if (defaultHandled)
    aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
  return rv;
}

NS_IMETHODIMP
nsXTFElementWrapper::SetIntrinsicState(nsEventStates::InternalType aNewState)
{
  nsIDocument *doc = GetCurrentDoc();
  nsEventStates newStates(aNewState);
  nsEventStates bits = mIntrinsicState ^ newStates;

  if (!doc || bits.IsEmpty())
    return NS_OK;

  NS_WARN_IF_FALSE(!newStates.HasAllStates(NS_EVENT_STATE_MOZ_READONLY |
                                           NS_EVENT_STATE_MOZ_READWRITE),
                   "Both READONLY and READWRITE are being set.  Yikes!!!");

  mIntrinsicState = newStates;
  UpdateState(true);

  return NS_OK;
}

nsIAtom *
nsXTFElementWrapper::GetClassAttributeName() const
{
  return mClassAttributeName;
}

const nsAttrValue*
nsXTFElementWrapper::DoGetClasses() const
{
  const nsAttrValue* val = nullptr;
  nsIAtom* clazzAttr = GetClassAttributeName();
  if (clazzAttr) {
    val = mAttrsAndChildren.GetAttr(clazzAttr);
    
    if (val && val->Type() == nsAttrValue::eString) {
      nsAutoString value;
      val->ToString(value);
      nsAttrValue newValue;
      newValue.ParseAtomArray(value);
      const_cast<nsAttrAndChildArray*>(&mAttrsAndChildren)->
        SetAndTakeAttr(clazzAttr, newValue);
    }
  }
  return val;
}

nsresult
nsXTFElementWrapper::SetClassAttributeName(nsIAtom* aName)
{
  
  if (mClassAttributeName || !aName)
    return NS_ERROR_FAILURE;
  
  mClassAttributeName = aName;
  return NS_OK;
}

void
nsXTFElementWrapper::RegUnregAccessKey(bool aDoReg)
{
  nsIDocument* doc = GetCurrentDoc();
  if (!doc)
    return;

  
  nsIPresShell *presShell = doc->GetShell();
  if (!presShell)
    return;

  nsPresContext *presContext = presShell->GetPresContext();
  if (!presContext)
    return;

  nsEventStateManager *esm = presContext->EventStateManager();
  if (!esm)
    return;

  
  nsCOMPtr<nsIDOMAttr> accesskeyNode;
  GetXTFElement()->GetAccesskeyNode(getter_AddRefs(accesskeyNode));
  if (!accesskeyNode)
    return;

  nsAutoString accessKey;
  accesskeyNode->GetValue(accessKey);

  if (aDoReg && !accessKey.IsEmpty())
    esm->RegisterAccessKey(this, (uint32_t)accessKey.First());
  else
    esm->UnregisterAccessKey(this, (uint32_t)accessKey.First());
}

nsresult
NS_NewXTFElementWrapper(nsIXTFElement* aXTFElement,
                        already_AddRefed<nsINodeInfo> aNodeInfo,
                        nsIContent** aResult)
{
  *aResult = nullptr;
   NS_ENSURE_ARG(aXTFElement);

  nsXTFElementWrapper* result = new nsXTFElementWrapper(aNodeInfo, aXTFElement);
  if (!result) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(result);

  nsresult rv = result->Init();
  if (NS_FAILED(rv)) {
    NS_RELEASE(result);
    return rv;
  }

  *aResult = result;
  return NS_OK;
}

NS_IMPL_ISUPPORTS3(nsXTFClassInfo,
                   nsIClassInfo,
                   nsXPCClassInfo,
                   nsIXPCScriptable)
