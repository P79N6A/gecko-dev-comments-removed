








































#include "nsDOMAttribute.h"
#include "nsGenericElement.h"
#include "nsIContent.h"
#include "nsContentCreatorFunctions.h"
#include "nsINameSpaceManager.h"
#include "nsDOMError.h"
#include "nsContentUtils.h"
#include "nsUnicharUtils.h"
#include "nsDOMString.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOM3Attr.h"
#include "nsIDOMUserDataHandler.h"
#include "nsEventDispatcher.h"
#include "nsGkAtoms.h"
#include "nsCOMArray.h"
#include "nsNodeUtils.h"


PRBool nsDOMAttribute::sInitialized;

nsDOMAttribute::nsDOMAttribute(nsDOMAttributeMap *aAttrMap,
                               nsINodeInfo       *aNodeInfo,
                               const nsAString   &aValue)
  : nsIAttribute(aAttrMap, aNodeInfo), mValue(aValue)
{
  NS_ABORT_IF_FALSE(mNodeInfo, "We must get a nodeinfo here!");


  
  
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMAttribute)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMAttribute)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mChild)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_LISTENERMANAGER
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_USERDATA
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMAttribute)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mChild)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_LISTENERMANAGER
  NS_IMPL_CYCLE_COLLECTION_UNLINK_USERDATA
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMAttribute)
  NS_INTERFACE_MAP_ENTRY(nsIDOMAttr)
  NS_INTERFACE_MAP_ENTRY(nsIAttribute)
  NS_INTERFACE_MAP_ENTRY(nsINode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOM3Node)
  NS_INTERFACE_MAP_ENTRY(nsIDOM3Attr)
  NS_INTERFACE_MAP_ENTRY(nsPIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsISupportsWeakReference,
                                 new nsNodeSupportsWeakRefTearoff(this))
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMAttr)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(Attr)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsDOMAttribute, nsIDOMAttr)
NS_IMPL_CYCLE_COLLECTING_RELEASE_FULL(nsDOMAttribute, nsIDOMAttr,
                                      nsNodeUtils::LastRelease(this))

void
nsDOMAttribute::SetMap(nsDOMAttributeMap *aMap)
{
  if (mAttrMap && !aMap && sInitialized) {
    
    
    GetValue(mValue);
  }
  
  mAttrMap = aMap;
}

nsIContent*
nsDOMAttribute::GetContent() const
{
  return GetContentInternal();
}

