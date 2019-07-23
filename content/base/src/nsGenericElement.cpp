











































#include "nsGenericElement.h"

#include "nsDOMAttribute.h"
#include "nsDOMAttributeMap.h"
#include "nsIAtom.h"
#include "nsINodeInfo.h"
#include "nsIDocument.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIDOMText.h"
#include "nsIContentIterator.h"
#include "nsIEventListenerManager.h"
#include "nsFocusManager.h"
#include "nsILinkHandler.h"
#include "nsIScriptGlobalObject.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsStyleConsts.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsIEventStateManager.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsDOMCID.h"
#include "nsIServiceManager.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsCSSDeclaration.h"
#include "nsDOMCSSDeclaration.h"
#include "nsDOMCSSAttrDeclaration.h"
#include "nsINameSpaceManager.h"
#include "nsContentList.h"
#include "nsDOMTokenList.h"
#include "nsDOMError.h"
#include "nsDOMString.h"
#include "nsIScriptSecurityManager.h"
#include "nsIDOMMutationEvent.h"
#include "nsMutationEvent.h"
#include "nsNodeUtils.h"
#include "nsDocument.h"
#ifdef MOZ_XUL
#include "nsXULElement.h"
#endif 
#include "nsFrameManager.h"
#include "nsFrameSelection.h"

#include "nsBindingManager.h"
#include "nsXBLBinding.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMViewCSS.h"
#include "nsIXBLService.h"
#include "nsPIDOMWindow.h"
#include "nsIBoxObject.h"
#include "nsPIBoxObject.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMNSElement.h"
#include "nsClientRect.h"
#ifdef MOZ_SVG
#include "nsSVGUtils.h"
#endif
#include "nsLayoutUtils.h"
#include "nsGkAtoms.h"
#include "nsContentUtils.h"
#include "nsIJSContextStack.h"

#include "nsIServiceManager.h"
#include "nsIDOMEventListener.h"

#include "nsIWebNavigation.h"
#include "nsIBaseWindow.h"

#include "jsapi.h"

#include "nsNodeInfoManager.h"
#include "nsICategoryManager.h"
#include "nsIDOMNSFeatureFactory.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMUserDataHandler.h"
#include "nsGenericHTMLElement.h"
#include "nsIEditor.h"
#include "nsIEditorDocShell.h"
#include "nsEventDispatcher.h"
#include "nsContentCreatorFunctions.h"
#include "nsIControllers.h"
#include "nsLayoutUtils.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIScrollableFrame.h"
#include "nsIScrollableView.h"
#include "nsIScrollableViewProvider.h"
#include "nsXBLInsertionPoint.h"
#include "nsICSSStyleRule.h" 
#include "nsCSSRuleProcessor.h"
#include "nsRuleProcessorData.h"

#ifdef MOZ_XUL
#include "nsIXULDocument.h"
#endif 

#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#include "nsIAccessibleEvent.h"
#endif 

#include "nsCycleCollectionParticipant.h"
#include "nsCCUncollectableMarker.h"

#include "mozAutoDocUpdate.h"

#include "nsICSSParser.h"

#ifdef MOZ_SVG
#include "nsSVGFeatures.h"
#endif 

#ifdef DEBUG_waterson




void
DebugListContentTree(nsIContent* aElement)
{
  aElement->List(stdout, 0);
  printf("\n");
}

#endif

NS_DEFINE_IID(kThisPtrOffsetsSID, NS_THISPTROFFSETS_SID);

PRInt32 nsIContent::sTabFocusModel = eTabFocus_any;
PRBool nsIContent::sTabFocusModelAppliesToXUL = PR_FALSE;
PRUint32 nsMutationGuard::sMutationCount = 0;

nsresult NS_NewContentIterator(nsIContentIterator** aInstancePtrResult);



nsINode::nsSlots::~nsSlots()
{
  if (mChildNodes) {
    mChildNodes->DropReference();
    NS_RELEASE(mChildNodes);
  }

  if (mWeakReference) {
    mWeakReference->NoticeNodeDestruction();
  }
}



nsINode::~nsINode()
{
  NS_ASSERTION(!HasSlots(), "nsNodeUtils::LastRelease was not called?");
}

void*
nsINode::GetProperty(PRUint16 aCategory, nsIAtom *aPropertyName,
                     nsresult *aStatus) const
{
  nsIDocument *doc = GetOwnerDoc();
  if (!doc)
    return nsnull;

  return doc->PropertyTable()->GetProperty(this, aCategory, aPropertyName,
                                           aStatus);
}

nsresult
nsINode::SetProperty(PRUint16 aCategory, nsIAtom *aPropertyName, void *aValue,
                     NSPropertyDtorFunc aDtor, PRBool aTransfer,
                     void **aOldValue)
{
  nsIDocument *doc = GetOwnerDoc();
  if (!doc)
    return NS_ERROR_FAILURE;

  nsresult rv = doc->PropertyTable()->SetProperty(this, aCategory,
                                                  aPropertyName, aValue, aDtor,
                                                  nsnull, aTransfer, aOldValue);
  if (NS_SUCCEEDED(rv)) {
    SetFlags(NODE_HAS_PROPERTIES);
  }

  return rv;
}

nsresult
nsINode::DeleteProperty(PRUint16 aCategory, nsIAtom *aPropertyName)
{
  nsIDocument *doc = GetOwnerDoc();
  if (!doc)
    return nsnull;

  return doc->PropertyTable()->DeleteProperty(this, aCategory, aPropertyName);
}

void*
nsINode::UnsetProperty(PRUint16 aCategory, nsIAtom *aPropertyName,
                       nsresult *aStatus)
{
  nsIDocument *doc = GetOwnerDoc();
  if (!doc)
    return nsnull;

  return doc->PropertyTable()->UnsetProperty(this, aCategory, aPropertyName,
                                             aStatus);
}

nsIEventListenerManager*
nsGenericElement::GetListenerManager(PRBool aCreateIfNotFound)
{
  return nsContentUtils::GetListenerManager(this, aCreateIfNotFound);
}

nsresult
nsGenericElement::AddEventListenerByIID(nsIDOMEventListener *aListener,
                                       const nsIID& aIID)
{
  nsIEventListenerManager* elm = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(elm);
  return elm->AddEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
}

nsresult
nsGenericElement::RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                           const nsIID& aIID)
{
  nsIEventListenerManager* elm = GetListenerManager(PR_FALSE);
  return elm ?
    elm->RemoveEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE) :
    NS_OK;
}

nsresult
nsGenericElement::GetSystemEventGroup(nsIDOMEventGroup** aGroup)
{
  nsIEventListenerManager* elm = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(elm);
  return elm->GetSystemEventGroupLM(aGroup);
}

nsINode::nsSlots*
nsINode::CreateSlots()
{
  return new nsSlots(mFlagsOrSlots);
}

void
nsINode::AddMutationObserver(nsIMutationObserver* aMutationObserver)
{
  nsSlots* slots = GetSlots();
  if (slots) {
    slots->mMutationObservers.AppendElementUnlessExists(aMutationObserver);
  }
}

void
nsINode::RemoveMutationObserver(nsIMutationObserver* aMutationObserver)
{
  nsSlots* slots = GetExistingSlots();
  if (slots) {
    slots->mMutationObservers.RemoveElement(aMutationObserver);
  }
}

PRBool
nsINode::IsEditableInternal() const
{
  if (HasFlag(NODE_IS_EDITABLE)) {
    
    return PR_TRUE;
  }

  nsIDocument *doc = GetCurrentDoc();

  
  return doc && doc->HasFlag(NODE_IS_EDITABLE);
}

static nsIContent* GetEditorRootContent(nsIEditor* aEditor)
{
  nsCOMPtr<nsIDOMElement> rootElement;
  aEditor->GetRootElement(getter_AddRefs(rootElement));
  nsCOMPtr<nsIContent> rootContent(do_QueryInterface(rootElement));
  return rootContent;
}

nsIContent*
nsINode::GetTextEditorRootContent(nsIEditor** aEditor)
{
  if (aEditor)
    *aEditor = nsnull;
  for (nsINode* node = this; node; node = node->GetNodeParent()) {
    if (!node->IsNodeOfType(eELEMENT) ||
        !static_cast<nsIContent*>(node)->IsHTML())
      continue;

    nsCOMPtr<nsIEditor> editor;
    static_cast<nsGenericHTMLElement*>(node)->
        GetEditorInternal(getter_AddRefs(editor));
    if (!editor)
      continue;

    nsIContent* rootContent = GetEditorRootContent(editor);
    if (aEditor)
      editor.swap(*aEditor);
    return rootContent;
  }
  return nsnull;
}

static nsIEditor* GetHTMLEditor(nsPresContext* aPresContext)
{
  nsCOMPtr<nsISupports> container = aPresContext->GetContainer();
  nsCOMPtr<nsIEditorDocShell> editorDocShell(do_QueryInterface(container));
  PRBool isEditable;
  if (!editorDocShell ||
      NS_FAILED(editorDocShell->GetEditable(&isEditable)) || !isEditable)
    return nsnull;

  nsCOMPtr<nsIEditor> editor;
  editorDocShell->GetEditor(getter_AddRefs(editor));
  return editor;
}

nsIContent*
nsINode::GetSelectionRootContent(nsIPresShell* aPresShell)
{
  NS_ENSURE_TRUE(aPresShell, nsnull);

  if (IsNodeOfType(eDOCUMENT))
    return static_cast<nsIDocument*>(this)->GetRootContent();
  if (!IsNodeOfType(eCONTENT))
    return nsnull;

  nsIFrame* frame =
    aPresShell->GetPrimaryFrameFor(static_cast<nsIContent*>(this));
  if (frame && frame->GetStateBits() & NS_FRAME_INDEPENDENT_SELECTION) {
    
    nsIContent* content = GetTextEditorRootContent();
    if (content)
      return content;
  }

  nsPresContext* presContext = aPresShell->GetPresContext();
  if (presContext) {
    nsIEditor* editor = GetHTMLEditor(presContext);
    if (editor) {
      
      nsIDocument* doc = GetCurrentDoc();
      if (!doc || doc->HasFlag(NODE_IS_EDITABLE) || !HasFlag(NODE_IS_EDITABLE))
        return GetEditorRootContent(editor);
      
      
      
      nsIContent* content = static_cast<nsIContent*>(this);
      for (nsIContent* parent = GetParent();
           parent && parent->HasFlag(NODE_IS_EDITABLE);
           parent = content->GetParent())
        content = parent;
      return content;
    }
  }

  nsCOMPtr<nsFrameSelection> fs = aPresShell->FrameSelection();
  nsIContent* content = fs->GetLimiter();
  if (content)
    return content;
  content = fs->GetAncestorLimiter();
  if (content)
    return content;
  nsIDocument* doc = aPresShell->GetDocument();
  NS_ENSURE_TRUE(doc, nsnull);
  return doc->GetRootContent();
}

nsINodeList*
nsINode::GetChildNodesList()
{
  nsSlots *slots = GetSlots();
  if (!slots) {
    return nsnull;
  }

  if (!slots->mChildNodes) {
    slots->mChildNodes = new nsChildContentList(this);
    if (slots->mChildNodes) {
      NS_ADDREF(slots->mChildNodes);
    }
  }

  return slots->mChildNodes;
}

#ifdef DEBUG
void
nsINode::CheckNotNativeAnonymous() const
{
  if (!IsNodeOfType(eCONTENT))
    return;
  nsIContent* content = static_cast<const nsIContent *>(this)->GetBindingParent();
  while (content) {
    if (content->IsRootOfNativeAnonymousSubtree()) {
      NS_ERROR("Element not marked to be in native anonymous subtree!");
      break;
    }
    content = content->GetBindingParent();
  }
}
#endif

nsresult
nsINode::GetParentNode(nsIDOMNode** aParentNode)
{
  *aParentNode = nsnull;

  nsINode *parent = GetNodeParent();

  return parent ? CallQueryInterface(parent, aParentNode) : NS_OK;
}

nsresult
nsINode::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
  *aChildNodes = GetChildNodesList();
  if (!*aChildNodes) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aChildNodes);

  return NS_OK;
}

nsresult
nsINode::GetFirstChild(nsIDOMNode** aNode)
{
  nsIContent* child = GetChildAt(0);
  if (child) {
    return CallQueryInterface(child, aNode);
  }

  *aNode = nsnull;

  return NS_OK;
}

nsresult
nsINode::GetLastChild(nsIDOMNode** aNode)
{
  nsIContent* child = GetLastChild();
  if (child) {
    return CallQueryInterface(child, aNode);
  }

  *aNode = nsnull;

  return NS_OK;
}

nsresult
nsINode::GetPreviousSibling(nsIDOMNode** aPrevSibling)
{
  *aPrevSibling = nsnull;

  nsIContent *sibling = GetSibling(-1);

  return sibling ? CallQueryInterface(sibling, aPrevSibling) : NS_OK;
}

nsresult
nsINode::GetNextSibling(nsIDOMNode** aNextSibling)
{
  *aNextSibling = nsnull;

  nsIContent *sibling = GetSibling(1);

  return sibling ? CallQueryInterface(sibling, aNextSibling) : NS_OK;
}

nsresult
nsINode::GetOwnerDocument(nsIDOMDocument** aOwnerDocument)
{
  *aOwnerDocument = nsnull;

  nsIDocument *ownerDoc = GetOwnerDocument();

  return ownerDoc ? CallQueryInterface(ownerDoc, aOwnerDocument) : NS_OK;
}



PRInt32
nsIContent::IntrinsicState() const
{
  return IsEditable() ? NS_EVENT_STATE_MOZ_READWRITE :
                        NS_EVENT_STATE_MOZ_READONLY;
}

void
nsIContent::UpdateEditableState()
{
  nsIContent *parent = GetParent();

  SetEditableFlag(parent && parent->HasFlag(NODE_IS_EDITABLE));
}

nsIContent*
nsIContent::FindFirstNonNativeAnonymous() const
{
  
  for (const nsIContent *content = this; content;
       content = content->GetBindingParent()) {
    if (!content->IsInNativeAnonymousSubtree()) {
      
      
      return const_cast<nsIContent*>(content);
    }
  }
  return nsnull;
}



NS_IMPL_ADDREF(nsChildContentList)
NS_IMPL_RELEASE(nsChildContentList)

NS_INTERFACE_TABLE_HEAD(nsChildContentList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_NODELIST_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsChildContentList)
    NS_INTERFACE_TABLE_ENTRY(nsChildContentList, nsINodeList)
    NS_INTERFACE_TABLE_ENTRY(nsChildContentList, nsIDOMNodeList)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(NodeList)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsChildContentList::GetLength(PRUint32* aLength)
{
  *aLength = mNode ? mNode->GetChildCount() : 0;

  return NS_OK;
}

NS_IMETHODIMP
nsChildContentList::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  nsINode* node = GetNodeAt(aIndex);
  if (!node) {
    *aReturn = nsnull;

    return NS_OK;
  }

  return CallQueryInterface(node, aReturn);
}

nsIContent*
nsChildContentList::GetNodeAt(PRUint32 aIndex)
{
  if (mNode) {
    return mNode->GetChildAt(aIndex);
  }

  return nsnull;
}

PRInt32
nsChildContentList::IndexOf(nsIContent* aContent)
{
  if (mNode) {
    return mNode->IndexOf(aContent);
  }

  return -1;
}



NS_IMPL_CYCLE_COLLECTION_1(nsNode3Tearoff, mContent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsNode3Tearoff)
  NS_INTERFACE_MAP_ENTRY(nsIDOM3Node)
  NS_INTERFACE_MAP_ENTRY(nsIDOMXPathNSResolver)
NS_INTERFACE_MAP_END_AGGREGATED(mContent)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsNode3Tearoff)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsNode3Tearoff)

NS_IMETHODIMP
nsNode3Tearoff::GetBaseURI(nsAString& aURI)
{
  nsCOMPtr<nsIURI> baseURI = mContent->GetBaseURI();
  nsCAutoString spec;

  if (baseURI) {
    baseURI->GetSpec(spec);
  }

  CopyUTF8toUTF16(spec, aURI);
  
  return NS_OK;
}

NS_IMETHODIMP
nsNode3Tearoff::GetTextContent(nsAString &aTextContent)
{
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mContent));
  NS_ASSERTION(node, "We have an nsIContent which doesn't support nsIDOMNode");

  PRUint16 nodeType;
  node->GetNodeType(&nodeType);
  if (nodeType == nsIDOMNode::DOCUMENT_TYPE_NODE ||
      nodeType == nsIDOMNode::NOTATION_NODE) {
    SetDOMStringToNull(aTextContent);

    return NS_OK;
  }

  if (nodeType == nsIDOMNode::TEXT_NODE ||
      nodeType == nsIDOMNode::CDATA_SECTION_NODE ||
      nodeType == nsIDOMNode::COMMENT_NODE ||
      nodeType == nsIDOMNode::PROCESSING_INSTRUCTION_NODE) {
    return node->GetNodeValue(aTextContent);
  }

  nsContentUtils::GetNodeTextContent(mContent, PR_TRUE, aTextContent);

  return NS_OK;
}

NS_IMETHODIMP
nsNode3Tearoff::SetTextContent(const nsAString &aTextContent)
{
  
  mozAutoSubtreeModified subtree(mContent->GetOwnerDoc(), nsnull);

  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mContent));
  NS_ASSERTION(node, "We have an nsIContent which doesn't support nsIDOMNode");

  PRUint16 nodeType;
  node->GetNodeType(&nodeType);
  if (nodeType == nsIDOMNode::DOCUMENT_TYPE_NODE ||
      nodeType == nsIDOMNode::NOTATION_NODE) {
    return NS_OK;
  }

  if (nodeType == nsIDOMNode::TEXT_NODE ||
      nodeType == nsIDOMNode::CDATA_SECTION_NODE ||
      nodeType == nsIDOMNode::COMMENT_NODE ||
      nodeType == nsIDOMNode::PROCESSING_INSTRUCTION_NODE) {
    return node->SetNodeValue(aTextContent);
  }

  return nsContentUtils::SetNodeTextContent(mContent, aTextContent, PR_FALSE);
}

NS_IMETHODIMP
nsNode3Tearoff::CompareDocumentPosition(nsIDOMNode* aOther,
                                        PRUint16* aReturn)
{
  NS_ENSURE_ARG_POINTER(aOther);

  nsCOMPtr<nsINode> other = do_QueryInterface(aOther);
  NS_ENSURE_TRUE(other, NS_ERROR_DOM_NOT_SUPPORTED_ERR);

  *aReturn = nsContentUtils::ComparePosition(other, mContent);
  return NS_OK;
}

NS_IMETHODIMP
nsNode3Tearoff::IsSameNode(nsIDOMNode* aOther,
                           PRBool* aReturn)
{
  nsCOMPtr<nsIContent> other(do_QueryInterface(aOther));
  *aReturn = mContent == other;

  return NS_OK;
}