nsresult
nsDOMAttribute::SetOwnerDocument(nsIDocument* aDocument)
{
  NS_ASSERTION(aDocument, "Missing document");

  nsIDocument *doc = GetOwnerDoc();
  NS_ASSERTION(doc != aDocument, "bad call to nsDOMAttribute::SetOwnerDocument");
  if (doc) {
    doc->PropertyTable()->DeleteAllPropertiesFor(this);
  }

  nsCOMPtr<nsINodeInfo> newNodeInfo;
  nsresult rv = aDocument->NodeInfoManager()->
    GetNodeInfo(mNodeInfo->NameAtom(), mNodeInfo->GetPrefixAtom(),
                mNodeInfo->NamespaceID(), getter_AddRefs(newNodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(newNodeInfo, "GetNodeInfo lies");
  mNodeInfo.swap(newNodeInfo);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::GetName(nsAString& aName)
{
  mNodeInfo->GetQualifiedName(aName);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::GetValue(nsAString& aValue)
{
  nsIContent* content = GetContentInternal();
  if (content) {
    content->GetAttr(mNodeInfo->NamespaceID(), mNodeInfo->NameAtom(), aValue);
  }
  else {
    aValue = mValue;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::SetValue(const nsAString& aValue)
{
  nsresult rv = NS_OK;
  nsIContent* content = GetContentInternal();
  if (content) {
    rv = content->SetAttr(mNodeInfo->NamespaceID(),
                          mNodeInfo->NameAtom(),
                          mNodeInfo->GetPrefixAtom(),
                          aValue,
                          PR_TRUE);
  }
  else {
    mValue = aValue;
  }

  return rv;
}

NS_IMETHODIMP
nsDOMAttribute::GetSpecified(PRBool* aSpecified)
{
  NS_ENSURE_ARG_POINTER(aSpecified);

  nsIContent* content = GetContentInternal();
  *aSpecified = content && content->HasAttr(mNodeInfo->NamespaceID(),
                                            mNodeInfo->NameAtom());

  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::GetOwnerElement(nsIDOMElement** aOwnerElement)
{
  NS_ENSURE_ARG_POINTER(aOwnerElement);

  nsIContent* content = GetContentInternal();
  if (content) {
    return CallQueryInterface(content, aOwnerElement);
  }

  *aOwnerElement = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::GetNodeName(nsAString& aNodeName)
{
  return GetName(aNodeName);
}

NS_IMETHODIMP
nsDOMAttribute::GetNodeValue(nsAString& aNodeValue)
{
  return GetValue(aNodeValue);
}

NS_IMETHODIMP
nsDOMAttribute::SetNodeValue(const nsAString& aNodeValue)
{
  return SetValue(aNodeValue);
}

NS_IMETHODIMP
nsDOMAttribute::GetNodeType(PRUint16* aNodeType)
{
  NS_ENSURE_ARG_POINTER(aNodeType);

  *aNodeType = (PRUint16)nsIDOMNode::ATTRIBUTE_NODE;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::GetParentNode(nsIDOMNode** aParentNode)
{
  NS_ENSURE_ARG_POINTER(aParentNode);

  *aParentNode = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
  nsSlots *slots = GetSlots();
  NS_ENSURE_TRUE(slots, NS_ERROR_OUT_OF_MEMORY);

  if (!slots->mChildNodes) {
    slots->mChildNodes = new nsChildContentList(this);
    NS_ENSURE_TRUE(slots->mChildNodes, NS_ERROR_OUT_OF_MEMORY);
    NS_ADDREF(slots->mChildNodes);
  }

  NS_ADDREF(*aChildNodes = slots->mChildNodes);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::HasChildNodes(PRBool* aHasChildNodes)
{
  PRBool hasChild;
  nsresult rv = EnsureChildState(PR_FALSE, hasChild);
  NS_ENSURE_SUCCESS(rv, rv);

  *aHasChildNodes = hasChild;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::HasAttributes(PRBool* aHasAttributes)
{
  NS_ENSURE_ARG_POINTER(aHasAttributes);

  *aHasAttributes = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::GetFirstChild(nsIDOMNode** aFirstChild)
{
  *aFirstChild = nsnull;

  PRBool hasChild;
  nsresult rv = EnsureChildState(PR_TRUE, hasChild);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mChild) {
    CallQueryInterface(mChild, aFirstChild);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::GetLastChild(nsIDOMNode** aLastChild)
{
  return GetFirstChild(aLastChild);
}

NS_IMETHODIMP
nsDOMAttribute::GetPreviousSibling(nsIDOMNode** aPreviousSibling)
{
  NS_ENSURE_ARG_POINTER(aPreviousSibling);

  *aPreviousSibling = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::GetNextSibling(nsIDOMNode** aNextSibling)
{
  NS_ENSURE_ARG_POINTER(aNextSibling);

  *aNextSibling = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::GetAttributes(nsIDOMNamedNodeMap** aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);

  *aAttributes = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, nsIDOMNode** aReturn)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDOMAttribute::ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDOMAttribute::RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDOMAttribute::AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsDOMAttribute::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  nsAutoString value;
  NS_CONST_CAST(nsDOMAttribute*, this)->GetValue(value);

  *aResult = new nsDOMAttribute(nsnull, aNodeInfo, value);
  if (!*aResult) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::CloneNode(PRBool aDeep, nsIDOMNode** aResult)
{
  return nsNodeUtils::CloneNodeImpl(this, aDeep, aResult);
}

NS_IMETHODIMP
nsDOMAttribute::GetOwnerDocument(nsIDOMDocument** aOwnerDocument)
{
  *aOwnerDocument = nsnull;

  nsIDocument *document = GetOwnerDoc();

  return document ? CallQueryInterface(document, aOwnerDocument) : NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::GetNamespaceURI(nsAString& aNamespaceURI)
{
  return mNodeInfo->GetNamespaceURI(aNamespaceURI);
}

NS_IMETHODIMP
nsDOMAttribute::GetPrefix(nsAString& aPrefix)
{
  mNodeInfo->GetPrefix(aPrefix);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::SetPrefix(const nsAString& aPrefix)
{
  

  nsCOMPtr<nsINodeInfo> newNodeInfo;
  nsCOMPtr<nsIAtom> prefix;

  if (!aPrefix.IsEmpty()) {
    prefix = do_GetAtom(aPrefix);
    if (!prefix) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  if (!nsContentUtils::IsValidNodeName(mNodeInfo->NameAtom(), prefix,
                                       mNodeInfo->NamespaceID())) {
    return NS_ERROR_DOM_NAMESPACE_ERR;
  }

  nsresult rv = nsContentUtils::PrefixChanged(mNodeInfo, prefix,
                                              getter_AddRefs(newNodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  nsIContent* content = GetContentInternal();
  if (content) {
    nsIAtom *name = mNodeInfo->NameAtom();
    PRInt32 nameSpaceID = mNodeInfo->NamespaceID();

    nsAutoString tmpValue;
    if (content->GetAttr(nameSpaceID, name, tmpValue)) {
      content->UnsetAttr(nameSpaceID, name, PR_TRUE);

      content->SetAttr(newNodeInfo->NamespaceID(), newNodeInfo->NameAtom(),
                       newNodeInfo->GetPrefixAtom(), tmpValue, PR_TRUE);
    }
  }

  newNodeInfo.swap(mNodeInfo);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::GetLocalName(nsAString& aLocalName)
{
  mNodeInfo->GetLocalName(aLocalName);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::Normalize()
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::IsSupported(const nsAString& aFeature,
                            const nsAString& aVersion,
                            PRBool* aReturn)
{
  return nsGenericElement::InternalIsSupported(NS_STATIC_CAST(nsIDOMAttr*, this), 
                                               aFeature, aVersion, aReturn);
}

NS_IMETHODIMP
nsDOMAttribute::GetBaseURI(nsAString &aURI)
{
  aURI.Truncate();
  nsresult rv = NS_OK;
  nsCOMPtr<nsIDOM3Node> node(do_QueryInterface(GetContentInternal()));
  if (node)
    rv = node->GetBaseURI(aURI);
  return rv;
}

NS_IMETHODIMP
nsDOMAttribute::CompareDocumentPosition(nsIDOMNode* aOther,
                                        PRUint16* aReturn)
{
  NS_ENSURE_ARG_POINTER(aOther);

  nsCOMPtr<nsINode> other = do_QueryInterface(aOther);
  NS_ENSURE_TRUE(other, NS_ERROR_DOM_NOT_SUPPORTED_ERR);

  *aReturn = nsContentUtils::ComparePosition(other, this);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::IsSameNode(nsIDOMNode* aOther,
                           PRBool* aReturn)
{
  NS_ASSERTION(aReturn, "IsSameNode() called with aReturn == nsnull!");
  
  *aReturn = SameCOMIdentity(NS_STATIC_CAST(nsIDOMNode*, this), aOther);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::IsEqualNode(nsIDOMNode* aOther,
                            PRBool* aReturn)
{
  NS_ENSURE_ARG_POINTER(aOther);

  *aReturn = PR_FALSE;

  
  nsCOMPtr<nsIAttribute> aOtherAttr = do_QueryInterface(aOther);
  if (!aOtherAttr) {
    return NS_OK;
  }

  
  if (!mNodeInfo->Equals(aOtherAttr->NodeInfo())) {
    return NS_OK;
  }

  
  nsAutoString ourValue, otherValue;
  nsresult rv = GetValue(ourValue);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOther->GetNodeValue(otherValue);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!ourValue.Equals(otherValue))
    return NS_OK;

  

  *aReturn = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::IsDefaultNamespace(const nsAString& aNamespaceURI,
                                   PRBool* aReturn)
{
  *aReturn = PR_FALSE;
  nsCOMPtr<nsIDOM3Node> node(do_QueryInterface(GetContentInternal()));
  if (node) {
    return node->IsDefaultNamespace(aNamespaceURI, aReturn);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::GetTextContent(nsAString &aTextContent)
{
  return GetNodeValue(aTextContent);
}

NS_IMETHODIMP
nsDOMAttribute::SetTextContent(const nsAString& aTextContent)
{
  return SetNodeValue(aTextContent);
}


NS_IMETHODIMP
nsDOMAttribute::GetFeature(const nsAString& aFeature,
                           const nsAString& aVersion,
                           nsISupports** aReturn)
{
  return nsGenericElement::InternalGetFeature(NS_STATIC_CAST(nsIDOMAttr*, this), 
                                              aFeature, aVersion, aReturn);
}

NS_IMETHODIMP
nsDOMAttribute::SetUserData(const nsAString& aKey, nsIVariant* aData,
                            nsIDOMUserDataHandler* aHandler,
                            nsIVariant** aResult)
{
  return nsNodeUtils::SetUserData(this, aKey, aData, aHandler, aResult);
}

NS_IMETHODIMP
nsDOMAttribute::GetUserData(const nsAString& aKey, nsIVariant** aResult)
{
  return nsNodeUtils::GetUserData(this, aKey, aResult);
}

NS_IMETHODIMP
nsDOMAttribute::GetIsId(PRBool* aReturn)
{
  nsIContent* content = GetContentInternal();
  if (!content)
  {
    *aReturn = PR_FALSE;
    return NS_OK;
  }

  nsIAtom* idAtom = content->GetIDAttributeName();
  if (!idAtom)
  {
    *aReturn = PR_FALSE;
    return NS_OK;
  }

  *aReturn = mNodeInfo->Equals(idAtom, kNameSpaceID_None);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::GetSchemaTypeInfo(nsIDOM3TypeInfo** aReturn)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDOMAttribute::LookupPrefix(const nsAString& aNamespaceURI,
                             nsAString& aPrefix)
{
  nsCOMPtr<nsIDOM3Node> node(do_QueryInterface(GetContentInternal()));
  if (node)
    return node->LookupPrefix(aNamespaceURI, aPrefix);

  SetDOMStringToNull(aPrefix);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMAttribute::LookupNamespaceURI(const nsAString& aNamespacePrefix,
                                   nsAString& aNamespaceURI)
{
  nsCOMPtr<nsIDOM3Node> node(do_QueryInterface(GetContentInternal()));
  if (node)
    return node->LookupNamespaceURI(aNamespacePrefix, aNamespaceURI);

  SetDOMStringToNull(aNamespaceURI);
  return NS_OK;
}

PRBool
nsDOMAttribute::IsNodeOfType(PRUint32 aFlags) const
{
    return !(aFlags & ~eATTRIBUTE);
}

PRUint32
nsDOMAttribute::GetChildCount() const
{
  PRBool hasChild;
  EnsureChildState(PR_FALSE, hasChild);

  return hasChild ? 1 : 0;
}

nsIContent *
nsDOMAttribute::GetChildAt(PRUint32 aIndex) const
{
  
  PRBool hasChild;
  EnsureChildState(PR_TRUE, hasChild);

  return aIndex == 0 && hasChild ? mChild.get() : nsnull;
}
  
PRInt32
nsDOMAttribute::IndexOf(nsINode* aPossibleChild) const
{
  
  
  if (!aPossibleChild || aPossibleChild != mChild) {
    return -1;
  }

  PRBool hasChild;
  EnsureChildState(PR_FALSE, hasChild);
  return hasChild ? 0 : -1;
}

nsresult
nsDOMAttribute::InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                              PRBool aNotify)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsDOMAttribute::AppendChildTo(nsIContent* aKid, PRBool aNotify)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsDOMAttribute::RemoveChildAt(PRUint32 aIndex, PRBool aNotify)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsDOMAttribute::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  
  aVisitor.mCanHandle = PR_FALSE;
  return NS_OK;
}

nsresult
nsDOMAttribute::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  return NS_OK;
}

nsresult
nsDOMAttribute::DispatchDOMEvent(nsEvent* aEvent, nsIDOMEvent* aDOMEvent,
                                 nsPresContext* aPresContext,
                                 nsEventStatus* aEventStatus)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDOMAttribute::GetListenerManager(PRBool aCreateIfNotFound,
                                   nsIEventListenerManager** aResult)
{
  return nsContentUtils::GetListenerManager(this, aCreateIfNotFound, aResult);
}

nsresult
nsDOMAttribute::EnsureChildState(PRBool aSetText, PRBool &aHasChild) const
{
  aHasChild = PR_FALSE;

  nsDOMAttribute* mutableThis = NS_CONST_CAST(nsDOMAttribute*, this);

  nsAutoString value;
  mutableThis->GetValue(value);

  if (!mChild && !value.IsEmpty()) {
    nsresult rv = NS_NewTextNode(getter_AddRefs(mutableThis->mChild),
                                 mNodeInfo->NodeInfoManager());
    NS_ENSURE_SUCCESS(rv, rv);

    
  }

  aHasChild = !value.IsEmpty();

  if (aSetText && aHasChild) {
    mChild->SetText(value, PR_TRUE);
  }

  return NS_OK;
}

void
nsDOMAttribute::Initialize()
{
  sInitialized = PR_TRUE;
}

void
nsDOMAttribute::Shutdown()
{
  sInitialized = PR_FALSE;
}