PRBool
nsNode3Tearoff::AreNodesEqual(nsIContent* aContent1,
                              nsIContent* aContent2)
{
  

  NS_PRECONDITION(aContent1 && aContent2, "Who called AreNodesEqual?");

  nsAutoString string1, string2;

  
  if (!aContent1->NodeInfo()->Equals(aContent2->NodeInfo())) {
    return PR_FALSE;
  }

  if (aContent1->Tag() == nsGkAtoms::documentTypeNodeName) {
    nsCOMPtr<nsIDOMDocumentType> docType1 = do_QueryInterface(aContent1);
    nsCOMPtr<nsIDOMDocumentType> docType2 = do_QueryInterface(aContent2);

    NS_ASSERTION(docType1 && docType2, "Why don't we have a document type node?");

    
    docType1->GetPublicId(string1);
    docType2->GetPublicId(string2);

    if (!string1.Equals(string2)) {
      return PR_FALSE;
    }

    
    docType1->GetSystemId(string1);
    docType2->GetSystemId(string2);

    if (!string1.Equals(string2)) {
      return PR_FALSE;
    }

    
    docType1->GetInternalSubset(string1);
    docType2->GetInternalSubset(string2);

    if (!string1.Equals(string2)) {
      return PR_FALSE;
    }
  }

  if (aContent1->IsNodeOfType(nsINode::eELEMENT)) {
    
    PRUint32 attrCount = aContent1->GetAttrCount();
    if (attrCount != aContent2->GetAttrCount()) {
      return PR_FALSE;
    }

    
    for (PRUint32 i = 0; i < attrCount; ++i) {
      const nsAttrName* attrName1 = aContent1->GetAttrNameAt(i);
#ifdef DEBUG
      PRBool hasAttr =
#endif
      aContent1->GetAttr(attrName1->NamespaceID(),
                         attrName1->LocalName(),
                         string1);
      NS_ASSERTION(hasAttr, "Why don't we have an attr?");

      if (!aContent2->AttrValueIs(attrName1->NamespaceID(),
                                  attrName1->LocalName(),
                                  string1,
                                  eCaseMatters)) {
        return PR_FALSE;
      }
    }
  } else {
    
    nsCOMPtr<nsIDOMNode> domNode1 = do_QueryInterface(aContent1);
    nsCOMPtr<nsIDOMNode> domNode2 = do_QueryInterface(aContent2);
    NS_ASSERTION(domNode1 && domNode2, "How'd we get nsIContent without nsIDOMNode?");
    domNode1->GetNodeValue(string1);
    domNode2->GetNodeValue(string2);
    if (!string1.Equals(string2)) {
      return PR_FALSE;
    }
  }

  
  PRUint32 childCount = aContent1->GetChildCount();
  if (childCount != aContent2->GetChildCount()) {
    return PR_FALSE;
  }

  
  for (PRUint32 i = 0; i < childCount; ++i) {
    nsIContent* child1 = aContent1->GetChildAt(i);
    nsIContent* child2 = aContent2->GetChildAt(i);
    if (!AreNodesEqual(child1, child2)) {
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsNode3Tearoff::IsEqualNode(nsIDOMNode* aOther, PRBool* aReturn)
{
  NS_ENSURE_ARG_POINTER(aOther);

  *aReturn = PR_FALSE;

  
  nsCOMPtr<nsIContent> aOtherContent = do_QueryInterface(aOther);
  
  if (!aOtherContent) {
    return NS_OK;
  }

  *aReturn = nsNode3Tearoff::AreNodesEqual(mContent, aOtherContent);
  return NS_OK;
}

NS_IMETHODIMP
nsNode3Tearoff::GetFeature(const nsAString& aFeature,
                           const nsAString& aVersion,
                           nsISupports** aReturn)
{
  return nsGenericElement::InternalGetFeature(static_cast<nsIDOM3Node*>(this),
                                              aFeature, aVersion, aReturn);
}

NS_IMETHODIMP
nsNode3Tearoff::SetUserData(const nsAString& aKey,
                            nsIVariant* aData,
                            nsIDOMUserDataHandler* aHandler,
                            nsIVariant** aResult)
{
  return nsNodeUtils::SetUserData(mContent, aKey, aData, aHandler, aResult);
}

NS_IMETHODIMP
nsNode3Tearoff::GetUserData(const nsAString& aKey,
                            nsIVariant** aResult)
{
  return nsNodeUtils::GetUserData(mContent, aKey, aResult);
}

NS_IMETHODIMP
nsNode3Tearoff::LookupPrefix(const nsAString& aNamespaceURI,
                             nsAString& aPrefix)
{
  SetDOMStringToNull(aPrefix);

  

  
  
  
  for (nsIContent* content = mContent; content;
       content = content->GetParent()) {
    PRUint32 attrCount = content->GetAttrCount();

    for (PRUint32 i = 0; i < attrCount; ++i) {
      const nsAttrName* name = content->GetAttrNameAt(i);

      if (name->NamespaceEquals(kNameSpaceID_XMLNS) &&
          content->AttrValueIs(kNameSpaceID_XMLNS, name->LocalName(),
                               aNamespaceURI, eCaseMatters)) {
        
        
        if (name->LocalName() != nsGkAtoms::xmlns) {
          name->LocalName()->ToString(aPrefix);
        }

        return NS_OK;
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNode3Tearoff::LookupNamespaceURI(const nsAString& aNamespacePrefix,
                                   nsAString& aNamespaceURI)
{
  if (NS_FAILED(nsContentUtils::LookupNamespaceURI(mContent,
                                                   aNamespacePrefix,
                                                   aNamespaceURI))) {
    SetDOMStringToNull(aNamespaceURI);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNode3Tearoff::IsDefaultNamespace(const nsAString& aNamespaceURI,
                                   PRBool* aReturn)
{
  nsAutoString defaultNamespace;
  LookupNamespaceURI(EmptyString(), defaultNamespace);
  *aReturn = aNamespaceURI.Equals(defaultNamespace);
  return NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::GetFirstElementChild(nsIDOMElement** aResult)
{
  *aResult = nsnull;

  nsAttrAndChildArray& children = mContent->mAttrsAndChildren;
  PRUint32 i, count = children.ChildCount();
  for (i = 0; i < count; ++i) {
    nsIContent* child = children.ChildAt(i);
    if (child->IsNodeOfType(nsINode::eELEMENT)) {
      return CallQueryInterface(child, aResult);
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::GetLastElementChild(nsIDOMElement** aResult)
{
  *aResult = nsnull;

  nsAttrAndChildArray& children = mContent->mAttrsAndChildren;
  PRUint32 i = children.ChildCount();
  while (i > 0) {
    nsIContent* child = children.ChildAt(--i);
    if (child->IsNodeOfType(nsINode::eELEMENT)) {
      return CallQueryInterface(child, aResult);
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::GetPreviousElementSibling(nsIDOMElement** aResult)
{
  *aResult = nsnull;

  nsIContent* parent = mContent->GetParent();
  if (!parent) {
    return NS_OK;
  }

  NS_ASSERTION(parent->IsNodeOfType(nsINode::eELEMENT) ||
               parent->IsNodeOfType(nsINode::eDOCUMENT_FRAGMENT),
               "Parent content must be an element or a doc fragment");

  nsAttrAndChildArray& children =
    static_cast<nsGenericElement*>(parent)->mAttrsAndChildren;
  PRInt32 index = children.IndexOfChild(mContent);
  if (index < 0) {
    return NS_OK;
  }

  PRUint32 i = index;
  while (i > 0) {
    nsIContent* child = children.ChildAt((PRUint32)--i);
    if (child->IsNodeOfType(nsINode::eELEMENT)) {
      return CallQueryInterface(child, aResult);
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::GetNextElementSibling(nsIDOMElement** aResult)
{
  *aResult = nsnull;

  nsIContent* parent = mContent->GetParent();
  if (!parent) {
    return NS_OK;
  }

  NS_ASSERTION(parent->IsNodeOfType(nsINode::eELEMENT) ||
               parent->IsNodeOfType(nsINode::eDOCUMENT_FRAGMENT),
               "Parent content must be an element or a doc fragment");

  nsAttrAndChildArray& children =
    static_cast<nsGenericElement*>(parent)->mAttrsAndChildren;
  PRInt32 index = children.IndexOfChild(mContent);
  if (index < 0) {
    return NS_OK;
  }

  PRUint32 i, count = children.ChildCount();
  for (i = (PRUint32)index + 1; i < count; ++i) {
    nsIContent* child = children.ChildAt(i);
    if (child->IsNodeOfType(nsINode::eELEMENT)) {
      return CallQueryInterface(child, aResult);
    }
  }
  
  return NS_OK;
}

nsContentList*
nsNSElementTearoff::GetChildrenList()
{
  nsGenericElement::nsDOMSlots *slots = mContent->GetDOMSlots();
  NS_ENSURE_TRUE(slots, nsnull);

  if (!slots->mChildrenList) {
    slots->mChildrenList = new nsContentList(mContent, nsGkAtoms::_asterix,
                                             kNameSpaceID_Wildcard, PR_FALSE);
  }

  return slots->mChildrenList;
}

NS_IMETHODIMP
nsNSElementTearoff::GetChildElementCount(PRUint32* aResult)
{
  *aResult = 0;

  nsContentList* list = GetChildrenList();
  NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

  *aResult = list->Length(PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::GetChildren(nsIDOMNodeList** aResult)
{
  *aResult = nsnull;

  nsContentList* list = GetChildrenList();
  NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aResult = list);

  return NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::GetClassList(nsIDOMDOMTokenList** aResult)
{
  nsGenericElement::nsDOMSlots *slots = mContent->GetDOMSlots();
  NS_ENSURE_TRUE(slots, nsnull);

  if (!slots->mClassList) {
    nsCOMPtr<nsIAtom> classAttr = mContent->GetClassAttributeName();
    NS_ENSURE_TRUE(classAttr, NS_OK);
    slots->mClassList = new nsDOMTokenList(mContent, classAttr);
    NS_ENSURE_TRUE(slots->mClassList, NS_ERROR_OUT_OF_MEMORY);
  }

  NS_ADDREF(*aResult = slots->mClassList);

  return NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::SetCapture(PRBool aRetargetToElement)
{
  
  
  
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(nsIPresShell::GetCapturingContent());
  if (node)
    return NS_OK;

  nsIPresShell::SetCapturingContent(mContent, aRetargetToElement ? CAPTURE_RETARGETTOELEMENT : 0);
  return NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::ReleaseCapture()
{
  if (nsIPresShell::GetCapturingContent() == mContent) {
    nsIPresShell::SetCapturingContent(nsnull, 0);
  }
  return NS_OK;
}




NS_IMPL_CYCLE_COLLECTION_1(nsNSElementTearoff, mContent)

NS_INTERFACE_MAP_BEGIN(nsNSElementTearoff)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSElement)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsNSElementTearoff)
NS_INTERFACE_MAP_END_AGGREGATED(mContent)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsNSElementTearoff)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsNSElementTearoff)

NS_IMETHODIMP
nsNSElementTearoff::GetElementsByClassName(const nsAString& aClasses,
                                           nsIDOMNodeList** aReturn)
{
  return nsDocument::GetElementsByClassNameHelper(mContent, aClasses, aReturn);
}

nsIFrame*
nsGenericElement::GetStyledFrame()
{
  nsIFrame *frame = GetPrimaryFrame(Flush_Layout);
  return frame ? nsLayoutUtils::GetStyleFrame(frame) : nsnull;
}

void
nsGenericElement::GetOffsetRect(nsRect& aRect, nsIContent** aOffsetParent)
{
  *aOffsetParent = nsnull;
  aRect = nsRect();

  nsIFrame* frame = GetStyledFrame();
  if (!frame) {
    return;
  }

  nsPoint origin = frame->GetPosition();
  aRect.x = nsPresContext::AppUnitsToIntCSSPixels(origin.x);
  aRect.y = nsPresContext::AppUnitsToIntCSSPixels(origin.y);

  
  
  
  
  nsIFrame* parent = frame->GetParent() ? frame->GetParent() : frame;
  nsRect rcFrame = nsLayoutUtils::GetAllInFlowRectsUnion(frame, parent);
  aRect.width = nsPresContext::AppUnitsToIntCSSPixels(rcFrame.width);
  aRect.height = nsPresContext::AppUnitsToIntCSSPixels(rcFrame.height);
}

void
nsNSElementTearoff::GetScrollInfo(nsIScrollableView **aScrollableView,
                                  nsIFrame **aFrame)
{
  *aScrollableView = nsnull;

  
  if (mContent->IsSVG()) {
    if (aFrame)
      *aFrame = nsnull;
    return;
  }

  nsIFrame* frame =
    (static_cast<nsGenericElement*>(mContent))->GetStyledFrame();

  if (aFrame) {
    *aFrame = frame;
  }
  if (!frame) {
    return;
  }

  
  nsIScrollableFrame *scrollFrame = do_QueryFrame(frame);
  if (!scrollFrame) {
    nsIScrollableViewProvider *scrollProvider = do_QueryFrame(frame);
    
    
    if (scrollProvider && frame->GetType() != nsGkAtoms::menuFrame) {
      *aScrollableView = scrollProvider->GetScrollableView();
      if (*aScrollableView) {
        return;
      }
    }

    nsIDocument* doc = mContent->GetCurrentDoc();
    PRBool quirksMode = doc &&
                        doc->GetCompatibilityMode() == eCompatibility_NavQuirks;
    if ((quirksMode && mContent->NodeInfo()->Equals(nsGkAtoms::body)) ||
        (!quirksMode && mContent->NodeInfo()->Equals(nsGkAtoms::html))) {
      
      
      
      
      
      

      do {
        frame = frame->GetParent();

        if (!frame) {
          break;
        }

        scrollFrame = do_QueryFrame(frame);
      } while (!scrollFrame);
    }

    if (!scrollFrame) {
      return;
    }
  }

  
  *aScrollableView = scrollFrame->GetScrollableView();
}

nsresult
nsNSElementTearoff::GetScrollTop(PRInt32* aScrollTop)
{
  NS_ENSURE_ARG_POINTER(aScrollTop);
  *aScrollTop = 0;

  nsIScrollableView *view;
  nsresult rv = NS_OK;

  GetScrollInfo(&view);

  if (view) {
    nscoord xPos, yPos;
    rv = view->GetScrollPosition(xPos, yPos);

    *aScrollTop = nsPresContext::AppUnitsToIntCSSPixels(yPos);
  }

  return rv;
}

nsresult
nsNSElementTearoff::SetScrollTop(PRInt32 aScrollTop)
{
  nsIScrollableView *view;
  nsresult rv = NS_OK;

  GetScrollInfo(&view);

  if (view) {
    nscoord xPos, yPos;

    rv = view->GetScrollPosition(xPos, yPos);

    if (NS_SUCCEEDED(rv)) {
      rv = view->ScrollTo(xPos, nsPresContext::CSSPixelsToAppUnits(aScrollTop),
                          0);
    }
  }

  return rv;
}

nsresult
nsNSElementTearoff::GetScrollLeft(PRInt32* aScrollLeft)
{
  NS_ENSURE_ARG_POINTER(aScrollLeft);
  *aScrollLeft = 0;

  nsIScrollableView *view;
  nsresult rv = NS_OK;

  GetScrollInfo(&view);

  if (view) {
    nscoord xPos, yPos;
    rv = view->GetScrollPosition(xPos, yPos);

    *aScrollLeft = nsPresContext::AppUnitsToIntCSSPixels(xPos);
  }

  return rv;
}

nsresult
nsNSElementTearoff::SetScrollLeft(PRInt32 aScrollLeft)
{
  nsIScrollableView *view;
  nsresult rv = NS_OK;

  GetScrollInfo(&view);

  if (view) {
    nscoord xPos, yPos;
    rv = view->GetScrollPosition(xPos, yPos);

    if (NS_SUCCEEDED(rv)) {
      rv = view->ScrollTo(nsPresContext::CSSPixelsToAppUnits(aScrollLeft),
                          yPos, 0);
    }
  }

  return rv;
}

nsresult
nsNSElementTearoff::GetScrollHeight(PRInt32* aScrollHeight)
{
  NS_ENSURE_ARG_POINTER(aScrollHeight);
  *aScrollHeight = 0;

  if (mContent->IsSVG())
    return NS_OK;

  nsIScrollableView *scrollView;
  nsresult rv = NS_OK;

  GetScrollInfo(&scrollView);

  if (!scrollView) {
    nsRect rcFrame;
    nsCOMPtr<nsIContent> parent;
    (static_cast<nsGenericElement *>(mContent))->GetOffsetRect(rcFrame, getter_AddRefs(parent));
    *aScrollHeight = rcFrame.height;
    return NS_OK;
  }

  
  nscoord xMax, yMax;
  rv = scrollView->GetContainerSize(&xMax, &yMax);

  *aScrollHeight = nsPresContext::AppUnitsToIntCSSPixels(yMax);

  return rv;
}

nsresult
nsNSElementTearoff::GetScrollWidth(PRInt32* aScrollWidth)
{
  NS_ENSURE_ARG_POINTER(aScrollWidth);
  *aScrollWidth = 0;

  if (mContent->IsSVG())
    return NS_OK;

  nsIScrollableView *scrollView;
  nsresult rv = NS_OK;

  GetScrollInfo(&scrollView);

  if (!scrollView) {
    nsRect rcFrame;
    nsCOMPtr<nsIContent> parent;
    (static_cast<nsGenericElement *>(mContent))->GetOffsetRect(rcFrame, getter_AddRefs(parent));
    *aScrollWidth = rcFrame.width;
    return NS_OK;
  }

  nscoord xMax, yMax;
  rv = scrollView->GetContainerSize(&xMax, &yMax);

  *aScrollWidth = nsPresContext::AppUnitsToIntCSSPixels(xMax);

  return rv;
}

nsRect
nsNSElementTearoff::GetClientAreaRect()
{
  nsIScrollableView *scrollView;
  nsIFrame *frame;

  
  if (mContent->IsSVG())
    return nsRect(0, 0, 0, 0);

  GetScrollInfo(&scrollView, &frame);

  if (scrollView) {
    return scrollView->View()->GetBounds();
  }

  if (frame &&
      (frame->GetStyleDisplay()->mDisplay != NS_STYLE_DISPLAY_INLINE ||
       frame->IsFrameOfType(nsIFrame::eReplaced))) {
    
    
    return frame->GetPaddingRect() - frame->GetPositionIgnoringScrolling();
  }

  return nsRect(0, 0, 0, 0);
}

nsresult
nsNSElementTearoff::GetClientTop(PRInt32* aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);
  *aLength = nsPresContext::AppUnitsToIntCSSPixels(GetClientAreaRect().y);
  return NS_OK;
}

nsresult
nsNSElementTearoff::GetClientLeft(PRInt32* aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);
  *aLength = nsPresContext::AppUnitsToIntCSSPixels(GetClientAreaRect().x);
  return NS_OK;
}

nsresult
nsNSElementTearoff::GetClientHeight(PRInt32* aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);
  *aLength = nsPresContext::AppUnitsToIntCSSPixels(GetClientAreaRect().height);
  return NS_OK;
}

nsresult
nsNSElementTearoff::GetClientWidth(PRInt32* aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);
  *aLength = nsPresContext::AppUnitsToIntCSSPixels(GetClientAreaRect().width);
  return NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::GetBoundingClientRect(nsIDOMClientRect** aResult)
{
  
  nsClientRect* rect = new nsClientRect();
  if (!rect)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult = rect);
  
  nsIFrame* frame = mContent->GetPrimaryFrame(Flush_Layout);
  if (!frame) {
    
    return NS_OK;
  }

  nsRect r = nsLayoutUtils::GetAllInFlowRectsUnion(frame,
          nsLayoutUtils::GetContainingBlockForClientRect(frame));
  rect->SetLayoutRect(r);
  return NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::GetClientRects(nsIDOMClientRectList** aResult)
{
  *aResult = nsnull;

  nsRefPtr<nsClientRectList> rectList = new nsClientRectList();
  if (!rectList)
    return NS_ERROR_OUT_OF_MEMORY;

  nsIFrame* frame = mContent->GetPrimaryFrame(Flush_Layout);
  if (!frame) {
    
    *aResult = rectList.forget().get();
    return NS_OK;
  }

  nsLayoutUtils::RectListBuilder builder(rectList);
  nsLayoutUtils::GetAllInFlowRects(frame,
          nsLayoutUtils::GetContainingBlockForClientRect(frame), &builder);
  if (NS_FAILED(builder.mRV))
    return builder.mRV;
  *aResult = rectList.forget().get();
  return NS_OK;
}




NS_IMPL_ISUPPORTS1(nsNodeWeakReference,
                   nsIWeakReference)

nsNodeWeakReference::~nsNodeWeakReference()
{
  if (mNode) {
    NS_ASSERTION(mNode->GetSlots() &&
                 mNode->GetSlots()->mWeakReference == this,
                 "Weak reference has wrong value");
    mNode->GetSlots()->mWeakReference = nsnull;
  }
}

NS_IMETHODIMP
nsNodeWeakReference::QueryReferent(const nsIID& aIID, void** aInstancePtr)
{
  return mNode ? mNode->QueryInterface(aIID, aInstancePtr) :
                 NS_ERROR_NULL_POINTER;
}


NS_IMPL_CYCLE_COLLECTION_1(nsNodeSupportsWeakRefTearoff, mNode)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsNodeSupportsWeakRefTearoff)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END_AGGREGATED(mNode)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsNodeSupportsWeakRefTearoff)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsNodeSupportsWeakRefTearoff)

NS_IMETHODIMP
nsNodeSupportsWeakRefTearoff::GetWeakReference(nsIWeakReference** aInstancePtr)
{
  nsINode::nsSlots* slots = mNode->GetSlots();
  NS_ENSURE_TRUE(slots, NS_ERROR_OUT_OF_MEMORY);

  if (!slots->mWeakReference) {
    slots->mWeakReference = new nsNodeWeakReference(mNode);
    NS_ENSURE_TRUE(slots->mWeakReference, NS_ERROR_OUT_OF_MEMORY);
  }

  NS_ADDREF(*aInstancePtr = slots->mWeakReference);

  return NS_OK;
}



nsDOMEventRTTearoff *
nsDOMEventRTTearoff::mCachedEventTearoff[NS_EVENT_TEAROFF_CACHE_SIZE];

PRUint32 nsDOMEventRTTearoff::mCachedEventTearoffCount = 0;


nsDOMEventRTTearoff::nsDOMEventRTTearoff(nsINode *aNode)
  : mNode(aNode)
{
}

nsDOMEventRTTearoff::~nsDOMEventRTTearoff()
{
}

NS_IMPL_CYCLE_COLLECTION_1(nsDOMEventRTTearoff, mNode)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMEventRTTearoff)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIDOM3EventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSEventTarget)
NS_INTERFACE_MAP_END_AGGREGATED(mNode)

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsDOMEventRTTearoff,
                                          nsIDOMEventTarget)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS_WITH_DESTROY(nsDOMEventRTTearoff,
                                                        nsIDOMEventTarget,
                                                        LastRelease())

nsDOMEventRTTearoff *
nsDOMEventRTTearoff::Create(nsINode *aNode)
{
  if (mCachedEventTearoffCount) {
    
    
    nsDOMEventRTTearoff *tearoff =
      mCachedEventTearoff[--mCachedEventTearoffCount];

    
    tearoff->mNode = aNode;

    return tearoff;
  }

  
  return new nsDOMEventRTTearoff(aNode);
}


void
nsDOMEventRTTearoff::Shutdown()
{
  
  while (mCachedEventTearoffCount) {
    delete mCachedEventTearoff[--mCachedEventTearoffCount];
  }
}

void
nsDOMEventRTTearoff::LastRelease()
{
  if (mCachedEventTearoffCount < NS_EVENT_TEAROFF_CACHE_SIZE) {
    
    
    mCachedEventTearoff[mCachedEventTearoffCount++] = this;

    
    
    
    
    nsCOMPtr<nsINode> kungFuDeathGrip;
    kungFuDeathGrip.swap(mNode);

    
    
    
    mRefCnt = 0;

    return;
  }

  delete this;
}

nsresult
nsDOMEventRTTearoff::GetDOM3EventTarget(nsIDOM3EventTarget **aTarget)
{
  nsIEventListenerManager* listener_manager =
    mNode->GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(listener_manager);
  return CallQueryInterface(listener_manager, aTarget);
}

NS_IMETHODIMP
nsDOMEventRTTearoff::GetScriptTypeID(PRUint32 *aLang)
{
  *aLang = mNode->GetScriptTypeID();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMEventRTTearoff::SetScriptTypeID(PRUint32 aLang)
{
  return mNode->SetScriptTypeID(aLang);
}



NS_IMETHODIMP
nsDOMEventRTTearoff::AddEventListener(const nsAString& aType,
                                      nsIDOMEventListener *aListener,
                                      PRBool useCapture)
{
  return
    AddEventListener(aType, aListener, useCapture,
                     !nsContentUtils::IsChromeDoc(mNode->GetOwnerDoc()));
}

NS_IMETHODIMP
nsDOMEventRTTearoff::RemoveEventListener(const nsAString& aType,
                                         nsIDOMEventListener* aListener,
                                         PRBool aUseCapture)
{
  return RemoveGroupedEventListener(aType, aListener, aUseCapture, nsnull);
}

NS_IMETHODIMP
nsDOMEventRTTearoff::DispatchEvent(nsIDOMEvent *aEvt, PRBool* _retval)
{
  nsCOMPtr<nsIDOMEventTarget> target =
    do_QueryInterface(mNode->GetListenerManager(PR_TRUE));
  NS_ENSURE_STATE(target);
  return target->DispatchEvent(aEvt, _retval);
}


NS_IMETHODIMP
nsDOMEventRTTearoff::AddGroupedEventListener(const nsAString& aType,
                                             nsIDOMEventListener *aListener,
                                             PRBool aUseCapture,
                                             nsIDOMEventGroup *aEvtGrp)
{
  nsCOMPtr<nsIDOM3EventTarget> event_target;
  nsresult rv = GetDOM3EventTarget(getter_AddRefs(event_target));
  NS_ENSURE_SUCCESS(rv, rv);

  return event_target->AddGroupedEventListener(aType, aListener, aUseCapture,
                                               aEvtGrp);
}

NS_IMETHODIMP
nsDOMEventRTTearoff::RemoveGroupedEventListener(const nsAString& aType,
                                                nsIDOMEventListener *aListener,
                                                PRBool aUseCapture,
                                                nsIDOMEventGroup *aEvtGrp)
{
  nsCOMPtr<nsIDOM3EventTarget> event_target;
  nsresult rv = GetDOM3EventTarget(getter_AddRefs(event_target));
  NS_ENSURE_SUCCESS(rv, rv);

  return event_target->RemoveGroupedEventListener(aType, aListener,
                                                  aUseCapture, aEvtGrp);
}

NS_IMETHODIMP
nsDOMEventRTTearoff::CanTrigger(const nsAString & type, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDOMEventRTTearoff::IsRegisteredHere(const nsAString & type, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDOMEventRTTearoff::AddEventListener(const nsAString& aType,
                                      nsIDOMEventListener *aListener,
                                      PRBool aUseCapture,
                                      PRBool aWantsUntrusted)
{
  nsIEventListenerManager* listener_manager =
    mNode->GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(listener_manager);

  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

  if (aWantsUntrusted) {
    flags |= NS_PRIV_EVENT_UNTRUSTED_PERMITTED;
  }

  return listener_manager->AddEventListenerByType(aListener, aType, flags,
                                                  nsnull);
}



NS_IMPL_CYCLE_COLLECTION_1(nsNodeSelectorTearoff, mContent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsNodeSelectorTearoff)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNodeSelector)
NS_INTERFACE_MAP_END_AGGREGATED(mContent)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsNodeSelectorTearoff)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsNodeSelectorTearoff)

NS_IMETHODIMP
nsNodeSelectorTearoff::QuerySelector(const nsAString& aSelector,
                                     nsIDOMElement **aReturn)
{
  return nsGenericElement::doQuerySelector(mContent, aSelector, aReturn);
}

NS_IMETHODIMP
nsNodeSelectorTearoff::QuerySelectorAll(const nsAString& aSelector,
                                        nsIDOMNodeList **aReturn)
{
  return nsGenericElement::doQuerySelectorAll(mContent, aSelector, aReturn);
}


nsGenericElement::nsDOMSlots::nsDOMSlots(PtrBits aFlags)
  : nsINode::nsSlots(aFlags),
    mBindingParent(nsnull)
{
}

nsGenericElement::nsDOMSlots::~nsDOMSlots()
{
  if (mAttributeMap) {
    mAttributeMap->DropReference();
  }

  if (mClassList) {
    mClassList->DropReference();
  }
}

nsGenericElement::nsGenericElement(nsINodeInfo *aNodeInfo)
  : nsIContent(aNodeInfo)
{
  
  
  SetFlags(nsIProgrammingLanguage::JAVASCRIPT << NODE_SCRIPT_TYPE_OFFSET);
}

nsGenericElement::~nsGenericElement()
{
  NS_PRECONDITION(!IsInDoc(),
                  "Please remove this from the document properly");
}

NS_IMETHODIMP
nsGenericElement::GetNodeName(nsAString& aNodeName)
{
  mNodeInfo->GetQualifiedName(aNodeName);
  return NS_OK;
}

NS_IMETHODIMP
nsGenericElement::GetLocalName(nsAString& aLocalName)
{
  mNodeInfo->GetLocalName(aLocalName);
  return NS_OK;
}

NS_IMETHODIMP
nsGenericElement::GetNodeValue(nsAString& aNodeValue)
{
  SetDOMStringToNull(aNodeValue);

  return NS_OK;
}

NS_IMETHODIMP
nsGenericElement::SetNodeValue(const nsAString& aNodeValue)
{
  
  
  return NS_OK;
}

NS_IMETHODIMP
nsGenericElement::GetNodeType(PRUint16* aNodeType)
{
  *aNodeType = (PRUint16)nsIDOMNode::ELEMENT_NODE;
  return NS_OK;
}

NS_IMETHODIMP
nsGenericElement::GetNamespaceURI(nsAString& aNamespaceURI)
{
  return mNodeInfo->GetNamespaceURI(aNamespaceURI);
}

NS_IMETHODIMP
nsGenericElement::GetPrefix(nsAString& aPrefix)
{
  mNodeInfo->GetPrefix(aPrefix);
  return NS_OK;
}

NS_IMETHODIMP
nsGenericElement::SetPrefix(const nsAString& aPrefix)
{
  

  nsCOMPtr<nsIAtom> prefix;

  if (!aPrefix.IsEmpty()) {
    prefix = do_GetAtom(aPrefix);
    NS_ENSURE_TRUE(prefix, NS_ERROR_OUT_OF_MEMORY);
  }

  if (!nsContentUtils::IsValidNodeName(mNodeInfo->NameAtom(), prefix,
                                       mNodeInfo->NamespaceID())) {
    return NS_ERROR_DOM_NAMESPACE_ERR;
  }

  nsCOMPtr<nsINodeInfo> newNodeInfo;
  nsresult rv = nsContentUtils::PrefixChanged(mNodeInfo, prefix,
                                              getter_AddRefs(newNodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  mNodeInfo = newNodeInfo;

  return NS_OK;
}

nsresult
nsGenericElement::InternalIsSupported(nsISupports* aObject,
                                      const nsAString& aFeature,
                                      const nsAString& aVersion,
                                      PRBool* aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = PR_FALSE;

  
  
  NS_ConvertUTF16toUTF8 feature(aFeature);
  NS_ConvertUTF16toUTF8 version(aVersion);

  const char *f = feature.get();
  const char *v = version.get();

  if (PL_strcasecmp(f, "XML") == 0 ||
      PL_strcasecmp(f, "HTML") == 0) {
    if (aVersion.IsEmpty() ||
        PL_strcmp(v, "1.0") == 0 ||
        PL_strcmp(v, "2.0") == 0) {
      *aReturn = PR_TRUE;
    }
  } else if (PL_strcasecmp(f, "Views") == 0 ||
             PL_strcasecmp(f, "StyleSheets") == 0 ||
             PL_strcasecmp(f, "Core") == 0 ||
             PL_strcasecmp(f, "CSS") == 0 ||
             PL_strcasecmp(f, "CSS2") == 0 ||
             PL_strcasecmp(f, "Events") == 0 ||
             PL_strcasecmp(f, "UIEvents") == 0 ||
             PL_strcasecmp(f, "MouseEvents") == 0 ||
             
             PL_strcasecmp(f, "MouseScrollEvents") == 0 ||
             PL_strcasecmp(f, "HTMLEvents") == 0 ||
             PL_strcasecmp(f, "Range") == 0 ||
             PL_strcasecmp(f, "XHTML") == 0) {
    if (aVersion.IsEmpty() ||
        PL_strcmp(v, "2.0") == 0) {
      *aReturn = PR_TRUE;
    }
  } else if (PL_strcasecmp(f, "XPath") == 0) {
    if (aVersion.IsEmpty() ||
        PL_strcmp(v, "3.0") == 0) {
      *aReturn = PR_TRUE;
    }
  }
#ifdef MOZ_SVG
  else if (PL_strcasecmp(f, "SVGEvents") == 0 ||
           PL_strcasecmp(f, "SVGZoomEvents") == 0 ||
           nsSVGFeatures::HaveFeature(aFeature)) {
    if (aVersion.IsEmpty() ||
        PL_strcmp(v, "1.0") == 0 ||
        PL_strcmp(v, "1.1") == 0) {
      *aReturn = PR_TRUE;
    }
  }
#endif 
#ifdef MOZ_SMIL
  else if (NS_SMILEnabled() && PL_strcasecmp(f, "TimeControl") == 0) {
    if (aVersion.IsEmpty() || PL_strcmp(v, "1.0") == 0) {
      *aReturn = PR_TRUE;
    }
  }
#endif 
  else {
    nsCOMPtr<nsIDOMNSFeatureFactory> factory =
      GetDOMFeatureFactory(aFeature, aVersion);

    if (factory) {
      factory->HasFeature(aObject, aFeature, aVersion, aReturn);
    }
  }
  return NS_OK;
}

nsresult
nsGenericElement::InternalGetFeature(nsISupports* aObject,
                                    const nsAString& aFeature,
                                    const nsAString& aVersion,
                                    nsISupports** aReturn)
{
  *aReturn = nsnull;
  nsCOMPtr<nsIDOMNSFeatureFactory> factory =
    GetDOMFeatureFactory(aFeature, aVersion);

  if (factory) {
    factory->GetFeature(aObject, aFeature, aVersion, aReturn);
  }

  return NS_OK;
}

already_AddRefed<nsIDOMNSFeatureFactory>
nsGenericElement::GetDOMFeatureFactory(const nsAString& aFeature,
                                       const nsAString& aVersion)
{
  nsIDOMNSFeatureFactory *factory = nsnull;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
  if (categoryManager) {
    nsCAutoString featureCategory(NS_DOMNS_FEATURE_PREFIX);
    AppendUTF16toUTF8(aFeature, featureCategory);
    nsXPIDLCString contractID;
    nsresult rv = categoryManager->GetCategoryEntry(featureCategory.get(),
                                                    NS_ConvertUTF16toUTF8(aVersion).get(),
                                                    getter_Copies(contractID));
    if (NS_SUCCEEDED(rv)) {
      CallGetService(contractID.get(), &factory);  
    }
  }
  return factory;
}

NS_IMETHODIMP
nsGenericElement::IsSupported(const nsAString& aFeature,
                              const nsAString& aVersion,
                              PRBool* aReturn)
{
  return InternalIsSupported(this, aFeature, aVersion, aReturn);
}

NS_IMETHODIMP
nsGenericElement::HasAttributes(PRBool* aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  *aReturn = GetAttrCount() > 0;

  return NS_OK;
}

NS_IMETHODIMP
nsGenericElement::GetAttributes(nsIDOMNamedNodeMap** aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);
  nsDOMSlots *slots = GetDOMSlots();

  if (!slots) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!slots->mAttributeMap) {
    slots->mAttributeMap = new nsDOMAttributeMap(this);
    if (!slots->mAttributeMap) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    if (!slots->mAttributeMap->Init()) {
      slots->mAttributeMap = nsnull;
      return NS_ERROR_FAILURE;
    }
  }

  NS_ADDREF(*aAttributes = slots->mAttributeMap);

  return NS_OK;
}

nsresult
nsGenericElement::HasChildNodes(PRBool* aReturn)
{
  *aReturn = mAttrsAndChildren.ChildCount() > 0;

  return NS_OK;
}

NS_IMETHODIMP
nsGenericElement::GetTagName(nsAString& aTagName)
{
  mNodeInfo->GetQualifiedName(aTagName);
  return NS_OK;
}

nsresult
nsGenericElement::GetAttribute(const nsAString& aName,
                               nsAString& aReturn)
{
  const nsAttrName* name = InternalGetExistingAttrNameFromQName(aName);

  if (!name) {
    if (mNodeInfo->NamespaceID() == kNameSpaceID_XUL) {
      
      
      aReturn.Truncate();
    }
    else {
      SetDOMStringToNull(aReturn);
    }

    return NS_OK;
  }

  GetAttr(name->NamespaceID(), name->LocalName(), aReturn);

  return NS_OK;
}

nsresult
nsGenericElement::SetAttribute(const nsAString& aName,
                               const nsAString& aValue)
{
  const nsAttrName* name = InternalGetExistingAttrNameFromQName(aName);

  if (!name) {
    nsresult rv = nsContentUtils::CheckQName(aName, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(aName);
    NS_ENSURE_TRUE(nameAtom, NS_ERROR_OUT_OF_MEMORY);

    return SetAttr(kNameSpaceID_None, nameAtom, aValue, PR_TRUE);
  }

  return SetAttr(name->NamespaceID(), name->LocalName(), name->GetPrefix(),
                 aValue, PR_TRUE);
}

nsresult
nsGenericElement::RemoveAttribute(const nsAString& aName)
{
  const nsAttrName* name = InternalGetExistingAttrNameFromQName(aName);

  if (!name) {
    return NS_OK;
  }

  
  
  
  nsAttrName tmp(*name);

  return UnsetAttr(name->NamespaceID(), name->LocalName(), PR_TRUE);
}

nsresult
nsGenericElement::GetAttributeNode(const nsAString& aName,
                                   nsIDOMAttr** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsCOMPtr<nsIDOMNamedNodeMap> map;
  nsresult rv = GetAttributes(getter_AddRefs(map));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> node;
  rv = map->GetNamedItem(aName, getter_AddRefs(node));

  if (NS_SUCCEEDED(rv) && node) {
    rv = CallQueryInterface(node, aReturn);
  }

  return rv;
}

nsresult
nsGenericElement::SetAttributeNode(nsIDOMAttr* aAttribute,
                                   nsIDOMAttr** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  NS_ENSURE_ARG_POINTER(aAttribute);

  *aReturn = nsnull;

  nsCOMPtr<nsIDOMNamedNodeMap> map;
  nsresult rv = GetAttributes(getter_AddRefs(map));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> returnNode;
  rv = map->SetNamedItem(aAttribute, getter_AddRefs(returnNode));
  NS_ENSURE_SUCCESS(rv, rv);

  if (returnNode) {
    rv = CallQueryInterface(returnNode, aReturn);
  }

  return rv;
}

nsresult
nsGenericElement::RemoveAttributeNode(nsIDOMAttr* aAttribute,
                                      nsIDOMAttr** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  NS_ENSURE_ARG_POINTER(aAttribute);

  *aReturn = nsnull;

  nsCOMPtr<nsIDOMNamedNodeMap> map;
  nsresult rv = GetAttributes(getter_AddRefs(map));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString name;

  rv = aAttribute->GetName(name);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIDOMNode> node;
    rv = map->RemoveNamedItem(name, getter_AddRefs(node));

    if (NS_SUCCEEDED(rv) && node) {
      rv = CallQueryInterface(node, aReturn);
    }
  }

  return rv;
}

nsresult
nsGenericElement::GetElementsByTagName(const nsAString& aTagname,
                                       nsIDOMNodeList** aReturn)
{
  nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(aTagname);
  NS_ENSURE_TRUE(nameAtom, NS_ERROR_OUT_OF_MEMORY);

  nsContentList *list = NS_GetContentList(this, nameAtom,
                                          kNameSpaceID_Unknown).get();
  NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

  
  *aReturn = list;
  return NS_OK;
}

nsresult
nsGenericElement::GetAttributeNS(const nsAString& aNamespaceURI,
                                 const nsAString& aLocalName,
                                 nsAString& aReturn)
{
  PRInt32 nsid =
    nsContentUtils::NameSpaceManager()->GetNameSpaceID(aNamespaceURI);

  if (nsid == kNameSpaceID_Unknown) {
    

    aReturn.Truncate();

    return NS_OK;
  }

  nsCOMPtr<nsIAtom> name = do_GetAtom(aLocalName);
  GetAttr(nsid, name, aReturn);

  return NS_OK;
}

nsresult
nsGenericElement::SetAttributeNS(const nsAString& aNamespaceURI,
                                 const nsAString& aQualifiedName,
                                 const nsAString& aValue)
{
  nsCOMPtr<nsINodeInfo> ni;
  nsresult rv =
    nsContentUtils::GetNodeInfoFromQName(aNamespaceURI, aQualifiedName,
                                         mNodeInfo->NodeInfoManager(),
                                         getter_AddRefs(ni));
  NS_ENSURE_SUCCESS(rv, rv);

  return SetAttr(ni->NamespaceID(), ni->NameAtom(), ni->GetPrefixAtom(),
                 aValue, PR_TRUE);
}

nsresult
nsGenericElement::RemoveAttributeNS(const nsAString& aNamespaceURI,
                                    const nsAString& aLocalName)
{
  nsCOMPtr<nsIAtom> name = do_GetAtom(aLocalName);
  PRInt32 nsid =
    nsContentUtils::NameSpaceManager()->GetNameSpaceID(aNamespaceURI);

  if (nsid == kNameSpaceID_Unknown) {
    

    return NS_OK;
  }

  nsAutoString tmp;
  UnsetAttr(nsid, name, PR_TRUE);

  return NS_OK;
}

nsresult
nsGenericElement::GetAttributeNodeNS(const nsAString& aNamespaceURI,
                                     const nsAString& aLocalName,
                                     nsIDOMAttr** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  *aReturn = nsnull;

  nsCOMPtr<nsIDOMNamedNodeMap> map;
  nsresult rv = GetAttributes(getter_AddRefs(map));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> node;
  rv = map->GetNamedItemNS(aNamespaceURI, aLocalName, getter_AddRefs(node));

  if (NS_SUCCEEDED(rv) && node) {
    rv = CallQueryInterface(node, aReturn);
  }

  return rv;
}

nsresult
nsGenericElement::SetAttributeNodeNS(nsIDOMAttr* aNewAttr,
                                     nsIDOMAttr** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  NS_ENSURE_ARG_POINTER(aNewAttr);

  *aReturn = nsnull;

  nsCOMPtr<nsIDOMNamedNodeMap> map;
  nsresult rv = GetAttributes(getter_AddRefs(map));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> returnNode;
  rv = map->SetNamedItemNS(aNewAttr, getter_AddRefs(returnNode));
  NS_ENSURE_SUCCESS(rv, rv);

  if (returnNode) {
    rv = CallQueryInterface(returnNode, aReturn);
  }

  return rv;
}

nsresult
nsGenericElement::GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                                         const nsAString& aLocalName,
                                         nsIDOMNodeList** aReturn)
{
  PRInt32 nameSpaceId = kNameSpaceID_Wildcard;

  if (!aNamespaceURI.EqualsLiteral("*")) {
    nsresult rv =
      nsContentUtils::NameSpaceManager()->RegisterNameSpace(aNamespaceURI,
                                                            nameSpaceId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(aLocalName);
  NS_ENSURE_TRUE(nameAtom, NS_ERROR_OUT_OF_MEMORY);

  nsContentList *list = NS_GetContentList(this, nameAtom, nameSpaceId).get();
  NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

  
  *aReturn = list;
  return NS_OK;
}

nsresult
nsGenericElement::HasAttribute(const nsAString& aName, PRBool* aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  const nsAttrName* name = InternalGetExistingAttrNameFromQName(aName);
  *aReturn = (name != nsnull);

  return NS_OK;
}

nsresult
nsGenericElement::HasAttributeNS(const nsAString& aNamespaceURI,
                                 const nsAString& aLocalName,
                                 PRBool* aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  PRInt32 nsid =
    nsContentUtils::NameSpaceManager()->GetNameSpaceID(aNamespaceURI);

  if (nsid == kNameSpaceID_Unknown) {
    

    *aReturn = PR_FALSE;

    return NS_OK;
  }

  nsCOMPtr<nsIAtom> name = do_GetAtom(aLocalName);
  *aReturn = HasAttr(nsid, name);

  return NS_OK;
}

nsresult
nsGenericElement::JoinTextNodes(nsIContent* aFirst,
                                nsIContent* aSecond)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIDOMText> firstText(do_QueryInterface(aFirst, &rv));

  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIDOMText> secondText(do_QueryInterface(aSecond, &rv));

    if (NS_SUCCEEDED(rv)) {
      nsAutoString str;

      rv = secondText->GetData(str);
      if (NS_SUCCEEDED(rv)) {
        rv = firstText->AppendData(str);
      }
    }
  }

  return rv;
}

nsresult
nsGenericElement::Normalize()
{
  
  mozAutoSubtreeModified subtree(GetOwnerDoc(), nsnull);

  nsresult result = NS_OK;
  PRUint32 index, count = GetChildCount();

  for (index = 0; (index < count) && (NS_OK == result); index++) {
    nsIContent *child = GetChildAt(index);

    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(child);
    if (node) {
      PRUint16 nodeType;
      node->GetNodeType(&nodeType);

      switch (nodeType) {
        case nsIDOMNode::TEXT_NODE:

          
          if (0 == child->TextLength()) {
            result = RemoveChildAt(index, PR_TRUE);
            if (NS_FAILED(result)) {
              return result;
            }

            count--;
            index--;
            break;
          }
 
          if (index+1 < count) {
            
            
            
            nsIContent *sibling = GetChildAt(index + 1);

            nsCOMPtr<nsIDOMNode> siblingNode = do_QueryInterface(sibling);

            if (siblingNode) {
              PRUint16 siblingNodeType;
              siblingNode->GetNodeType(&siblingNodeType);

              if (siblingNodeType == nsIDOMNode::TEXT_NODE) {
                result = RemoveChildAt(index+1, PR_TRUE);
                if (NS_FAILED(result)) {
                  return result;
                }

                result = JoinTextNodes(child, sibling);
                if (NS_FAILED(result)) {
                  return result;
                }
                count--;
                index--;
              }
            }
          }
          break;

        case nsIDOMNode::ELEMENT_NODE:
          nsCOMPtr<nsIDOMElement> element = do_QueryInterface(child);

          if (element) {
            result = element->Normalize();
          }
          break;
      }
    }
  }

  return result;
}

static nsXBLBinding*
GetFirstBindingWithContent(nsBindingManager* aBmgr, nsIContent* aBoundElem)
{
  nsXBLBinding* binding = aBmgr->GetBinding(aBoundElem);
  while (binding) {
    if (binding->GetAnonymousContent()) {
      return binding;
    }
    binding = binding->GetBaseBinding();
  }
  
  return nsnull;
}

static nsresult
BindNodesInInsertPoints(nsXBLBinding* aBinding, nsIContent* aInsertParent,
                        nsIDocument* aDocument)
{
  NS_PRECONDITION(aBinding && aInsertParent, "Missing arguments");

  nsresult rv;
  
  nsInsertionPointList* inserts =
    aBinding->GetExistingInsertionPointsFor(aInsertParent);
  if (inserts) {
    PRBool allowScripts = aBinding->AllowScripts();
#ifdef MOZ_XUL
    nsCOMPtr<nsIXULDocument> xulDoc = do_QueryInterface(aDocument);
#endif
    PRUint32 i;
    for (i = 0; i < inserts->Length(); ++i) {
      nsCOMPtr<nsIContent> insertRoot =
        inserts->ElementAt(i)->GetDefaultContent();
      if (insertRoot) {
        PRUint32 j;
        for (j = 0; j < insertRoot->GetChildCount(); ++j) {
          nsCOMPtr<nsIContent> child = insertRoot->GetChildAt(j);
          rv = child->BindToTree(aDocument, aInsertParent,
                                 aBinding->GetBoundElement(), allowScripts);
          NS_ENSURE_SUCCESS(rv, rv);

#ifdef MOZ_XUL
          if (xulDoc) {
            xulDoc->AddSubtreeToDocument(child);
          }
#endif
        }
      }
    }
  }

  return NS_OK;
}

nsresult
nsGenericElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                             nsIContent* aBindingParent,
                             PRBool aCompileEventHandlers)
{
  NS_PRECONDITION(aParent || aDocument, "Must have document if no parent!");
  NS_PRECONDITION(HasSameOwnerDoc(NODE_FROM(aParent, aDocument)),
                  "Must have the same owner document");
  NS_PRECONDITION(!aParent || aDocument == aParent->GetCurrentDoc(),
                  "aDocument must be current doc of aParent");
  NS_PRECONDITION(!GetCurrentDoc(), "Already have a document.  Unbind first!");
  
  
  NS_PRECONDITION(!GetParent() || aParent == GetParent(),
                  "Already have a parent.  Unbind first!");
  NS_PRECONDITION(!GetBindingParent() ||
                  aBindingParent == GetBindingParent() ||
                  (!aBindingParent && aParent &&
                   aParent->GetBindingParent() == GetBindingParent()),
                  "Already have a binding parent.  Unbind first!");
  NS_PRECONDITION(!aParent || !aDocument ||
                  !aParent->HasFlag(NODE_FORCE_XBL_BINDINGS),
                  "Parent in document but flagged as forcing XBL");
  NS_PRECONDITION(aBindingParent != this,
                  "Content must not be its own binding parent");
  NS_PRECONDITION(!IsRootOfNativeAnonymousSubtree() ||
                  aBindingParent == aParent,
                  "Native anonymous content must have its parent as its "
                  "own binding parent");

  if (!aBindingParent && aParent) {
    aBindingParent = aParent->GetBindingParent();
  }

#ifdef MOZ_XUL
  
  nsXULElement* xulElem = nsXULElement::FromContent(this);
  if (xulElem) {
    xulElem->SetXULBindingParent(aBindingParent);
  }
  else 
#endif
  {
    if (aBindingParent) {
      nsDOMSlots *slots = GetDOMSlots();

      if (!slots) {
        return NS_ERROR_OUT_OF_MEMORY;
      }

      slots->mBindingParent = aBindingParent; 
    }
  }
  NS_ASSERTION(!aBindingParent || IsRootOfNativeAnonymousSubtree() ||
               !HasFlag(NODE_IS_IN_ANONYMOUS_SUBTREE) ||
               (aParent && aParent->IsInNativeAnonymousSubtree()),
               "Trying to re-bind content from native anonymous subtree to "
               "non-native anonymous parent!");
  if (aParent && aParent->IsInNativeAnonymousSubtree()) {
    SetFlags(NODE_IS_IN_ANONYMOUS_SUBTREE);
  }

  PRBool hadForceXBL = HasFlag(NODE_FORCE_XBL_BINDINGS);

  
  if (aParent) {
    mParentPtrBits = reinterpret_cast<PtrBits>(aParent) | PARENT_BIT_PARENT_IS_CONTENT;

    if (aParent->HasFlag(NODE_FORCE_XBL_BINDINGS)) {
      SetFlags(NODE_FORCE_XBL_BINDINGS);
    }
  }
  else {
    mParentPtrBits = reinterpret_cast<PtrBits>(aDocument);
  }

  

  
  if (aDocument) {
    
    
    
    
    
    
    
    

    
    mParentPtrBits |= PARENT_BIT_INDOCUMENT;

    
    UnsetFlags(NODE_FORCE_XBL_BINDINGS);
  }

  
  
  nsresult rv;
  if (hadForceXBL) {
    nsIDocument* ownerDoc = GetOwnerDoc();
    if (ownerDoc) {
      nsBindingManager* bmgr = ownerDoc->BindingManager();

      
      nsXBLBinding* contBinding =
        GetFirstBindingWithContent(bmgr, this);
      if (contBinding) {
        nsCOMPtr<nsIContent> anonRoot = contBinding->GetAnonymousContent();
        PRBool allowScripts = contBinding->AllowScripts();
        PRUint32 i;
        for (i = 0; i < anonRoot->GetChildCount(); ++i) {
          nsCOMPtr<nsIContent> child = anonRoot->GetChildAt(i);
          rv = child->BindToTree(aDocument, this, this, allowScripts);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        
        
        rv = BindNodesInInsertPoints(contBinding, this, aDocument);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      
      if (aBindingParent) {
        nsXBLBinding* binding = bmgr->GetBinding(aBindingParent);
        if (binding) {
          rv = BindNodesInInsertPoints(binding, this, aDocument);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }
  }

  UpdateEditableState();

  
  PRUint32 i;
  
  
  
  for (i = 0; i < mAttrsAndChildren.ChildCount(); ++i) {
    
    nsCOMPtr<nsIContent> child = mAttrsAndChildren.ChildAt(i);
    rv = child->BindToTree(aDocument, this, aBindingParent,
                           aCompileEventHandlers);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsNodeUtils::ParentChainChanged(this);

  
  
  
  NS_POSTCONDITION(aDocument == GetCurrentDoc(), "Bound to wrong document");
  NS_POSTCONDITION(aParent == GetParent(), "Bound to wrong parent");
  NS_POSTCONDITION(aBindingParent == GetBindingParent(),
                   "Bound to wrong binding parent");

  return NS_OK;
}

void
nsGenericElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  NS_PRECONDITION(aDeep || (!GetCurrentDoc() && !GetBindingParent()),
                  "Shallow unbind won't clear document and binding parent on "
                  "kids!");
  
  nsIDocument *document =
    HasFlag(NODE_FORCE_XBL_BINDINGS) ? GetOwnerDoc() : GetCurrentDoc();

  mParentPtrBits = aNullParent ? 0 : mParentPtrBits & ~PARENT_BIT_INDOCUMENT;

  if (document) {
    
    
    document->BindingManager()->ChangeDocumentFor(this, document, nsnull);

    if (HasAttr(kNameSpaceID_XLink, nsGkAtoms::href)) {
      document->ForgetLink(this);
    }

    document->ClearBoxObjectFor(this);
  }

  
  
  
  
  if (HasFlag(NODE_HAS_PROPERTIES)) {
    DeleteProperty(nsGkAtoms::transitionsOfBeforeProperty);
    DeleteProperty(nsGkAtoms::transitionsOfAfterProperty);
    DeleteProperty(nsGkAtoms::transitionsProperty);
  }

  
  UnsetFlags(NODE_FORCE_XBL_BINDINGS);
  
#ifdef MOZ_XUL
  nsXULElement* xulElem = nsXULElement::FromContent(this);
  if (xulElem) {
    xulElem->SetXULBindingParent(nsnull);
  }
  else
#endif
  {
    nsDOMSlots *slots = GetExistingDOMSlots();
    if (slots) {
      slots->mBindingParent = nsnull;
    }
  }

  if (aDeep) {
    
    
    
    PRUint32 i, n = mAttrsAndChildren.ChildCount();

    for (i = 0; i < n; ++i) {
      
      
      
      mAttrsAndChildren.ChildAt(i)->UnbindFromTree(PR_TRUE, PR_FALSE);
    }
  }

  nsNodeUtils::ParentChainChanged(this);
}

nsresult
nsGenericElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  return nsGenericElement::doPreHandleEvent(this, aVisitor);
}

static nsIContent*
FindNativeAnonymousSubtreeOwner(nsIContent* aContent)
{
  if (aContent->IsInNativeAnonymousSubtree()) {
    PRBool isNativeAnon = PR_FALSE;
    while (aContent && !isNativeAnon) {
      isNativeAnon = aContent->IsRootOfNativeAnonymousSubtree();
      aContent = aContent->GetParent();
    }
  }
  return aContent;
}

nsresult
nsGenericElement::doPreHandleEvent(nsIContent* aContent,
                                   nsEventChainPreVisitor& aVisitor)
{
  
  aVisitor.mCanHandle = PR_TRUE;

  
  
  PRBool isAnonForEvents = aContent->IsRootOfNativeAnonymousSubtree();
  if ((aVisitor.mEvent->message == NS_MOUSE_ENTER_SYNTH ||
       aVisitor.mEvent->message == NS_MOUSE_EXIT_SYNTH) &&
      
      
      
      ((static_cast<nsISupports*>(aContent) == aVisitor.mEvent->originalTarget &&
        !aContent->IsInNativeAnonymousSubtree()) || isAnonForEvents)) {
     nsCOMPtr<nsIContent> relatedTarget =
       do_QueryInterface(static_cast<nsMouseEvent*>
                                    (aVisitor.mEvent)->relatedTarget);
    if (relatedTarget &&
        relatedTarget->GetOwnerDoc() == aContent->GetOwnerDoc()) {

      
      
      
      
      
      if (isAnonForEvents || aVisitor.mRelatedTargetIsInAnon ||
          (aVisitor.mEvent->originalTarget == aContent &&
           (aVisitor.mRelatedTargetIsInAnon =
            relatedTarget->IsInNativeAnonymousSubtree()))) {
        nsIContent* anonOwner = FindNativeAnonymousSubtreeOwner(aContent);
        if (anonOwner) {
          nsIContent* anonOwnerRelated =
            FindNativeAnonymousSubtreeOwner(relatedTarget);
          if (anonOwnerRelated) {
            
            
            
            
            while (anonOwner != anonOwnerRelated &&
                   anonOwnerRelated->IsInNativeAnonymousSubtree()) {
              anonOwnerRelated = FindNativeAnonymousSubtreeOwner(anonOwnerRelated);
            }
            if (anonOwner == anonOwnerRelated) {
#ifdef DEBUG_smaug
              nsCOMPtr<nsIContent> originalTarget =
                do_QueryInterface(aVisitor.mEvent->originalTarget);
              nsAutoString ot, ct, rt;
              if (originalTarget) {
                originalTarget->Tag()->ToString(ot);
              }
              aContent->Tag()->ToString(ct);
              relatedTarget->Tag()->ToString(rt);
              printf("Stopping %s propagation:"
                     "\n\toriginalTarget=%s \n\tcurrentTarget=%s %s"
                     "\n\trelatedTarget=%s %s \n%s",
                     (aVisitor.mEvent->message == NS_MOUSE_ENTER_SYNTH)
                       ? "mouseover" : "mouseout",
                     NS_ConvertUTF16toUTF8(ot).get(),
                     NS_ConvertUTF16toUTF8(ct).get(),
                     isAnonForEvents
                       ? "(is native anonymous)"
                       : (aContent->IsInNativeAnonymousSubtree()
                           ? "(is in native anonymous subtree)" : ""),
                     NS_ConvertUTF16toUTF8(rt).get(),
                     relatedTarget->IsInNativeAnonymousSubtree()
                       ? "(is in native anonymous subtree)" : "",
                     (originalTarget && relatedTarget->FindFirstNonNativeAnonymous() ==
                       originalTarget->FindFirstNonNativeAnonymous())
                       ? "" : "Wrong event propagation!?!\n");
#endif
              aVisitor.mParentTarget = nsnull;
              
              aVisitor.mCanHandle = isAnonForEvents;
              return NS_OK;
            }
          }
        }
      }
    }
  }

  nsIContent* parent = aContent->GetParent();
  
  
  if (isAnonForEvents) {
    
    
    NS_ASSERTION(aVisitor.mEvent->eventStructType != NS_MUTATION_EVENT ||
                 aVisitor.mDOMEvent,
                 "Mutation event dispatched in native anonymous content!?!");
    aVisitor.mEventTargetAtParent = parent;
  } else if (parent && aVisitor.mOriginalTargetIsInAnon) {
    nsCOMPtr<nsIContent> content(do_QueryInterface(aVisitor.mEvent->target));
    if (content && content->GetBindingParent() == parent) {
      aVisitor.mEventTargetAtParent = parent;
    }
  }

  
  
  nsIDocument* ownerDoc = aContent->GetOwnerDoc();
  if (ownerDoc) {
    nsIContent* insertionParent = ownerDoc->BindingManager()->
      GetInsertionParent(aContent);
    NS_ASSERTION(!(aVisitor.mEventTargetAtParent && insertionParent &&
                   aVisitor.mEventTargetAtParent != insertionParent),
                 "Retargeting and having insertion parent!");
    if (insertionParent) {
      parent = insertionParent;
    }
  }

  if (parent) {
    aVisitor.mParentTarget = parent;
  } else {
    aVisitor.mParentTarget = aContent->GetCurrentDoc();
  }
  return NS_OK;
}

nsresult
nsGenericElement::PostHandleEvent(nsEventChainPostVisitor& )
{
  return NS_OK;
}

nsresult
nsGenericElement::DispatchDOMEvent(nsEvent* aEvent,
                                   nsIDOMEvent* aDOMEvent,
                                   nsPresContext* aPresContext,
                                   nsEventStatus* aEventStatus)
{
  return nsEventDispatcher::DispatchDOMEvent(static_cast<nsIContent*>(this),
                                             aEvent, aDOMEvent,
                                             aPresContext, aEventStatus);
}

nsIAtom*
nsGenericElement::GetID() const
{
  if (!HasFlag(NODE_MAY_HAVE_ID)) {
    return nsnull;
  }

  nsIAtom* IDName = GetIDAttributeName();
  if (IDName) {
    const nsAttrValue* attrVal = mAttrsAndChildren.GetAttr(IDName);
    if (attrVal){
      if (attrVal->Type() == nsAttrValue::eAtom) {
        return attrVal->GetAtomValue();
      }
      if(attrVal->IsEmptyString()){
        return nsnull;
      }
      
      
      
      if (attrVal->Type() == nsAttrValue::eString) {
        nsAutoString idVal(attrVal->GetStringValue());

        
        const_cast<nsAttrValue*>(attrVal)->ParseAtom(idVal);
        return attrVal->GetAtomValue();
      }
    }
  }
  return nsnull;
}

const nsAttrValue*
nsGenericElement::DoGetClasses() const
{
  NS_NOTREACHED("Shouldn't ever be called");
  return nsnull;
}

NS_IMETHODIMP
nsGenericElement::WalkContentStyleRules(nsRuleWalker* aRuleWalker)
{
  return NS_OK;
}

#ifdef MOZ_SMIL
nsresult
nsGenericElement::GetSMILOverrideStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  nsGenericElement::nsDOMSlots *slots = GetDOMSlots();
  NS_ENSURE_TRUE(slots, NS_ERROR_OUT_OF_MEMORY);

  if (!slots->mSMILOverrideStyle) {
    slots->mSMILOverrideStyle = new nsDOMCSSAttributeDeclaration(this, PR_TRUE);
    NS_ENSURE_TRUE(slots->mSMILOverrideStyle, NS_ERROR_OUT_OF_MEMORY);
  }

  
  NS_ADDREF(*aStyle = slots->mSMILOverrideStyle);
  return NS_OK;
}

nsICSSStyleRule*
nsGenericElement::GetSMILOverrideStyleRule()
{
  nsGenericElement::nsDOMSlots *slots = GetExistingDOMSlots();
  return slots ? slots->mSMILOverrideStyleRule.get() : nsnull;
}

nsresult
nsGenericElement::SetSMILOverrideStyleRule(nsICSSStyleRule* aStyleRule,
                                           PRBool aNotify)
{
  nsGenericElement::nsDOMSlots *slots = GetDOMSlots();
  NS_ENSURE_TRUE(slots, NS_ERROR_OUT_OF_MEMORY);

  slots->mSMILOverrideStyleRule = aStyleRule;

  if (aNotify) {
    nsIDocument* doc = GetCurrentDoc();
    
    
    
    if (doc) {
      nsPresShellIterator iter(doc);
      nsCOMPtr<nsIPresShell> shell;
      while (shell = iter.GetNextShell()) {
        nsPresContext* presContext = shell->GetPresContext();
        presContext->SMILOverrideStyleChanged(this);
      }
    }
  }

  return NS_OK;
}
#endif 

nsICSSStyleRule*
nsGenericElement::GetInlineStyleRule()
{
  return nsnull;
}

NS_IMETHODIMP
nsGenericElement::SetInlineStyleRule(nsICSSStyleRule* aStyleRule,
                                     PRBool aNotify)
{
  NS_NOTYETIMPLEMENTED("nsGenericElement::SetInlineStyleRule");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP_(PRBool)
nsGenericElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  return PR_FALSE;
}

nsChangeHint
nsGenericElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                         PRInt32 aModType) const
{
  return nsChangeHint(0);
}

nsIAtom *
nsGenericElement::GetIDAttributeName() const
{
  return mNodeInfo->GetIDAttributeAtom();
}

nsIAtom *
nsGenericElement::GetClassAttributeName() const
{
  return nsnull;
}

PRBool
nsGenericElement::FindAttributeDependence(const nsIAtom* aAttribute,
                                          const MappedAttributeEntry* const aMaps[],
                                          PRUint32 aMapCount)
{
  for (PRUint32 mapindex = 0; mapindex < aMapCount; ++mapindex) {
    for (const MappedAttributeEntry* map = aMaps[mapindex];
         map->attribute; ++map) {
      if (aAttribute == *map->attribute) {
        return PR_TRUE;
      }
    }
  }

  return PR_FALSE;
}

already_AddRefed<nsINodeInfo>
nsGenericElement::GetExistingAttrNameFromQName(const nsAString& aStr) const
{
  const nsAttrName* name = InternalGetExistingAttrNameFromQName(aStr);
  if (!name) {
    return nsnull;
  }

  nsINodeInfo* nodeInfo;
  if (name->IsAtom()) {
    nodeInfo = mNodeInfo->NodeInfoManager()->GetNodeInfo(name->Atom(), nsnull,
                                                         kNameSpaceID_None).get();
  }
  else {
    NS_ADDREF(nodeInfo = name->NodeInfo());
  }

  return nodeInfo;
}

already_AddRefed<nsIURI>
nsGenericElement::GetBaseURI() const
{
  nsIDocument* doc = GetOwnerDoc();
  if (!doc) {
    
    
    NS_ERROR("Element without owner document");
    return nsnull;
  }

  
  
  nsCOMPtr<nsIURI> parentBase;

  nsIContent *parent = GetParent();
  if (parent) {
    parentBase = parent->GetBaseURI();
  } else {
    
    
    parentBase = doc->GetBaseURI();
  }
  
  
  nsAutoString value;
  GetAttr(kNameSpaceID_XML, nsGkAtoms::base, value);
  if (value.IsEmpty()) {
    
    nsIURI *base = nsnull;
    parentBase.swap(base);

    return base;
  }

  nsCOMPtr<nsIURI> ourBase;
  nsresult rv = NS_NewURI(getter_AddRefs(ourBase), value,
                          doc->GetDocumentCharacterSet().get(), parentBase);
  if (NS_SUCCEEDED(rv)) {
    
    rv = nsContentUtils::GetSecurityManager()->
      CheckLoadURIWithPrincipal(NodePrincipal(), ourBase,
                                nsIScriptSecurityManager::STANDARD);
  }

  nsIURI *base;
  if (NS_FAILED(rv)) {
    base = parentBase;
  } else {
    base = ourBase;
  }

  NS_IF_ADDREF(base);

  return base;    
}

PRBool
nsGenericElement::IsLink(nsIURI** aURI) const
{
  *aURI = nsnull;
  return PR_FALSE;
}


PRBool
nsGenericElement::ShouldBlur(nsIContent *aContent)
{
  
  
  nsIDocument *document = aContent->GetDocument();
  if (!document)
    return PR_FALSE;

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(document->GetWindow());
  if (!window)
    return PR_FALSE;

  nsCOMPtr<nsPIDOMWindow> focusedFrame;
  nsIContent* contentToBlur =
    nsFocusManager::GetFocusedDescendant(window, PR_FALSE, getter_AddRefs(focusedFrame));
  if (contentToBlur == aContent)
    return PR_TRUE;

  
  
  return (contentToBlur && nsFocusManager::GetRedirectedFocus(aContent) == contentToBlur);
}

nsIContent*
nsGenericElement::GetBindingParent() const
{
  nsDOMSlots *slots = GetExistingDOMSlots();

  if (slots) {
    return slots->mBindingParent;
  }
  return nsnull;
}

PRBool
nsGenericElement::IsNodeOfType(PRUint32 aFlags) const
{
  return !(aFlags & ~(eCONTENT | eELEMENT));
}




void
nsGenericElement::SetMayHaveFrame(PRBool aMayHaveFrame)
{
  if (aMayHaveFrame) {
    SetFlags(NODE_MAY_HAVE_FRAME);
  } else {
    UnsetFlags(NODE_MAY_HAVE_FRAME);
  }
}


PRBool
nsGenericElement::MayHaveFrame() const
{
  return HasFlag(NODE_MAY_HAVE_FRAME);
}

PRUint32
nsGenericElement::GetScriptTypeID() const
{
    PtrBits flags = GetFlags();

    
    return (flags >> NODE_SCRIPT_TYPE_OFFSET) & 0x000F;
}

NS_IMETHODIMP
nsGenericElement::SetScriptTypeID(PRUint32 aLang)
{
    if ((aLang & 0x000F) != aLang) {
        NS_ERROR("script ID too large!");
        return NS_ERROR_FAILURE;
    }
    

    UnsetFlags(0x000FU << NODE_SCRIPT_TYPE_OFFSET);
    SetFlags(aLang << NODE_SCRIPT_TYPE_OFFSET);
    return NS_OK;
}

nsresult
nsGenericElement::InsertChildAt(nsIContent* aKid,
                                PRUint32 aIndex,
                                PRBool aNotify)
{
  NS_PRECONDITION(aKid, "null ptr");

  return doInsertChildAt(aKid, aIndex, aNotify, this, GetCurrentDoc(),
                         mAttrsAndChildren);
}



nsresult
nsGenericElement::doInsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                  PRBool aNotify, nsIContent* aParent,
                                  nsIDocument* aDocument,
                                  nsAttrAndChildArray& aChildArray)
{
  NS_PRECONDITION(aParent || aDocument, "Must have document if no parent!");
  NS_PRECONDITION(!aParent || aParent->GetCurrentDoc() == aDocument,
                  "Incorrect aDocument");

  nsresult rv;
  nsINode* container = NODE_FROM(aParent, aDocument);

  if (!container->HasSameOwnerDoc(aKid)) {
    nsCOMPtr<nsIDOMNode> kid = do_QueryInterface(aKid, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
 
    PRUint16 nodeType = 0;
    rv = kid->GetNodeType(&nodeType);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOM3Document> domDoc =
      do_QueryInterface(container->GetOwnerDoc());

    
    
    

    if (domDoc && (nodeType != nsIDOMNode::DOCUMENT_TYPE_NODE ||
                   aKid->GetOwnerDoc())) {
      nsCOMPtr<nsIDOMNode> adoptedKid;
      rv = domDoc->AdoptNode(kid, getter_AddRefs(adoptedKid));
      NS_ENSURE_SUCCESS(rv, rv);

      NS_ASSERTION(adoptedKid == kid, "Uh, adopt node changed nodes?");
    }
  }

  PRUint32 childCount = aChildArray.ChildCount();
  NS_ENSURE_TRUE(aIndex <= childCount, NS_ERROR_ILLEGAL_VALUE);

  nsMutationGuard::DidMutate();

  PRBool isAppend = (aIndex == childCount);

  mozAutoDocUpdate updateBatch(aDocument, UPDATE_CONTENT_MODEL, aNotify);

  rv = aChildArray.InsertChildAt(aKid, aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aKid->BindToTree(aDocument, aParent, nsnull, PR_TRUE);
  if (NS_FAILED(rv)) {
    aChildArray.RemoveChildAt(aIndex);
    aKid->UnbindFromTree();
    return rv;
  }

  NS_ASSERTION(aKid->GetNodeParent() == container,
               "Did we run script inappropriately?");

  if (aNotify) {
    
    
    if (aParent && isAppend) {
      nsNodeUtils::ContentAppended(aParent, aIndex);
    } else {
      nsNodeUtils::ContentInserted(container, aKid, aIndex);
    }

    if (nsContentUtils::HasMutationListeners(aKid,
          NS_EVENT_BITS_MUTATION_NODEINSERTED, container)) {
      mozAutoRemovableBlockerRemover blockerRemover(container->GetOwnerDoc());
      
      nsMutationEvent mutation(PR_TRUE, NS_MUTATION_NODEINSERTED);
      mutation.mRelatedNode = do_QueryInterface(container);

      mozAutoSubtreeModified subtree(container->GetOwnerDoc(), container);
      nsEventDispatcher::Dispatch(aKid, nsnull, &mutation);
    }
  }

  return NS_OK;
}

nsresult
nsGenericElement::RemoveChildAt(PRUint32 aIndex, PRBool aNotify, PRBool aMutationEvent)
{
  nsCOMPtr<nsIContent> oldKid = mAttrsAndChildren.GetSafeChildAt(aIndex);
  NS_ASSERTION(oldKid == GetChildAt(aIndex), "Unexpected child in RemoveChildAt");

  if (oldKid) {
    return doRemoveChildAt(aIndex, aNotify, oldKid, this, GetCurrentDoc(),
                           mAttrsAndChildren, aMutationEvent);
  }

  return NS_OK;
}


nsresult
nsGenericElement::doRemoveChildAt(PRUint32 aIndex, PRBool aNotify,
                                  nsIContent* aKid, nsIContent* aParent,
                                  nsIDocument* aDocument,
                                  nsAttrAndChildArray& aChildArray,
                                  PRBool aMutationEvent)
{
  NS_PRECONDITION(aParent || aDocument, "Must have document if no parent!");
  NS_PRECONDITION(!aParent || aParent->GetCurrentDoc() == aDocument,
                  "Incorrect aDocument");

#ifdef ACCESSIBILITY
  
  
  if (aNotify && aDocument) {
    nsIPresShell *presShell = aDocument->GetPrimaryShell();
    if (presShell && presShell->IsAccessibilityActive()) {
      nsCOMPtr<nsIAccessibilityService> accService = 
        do_GetService("@mozilla.org/accessibilityService;1");
      if (accService) {
        accService->InvalidateSubtreeFor(presShell, aKid,
                                         nsIAccessibilityService::NODE_REMOVE);
      }
    }
  }
#endif

  nsMutationGuard::DidMutate();

  nsINode* container = NODE_FROM(aParent, aDocument);
  
  NS_PRECONDITION(aKid && aKid->GetParent() == aParent &&
                  aKid == container->GetChildAt(aIndex) &&
                  container->IndexOf(aKid) == (PRInt32)aIndex, "Bogus aKid");

  mozAutoDocUpdate updateBatch(aDocument, UPDATE_CONTENT_MODEL, aNotify);

  nsMutationGuard guard;

  mozAutoSubtreeModified subtree(nsnull, nsnull);
  if (aNotify &&
      aMutationEvent &&
      nsContentUtils::HasMutationListeners(aKid,
        NS_EVENT_BITS_MUTATION_NODEREMOVED, container)) {
    mozAutoRemovableBlockerRemover blockerRemover(container->GetOwnerDoc());

    nsMutationEvent mutation(PR_TRUE, NS_MUTATION_NODEREMOVED);
    mutation.mRelatedNode = do_QueryInterface(container);

    subtree.UpdateTarget(container->GetOwnerDoc(), container);
    nsEventDispatcher::Dispatch(aKid, nsnull, &mutation);
  }

  
  
  if (guard.Mutated(0)) {
    aIndex = container->IndexOf(aKid);
    if (static_cast<PRInt32>(aIndex) < 0) {
      return NS_OK;
    }
  }

  aChildArray.RemoveChildAt(aIndex);

  if (aNotify) {
    nsNodeUtils::ContentRemoved(container, aKid, aIndex);
  }

  aKid->UnbindFromTree();

  return NS_OK;
}


nsresult
nsGenericElement::DispatchEvent(nsPresContext* aPresContext,
                                nsEvent* aEvent,
                                nsIContent* aTarget,
                                PRBool aFullDispatch,
                                nsEventStatus* aStatus)
{
  NS_PRECONDITION(aTarget, "Must have target");
  NS_PRECONDITION(aEvent, "Must have source event");
  NS_PRECONDITION(aStatus, "Null out param?");

  if (!aPresContext) {
    return NS_OK;
  }

  nsCOMPtr<nsIPresShell> shell = aPresContext->GetPresShell();
  if (!shell) {
    return NS_OK;
  }

  if (aFullDispatch) {
    return shell->HandleEventWithTarget(aEvent, nsnull, aTarget, aStatus);
  }

  return shell->HandleDOMEventWithTarget(aTarget, aEvent, aStatus);
}


nsresult
nsGenericElement::DispatchClickEvent(nsPresContext* aPresContext,
                                     nsInputEvent* aSourceEvent,
                                     nsIContent* aTarget,
                                     PRBool aFullDispatch,
                                     nsEventStatus* aStatus)
{
  NS_PRECONDITION(aTarget, "Must have target");
  NS_PRECONDITION(aSourceEvent, "Must have source event");
  NS_PRECONDITION(aStatus, "Null out param?");

  nsMouseEvent event(NS_IS_TRUSTED_EVENT(aSourceEvent), NS_MOUSE_CLICK,
                     aSourceEvent->widget, nsMouseEvent::eReal);
  event.refPoint = aSourceEvent->refPoint;
  PRUint32 clickCount = 1;
  float pressure = 0;
  if (aSourceEvent->eventStructType == NS_MOUSE_EVENT) {
    clickCount = static_cast<nsMouseEvent*>(aSourceEvent)->clickCount;
    pressure = static_cast<nsMouseEvent*>(aSourceEvent)->pressure;
  }
  event.pressure = pressure;
  event.clickCount = clickCount;
  event.isShift = aSourceEvent->isShift;
  event.isControl = aSourceEvent->isControl;
  event.isAlt = aSourceEvent->isAlt;
  event.isMeta = aSourceEvent->isMeta;

  return DispatchEvent(aPresContext, &event, aTarget, aFullDispatch, aStatus);
}

nsIFrame*
nsGenericElement::GetPrimaryFrame()
{
  nsIDocument* doc = GetCurrentDoc();
  if (!doc) {
    return nsnull;
  }

  return GetPrimaryFrameFor(this, doc);
}

nsIFrame*
nsGenericElement::GetPrimaryFrame(mozFlushType aType)
{
  nsIDocument* doc = GetCurrentDoc();
  if (!doc) {
    return nsnull;
  }

  
  
  doc->FlushPendingNotifications(aType);

  return GetPrimaryFrameFor(this, doc);
}


nsIFrame*
nsGenericElement::GetPrimaryFrameFor(nsIContent* aContent,
                                     nsIDocument* aDocument)
{
  
  nsIPresShell *presShell = aDocument->GetPrimaryShell();
  if (!presShell) {
    return nsnull;
  }

  return presShell->GetPrimaryFrameFor(aContent);
}

void
nsGenericElement::DestroyContent()
{
  nsIDocument *document = GetOwnerDoc();
  if (document) {
    document->BindingManager()->ChangeDocumentFor(this, document, nsnull);
    document->ClearBoxObjectFor(this);
  }

  
  
  nsContentUtils::ReleaseWrapper(this, this);

  PRUint32 i, count = mAttrsAndChildren.ChildCount();
  for (i = 0; i < count; ++i) {
    
    mAttrsAndChildren.ChildAt(i)->DestroyContent();
  }
}

void
nsGenericElement::SaveSubtreeState()
{
  PRUint32 i, count = mAttrsAndChildren.ChildCount();
  for (i = 0; i < count; ++i) {
    mAttrsAndChildren.ChildAt(i)->SaveSubtreeState();
  }
}










NS_IMETHODIMP
nsGenericElement::InsertBefore(nsIDOMNode *aNewChild, nsIDOMNode *aRefChild,
                               nsIDOMNode **aReturn)
{
  return doReplaceOrInsertBefore(PR_FALSE, aNewChild, aRefChild, this, GetCurrentDoc(),
                                 aReturn);
}

NS_IMETHODIMP
nsGenericElement::ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                               nsIDOMNode** aReturn)
{
  return doReplaceOrInsertBefore(PR_TRUE, aNewChild, aOldChild, this, GetCurrentDoc(),
                                 aReturn);
}

NS_IMETHODIMP
nsGenericElement::RemoveChild(nsIDOMNode *aOldChild, nsIDOMNode **aReturn)
{
  return doRemoveChild(aOldChild, this, GetCurrentDoc(),
                       aReturn);
}




static
PRBool IsAllowedAsChild(nsIContent* aNewChild, PRUint16 aNewNodeType,
                        nsIContent* aParent, nsIDocument* aDocument,
                        PRBool aIsReplace, nsIContent* aRefContent)
{
  NS_PRECONDITION(aNewChild, "Must have new child");
  NS_PRECONDITION(!aIsReplace || aRefContent,
                  "Must have ref content for replace");
#ifdef DEBUG
  PRUint16 debugNodeType = 0;
  nsCOMPtr<nsIDOMNode> debugNode(do_QueryInterface(aNewChild));
  nsresult debugRv = debugNode->GetNodeType(&debugNodeType);

  NS_PRECONDITION(NS_SUCCEEDED(debugRv) && debugNodeType == aNewNodeType,
                  "Bogus node type passed");
#endif

  if (aParent && nsContentUtils::ContentIsDescendantOf(aParent, aNewChild)) {
    return PR_FALSE;
  }

  
  switch (aNewNodeType) {
  case nsIDOMNode::COMMENT_NODE :
  case nsIDOMNode::PROCESSING_INSTRUCTION_NODE :
    
    return PR_TRUE;
  case nsIDOMNode::TEXT_NODE :
  case nsIDOMNode::CDATA_SECTION_NODE :
  case nsIDOMNode::ENTITY_REFERENCE_NODE :
    
    return aParent != nsnull;
  case nsIDOMNode::ELEMENT_NODE :
    {
      if (aParent) {
        
        return PR_TRUE;
      }

      nsIContent* rootContent = aDocument->GetRootContent();
      if (rootContent) {
        
        
        return aIsReplace && rootContent == aRefContent;
      }

      
      
      if (!aRefContent) {
        
        return PR_TRUE;
      }

      
      nsCOMPtr<nsIDOMDocument> doc = do_QueryInterface(aDocument);
      NS_ASSERTION(doc, "Shouldn't happen");
      nsCOMPtr<nsIDOMDocumentType> docType;
      doc->GetDoctype(getter_AddRefs(docType));
      nsCOMPtr<nsIContent> docTypeContent = do_QueryInterface(docType);
      
      if (!docTypeContent) {
        
        return PR_TRUE;
      }

      PRInt32 doctypeIndex = aDocument->IndexOf(docTypeContent);
      PRInt32 insertIndex = aDocument->IndexOf(aRefContent);

      
      
      
      return aIsReplace ? (insertIndex >= doctypeIndex) :
        insertIndex > doctypeIndex;
    }
  case nsIDOMNode::DOCUMENT_TYPE_NODE :
    {
      if (aParent) {
        
        return PR_FALSE;
      }

      nsCOMPtr<nsIDOMDocument> doc = do_QueryInterface(aDocument);
      NS_ASSERTION(doc, "Shouldn't happen");
      nsCOMPtr<nsIDOMDocumentType> docType;
      doc->GetDoctype(getter_AddRefs(docType));
      nsCOMPtr<nsIContent> docTypeContent = do_QueryInterface(docType);
      if (docTypeContent) {
        
        return aIsReplace && docTypeContent == aRefContent;
      }

      
      
      nsIContent* rootContent = aDocument->GetRootContent();
      if (!rootContent) {
        
        return PR_TRUE;
      }

      if (!aRefContent) {
        
        return PR_FALSE;
      }

      PRInt32 rootIndex = aDocument->IndexOf(rootContent);
      PRInt32 insertIndex = aDocument->IndexOf(aRefContent);

      
      
      
      return insertIndex <= rootIndex;
    }
  case nsIDOMNode::DOCUMENT_FRAGMENT_NODE :
    {
      
      
      
      if (aParent) {
        
        return PR_TRUE;
      }

      PRBool sawElement = PR_FALSE;
      PRUint32 count = aNewChild->GetChildCount();
      for (PRUint32 index = 0; index < count; ++index) {
        nsIContent* childContent = aNewChild->GetChildAt(index);
        NS_ASSERTION(childContent, "Something went wrong");
        if (childContent->IsNodeOfType(nsINode::eELEMENT)) {
          if (sawElement) {
            
            return PR_FALSE;
          }
          sawElement = PR_TRUE;
        }
        
        
        nsCOMPtr<nsIDOMNode> childNode(do_QueryInterface(childContent));
        PRUint16 type;
        childNode->GetNodeType(&type);
        if (!IsAllowedAsChild(childContent, type, aParent, aDocument,
                              aIsReplace, aRefContent)) {
          return PR_FALSE;
        }
      }

      
      return PR_TRUE;
    }
  default:
    


    break;
  }

  return PR_FALSE;
}


nsresult
nsGenericElement::doReplaceOrInsertBefore(PRBool aReplace,
                                          nsIDOMNode* aNewChild,
                                          nsIDOMNode* aRefChild,
                                          nsIContent* aParent,
                                          nsIDocument* aDocument,
                                          nsIDOMNode** aReturn)
{
  NS_PRECONDITION(aParent || aDocument, "Must have document if no parent!");
  NS_PRECONDITION(!aParent || aParent->GetCurrentDoc() == aDocument,
                  "Incorrect aDocument");

  *aReturn = nsnull;

  if (!aNewChild || (aReplace && !aRefChild)) {
    return NS_ERROR_NULL_POINTER;
  }

  
  
  nsCOMPtr<nsIDOMNode> returnVal = aReplace ? aRefChild : aNewChild;

  nsCOMPtr<nsIContent> refContent;
  nsresult res = NS_OK;
  PRInt32 insPos;

  nsINode* container = NODE_FROM(aParent, aDocument);

  
  if (aRefChild) {
    refContent = do_QueryInterface(aRefChild);
    insPos = container->IndexOf(refContent);
    if (insPos < 0) {
      return NS_ERROR_DOM_NOT_FOUND_ERR;
    }

    if (aRefChild == aNewChild) {
      NS_ADDREF(*aReturn = aNewChild);

      return NS_OK;
    }
  } else {
    insPos = container->GetChildCount();
  }

  nsCOMPtr<nsIContent> newContent = do_QueryInterface(aNewChild);
  if (!newContent) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  PRUint16 nodeType = 0;
  res = aNewChild->GetNodeType(&nodeType);
  NS_ENSURE_SUCCESS(res, res);

  
  if (!IsAllowedAsChild(newContent, nodeType, aParent, aDocument, aReplace,
                        refContent)) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  
  
  
  if (!container->HasSameOwnerDoc(newContent) &&
      (nodeType != nsIDOMNode::DOCUMENT_TYPE_NODE ||
       newContent->GetOwnerDoc())) {
    nsCOMPtr<nsIDOM3Document> domDoc = do_QueryInterface(aDocument);

    if (domDoc) {
      nsCOMPtr<nsIDOMNode> adoptedKid;
      nsresult rv = domDoc->AdoptNode(aNewChild, getter_AddRefs(adoptedKid));
      NS_ENSURE_SUCCESS(rv, rv);

      NS_ASSERTION(adoptedKid == aNewChild, "Uh, adopt node changed nodes?");
    }
  }

  
  
  mozAutoDocConditionalContentUpdateBatch batch(aDocument,
    aReplace || nodeType == nsIDOMNode::DOCUMENT_FRAGMENT_NODE);

  
  if (aReplace) {
    
    
    refContent = container->GetChildAt(insPos + 1);

    nsMutationGuard guard;

    res = container->RemoveChildAt(insPos, PR_TRUE);
    NS_ENSURE_SUCCESS(res, res);

    if (guard.Mutated(1)) {
      insPos = refContent ? container->IndexOf(refContent) :
                            container->GetChildCount();
      if (insPos < 0) {
        return NS_ERROR_DOM_NOT_FOUND_ERR;
      }

      
      
      if (!IsAllowedAsChild(newContent, nodeType, aParent, aDocument,
                            PR_FALSE, refContent)) {
        return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
      }
    }
  }

  




  if (nodeType == nsIDOMNode::DOCUMENT_FRAGMENT_NODE) {
    PRUint32 count = newContent->GetChildCount();

    if (!count) {
      returnVal.swap(*aReturn);

      return NS_OK;
    }

    
    
    nsCOMArray<nsIContent> fragChildren;
    if (!fragChildren.SetCapacity(count)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    PRUint32 i;
    for (i = 0; i < count; i++) {
      nsIContent* child = newContent->GetChildAt(i);
      NS_ASSERTION(child->GetCurrentDoc() == nsnull,
                   "How did we get a child with a current doc?");
      fragChildren.AppendObject(child);
    }

    
    PRBool mutated = PR_FALSE;
    for (i = count; i > 0;) {
      
      
      
      nsMutationGuard guard;
      newContent->RemoveChildAt(--i, PR_TRUE);
      mutated = mutated || guard.Mutated(1);
    }

    
    
    if (mutated) {
      for (i = 0; i < count; ++i) {
        
        nsIContent* childContent = fragChildren[i];

        nsCOMPtr<nsIDOMNode> tmpNode = do_QueryInterface(childContent);
        PRUint16 tmpType = 0;
        tmpNode->GetNodeType(&tmpType);

        if (childContent->GetNodeParent() ||
            !IsAllowedAsChild(childContent, tmpType, aParent, aDocument, PR_FALSE,
                              refContent)) {
          return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
        }
      }

      insPos = refContent ? container->IndexOf(refContent) :
                            container->GetChildCount();
      if (insPos < 0) {
        
        
        return NS_ERROR_DOM_NOT_FOUND_ERR;
      }
    }

    PRBool appending = aParent && (insPos == container->GetChildCount());
    PRBool firstInsPos = insPos;

    
    
    for (i = 0; i < count; ++i, ++insPos) {
      nsIContent* childContent = fragChildren[i];

      
      
      res = container->InsertChildAt(childContent, insPos, PR_FALSE);
      if (NS_FAILED(res)) {
        
        if (appending && i != 0) {
          nsNodeUtils::ContentAppended(aParent, firstInsPos);
        }
        return res;
      }

      if (!appending) {
        nsNodeUtils::ContentInserted(container, childContent, insPos);
      }
    }

    
    if (appending) {
      nsNodeUtils::ContentAppended(aParent, firstInsPos);
    }

    
    nsIDocument* doc = container->GetOwnerDoc();
    nsPIDOMWindow* window = nsnull;
    if (doc && (window = doc->GetInnerWindow()) &&
        window->HasMutationListeners(NS_EVENT_BITS_MUTATION_NODEINSERTED)) {

      for (i = 0; i < count; ++i, ++insPos) {
        nsIContent* childContent = fragChildren[i];

        if (nsContentUtils::HasMutationListeners(childContent,
              NS_EVENT_BITS_MUTATION_NODEINSERTED, container)) {
          mozAutoRemovableBlockerRemover blockerRemover(container->GetOwnerDoc());

          nsMutationEvent mutation(PR_TRUE, NS_MUTATION_NODEINSERTED);
          mutation.mRelatedNode = do_QueryInterface(container);

          mozAutoSubtreeModified subtree(container->GetOwnerDoc(), container);
          nsEventDispatcher::Dispatch(childContent, nsnull, &mutation);
        }
      }
    }
  }
  else {
    

    if (newContent->IsRootOfAnonymousSubtree()) {
      
      
      
      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }

    
    nsINode* oldParent = newContent->GetNodeParent();

    if (oldParent) {
      PRInt32 removeIndex = oldParent->IndexOf(newContent);

      if (removeIndex < 0) {
        
        NS_ERROR("How come our flags didn't catch this?");
        return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
      }
      
      NS_ASSERTION(!(oldParent == container && removeIndex == insPos),
                   "invalid removeIndex");

      nsMutationGuard guard;

      res = oldParent->RemoveChildAt(removeIndex, PR_TRUE);
      NS_ENSURE_SUCCESS(res, res);

      
      
      if (oldParent == container && removeIndex < insPos) {
        --insPos;
      }

      if (guard.Mutated(1)) {
        insPos = refContent ? container->IndexOf(refContent) :
                              container->GetChildCount();
        if (insPos < 0) {
          
          
          return NS_ERROR_DOM_NOT_FOUND_ERR;
        }

        if (newContent->GetNodeParent() ||
            !IsAllowedAsChild(newContent, nodeType, aParent, aDocument,
                              PR_FALSE, refContent)) {
          return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
        }
      }
    }

    if (!newContent->IsXUL()) {
      nsContentUtils::ReparentContentWrapper(newContent, aParent,
                                             container->GetOwnerDoc(),
                                             container->GetOwnerDoc());
    }

    res = container->InsertChildAt(newContent, insPos, PR_TRUE);
    NS_ENSURE_SUCCESS(res, res);
  }

  returnVal.swap(*aReturn);

  return res;
}


nsresult
nsGenericElement::doRemoveChild(nsIDOMNode* aOldChild, nsIContent* aParent,
                                nsIDocument* aDocument, nsIDOMNode** aReturn)
{
  NS_PRECONDITION(aParent || aDocument, "Must have document if no parent!");
  NS_PRECONDITION(!aParent || aParent->GetCurrentDoc() == aDocument,
                  "Incorrect aDocument");

  *aReturn = nsnull;
  NS_ENSURE_TRUE(aOldChild, NS_ERROR_NULL_POINTER);

  nsINode* container = NODE_FROM(aParent, aDocument);

  nsCOMPtr<nsIContent> content = do_QueryInterface(aOldChild);
  
  PRInt32 index = container->IndexOf(content);
  if (index == -1) {
    
    return NS_ERROR_DOM_NOT_FOUND_ERR;
  }

  nsresult rv = container->RemoveChildAt(index, PR_TRUE);

  *aReturn = aOldChild;
  NS_ADDREF(aOldChild);

  return rv;
}





NS_IMPL_CYCLE_COLLECTION_CLASS(nsGenericElement)

NS_IMPL_CYCLE_COLLECTION_ROOT_BEGIN(nsGenericElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_ROOT_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsGenericElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_LISTENERMANAGER
  NS_IMPL_CYCLE_COLLECTION_UNLINK_USERDATA

  if (tmp->HasProperties() && tmp->IsXUL()) {
    tmp->DeleteProperty(nsGkAtoms::contextmenulistener);
    tmp->DeleteProperty(nsGkAtoms::popuplistener);
  }

  
  {
    PRUint32 childCount = tmp->mAttrsAndChildren.ChildCount();
    if (childCount) {
      
      nsAutoScriptBlocker scriptBlocker;
      while (childCount-- > 0) {
        
        
        
        tmp->mAttrsAndChildren.ChildAt(childCount)->UnbindFromTree();
        tmp->mAttrsAndChildren.RemoveChildAt(childCount);
      }
    }
  }  

  
  {
    nsDOMSlots *slots = tmp->GetExistingDOMSlots();
    if (slots) {
      slots->mStyle = nsnull;
#ifdef MOZ_SMIL
      slots->mSMILOverrideStyle = nsnull;
#endif 
      if (slots->mAttributeMap) {
        slots->mAttributeMap->DropReference();
        slots->mAttributeMap = nsnull;
      }
      if (tmp->IsXUL())
        NS_IF_RELEASE(slots->mControllers);
      slots->mChildrenList = nsnull;
    }
  }

  {
    nsIDocument *doc;
    if (!tmp->GetNodeParent() && (doc = tmp->GetOwnerDoc()) &&
        tmp->HasFlag(NODE_MAY_BE_IN_BINDING_MNGR)) {
      doc->BindingManager()->ChangeDocumentFor(tmp, doc, nsnull);
    }
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsGenericElement)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsGenericElement)
  
  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS

  nsIDocument* currentDoc = tmp->GetCurrentDoc();
  if (currentDoc && nsCCUncollectableMarker::InGeneration(
                      cb, currentDoc->GetMarkedCCGeneration())) {
    return NS_SUCCESS_INTERRUPTED_TRAVERSE;
  }

  nsIDocument* ownerDoc = tmp->GetOwnerDoc();
  if (ownerDoc) {
    ownerDoc->BindingManager()->Traverse(tmp, cb);
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_LISTENERMANAGER
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_USERDATA

  if (tmp->HasProperties() && tmp->IsXUL()) {
    nsISupports* property =
      static_cast<nsISupports*>
                 (tmp->GetProperty(nsGkAtoms::contextmenulistener));
    cb.NoteXPCOMChild(property);
    property = static_cast<nsISupports*>
                          (tmp->GetProperty(nsGkAtoms::popuplistener));
    cb.NoteXPCOMChild(property);
  }

  
  {
    PRUint32 i;
    PRUint32 attrs = tmp->mAttrsAndChildren.AttrCount();
    for (i = 0; i < attrs; i++) {
      const nsAttrName* name = tmp->mAttrsAndChildren.AttrNameAt(i);
      if (!name->IsAtom()) {
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb,
                                           "mAttrsAndChildren[i]->NodeInfo()");
        cb.NoteXPCOMChild(name->NodeInfo());
      }
    }

    PRUint32 kids = tmp->mAttrsAndChildren.ChildCount();
    for (i = 0; i < kids; i++) {
      NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mAttrsAndChildren[i]");
      cb.NoteXPCOMChild(tmp->mAttrsAndChildren.GetSafeChildAt(i));
    }
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mNodeInfo)

  
  {
    nsDOMSlots *slots = tmp->GetExistingDOMSlots();
    if (slots) {
      NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "slots mStyle");
      cb.NoteXPCOMChild(slots->mStyle.get());

#ifdef MOZ_SMIL
      NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "slots mSMILOverrideStyle");
      cb.NoteXPCOMChild(slots->mSMILOverrideStyle.get());
#endif 

      NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "slots mAttributeMap");
      cb.NoteXPCOMChild(slots->mAttributeMap.get());

      if (tmp->IsXUL())
        cb.NoteXPCOMChild(slots->mControllers);
      cb.NoteXPCOMChild(
        static_cast<nsIDOMNodeList*>(slots->mChildrenList.get()));
    }
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_INTERFACE_MAP_BEGIN(nsGenericElement)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsGenericElement)
  NS_INTERFACE_MAP_ENTRY(nsIContent)
  NS_INTERFACE_MAP_ENTRY(nsINode)
  NS_INTERFACE_MAP_ENTRY(nsPIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOM3Node, new nsNode3Tearoff(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOMNSElement, new nsNSElementTearoff(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOMEventTarget,
                                 nsDOMEventRTTearoff::Create(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOM3EventTarget,
                                 nsDOMEventRTTearoff::Create(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOMNSEventTarget,
                                 nsDOMEventRTTearoff::Create(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsISupportsWeakReference,
                                 new nsNodeSupportsWeakRefTearoff(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOMNodeSelector,
                                 new nsNodeSelectorTearoff(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOMXPathNSResolver,
                                 new nsNode3Tearoff(this))
  
  
  
  
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContent)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsGenericElement, nsIContent)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS_WITH_DESTROY(nsGenericElement,
                                                        nsIContent,
                                                        nsNodeUtils::LastRelease(this))

nsresult
nsGenericElement::PostQueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  nsIDocument *document = GetOwnerDoc();
  if (document) {
    return document->BindingManager()->GetBindingImplementation(this, aIID,
                                                                aInstancePtr);
  }

  *aInstancePtr = nsnull;
  return NS_ERROR_NO_INTERFACE;
}


nsresult
nsGenericElement::LeaveLink(nsPresContext* aPresContext)
{
  nsILinkHandler *handler = aPresContext->GetLinkHandler();
  if (!handler) {
    return NS_OK;
  }

  return handler->OnLeaveLink();
}

nsresult
nsGenericElement::AddScriptEventListener(nsIAtom* aEventName,
                                         const nsAString& aValue,
                                         PRBool aDefer)
{
  nsIDocument *ownerDoc = GetOwnerDoc();
  if (!ownerDoc || ownerDoc->IsLoadedAsData()) {
    
    
    return NS_OK;
  }

  NS_PRECONDITION(aEventName, "Must have event name!");
  nsCOMPtr<nsISupports> target;
  PRBool defer = PR_TRUE;
  nsCOMPtr<nsIEventListenerManager> manager;

  GetEventListenerManagerForAttr(getter_AddRefs(manager),
                                 getter_AddRefs(target),
                                 &defer);
  if (!manager) {
    return NS_OK;
  }

  defer = defer && aDefer; 
  PRUint32 lang = GetScriptTypeID();
  return
    manager->AddScriptEventListener(target, aEventName, aValue, lang, defer,
                                    !nsContentUtils::IsChromeDoc(ownerDoc));
}




const nsAttrName*
nsGenericElement::InternalGetExistingAttrNameFromQName(const nsAString& aStr) const
{
  return mAttrsAndChildren.GetExistingAttrNameFromQName(
    NS_ConvertUTF16toUTF8(aStr));
}

nsresult
nsGenericElement::CopyInnerTo(nsGenericElement* aDst) const
{
  PRUint32 i, count = mAttrsAndChildren.AttrCount();
  for (i = 0; i < count; ++i) {
    const nsAttrName* name = mAttrsAndChildren.AttrNameAt(i);
    const nsAttrValue* value = mAttrsAndChildren.AttrAt(i);
    nsAutoString valStr;
    value->ToString(valStr);
    nsresult rv = aDst->SetAttr(name->NamespaceID(), name->LocalName(),
                                name->GetPrefix(), valStr, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
nsGenericElement::SetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                          nsIAtom* aPrefix, const nsAString& aValue,
                          PRBool aNotify)
{
  NS_ENSURE_ARG_POINTER(aName);
  NS_ASSERTION(aNamespaceID != kNameSpaceID_Unknown,
               "Don't call SetAttr with unknown namespace");

  nsIDocument* doc = GetCurrentDoc();
  if (kNameSpaceID_XLink == aNamespaceID && nsGkAtoms::href == aName) {
    
    
    
    
    
    
    
    if (doc) {
      doc->ForgetLink(this);
    }
  }

  nsAutoString oldValue;
  PRBool modification = PR_FALSE;
  PRBool hasListeners = aNotify &&
    nsContentUtils::HasMutationListeners(this,
                                         NS_EVENT_BITS_MUTATION_ATTRMODIFIED,
                                         this);
  
  
  
  
  
  
  if (hasListeners || aNotify) {
    nsAttrInfo info(GetAttrInfo(aNamespaceID, aName));
    if (info.mValue) {
      
      
      PRBool valueMatches;
      if (hasListeners) {
        
        info.mValue->ToString(oldValue);
        valueMatches = aValue.Equals(oldValue);
      } else if (aNotify) {
        valueMatches = info.mValue->Equals(aValue, eCaseMatters);
      }
      if (valueMatches && aPrefix == info.mName->GetPrefix()) {
        return NS_OK;
      }
      modification = PR_TRUE;
    }
  }

  nsresult rv = BeforeSetAttr(aNamespaceID, aName, &aValue, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsAttrValue attrValue;
  if (!ParseAttribute(aNamespaceID, aName, aValue, attrValue)) {
    attrValue.SetTo(aValue);
  }

  return SetAttrAndNotify(aNamespaceID, aName, aPrefix, oldValue,
                          attrValue, modification, hasListeners, aNotify,
                          &aValue);
}
  
nsresult
nsGenericElement::SetAttrAndNotify(PRInt32 aNamespaceID,
                                   nsIAtom* aName,
                                   nsIAtom* aPrefix,
                                   const nsAString& aOldValue,
                                   nsAttrValue& aParsedValue,
                                   PRBool aModification,
                                   PRBool aFireMutation,
                                   PRBool aNotify,
                                   const nsAString* aValueForAfterSetAttr)
{
  nsresult rv;
  PRUint8 modType = aModification ?
    static_cast<PRUint8>(nsIDOMMutationEvent::MODIFICATION) :
    static_cast<PRUint8>(nsIDOMMutationEvent::ADDITION);

  nsIDocument* document = GetCurrentDoc();
  mozAutoDocUpdate updateBatch(document, UPDATE_CONTENT_MODEL, aNotify);

  
  
  PRUint32 stateMask;
  if (aNotify) {
    stateMask = PRUint32(IntrinsicState());
    
    nsNodeUtils::AttributeWillChange(this, aNamespaceID, aName, modType);
  }

  if (aNamespaceID == kNameSpaceID_None) {
    
    
    if (!IsAttributeMapped(aName) ||
        !SetMappedAttribute(document, aName, aParsedValue, &rv)) {
      rv = mAttrsAndChildren.SetAndTakeAttr(aName, aParsedValue);
    }
  }
  else {
    nsCOMPtr<nsINodeInfo> ni;
    ni = mNodeInfo->NodeInfoManager()->GetNodeInfo(aName, aPrefix,
                                                   aNamespaceID);
    NS_ENSURE_TRUE(ni, NS_ERROR_OUT_OF_MEMORY);

    rv = mAttrsAndChildren.SetAndTakeAttr(ni, aParsedValue);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  if (document || HasFlag(NODE_FORCE_XBL_BINDINGS)) {
    nsIDocument* ownerDoc = GetOwnerDoc();
    if (ownerDoc) {
      nsRefPtr<nsXBLBinding> binding =
        ownerDoc->BindingManager()->GetBinding(this);
      if (binding) {
        binding->AttributeChanged(aName, aNamespaceID, PR_FALSE, aNotify);
      }
    }
  }

  if (aNotify) {
    stateMask = stateMask ^ PRUint32(IntrinsicState());
    if (stateMask && document) {
      MOZ_AUTO_DOC_UPDATE(document, UPDATE_CONTENT_STATE, aNotify);
      document->ContentStatesChanged(this, nsnull, stateMask);
    }
    nsNodeUtils::AttributeChanged(this, aNamespaceID, aName, modType);
  }

  if (aNamespaceID == kNameSpaceID_XMLEvents && 
      aName == nsGkAtoms::event && mNodeInfo->GetDocument()) {
    mNodeInfo->GetDocument()->AddXMLEventsContent(this);
  }
  if (aValueForAfterSetAttr) {
    rv = AfterSetAttr(aNamespaceID, aName, aValueForAfterSetAttr, aNotify);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aFireMutation) {
    mozAutoRemovableBlockerRemover blockerRemover(GetOwnerDoc());
    
    nsMutationEvent mutation(PR_TRUE, NS_MUTATION_ATTRMODIFIED);

    nsAutoString attrName;
    aName->ToString(attrName);
    nsCOMPtr<nsIDOMAttr> attrNode;
    nsAutoString ns;
    nsContentUtils::NameSpaceManager()->GetNameSpaceURI(aNamespaceID, ns);
    GetAttributeNodeNS(ns, attrName, getter_AddRefs(attrNode));
    mutation.mRelatedNode = attrNode;

    mutation.mAttrName = aName;
    nsAutoString newValue;
    GetAttr(aNamespaceID, aName, newValue);
    if (!newValue.IsEmpty()) {
      mutation.mNewAttrValue = do_GetAtom(newValue);
    }
    if (!aOldValue.IsEmpty()) {
      mutation.mPrevAttrValue = do_GetAtom(aOldValue);
    }
    mutation.mAttrChange = modType;

    mozAutoSubtreeModified subtree(GetOwnerDoc(), this);
    nsEventDispatcher::Dispatch(this, nsnull, &mutation);
  }

  return NS_OK;
}

PRBool
nsGenericElement::ParseAttribute(PRInt32 aNamespaceID,
                                 nsIAtom* aAttribute,
                                 const nsAString& aValue,
                                 nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None &&
      aAttribute == GetIDAttributeName() && !aValue.IsEmpty()) {
    SetFlags(NODE_MAY_HAVE_ID);
    
    
    aResult.ParseAtom(aValue);
    return PR_TRUE;
  }

  return PR_FALSE;
}

PRBool
nsGenericElement::SetMappedAttribute(nsIDocument* aDocument,
                                     nsIAtom* aName,
                                     nsAttrValue& aValue,
                                     nsresult* aRetval)
{
  *aRetval = NS_OK;
  return PR_FALSE;
}

nsresult
nsGenericElement::GetEventListenerManagerForAttr(nsIEventListenerManager** aManager,
                                                 nsISupports** aTarget,
                                                 PRBool* aDefer)
{
  *aManager = GetListenerManager(PR_TRUE);
  *aDefer = PR_TRUE;
  NS_ENSURE_STATE(*aManager);
  NS_ADDREF(*aManager);
  NS_ADDREF(*aTarget = static_cast<nsIContent*>(this));
  return NS_OK;
}

nsGenericElement::nsAttrInfo
nsGenericElement::GetAttrInfo(PRInt32 aNamespaceID, nsIAtom* aName) const
{
  NS_ASSERTION(nsnull != aName, "must have attribute name");
  NS_ASSERTION(aNamespaceID != kNameSpaceID_Unknown,
               "must have a real namespace ID!");

  PRInt32 index = mAttrsAndChildren.IndexOfAttr(aName, aNamespaceID);
  if (index >= 0) {
    return nsAttrInfo(mAttrsAndChildren.AttrNameAt(index),
                      mAttrsAndChildren.AttrAt(index));
  }

  return nsAttrInfo(nsnull, nsnull);
}
  

PRBool
nsGenericElement::GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                          nsAString& aResult) const
{
  NS_ASSERTION(nsnull != aName, "must have attribute name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown,
               "must have a real namespace ID!");

  const nsAttrValue* val = mAttrsAndChildren.GetAttr(aName, aNameSpaceID);
  if (!val) {
    
    
    
    aResult.Truncate();
    
    return PR_FALSE;
  }

  val->ToString(aResult);

  return PR_TRUE;
}

PRBool
nsGenericElement::HasAttr(PRInt32 aNameSpaceID, nsIAtom* aName) const
{
  NS_ASSERTION(nsnull != aName, "must have attribute name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown,
               "must have a real namespace ID!");

  return mAttrsAndChildren.IndexOfAttr(aName, aNameSpaceID) >= 0;
}

PRBool
nsGenericElement::AttrValueIs(PRInt32 aNameSpaceID,
                              nsIAtom* aName,
                              const nsAString& aValue,
                              nsCaseTreatment aCaseSensitive) const
{
  NS_ASSERTION(aName, "Must have attr name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown, "Must have namespace");

  const nsAttrValue* val = mAttrsAndChildren.GetAttr(aName, aNameSpaceID);
  return val && val->Equals(aValue, aCaseSensitive);
}

PRBool
nsGenericElement::AttrValueIs(PRInt32 aNameSpaceID,
                              nsIAtom* aName,
                              nsIAtom* aValue,
                              nsCaseTreatment aCaseSensitive) const
{
  NS_ASSERTION(aName, "Must have attr name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown, "Must have namespace");
  NS_ASSERTION(aValue, "Null value atom");

  const nsAttrValue* val = mAttrsAndChildren.GetAttr(aName, aNameSpaceID);
  return val && val->Equals(aValue, aCaseSensitive);
}

PRInt32
nsGenericElement::FindAttrValueIn(PRInt32 aNameSpaceID,
                                  nsIAtom* aName,
                                  AttrValuesArray* aValues,
                                  nsCaseTreatment aCaseSensitive) const
{
  NS_ASSERTION(aName, "Must have attr name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown, "Must have namespace");
  NS_ASSERTION(aValues, "Null value array");
  
  const nsAttrValue* val = mAttrsAndChildren.GetAttr(aName, aNameSpaceID);
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
nsGenericElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                            PRBool aNotify)
{
  NS_ASSERTION(nsnull != aName, "must have attribute name");

  PRInt32 index = mAttrsAndChildren.IndexOfAttr(aName, aNameSpaceID);
  if (index < 0) {
    return NS_OK;
  }

  nsresult rv = BeforeSetAttr(aNameSpaceID, aName, nsnull, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsIDocument *document = GetCurrentDoc();    
  mozAutoDocUpdate updateBatch(document, UPDATE_CONTENT_MODEL, aNotify);

  if (aNotify) {
    nsNodeUtils::AttributeWillChange(this, aNameSpaceID, aName,
                                     nsIDOMMutationEvent::REMOVAL);
  }

  if (document && kNameSpaceID_XLink == aNameSpaceID &&
      nsGkAtoms::href == aName) {
    
    document->ForgetLink(this);
  }

  
  
  PRUint32 stateMask;
  if (aNotify) {
    stateMask = PRUint32(IntrinsicState());
  }    

  PRBool hasMutationListeners = aNotify &&
    nsContentUtils::HasMutationListeners(this,
                                         NS_EVENT_BITS_MUTATION_ATTRMODIFIED,
                                         this);

  
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

  nsAttrValue oldValue;
  rv = mAttrsAndChildren.RemoveAttrAt(index, oldValue);
  NS_ENSURE_SUCCESS(rv, rv);

  if (document || HasFlag(NODE_FORCE_XBL_BINDINGS)) {
    nsIDocument* ownerDoc = GetOwnerDoc();
    if (ownerDoc) {
      nsRefPtr<nsXBLBinding> binding =
        ownerDoc->BindingManager()->GetBinding(this);
      if (binding) {
        binding->AttributeChanged(aName, aNameSpaceID, PR_TRUE, aNotify);
      }
    }
  }

  if (aNotify) {
    stateMask = stateMask ^ PRUint32(IntrinsicState());
    if (stateMask && document) {
      MOZ_AUTO_DOC_UPDATE(document, UPDATE_CONTENT_STATE, aNotify);
      document->ContentStatesChanged(this, nsnull, stateMask);
    }
    nsNodeUtils::AttributeChanged(this, aNameSpaceID, aName,
                                  nsIDOMMutationEvent::REMOVAL);
  }

  rv = AfterSetAttr(aNameSpaceID, aName, nsnull, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasMutationListeners) {
    mozAutoRemovableBlockerRemover blockerRemover(GetOwnerDoc());

    nsCOMPtr<nsIDOMEventTarget> node =
      do_QueryInterface(static_cast<nsIContent *>(this));
    nsMutationEvent mutation(PR_TRUE, NS_MUTATION_ATTRMODIFIED);

    mutation.mRelatedNode = attrNode;
    mutation.mAttrName = aName;

    nsAutoString value;
    oldValue.ToString(value);
    if (!value.IsEmpty())
      mutation.mPrevAttrValue = do_GetAtom(value);
    mutation.mAttrChange = nsIDOMMutationEvent::REMOVAL;

    mozAutoSubtreeModified subtree(GetOwnerDoc(), this);
    nsEventDispatcher::Dispatch(this, nsnull, &mutation);
  }

  return NS_OK;
}

const nsAttrName*
nsGenericElement::GetAttrNameAt(PRUint32 aIndex) const
{
  return mAttrsAndChildren.GetSafeAttrNameAt(aIndex);
}

PRUint32
nsGenericElement::GetAttrCount() const
{
  return mAttrsAndChildren.AttrCount();
}

const nsTextFragment*
nsGenericElement::GetText()
{
  return nsnull;
}

PRUint32
nsGenericElement::TextLength()
{
  
  
  NS_NOTREACHED("called nsGenericElement::TextLength");

  return 0;
}

nsresult
nsGenericElement::SetText(const PRUnichar* aBuffer, PRUint32 aLength,
                          PRBool aNotify)
{
  NS_ERROR("called nsGenericElement::SetText");

  return NS_ERROR_FAILURE;
}

nsresult
nsGenericElement::AppendText(const PRUnichar* aBuffer, PRUint32 aLength,
                             PRBool aNotify)
{
  NS_ERROR("called nsGenericElement::AppendText");

  return NS_ERROR_FAILURE;
}

PRBool
nsGenericElement::TextIsOnlyWhitespace()
{
  return PR_FALSE;
}

void
nsGenericElement::AppendTextTo(nsAString& aResult)
{
  
  
  NS_NOTREACHED("called nsGenericElement::TextLength");
}

#ifdef DEBUG
void
nsGenericElement::ListAttributes(FILE* out) const
{
  PRUint32 index, count = mAttrsAndChildren.AttrCount();
  for (index = 0; index < count; index++) {
    nsAutoString buffer;

    
    mAttrsAndChildren.AttrNameAt(index)->GetQualifiedName(buffer);

    
    buffer.AppendLiteral("=\"");
    nsAutoString value;
    mAttrsAndChildren.AttrAt(index)->ToString(value);
    for (int i = value.Length(); i >= 0; --i) {
      if (value[i] == PRUnichar('"'))
        value.Insert(PRUnichar('\\'), PRUint32(i));
    }
    buffer.Append(value);
    buffer.AppendLiteral("\"");

    fputs(" ", out);
    fputs(NS_LossyConvertUTF16toASCII(buffer).get(), out);
  }
}

void
nsGenericElement::List(FILE* out, PRInt32 aIndent,
                       const nsCString& aPrefix) const
{
  PRInt32 indent;
  for (indent = aIndent; --indent >= 0; ) fputs("  ", out);

  fputs(aPrefix.get(), out);

  nsAutoString buf;
  mNodeInfo->GetQualifiedName(buf);
  fputs(NS_LossyConvertUTF16toASCII(buf).get(), out);

  fprintf(out, "@%p", (void *)this);

  ListAttributes(out);

  fprintf(out, " intrinsicstate=[%08x]", IntrinsicState());
  fprintf(out, " refcount=%d<", mRefCnt.get());

  PRUint32 i, length = GetChildCount();
  if (length > 0) {
    fputs("\n", out);

    for (i = 0; i < length; ++i) {
      nsIContent *kid = GetChildAt(i);
      kid->List(out, aIndent + 1);
    }

    for (indent = aIndent; --indent >= 0; ) fputs("  ", out);
  }

  fputs(">\n", out);
  
  nsGenericElement* nonConstThis = const_cast<nsGenericElement*>(this);

  
  nsIDocument *document = GetOwnerDoc();
  if (document) {
    

    nsBindingManager* bindingManager = document->BindingManager();
    nsCOMPtr<nsIDOMNodeList> anonymousChildren;
    bindingManager->GetAnonymousNodesFor(nonConstThis,
                                         getter_AddRefs(anonymousChildren));

    if (anonymousChildren) {
      anonymousChildren->GetLength(&length);
      if (length > 0) {
        for (indent = aIndent; --indent >= 0; ) fputs("  ", out);
        fputs("anonymous-children<\n", out);

        for (i = 0; i < length; ++i) {
          nsCOMPtr<nsIDOMNode> node;
          anonymousChildren->Item(i, getter_AddRefs(node));
          nsCOMPtr<nsIContent> child = do_QueryInterface(node);
          child->List(out, aIndent + 1);
        }

        for (indent = aIndent; --indent >= 0; ) fputs("  ", out);
        fputs(">\n", out);
      }
    }

    if (bindingManager->HasContentListFor(nonConstThis)) {
      nsCOMPtr<nsIDOMNodeList> contentList;
      bindingManager->GetContentListFor(nonConstThis,
                                        getter_AddRefs(contentList));

      NS_ASSERTION(contentList != nsnull, "oops, binding manager lied");

      contentList->GetLength(&length);
      if (length > 0) {
        for (indent = aIndent; --indent >= 0; ) fputs("  ", out);
        fputs("content-list<\n", out);

        for (i = 0; i < length; ++i) {
          nsCOMPtr<nsIDOMNode> node;
          contentList->Item(i, getter_AddRefs(node));
          nsCOMPtr<nsIContent> child = do_QueryInterface(node);
          child->List(out, aIndent + 1);
        }

        for (indent = aIndent; --indent >= 0; ) fputs("  ", out);
        fputs(">\n", out);
      }
    }
  }
}

void
nsGenericElement::DumpContent(FILE* out, PRInt32 aIndent,
                              PRBool aDumpAll) const
{
  PRInt32 indent;
  for (indent = aIndent; --indent >= 0; ) fputs("  ", out);

  nsAutoString buf;
  mNodeInfo->GetQualifiedName(buf);
  fputs("<", out);
  fputs(NS_LossyConvertUTF16toASCII(buf).get(), out);

  if(aDumpAll) ListAttributes(out);

  fputs(">", out);

  if(aIndent) fputs("\n", out);

  PRInt32 index, kids = GetChildCount();
  for (index = 0; index < kids; index++) {
    nsIContent *kid = GetChildAt(index);
    PRInt32 indent = aIndent ? aIndent + 1 : 0;
    kid->DumpContent(out, indent, aDumpAll);
  }
  for (indent = aIndent; --indent >= 0; ) fputs("  ", out);
  fputs("</", out);
  fputs(NS_LossyConvertUTF16toASCII(buf).get(), out);
  fputs(">", out);

  if(aIndent) fputs("\n", out);
}
#endif

PRUint32
nsGenericElement::GetChildCount() const
{
  return mAttrsAndChildren.ChildCount();
}

nsIContent *
nsGenericElement::GetChildAt(PRUint32 aIndex) const
{
  return mAttrsAndChildren.GetSafeChildAt(aIndex);
}

nsIContent * const *
nsGenericElement::GetChildArray(PRUint32* aChildCount) const
{
  return mAttrsAndChildren.GetChildArray(aChildCount);
}

PRInt32
nsGenericElement::IndexOf(nsINode* aPossibleChild) const
{
  return mAttrsAndChildren.IndexOfChild(aPossibleChild);
}

nsINode::nsSlots*
nsGenericElement::CreateSlots()
{
  return new nsDOMSlots(mFlagsOrSlots);
}

PRBool
nsGenericElement::CheckHandleEventForLinksPrecondition(nsEventChainVisitor& aVisitor,
                                                       nsIURI** aURI) const
{
  if (aVisitor.mEventStatus == nsEventStatus_eConsumeNoDefault ||
      !NS_IS_TRUSTED_EVENT(aVisitor.mEvent) ||
      !aVisitor.mPresContext) {
    return PR_FALSE;
  }

  
  return IsLink(aURI);
}

nsresult
nsGenericElement::PreHandleEventForLinks(nsEventChainPreVisitor& aVisitor)
{
  
  
  switch (aVisitor.mEvent->message) {
  case NS_MOUSE_ENTER_SYNTH:
  case NS_FOCUS_CONTENT:
  case NS_MOUSE_EXIT_SYNTH:
  case NS_BLUR_CONTENT:
    break;
  default:
    return NS_OK;
  }

  
  nsCOMPtr<nsIURI> absURI;
  if (!CheckHandleEventForLinksPrecondition(aVisitor, getter_AddRefs(absURI))) {
    return NS_OK;
  }

  nsresult rv = NS_OK;

  
  
  switch (aVisitor.mEvent->message) {
  
  case NS_MOUSE_ENTER_SYNTH:
    aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
    
  case NS_FOCUS_CONTENT:
    {
      nsAutoString target;
      GetLinkTarget(target);
      nsContentUtils::TriggerLink(this, aVisitor.mPresContext, absURI, target,
                                  PR_FALSE, PR_TRUE);
    }
    break;

  case NS_MOUSE_EXIT_SYNTH:
    aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
    
  case NS_BLUR_CONTENT:
    rv = LeaveLink(aVisitor.mPresContext);
    break;

  default:
    
    NS_NOTREACHED("switch statements not in sync");
    return NS_ERROR_UNEXPECTED;
  }

  return rv;
}

nsresult
nsGenericElement::PostHandleEventForLinks(nsEventChainPostVisitor& aVisitor)
{
  
  
  switch (aVisitor.mEvent->message) {
  case NS_MOUSE_BUTTON_DOWN:
  case NS_MOUSE_CLICK:
  case NS_UI_ACTIVATE:
  case NS_KEY_PRESS:
    break;
  default:
    return NS_OK;
  }

  
  nsCOMPtr<nsIURI> absURI;
  if (!CheckHandleEventForLinksPrecondition(aVisitor, getter_AddRefs(absURI))) {
    return NS_OK;
  }

  nsresult rv = NS_OK;

  switch (aVisitor.mEvent->message) {
  case NS_MOUSE_BUTTON_DOWN:
    {
      if (aVisitor.mEvent->eventStructType == NS_MOUSE_EVENT &&
          static_cast<nsMouseEvent*>(aVisitor.mEvent)->button ==
          nsMouseEvent::eLeftButton) {
        
        nsILinkHandler *handler = aVisitor.mPresContext->GetLinkHandler();
        nsIDocument *document = GetCurrentDoc();
        if (handler && document) {
          nsIFocusManager* fm = nsFocusManager::GetFocusManager();
          if (fm) {
            nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(this);
            fm->SetFocus(elem, nsIFocusManager::FLAG_BYMOUSE |
                               nsIFocusManager::FLAG_NOSCROLL);
          }

          aVisitor.mPresContext->EventStateManager()->
            SetContentState(this, NS_EVENT_STATE_ACTIVE);
        }
      }
    }
    break;

  case NS_MOUSE_CLICK:
    if (NS_IS_MOUSE_LEFT_CLICK(aVisitor.mEvent)) {
      nsInputEvent* inputEvent = static_cast<nsInputEvent*>(aVisitor.mEvent);
      if (inputEvent->isControl || inputEvent->isMeta ||
          inputEvent->isAlt ||inputEvent->isShift) {
        break;
      }

      
      nsCOMPtr<nsIPresShell> shell = aVisitor.mPresContext->GetPresShell();
      if (shell) {
        
        nsEventStatus status = nsEventStatus_eIgnore;
        nsUIEvent actEvent(NS_IS_TRUSTED_EVENT(aVisitor.mEvent),
                           NS_UI_ACTIVATE, 1);

        rv = shell->HandleDOMEventWithTarget(this, &actEvent, &status);
      }
    }
    break;

  case NS_UI_ACTIVATE:
    {
      nsAutoString target;
      GetLinkTarget(target);
      nsContentUtils::TriggerLink(this, aVisitor.mPresContext, absURI, target,
                                  PR_TRUE, PR_TRUE);
    }
    break;

  case NS_KEY_PRESS:
    {
      if (aVisitor.mEvent->eventStructType == NS_KEY_EVENT) {
        nsKeyEvent* keyEvent = static_cast<nsKeyEvent*>(aVisitor.mEvent);
        if (keyEvent->keyCode == NS_VK_RETURN) {
          nsEventStatus status = nsEventStatus_eIgnore;
          rv = DispatchClickEvent(aVisitor.mPresContext, keyEvent, this,
                                  PR_FALSE, &status);
          if (NS_SUCCEEDED(rv)) {
            aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
          }
        }
      }
    }
    break;

  default:
    
    NS_NOTREACHED("switch statements not in sync");
    return NS_ERROR_UNEXPECTED;
  }

  return rv;
}

void
nsGenericElement::GetLinkTarget(nsAString& aTarget)
{
  aTarget.Truncate();
}


static nsresult
ParseSelectorList(nsINode* aNode,
                  const nsAString& aSelectorString,
                  nsCSSSelectorList** aSelectorList,
                  nsPresContext** aPresContext)
{
  NS_ENSURE_ARG(aNode);

  nsIDocument* doc = aNode->GetOwnerDoc();
  NS_ENSURE_STATE(doc);

  nsCOMPtr<nsICSSParser> parser;
  nsresult rv = doc->CSSLoader()->GetParserFor(nsnull, getter_AddRefs(parser));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = parser->ParseSelectorString(aSelectorString,
                                   doc->GetDocumentURI(),
                                   0, 
                                   aSelectorList);
  doc->CSSLoader()->RecycleParser(parser);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  *aPresContext = nsnull;
  nsIPresShell* shell = doc->GetPrimaryShell();
  if (shell) {
    *aPresContext = shell->GetPresContext();
  }

  return NS_OK;
}





typedef PRBool
(* ElementMatchedCallback)(nsIContent* aMatchingElement, void* aClosure);


static PRBool
TryMatchingElementsInSubtree(nsINode* aRoot,
                             RuleProcessorData* aParentData,
                             nsPresContext* aPresContext,
                             nsCSSSelectorList* aSelectorList,
                             ElementMatchedCallback aCallback,
                             void* aClosure)
{
  




  char databuf[2 * sizeof(RuleProcessorData)];
  RuleProcessorData* prevSibling = nsnull;
  RuleProcessorData* data = reinterpret_cast<RuleProcessorData*>(databuf);

  PRBool continueIteration = PR_TRUE;
  for (nsINode::ChildIterator iter(aRoot); !iter.IsDone(); iter.Next()) {
    nsIContent* kid = iter;
    if (!kid->IsNodeOfType(nsINode::eELEMENT)) {
      continue;
    }
    
    new (data) RuleProcessorData(aPresContext, kid, nsnull);
    NS_ASSERTION(!data->mParentData, "Shouldn't happen");
    NS_ASSERTION(!data->mPreviousSiblingData, "Shouldn't happen");
    data->mParentData = aParentData;
    data->mPreviousSiblingData = prevSibling;

    if (nsCSSRuleProcessor::SelectorListMatches(*data, aSelectorList)) {
      continueIteration = (*aCallback)(kid, aClosure);
    }

    if (continueIteration) {
      continueIteration =
        TryMatchingElementsInSubtree(kid, data, aPresContext, aSelectorList,
                                     aCallback, aClosure);
    }
    
    






    NS_ASSERTION(!aParentData || data->mParentData == aParentData,
                 "Unexpected parent");
    NS_ASSERTION(data->mPreviousSiblingData == prevSibling,
                 "Unexpected prev sibling");
    data->mPreviousSiblingData = nsnull;
    if (prevSibling) {
      if (aParentData) {
        prevSibling->mParentData = nsnull;
      }
      prevSibling->~RuleProcessorData();
    } else {
      

      prevSibling = data + 1;
    }

    
    RuleProcessorData* temp = prevSibling;
    prevSibling = data;
    data = temp;
    if (!continueIteration) {
      break;
    }
  }
  if (prevSibling) {
    if (aParentData) {
      prevSibling->mParentData = nsnull;
    }
    
    prevSibling->~RuleProcessorData();
  }

  return continueIteration;
}

static PRBool
FindFirstMatchingElement(nsIContent* aMatchingElement,
                         void* aClosure)
{
  NS_PRECONDITION(aMatchingElement && aClosure, "How did that happen?");
  nsIContent** slot = static_cast<nsIContent**>(aClosure);
  *slot = aMatchingElement;
  return PR_FALSE;
}


nsresult
nsGenericElement::doQuerySelector(nsINode* aRoot, const nsAString& aSelector,
                                  nsIDOMElement **aReturn)
{
  NS_PRECONDITION(aReturn, "Null out param?");

  nsAutoPtr<nsCSSSelectorList> selectorList;
  nsPresContext* presContext;
  nsresult rv = ParseSelectorList(aRoot, aSelector,
                                  getter_Transfers(selectorList),
                                  &presContext);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIContent* foundElement = nsnull;
  TryMatchingElementsInSubtree(aRoot, nsnull, presContext, selectorList,
                               FindFirstMatchingElement, &foundElement);

  if (foundElement) {
    return CallQueryInterface(foundElement, aReturn);
  }

  *aReturn = nsnull;
  return NS_OK;
}

static PRBool
AppendAllMatchingElements(nsIContent* aMatchingElement,
                          void* aClosure)
{
  NS_PRECONDITION(aMatchingElement && aClosure, "How did that happen?");
  static_cast<nsBaseContentList*>(aClosure)->AppendElement(aMatchingElement);
  return PR_TRUE;
}


nsresult
nsGenericElement::doQuerySelectorAll(nsINode* aRoot,
                                     const nsAString& aSelector,
                                     nsIDOMNodeList **aReturn)
{
  NS_PRECONDITION(aReturn, "Null out param?");

  nsBaseContentList* contentList = new nsBaseContentList();
  NS_ENSURE_TRUE(contentList, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aReturn = contentList);
  
  nsAutoPtr<nsCSSSelectorList> selectorList;
  nsPresContext* presContext;
  nsresult rv = ParseSelectorList(aRoot, aSelector,
                                  getter_Transfers(selectorList),
                                  &presContext);
  NS_ENSURE_SUCCESS(rv, rv);

  TryMatchingElementsInSubtree(aRoot, nsnull, presContext, selectorList,
                               AppendAllMatchingElements, contentList);
  return NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::MozMatchesSelector(const nsAString& aSelector, PRBool* aReturn)
{
  NS_PRECONDITION(aReturn, "Null out param?");
  *aReturn = nsGenericElement::doMatchesSelector(mContent, aSelector);
  return NS_OK;
}


PRBool
nsGenericElement::doMatchesSelector(nsIContent* aNode, const nsAString& aSelector)
{
  nsAutoPtr<nsCSSSelectorList> selectorList;
  nsPresContext* presContext;
  PRBool matches = PR_FALSE;

  if (NS_SUCCEEDED(ParseSelectorList(aNode, aSelector,
                                     getter_Transfers(selectorList),
                                     &presContext)))
  {
    RuleProcessorData data(presContext, aNode, nsnull);
    matches = nsCSSRuleProcessor::SelectorListMatches(data, selectorList);
  }

  return matches;
}
