












































#include "nsGenericElement.h"

#include "nsDOMAttribute.h"
#include "nsDOMAttributeMap.h"
#include "nsIAtom.h"
#include "nsINodeInfo.h"
#include "nsIDocument.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIContentIterator.h"
#include "nsEventListenerManager.h"
#include "nsFocusManager.h"
#include "nsILinkHandler.h"
#include "nsIScriptGlobalObject.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsStyleConsts.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsEventStateManager.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsDOMCID.h"
#include "nsIServiceManager.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsDOMCSSAttrDeclaration.h"
#include "nsINameSpaceManager.h"
#include "nsContentList.h"
#include "nsDOMTokenList.h"
#include "nsXBLPrototypeBinding.h"
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
#include "nsIXBLService.h"
#include "nsPIDOMWindow.h"
#include "nsPIBoxObject.h"
#include "nsIDOMNSElement.h"
#include "nsClientRect.h"
#include "nsSVGUtils.h"
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
#include "nsIDOMDocumentType.h"
#include "nsIDOMUserDataHandler.h"
#include "nsGenericHTMLElement.h"
#include "nsIEditor.h"
#include "nsIEditorIMESupport.h"
#include "nsIEditorDocShell.h"
#include "nsEventDispatcher.h"
#include "nsContentCreatorFunctions.h"
#include "nsIControllers.h"
#include "nsLayoutUtils.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIScrollableFrame.h"
#include "nsXBLInsertionPoint.h"
#include "mozilla/css/StyleRule.h" 
#include "nsCSSRuleProcessor.h"
#include "nsRuleProcessorData.h"
#include "nsPLDOMEvent.h"
#include "nsTextNode.h"
#include "dombindings.h"

#ifdef MOZ_XUL
#include "nsIXULDocument.h"
#endif 

#include "nsCycleCollectionParticipant.h"
#include "nsCCUncollectableMarker.h"

#include "mozAutoDocUpdate.h"

#include "nsCSSParser.h"
#include "prprf.h"

#include "nsSVGFeatures.h"
#include "nsDOMMemoryReporter.h"
#include "nsWrapperCacheInlines.h"

#include "xpcpublic.h"

using namespace mozilla::dom;
namespace css = mozilla::css;

NS_DEFINE_IID(kThisPtrOffsetsSID, NS_THISPTROFFSETS_SID);

PRInt32 nsIContent::sTabFocusModel = eTabFocus_any;
bool nsIContent::sTabFocusModelAppliesToXUL = false;
PRUint32 nsMutationGuard::sMutationCount = 0;

nsresult NS_NewContentIterator(nsIContentIterator** aInstancePtrResult);

void
nsWrapperCache::RemoveExpandoObject()
{
  JSObject *expando = GetExpandoObjectPreserveColor();
  if (expando) {
    JSCompartment *compartment = js::GetObjectCompartment(expando);
    xpc::CompartmentPrivate *priv =
      static_cast<xpc::CompartmentPrivate *>(js_GetCompartmentPrivate(compartment));
    priv->RemoveDOMExpandoObject(expando);
  }
}



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

void
nsINode::nsSlots::Traverse(nsCycleCollectionTraversalCallback &cb)
{
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mSlots->mChildNodes");
  cb.NoteXPCOMChild(mChildNodes);
}

void
nsINode::nsSlots::Unlink()
{
  if (mChildNodes) {
    mChildNodes->DropReference();
    NS_RELEASE(mChildNodes);
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

  return doc->PropertyTable(aCategory)->GetProperty(this, aPropertyName,
                                                    aStatus);
}

nsresult
nsINode::SetProperty(PRUint16 aCategory, nsIAtom *aPropertyName, void *aValue,
                     NSPropertyDtorFunc aDtor, bool aTransfer,
                     void **aOldValue)
{
  nsIDocument *doc = GetOwnerDoc();
  if (!doc)
    return NS_ERROR_FAILURE;

  nsresult rv = doc->PropertyTable(aCategory)->SetProperty(this,
                                                           aPropertyName, aValue, aDtor,
                                                           nsnull, aTransfer, aOldValue);
  if (NS_SUCCEEDED(rv)) {
    SetFlags(NODE_HAS_PROPERTIES);
  }

  return rv;
}

void
nsINode::DeleteProperty(PRUint16 aCategory, nsIAtom *aPropertyName)
{
  nsIDocument *doc = GetOwnerDoc();
  if (doc)
    doc->PropertyTable(aCategory)->DeleteProperty(this, aPropertyName);
}

void*
nsINode::UnsetProperty(PRUint16 aCategory, nsIAtom *aPropertyName,
                       nsresult *aStatus)
{
  nsIDocument *doc = GetOwnerDoc();
  if (!doc)
    return nsnull;

  return doc->PropertyTable(aCategory)->UnsetProperty(this, aPropertyName,
                                                      aStatus);
}

nsINode::nsSlots*
nsINode::CreateSlots()
{
  return new nsSlots();
}

bool
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
    if (!node->IsElement() ||
        !node->AsElement()->IsHTML())
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
  bool isEditable;
  if (!editorDocShell ||
      NS_FAILED(editorDocShell->GetEditable(&isEditable)) || !isEditable)
    return nsnull;

  nsCOMPtr<nsIEditor> editor;
  editorDocShell->GetEditor(getter_AddRefs(editor));
  return editor;
}

static nsIContent* GetRootForContentSubtree(nsIContent* aContent)
{
  NS_ENSURE_TRUE(aContent, nsnull);
  nsIContent* stop = aContent->GetBindingParent();
  while (aContent) {
    nsIContent* parent = aContent->GetParent();
    if (parent == stop) {
      break;
    }
    aContent = parent;
  }
  return aContent;
}

nsIContent*
nsINode::GetSelectionRootContent(nsIPresShell* aPresShell)
{
  NS_ENSURE_TRUE(aPresShell, nsnull);

  if (IsNodeOfType(eDOCUMENT))
    return static_cast<nsIDocument*>(this)->GetRootElement();
  if (!IsNodeOfType(eCONTENT))
    return nsnull;

  if (GetCurrentDoc() != aPresShell->GetDocument()) {
    return nsnull;
  }

  if (static_cast<nsIContent*>(this)->HasIndependentSelection()) {
    
    nsIContent* content = GetTextEditorRootContent();
    if (content)
      return content;
  }

  nsPresContext* presContext = aPresShell->GetPresContext();
  if (presContext) {
    nsIEditor* editor = GetHTMLEditor(presContext);
    if (editor) {
      
      nsIDocument* doc = GetCurrentDoc();
      if (!doc || doc->HasFlag(NODE_IS_EDITABLE) ||
          !HasFlag(NODE_IS_EDITABLE)) {
        nsIContent* editorRoot = GetEditorRootContent(editor);
        NS_ENSURE_TRUE(editorRoot, nsnull);
        return nsContentUtils::IsInSameAnonymousTree(this, editorRoot) ?
                 editorRoot :
                 GetRootForContentSubtree(static_cast<nsIContent*>(this));
      }
      
      
      return static_cast<nsIContent*>(this)->GetEditingHost();
    }
  }

  nsRefPtr<nsFrameSelection> fs = aPresShell->FrameSelection();
  nsIContent* content = fs->GetLimiter();
  if (!content) {
    content = fs->GetAncestorLimiter();
    if (!content) {
      nsIDocument* doc = aPresShell->GetDocument();
      NS_ENSURE_TRUE(doc, nsnull);
      content = doc->GetRootElement();
      if (!content)
        return nsnull;
    }
  }

  
  
  NS_ENSURE_TRUE(content, nsnull);
  return nsContentUtils::IsInSameAnonymousTree(this, content) ?
           content : GetRootForContentSubtree(static_cast<nsIContent*>(this));
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
nsINode::GetParentElement(nsIDOMElement** aParentElement)
{
  *aParentElement = nsnull;
  nsINode* parent = GetElementParent();
  return parent ? CallQueryInterface(parent, aParentElement) : NS_OK;
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
  nsIContent* child = GetFirstChild();
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

  nsIContent *sibling = GetPreviousSibling();

  return sibling ? CallQueryInterface(sibling, aPrevSibling) : NS_OK;
}

nsresult
nsINode::GetNextSibling(nsIDOMNode** aNextSibling)
{
  *aNextSibling = nsnull;

  nsIContent *sibling = GetNextSibling();

  return sibling ? CallQueryInterface(sibling, aNextSibling) : NS_OK;
}

nsresult
nsINode::GetOwnerDocument(nsIDOMDocument** aOwnerDocument)
{
  *aOwnerDocument = nsnull;

  nsIDocument *ownerDoc = GetOwnerDocument();

  return ownerDoc ? CallQueryInterface(ownerDoc, aOwnerDocument) : NS_OK;
}

nsresult
nsINode::RemoveChild(nsINode *aOldChild)
{
  if (!aOldChild) {
    return NS_ERROR_NULL_POINTER;
  }

  if (IsNodeOfType(eDATA_NODE)) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  if (aOldChild && aOldChild->GetNodeParent() == this) {
    nsContentUtils::MaybeFireNodeRemoved(aOldChild, this, GetOwnerDoc());
  }

  PRInt32 index = IndexOf(aOldChild);
  if (index == -1) {
    
    return NS_ERROR_DOM_NOT_FOUND_ERR;
  }

  return RemoveChildAt(index, PR_TRUE);
}

nsresult
nsINode::ReplaceOrInsertBefore(bool aReplace, nsIDOMNode* aNewChild,
                               nsIDOMNode* aRefChild, nsIDOMNode** aReturn)
{
  nsCOMPtr<nsINode> newChild = do_QueryInterface(aNewChild);

  nsresult rv;
  nsCOMPtr<nsINode> refChild;
  if (aRefChild) {
      refChild = do_QueryInterface(aRefChild, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = ReplaceOrInsertBefore(aReplace, newChild, refChild);
  if (NS_SUCCEEDED(rv)) {
    NS_ADDREF(*aReturn = aReplace ? aRefChild : aNewChild);
  }

  return rv;
}

nsresult
nsINode::RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
  nsCOMPtr<nsIContent> oldChild = do_QueryInterface(aOldChild);
  nsresult rv = RemoveChild(oldChild);
  if (NS_SUCCEEDED(rv)) {
    NS_ADDREF(*aReturn = aOldChild);
  }
  return rv;
}

nsresult
nsINode::Normalize()
{
  
  nsAutoTArray<nsCOMPtr<nsIContent>, 50> nodes;

  bool canMerge = false;
  for (nsIContent* node = this->GetFirstChild();
       node;
       node = node->GetNextNode(this)) {
    if (node->NodeType() != nsIDOMNode::TEXT_NODE) {
      canMerge = PR_FALSE;
      continue;
    }

    if (canMerge || node->TextLength() == 0) {
      
      
      nodes.AppendElement(node);
    }
    else {
      canMerge = PR_TRUE;
    }

    
    
    canMerge = canMerge && !!node->GetNextSibling();
  }

  if (nodes.IsEmpty()) {
    return NS_OK;
  }

  
  nsIDocument* doc = GetOwnerDoc();

  
  mozAutoSubtreeModified subtree(doc, nsnull);

  
  
  bool hasRemoveListeners = nsContentUtils::
      HasMutationListeners(doc, NS_EVENT_BITS_MUTATION_NODEREMOVED);
  if (hasRemoveListeners) {
    for (PRUint32 i = 0; i < nodes.Length(); ++i) {
      nsContentUtils::MaybeFireNodeRemoved(nodes[i], nodes[i]->GetNodeParent(),
                                           doc);
    }
  }

  mozAutoDocUpdate batch(doc, UPDATE_CONTENT_MODEL, PR_TRUE);

  
  nsAutoString tmpStr;
  for (PRUint32 i = 0; i < nodes.Length(); ++i) {
    nsIContent* node = nodes[i];
    
    const nsTextFragment* text = node->GetText();
    if (text->GetLength()) {
      nsIContent* target = node->GetPreviousSibling();
      NS_ASSERTION((target && target->NodeType() == nsIDOMNode::TEXT_NODE) ||
                   hasRemoveListeners,
                   "Should always have a previous text sibling unless "
                   "mutation events messed us up");
      if (!hasRemoveListeners ||
          (target && target->NodeType() == nsIDOMNode::TEXT_NODE)) {
        nsTextNode* t = static_cast<nsTextNode*>(target);
        if (text->Is2b()) {
          t->AppendTextForNormalize(text->Get2b(), text->GetLength(), PR_TRUE, node);
        }
        else {
          tmpStr.Truncate();
          text->AppendTo(tmpStr);
          t->AppendTextForNormalize(tmpStr.get(), tmpStr.Length(), PR_TRUE, node);
        }
      }
    }

    
    nsCOMPtr<nsINode> parent = node->GetNodeParent();
    NS_ASSERTION(parent || hasRemoveListeners,
                 "Should always have a parent unless "
                 "mutation events messed us up");
    if (parent) {
      parent->RemoveChildAt(parent->IndexOf(node), PR_TRUE);
    }
  }

  return NS_OK;
}

nsresult
nsINode::GetDOMBaseURI(nsAString &aURI) const
{
  nsCOMPtr<nsIURI> baseURI = GetBaseURI();

  nsCAutoString spec;
  if (baseURI) {
    baseURI->GetSpec(spec);
  }

  CopyUTF8toUTF16(spec, aURI);

  return NS_OK;
}

nsresult
nsINode::LookupPrefix(const nsAString& aNamespaceURI, nsAString& aPrefix)
{
  Element *element = GetNameSpaceElement();
  if (element) {
    
  
    
    
    
    for (nsIContent* content = element; content;
         content = content->GetParent()) {
      PRUint32 attrCount = content->GetAttrCount();
  
      for (PRUint32 i = 0; i < attrCount; ++i) {
        const nsAttrName* name = content->GetAttrNameAt(i);
  
        if (name->NamespaceEquals(kNameSpaceID_XMLNS) &&
            content->AttrValueIs(kNameSpaceID_XMLNS, name->LocalName(),
                                 aNamespaceURI, eCaseMatters)) {
          
          
          nsIAtom *localName = name->LocalName();
  
          if (localName != nsGkAtoms::xmlns) {
            localName->ToString(aPrefix);
          }
          else {
            SetDOMStringToNull(aPrefix);
          }
          return NS_OK;
        }
      }
    }
  }

  SetDOMStringToNull(aPrefix);

  return NS_OK;
}

static nsresult
SetUserDataProperty(PRUint16 aCategory, nsINode *aNode, nsIAtom *aKey,
                    nsISupports* aValue, void** aOldValue)
{
  nsresult rv = aNode->SetProperty(aCategory, aKey, aValue,
                                   nsPropertyTable::SupportsDtorFunc, PR_TRUE,
                                   aOldValue);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NS_ADDREF(aValue);

  return NS_OK;
}

nsresult
nsINode::SetUserData(const nsAString &aKey, nsIVariant *aData,
                     nsIDOMUserDataHandler *aHandler, nsIVariant **aResult)
{
  *aResult = nsnull;

  nsCOMPtr<nsIAtom> key = do_GetAtom(aKey);
  if (!key) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv;
  void *data;
  if (aData) {
    rv = SetUserDataProperty(DOM_USER_DATA, this, key, aData, &data);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    data = UnsetProperty(DOM_USER_DATA, key);
  }

  
  nsCOMPtr<nsIVariant> oldData = dont_AddRef(static_cast<nsIVariant*>(data));

  if (aData && aHandler) {
    nsCOMPtr<nsIDOMUserDataHandler> oldHandler;
    rv = SetUserDataProperty(DOM_USER_DATA_HANDLER, this, key, aHandler,
                             getter_AddRefs(oldHandler));
    if (NS_FAILED(rv)) {
      
      DeleteProperty(DOM_USER_DATA, key);

      return rv;
    }
  }
  else {
    DeleteProperty(DOM_USER_DATA_HANDLER, key);
  }

  oldData.swap(*aResult);

  return NS_OK;
}

PRUint16
nsINode::CompareDocPosition(nsINode* aOtherNode)
{
  NS_PRECONDITION(aOtherNode, "don't pass null");

  if (this == aOtherNode) {
    return 0;
  }

  nsAutoTArray<nsINode*, 32> parents1, parents2;

  nsINode *node1 = aOtherNode, *node2 = this;

  
  nsIAttribute* attr1 = nsnull;
  if (node1->IsNodeOfType(nsINode::eATTRIBUTE)) {
    attr1 = static_cast<nsIAttribute*>(node1);
    nsIContent* elem = attr1->GetContent();
    
    
    if (elem) {
      node1 = elem;
      parents1.AppendElement(static_cast<nsINode*>(attr1));
    }
  }
  if (node2->IsNodeOfType(nsINode::eATTRIBUTE)) {
    nsIAttribute* attr2 = static_cast<nsIAttribute*>(node2);
    nsIContent* elem = attr2->GetContent();
    if (elem == node1 && attr1) {
      
      

      PRUint32 i;
      const nsAttrName* attrName;
      for (i = 0; (attrName = elem->GetAttrNameAt(i)); ++i) {
        if (attrName->Equals(attr1->NodeInfo())) {
          NS_ASSERTION(!attrName->Equals(attr2->NodeInfo()),
                       "Different attrs at same position");
          return nsIDOMNode::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC |
            nsIDOMNode::DOCUMENT_POSITION_PRECEDING;
        }
        if (attrName->Equals(attr2->NodeInfo())) {
          return nsIDOMNode::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC |
            nsIDOMNode::DOCUMENT_POSITION_FOLLOWING;
        }
      }
      NS_NOTREACHED("neither attribute in the element");
      return nsIDOMNode::DOCUMENT_POSITION_DISCONNECTED;
    }

    if (elem) {
      node2 = elem;
      parents2.AppendElement(static_cast<nsINode*>(attr2));
    }
  }

  
  
  
  

  
  do {
    parents1.AppendElement(node1);
    node1 = node1->GetNodeParent();
  } while (node1);
  do {
    parents2.AppendElement(node2);
    node2 = node2->GetNodeParent();
  } while (node2);

  
  PRUint32 pos1 = parents1.Length();
  PRUint32 pos2 = parents2.Length();
  nsINode* top1 = parents1.ElementAt(--pos1);
  nsINode* top2 = parents2.ElementAt(--pos2);
  if (top1 != top2) {
    return top1 < top2 ?
      (nsIDOMNode::DOCUMENT_POSITION_PRECEDING |
       nsIDOMNode::DOCUMENT_POSITION_DISCONNECTED |
       nsIDOMNode::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC) :
      (nsIDOMNode::DOCUMENT_POSITION_FOLLOWING |
       nsIDOMNode::DOCUMENT_POSITION_DISCONNECTED |
       nsIDOMNode::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC);
  }

  
  nsINode* parent = top1;
  PRUint32 len;
  for (len = NS_MIN(pos1, pos2); len > 0; --len) {
    nsINode* child1 = parents1.ElementAt(--pos1);
    nsINode* child2 = parents2.ElementAt(--pos2);
    if (child1 != child2) {
      
      
      
      return parent->IndexOf(child1) < parent->IndexOf(child2) ?
        static_cast<PRUint16>(nsIDOMNode::DOCUMENT_POSITION_PRECEDING) :
        static_cast<PRUint16>(nsIDOMNode::DOCUMENT_POSITION_FOLLOWING);
    }
    parent = child1;
  }

  
  
  
  return pos1 < pos2 ?
    (nsIDOMNode::DOCUMENT_POSITION_PRECEDING |
     nsIDOMNode::DOCUMENT_POSITION_CONTAINS) :
    (nsIDOMNode::DOCUMENT_POSITION_FOLLOWING |
     nsIDOMNode::DOCUMENT_POSITION_CONTAINED_BY);    
}

bool
nsINode::IsEqualTo(nsINode* aOther)
{
  if (!aOther) {
    return PR_FALSE;
  }

  nsAutoString string1, string2;

  nsINode* node1 = this;
  nsINode* node2 = aOther;
  do {
    PRUint16 nodeType = node1->NodeType();
    if (nodeType != node2->NodeType()) {
      return PR_FALSE;
    }

    nsINodeInfo* nodeInfo1 = node1->mNodeInfo;
    nsINodeInfo* nodeInfo2 = node2->mNodeInfo;
    if (!nodeInfo1->Equals(nodeInfo2) ||
        nodeInfo1->GetExtraName() != nodeInfo2->GetExtraName()) {
      return PR_FALSE;
    }

    switch(nodeType) {
      case nsIDOMNode::ELEMENT_NODE:
      {
        
        
        Element* element1 = node1->AsElement();
        Element* element2 = node2->AsElement();
        PRUint32 attrCount = element1->GetAttrCount();
        if (attrCount != element2->GetAttrCount()) {
          return PR_FALSE;
        }

        
        for (PRUint32 i = 0; i < attrCount; ++i) {
          const nsAttrName* attrName = element1->GetAttrNameAt(i);
#ifdef DEBUG
          bool hasAttr =
#endif
          element1->GetAttr(attrName->NamespaceID(), attrName->LocalName(),
                            string1);
          NS_ASSERTION(hasAttr, "Why don't we have an attr?");
    
          if (!element2->AttrValueIs(attrName->NamespaceID(),
                                     attrName->LocalName(),
                                     string1,
                                     eCaseMatters)) {
            return PR_FALSE;
          }
        }
        break;
      }
      case nsIDOMNode::TEXT_NODE:
      case nsIDOMNode::COMMENT_NODE:
      case nsIDOMNode::CDATA_SECTION_NODE:
      case nsIDOMNode::PROCESSING_INSTRUCTION_NODE:
      {
        string1.Truncate();
        static_cast<nsIContent*>(node1)->AppendTextTo(string1);
        string2.Truncate();
        static_cast<nsIContent*>(node2)->AppendTextTo(string2);

        if (!string1.Equals(string2)) {
          return PR_FALSE;
        }

        break;
      }
      case nsIDOMNode::DOCUMENT_NODE:
      case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
        break;
      case nsIDOMNode::ATTRIBUTE_NODE:
      {
        NS_ASSERTION(node1 == this && node2 == aOther,
                     "Did we come upon an attribute node while walking a "
                     "subtree?");
        nsCOMPtr<nsIDOMNode> domNode1 = do_QueryInterface(node1);
        nsCOMPtr<nsIDOMNode> domNode2 = do_QueryInterface(node2);
        domNode1->GetNodeValue(string1);
        domNode2->GetNodeValue(string2);
        
        
        
        
        return string1.Equals(string2);
      }
      case nsIDOMNode::DOCUMENT_TYPE_NODE:
      {
        nsCOMPtr<nsIDOMDocumentType> docType1 = do_QueryInterface(node1);
        nsCOMPtr<nsIDOMDocumentType> docType2 = do_QueryInterface(node2);
    
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

        break;
      }
      default:
        NS_ABORT_IF_FALSE(PR_FALSE, "Unknown node type");
    }

    nsINode* nextNode = node1->GetFirstChild();
    if (nextNode) {
      node1 = nextNode;
      node2 = node2->GetFirstChild();
    }
    else {
      if (node2->GetFirstChild()) {
        
        return PR_FALSE;
      }

      
      while (1) {
        if (node1 == this) {
          NS_ASSERTION(node2 == aOther, "Should have reached the start node "
                                        "for both trees at the same time");
          return PR_TRUE;
        }

        nextNode = node1->GetNextSibling();
        if (nextNode) {
          node1 = nextNode;
          node2 = node2->GetNextSibling();
          break;
        }

        if (node2->GetNextSibling()) {
          
          return PR_FALSE;
        }
        
        node1 = node1->GetNodeParent();
        node2 = node2->GetNodeParent();
        NS_ASSERTION(node1 && node2, "no parent while walking subtree");
      }
    }
  } while(node2);

  return PR_FALSE;
}

nsresult
nsINode::LookupNamespaceURI(const nsAString& aNamespacePrefix,
                            nsAString& aNamespaceURI)
{
  Element *element = GetNameSpaceElement();
  if (!element || NS_FAILED(element->LookupNamespaceURI(aNamespacePrefix,
                                                        aNamespaceURI))) {
    SetDOMStringToNull(aNamespaceURI);
  }

  return NS_OK;
}

NS_IMPL_DOMTARGET_DEFAULTS(nsINode)

NS_IMETHODIMP
nsINode::AddEventListener(const nsAString& aType,
                          nsIDOMEventListener *aListener,
                          bool aUseCapture,
                          bool aWantsUntrusted,
                          PRUint8 aOptionalArgc)
{
  NS_ASSERTION(!aWantsUntrusted || aOptionalArgc > 1,
               "Won't check if this is chrome, you want to set "
               "aWantsUntrusted to PR_FALSE or make the aWantsUntrusted "
               "explicit by making aOptionalArgc non-zero.");

  if (!aWantsUntrusted &&
      (aOptionalArgc < 2 &&
       !nsContentUtils::IsChromeDoc(GetOwnerDoc()))) {
    aWantsUntrusted = PR_TRUE;
  }

  nsEventListenerManager* listener_manager = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(listener_manager);
  listener_manager->AddEventListener(aType, aListener, aUseCapture,
                                     aWantsUntrusted);
  return NS_OK;
}

NS_IMETHODIMP
nsINode::RemoveEventListener(const nsAString& aType,
                             nsIDOMEventListener* aListener,
                             bool aUseCapture)
{
  nsEventListenerManager* elm = GetListenerManager(PR_FALSE);
  if (elm) {
    elm->RemoveEventListener(aType, aListener, aUseCapture);
  }
  return NS_OK;
}

nsresult
nsINode::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  
  NS_ABORT();
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsINode::DispatchEvent(nsIDOMEvent *aEvent, bool* aRetVal)
{
  
  
  nsCOMPtr<nsIDocument> document = GetOwnerDoc();

  
  if (!document) {
    *aRetVal = PR_TRUE;
    return NS_OK;
  }

  
  nsIPresShell *shell = document->GetShell();
  nsRefPtr<nsPresContext> context;
  if (shell) {
    context = shell->GetPresContext();
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  nsresult rv =
    nsEventDispatcher::DispatchDOMEvent(this, nsnull, aEvent, context,
                                        &status);
  *aRetVal = (status != nsEventStatus_eConsumeNoDefault);
  return rv;
}

nsresult
nsINode::PostHandleEvent(nsEventChainPostVisitor& )
{
  return NS_OK;
}

nsresult
nsINode::DispatchDOMEvent(nsEvent* aEvent,
                          nsIDOMEvent* aDOMEvent,
                          nsPresContext* aPresContext,
                          nsEventStatus* aEventStatus)
{
  return nsEventDispatcher::DispatchDOMEvent(this, aEvent, aDOMEvent,
                                             aPresContext, aEventStatus);
}

nsEventListenerManager*
nsINode::GetListenerManager(bool aCreateIfNotFound)
{
  return nsContentUtils::GetListenerManager(this, aCreateIfNotFound);
}

nsIScriptContext*
nsINode::GetContextForEventHandlers(nsresult* aRv)
{
  return nsContentUtils::GetContextForEventHandlers(this, aRv);
}


void
nsINode::Trace(nsINode *tmp, TraceCallback cb, void *closure)
{
  nsContentUtils::TraceWrapper(tmp, cb, closure);
}


bool
nsINode::Traverse(nsINode *tmp, nsCycleCollectionTraversalCallback &cb)
{
  nsIDocument *currentDoc = tmp->GetCurrentDoc();
  if (currentDoc &&
      nsCCUncollectableMarker::InGeneration(cb, currentDoc->GetMarkedCCGeneration())) {
    return false;
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mNodeInfo)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(GetParent())

  nsSlots *slots = tmp->GetExistingSlots();
  if (slots) {
    slots->Traverse(cb);
  }

  if (tmp->HasProperties()) {
    nsNodeUtils::TraverseUserData(tmp, cb);
  }

  if (tmp->HasFlag(NODE_HAS_LISTENERMANAGER)) {
    nsContentUtils::TraverseListenerManager(tmp, cb);
  }

  return true;
}


void
nsINode::Unlink(nsINode *tmp)
{
  nsContentUtils::ReleaseWrapper(tmp, tmp);

  nsSlots *slots = tmp->GetExistingSlots();
  if (slots) {
    slots->Unlink();
  }

  if (tmp->HasFlag(NODE_HAS_LISTENERMANAGER)) {
    nsContentUtils::RemoveListenerManager(tmp);
    tmp->UnsetFlags(NODE_HAS_LISTENERMANAGER);
  }

  if (tmp->HasProperties()) {
    nsNodeUtils::UnlinkUserData(tmp);
  }
}



nsEventStates
Element::IntrinsicState() const
{
  return IsEditable() ? NS_EVENT_STATE_MOZ_READWRITE :
                        NS_EVENT_STATE_MOZ_READONLY;
}

void
Element::NotifyStateChange(nsEventStates aStates)
{
  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    nsAutoScriptBlocker scriptBlocker;
    doc->ContentStateChanged(this, aStates);
  }
}

void
Element::RequestLinkStateUpdate()
{
}

void
Element::UpdateLinkState(nsEventStates aState)
{
  NS_ABORT_IF_FALSE(!aState.HasAtLeastOneOfStates(~(NS_EVENT_STATE_VISITED |
                                                    NS_EVENT_STATE_UNVISITED)),
                    "Unexpected link state bits");
  mState =
    (mState & ~(NS_EVENT_STATE_VISITED | NS_EVENT_STATE_UNVISITED)) |
    aState;
}

void
Element::UpdateState(bool aNotify)
{
  nsEventStates oldState = mState;
  mState = IntrinsicState() | (oldState & ESM_MANAGED_STATES);
  if (aNotify) {
    nsEventStates changedStates = oldState ^ mState;
    if (!changedStates.IsEmpty()) {
      nsIDocument* doc = GetCurrentDoc();
      if (doc) {
        nsAutoScriptBlocker scriptBlocker;
        doc->ContentStateChanged(this, changedStates);
      }
    }
  }
}

void
nsIContent::UpdateEditableState(bool aNotify)
{
  
  NS_ASSERTION(!IsElement(), "What happened here?");
  nsIContent *parent = GetParent();

  SetEditableFlag(parent && parent->HasFlag(NODE_IS_EDITABLE));
}

void
nsGenericElement::UpdateEditableState(bool aNotify)
{
  nsIContent *parent = GetParent();

  SetEditableFlag(parent && parent->HasFlag(NODE_IS_EDITABLE));
  if (aNotify) {
    UpdateState(aNotify);
  } else {
    
    
    
    
    if (IsEditable()) {
      RemoveStatesSilently(NS_EVENT_STATE_MOZ_READONLY);
      AddStatesSilently(NS_EVENT_STATE_MOZ_READWRITE);
    } else {
      RemoveStatesSilently(NS_EVENT_STATE_MOZ_READWRITE);
      AddStatesSilently(NS_EVENT_STATE_MOZ_READONLY);
    }
  }
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

nsIContent*
nsIContent::GetFlattenedTreeParent() const
{
  nsIContent *parent = GetParent();
  if (parent && parent->HasFlag(NODE_MAY_BE_IN_BINDING_MNGR)) {
    nsIDocument *doc = parent->GetOwnerDoc();
    if (doc) {
      nsIContent* insertionElement =
        doc->BindingManager()->GetNestedInsertionPoint(parent, this);
      if (insertionElement) {
        parent = insertionElement;
      }
    }
  }
  return parent;
}

PRUint32
nsIContent::GetDesiredIMEState()
{
  if (!IsEditableInternal()) {
    return IME_STATUS_DISABLE;
  }
  
  
  
  nsIContent *editableAncestor = GetEditingHost();

  
  if (editableAncestor && editableAncestor != this) {
    return editableAncestor->GetDesiredIMEState();
  }
  nsIDocument* doc = GetCurrentDoc();
  if (!doc) {
    return IME_STATUS_DISABLE;
  }
  nsIPresShell* ps = doc->GetShell();
  if (!ps) {
    return IME_STATUS_DISABLE;
  }
  nsPresContext* pc = ps->GetPresContext();
  if (!pc) {
    return IME_STATUS_DISABLE;
  }
  nsIEditor* editor = GetHTMLEditor(pc);
  nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(editor);
  if (!imeEditor) {
    return IME_STATUS_DISABLE;
  }
  
  
  PRUint32 state = IME_STATUS_ENABLE;
  nsresult rv = imeEditor->GetPreferredIMEState(&state);
  NS_ENSURE_SUCCESS(rv, IME_STATUS_ENABLE);
  return state;
}

bool
nsIContent::HasIndependentSelection()
{
  nsIFrame* frame = GetPrimaryFrame();
  return (frame && frame->GetStateBits() & NS_FRAME_INDEPENDENT_SELECTION);
}

nsIContent*
nsIContent::GetEditingHost()
{
  
  NS_ENSURE_TRUE(IsEditableInternal(), nsnull);

  nsIDocument* doc = GetCurrentDoc();
  NS_ENSURE_TRUE(doc, nsnull);
  
  if (doc->HasFlag(NODE_IS_EDITABLE)) {
    return doc->GetBodyElement();
  }

  nsIContent* content = this;
  for (nsIContent* parent = GetParent();
       parent && parent->HasFlag(NODE_IS_EDITABLE);
       parent = content->GetParent()) {
    content = parent;
  }
  return content;
}

nsresult
nsIContent::LookupNamespaceURI(const nsAString& aNamespacePrefix,
                               nsAString& aNamespaceURI) const
{
  if (aNamespacePrefix.EqualsLiteral("xml")) {
    
    aNamespaceURI.AssignLiteral("http://www.w3.org/XML/1998/namespace");
    return NS_OK;
  }

  if (aNamespacePrefix.EqualsLiteral("xmlns")) {
    
    aNamespaceURI.AssignLiteral("http://www.w3.org/2000/xmlns/");
    return NS_OK;
  }

  nsCOMPtr<nsIAtom> name;
  if (!aNamespacePrefix.IsEmpty()) {
    name = do_GetAtom(aNamespacePrefix);
    NS_ENSURE_TRUE(name, NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    name = nsGkAtoms::xmlns;
  }
  
  
  const nsIContent* content = this;
  do {
    if (content->GetAttr(kNameSpaceID_XMLNS, name, aNamespaceURI))
      return NS_OK;
  } while ((content = content->GetParent()));
  return NS_ERROR_FAILURE;
}

already_AddRefed<nsIURI>
nsIContent::GetBaseURI() const
{
  nsIDocument* doc = GetOwnerDoc();
  if (!doc) {
    
    
    NS_ERROR("Element without owner document");
    return nsnull;
  }

  
  nsCOMPtr<nsIURI> base = doc->GetDocBaseURI();

  
  
  
  
  
  nsAutoTArray<nsString, 5> baseAttrs;
  nsString attr;
  const nsIContent *elem = this;
  do {
    
    if (elem->IsSVG()) {
      nsIContent* bindingParent = elem->GetBindingParent();
      if (bindingParent) {
        nsIDocument* bindingDoc = bindingParent->GetOwnerDoc();
        if (bindingDoc) {
          nsXBLBinding* binding =
            bindingDoc->BindingManager()->GetBinding(bindingParent);
          if (binding) {
            
            
            
            
            base = binding->PrototypeBinding()->DocURI();
            break;
          }
        }
      }
    }
    
    
    elem->GetAttr(kNameSpaceID_XML, nsGkAtoms::base, attr);
    if (!attr.IsEmpty()) {
      baseAttrs.AppendElement(attr);
    }
    elem = elem->GetParent();
  } while(elem);
  
  
  for (PRUint32 i = baseAttrs.Length() - 1; i != PRUint32(-1); --i) {
    nsCOMPtr<nsIURI> newBase;
    nsresult rv = NS_NewURI(getter_AddRefs(newBase), baseAttrs[i],
                            doc->GetDocumentCharacterSet().get(), base);
    
    
    if (NS_SUCCEEDED(rv) && i == 0) {
      rv = nsContentUtils::GetSecurityManager()->
        CheckLoadURIWithPrincipal(NodePrincipal(), newBase,
                                  nsIScriptSecurityManager::STANDARD);
    }
    if (NS_SUCCEEDED(rv)) {
      base.swap(newBase);
    }
  }

  return base.forget();
}



NS_IMPL_CYCLE_COLLECTING_ADDREF(nsChildContentList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsChildContentList)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsChildContentList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsChildContentList)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsChildContentList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsChildContentList)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_TABLE_HEAD(nsChildContentList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_NODELIST_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsChildContentList)
    NS_INTERFACE_TABLE_ENTRY(nsChildContentList, nsINodeList)
    NS_INTERFACE_TABLE_ENTRY(nsChildContentList, nsIDOMNodeList)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsChildContentList)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(NodeList)
NS_INTERFACE_MAP_END

JSObject*
nsChildContentList::WrapObject(JSContext *cx, XPCWrappedNativeScope *scope,
                               bool *triedToWrap)
{
  return mozilla::dom::binding::NodeList::create(cx, scope, this, triedToWrap);
}

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



NS_IMPL_CYCLE_COLLECTION_1(nsNode3Tearoff, mNode)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsNode3Tearoff)
  NS_INTERFACE_MAP_ENTRY(nsIDOMXPathNSResolver)
NS_INTERFACE_MAP_END_AGGREGATED(mNode)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsNode3Tearoff)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsNode3Tearoff)

NS_IMETHODIMP
nsNode3Tearoff::LookupNamespaceURI(const nsAString& aNamespacePrefix,
                                   nsAString& aNamespaceURI)
{
  return mNode->LookupNamespaceURI(aNamespacePrefix, aNamespaceURI);
}

nsIContent*
nsGenericElement::GetFirstElementChild()
{
  nsAttrAndChildArray& children = mAttrsAndChildren;
  PRUint32 i, count = children.ChildCount();
  for (i = 0; i < count; ++i) {
    nsIContent* child = children.ChildAt(i);
    if (child->IsElement()) {
      return child;
    }
  }
  
  return nsnull;
}

nsIContent*
nsGenericElement::GetLastElementChild()
{
  nsAttrAndChildArray& children = mAttrsAndChildren;
  PRUint32 i = children.ChildCount();
  while (i > 0) {
    nsIContent* child = children.ChildAt(--i);
    if (child->IsElement()) {
      return child;
    }
  }
  
  return nsnull;
}

nsIContent*
nsGenericElement::GetPreviousElementSibling()
{
  nsIContent* parent = GetParent();
  if (!parent) {
    return nsnull;
  }

  NS_ASSERTION(parent->IsElement() ||
               parent->IsNodeOfType(nsINode::eDOCUMENT_FRAGMENT),
               "Parent content must be an element or a doc fragment");

  nsAttrAndChildArray& children =
    static_cast<nsGenericElement*>(parent)->mAttrsAndChildren;
  PRInt32 index = children.IndexOfChild(this);
  if (index < 0) {
    return nsnull;
  }

  PRUint32 i = index;
  while (i > 0) {
    nsIContent* child = children.ChildAt((PRUint32)--i);
    if (child->IsElement()) {
      return child;
    }
  }
  
  return nsnull;
}

nsIContent*
nsGenericElement::GetNextElementSibling()
{
  nsIContent* parent = GetParent();
  if (!parent) {
    return nsnull;
  }

  NS_ASSERTION(parent->IsElement() ||
               parent->IsNodeOfType(nsINode::eDOCUMENT_FRAGMENT),
               "Parent content must be an element or a doc fragment");

  nsAttrAndChildArray& children =
    static_cast<nsGenericElement*>(parent)->mAttrsAndChildren;
  PRInt32 index = children.IndexOfChild(this);
  if (index < 0) {
    return nsnull;
  }

  PRUint32 i, count = children.ChildCount();
  for (i = (PRUint32)index + 1; i < count; ++i) {
    nsIContent* child = children.ChildAt(i);
    if (child->IsElement()) {
      return child;
    }
  }
  
  return nsnull;
}

NS_IMETHODIMP
nsNSElementTearoff::GetFirstElementChild(nsIDOMElement** aResult)
{
  *aResult = nsnull;

  nsIContent *result = mContent->GetFirstElementChild();

  return result ? CallQueryInterface(result, aResult) : NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::GetLastElementChild(nsIDOMElement** aResult)
{
  *aResult = nsnull;

  nsIContent *result = mContent->GetLastElementChild();

  return result ? CallQueryInterface(result, aResult) : NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::GetPreviousElementSibling(nsIDOMElement** aResult)
{
  *aResult = nsnull;

  nsIContent *result = mContent->GetPreviousElementSibling();

  return result ? CallQueryInterface(result, aResult) : NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::GetNextElementSibling(nsIDOMElement** aResult)
{
  *aResult = nsnull;

  nsIContent *result = mContent->GetNextElementSibling();

  return result ? CallQueryInterface(result, aResult) : NS_OK;
}

nsContentList*
nsGenericElement::GetChildrenList()
{
  nsGenericElement::nsDOMSlots *slots = DOMSlots();

  if (!slots->mChildrenList) {
    slots->mChildrenList = new nsContentList(this, kNameSpaceID_Wildcard, 
                                             nsGkAtoms::_asterix, nsGkAtoms::_asterix,
                                             PR_FALSE);
  }

  return slots->mChildrenList;
}

NS_IMETHODIMP
nsNSElementTearoff::GetChildElementCount(PRUint32* aResult)
{
  return mContent->GetChildElementCount(aResult);
}

NS_IMETHODIMP
nsNSElementTearoff::GetChildren(nsIDOMNodeList** aResult)
{
  return mContent->GetChildren(aResult);
}

nsIDOMDOMTokenList*
nsGenericElement::GetClassList(nsresult *aResult)
{
  *aResult = NS_ERROR_OUT_OF_MEMORY;

  nsGenericElement::nsDOMSlots *slots = DOMSlots();

  if (!slots->mClassList) {
    nsCOMPtr<nsIAtom> classAttr = GetClassAttributeName();
    if (!classAttr) {
      *aResult = NS_OK;

      return nsnull;
    }

    slots->mClassList = new nsDOMTokenList(this, classAttr);
    NS_ENSURE_TRUE(slots->mClassList, nsnull);
  }

  *aResult = NS_OK;

  return slots->mClassList;
}

NS_IMETHODIMP
nsNSElementTearoff::GetClassList(nsIDOMDOMTokenList** aResult)
{
  *aResult = nsnull;

  nsresult rv;
  nsIDOMDOMTokenList* list = mContent->GetClassList(&rv);
  NS_ENSURE_TRUE(list, rv);

  NS_ADDREF(*aResult = list);

  return NS_OK;
}

void
nsGenericElement::SetCapture(bool aRetargetToElement)
{
  
  
  
  if (nsIPresShell::GetCapturingContent())
    return;

  nsIPresShell::SetCapturingContent(this, CAPTURE_PREVENTDRAG |
    (aRetargetToElement ? CAPTURE_RETARGETTOELEMENT : 0));
}

NS_IMETHODIMP
nsNSElementTearoff::SetCapture(bool aRetargetToElement)
{
  mContent->SetCapture(aRetargetToElement);

  return NS_OK;
}

void
nsGenericElement::ReleaseCapture()
{
  if (nsIPresShell::GetCapturingContent() == this) {
    nsIPresShell::SetCapturingContent(nsnull, 0);
  }
}

NS_IMETHODIMP
nsNSElementTearoff::ReleaseCapture()
{
  mContent->ReleaseCapture();

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
  return mContent->GetElementsByClassName(aClasses, aReturn);
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

nsIScrollableFrame*
nsGenericElement::GetScrollFrame(nsIFrame **aStyledFrame)
{
  
  if (IsSVG()) {
    if (aStyledFrame) {
      *aStyledFrame = nsnull;
    }
    return nsnull;
  }

  nsIFrame* frame = GetStyledFrame();

  if (aStyledFrame) {
    *aStyledFrame = frame;
  }
  if (!frame) {
    return nsnull;
  }

  
  
  if (frame->GetType() != nsGkAtoms::menuFrame) {
    nsIScrollableFrame *scrollFrame = frame->GetScrollTargetFrame();
    if (scrollFrame)
      return scrollFrame;
  }

  nsIDocument* doc = GetOwnerDoc();
  bool quirksMode = doc->GetCompatibilityMode() == eCompatibility_NavQuirks;
  Element* elementWithRootScrollInfo =
    quirksMode ? doc->GetBodyElement() : doc->GetRootElement();
  if (this == elementWithRootScrollInfo) {
    
    
    
    
    return frame->PresContext()->PresShell()->GetRootScrollFrameAsScrollable();
  }

  return nsnull;
}

PRInt32
nsGenericElement::GetScrollTop()
{
  nsIScrollableFrame* sf = GetScrollFrame();

  return sf ?
         nsPresContext::AppUnitsToIntCSSPixels(sf->GetScrollPosition().y) :
         0;
}

NS_IMETHODIMP
nsNSElementTearoff::GetScrollTop(PRInt32* aScrollTop)
{
  *aScrollTop = mContent->GetScrollTop();

  return NS_OK;
}

void
nsGenericElement::SetScrollTop(PRInt32 aScrollTop)
{
  nsIScrollableFrame* sf = GetScrollFrame();
  if (sf) {
    nsPoint pt = sf->GetScrollPosition();
    pt.y = nsPresContext::CSSPixelsToAppUnits(aScrollTop);
    sf->ScrollTo(pt, nsIScrollableFrame::INSTANT);
  }
}

NS_IMETHODIMP
nsNSElementTearoff::SetScrollTop(PRInt32 aScrollTop)
{
  mContent->SetScrollTop(aScrollTop);

  return NS_OK;
}

PRInt32
nsGenericElement::GetScrollLeft()
{
  nsIScrollableFrame* sf = GetScrollFrame();

  return sf ?
         nsPresContext::AppUnitsToIntCSSPixels(sf->GetScrollPosition().x) :
         0;
}

NS_IMETHODIMP
nsNSElementTearoff::GetScrollLeft(PRInt32* aScrollLeft)
{
  *aScrollLeft = mContent->GetScrollLeft();

  return NS_OK;
}

void
nsGenericElement::SetScrollLeft(PRInt32 aScrollLeft)
{
  nsIScrollableFrame* sf = GetScrollFrame();
  if (sf) {
    nsPoint pt = sf->GetScrollPosition();
    pt.x = nsPresContext::CSSPixelsToAppUnits(aScrollLeft);
    sf->ScrollTo(pt, nsIScrollableFrame::INSTANT);
  }
}

NS_IMETHODIMP
nsNSElementTearoff::SetScrollLeft(PRInt32 aScrollLeft)
{
  mContent->SetScrollLeft(aScrollLeft);

  return NS_OK;
}

PRInt32
nsGenericElement::GetScrollHeight()
{
  if (IsSVG())
    return 0;

  nsIScrollableFrame* sf = GetScrollFrame();
  if (!sf) {
    nsRect rcFrame;
    nsCOMPtr<nsIContent> parent;
    GetOffsetRect(rcFrame, getter_AddRefs(parent));
    return rcFrame.height;
  }

  nscoord height = sf->GetScrollRange().height + sf->GetScrollPortRect().height;
  return nsPresContext::AppUnitsToIntCSSPixels(height);
}

NS_IMETHODIMP
nsNSElementTearoff::GetScrollHeight(PRInt32* aScrollHeight)
{
  *aScrollHeight = mContent->GetScrollHeight();

  return NS_OK;
}

PRInt32
nsGenericElement::GetScrollWidth()
{
  if (IsSVG())
    return 0;

  nsIScrollableFrame* sf = GetScrollFrame();
  if (!sf) {
    nsRect rcFrame;
    nsCOMPtr<nsIContent> parent;
    GetOffsetRect(rcFrame, getter_AddRefs(parent));
    return rcFrame.width;
  }

  nscoord width = sf->GetScrollRange().width + sf->GetScrollPortRect().width;
  return nsPresContext::AppUnitsToIntCSSPixels(width);
}

NS_IMETHODIMP
nsNSElementTearoff::GetScrollWidth(PRInt32 *aScrollWidth)
{
  *aScrollWidth = mContent->GetScrollWidth();

  return NS_OK;
}

nsRect
nsGenericElement::GetClientAreaRect()
{
  nsIFrame* styledFrame;
  nsIScrollableFrame* sf = GetScrollFrame(&styledFrame);

  if (sf) {
    return sf->GetScrollPortRect();
  }

  if (styledFrame &&
      (styledFrame->GetStyleDisplay()->mDisplay != NS_STYLE_DISPLAY_INLINE ||
       styledFrame->IsFrameOfType(nsIFrame::eReplaced))) {
    
    
    return styledFrame->GetPaddingRect() - styledFrame->GetPositionIgnoringScrolling();
  }

  
  return nsRect(0, 0, 0, 0);
}

NS_IMETHODIMP
nsNSElementTearoff::GetClientTop(PRInt32 *aClientTop)
{
  *aClientTop = mContent->GetClientTop();
  return NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::GetClientLeft(PRInt32 *aClientLeft)
{
  *aClientLeft = mContent->GetClientLeft();
  return NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::GetClientHeight(PRInt32 *aClientHeight)
{
  *aClientHeight = mContent->GetClientHeight();
  return NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::GetClientWidth(PRInt32 *aClientWidth)
{
  *aClientWidth = mContent->GetClientWidth();
  return NS_OK;
}

nsresult
nsGenericElement::GetBoundingClientRect(nsIDOMClientRect** aResult)
{
  
  nsClientRect* rect = new nsClientRect();
  if (!rect)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult = rect);
  
  nsIFrame* frame = GetPrimaryFrame(Flush_Layout);
  if (!frame) {
    
    return NS_OK;
  }

  nsRect r = nsLayoutUtils::GetAllInFlowRectsUnion(frame,
          nsLayoutUtils::GetContainingBlockForClientRect(frame));
  rect->SetLayoutRect(r);
  return NS_OK;
}

NS_IMETHODIMP
nsNSElementTearoff::GetBoundingClientRect(nsIDOMClientRect** aResult)
{
  return mContent->GetBoundingClientRect(aResult);
}

nsresult
nsGenericElement::GetElementsByClassName(const nsAString& aClasses,
                                         nsIDOMNodeList** aReturn)
{
  return nsContentUtils::GetElementsByClassName(this, aClasses, aReturn);
}

nsresult
nsGenericElement::GetClientRects(nsIDOMClientRectList** aResult)
{
  *aResult = nsnull;

  nsRefPtr<nsClientRectList> rectList = new nsClientRectList();
  if (!rectList)
    return NS_ERROR_OUT_OF_MEMORY;

  nsIFrame* frame = GetPrimaryFrame(Flush_Layout);
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

NS_IMETHODIMP
nsNSElementTearoff::GetClientRects(nsIDOMClientRectList** aResult)
{
  return mContent->GetClientRects(aResult);
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



NS_IMPL_CYCLE_COLLECTION_1(nsNodeSelectorTearoff, mNode)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsNodeSelectorTearoff)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNodeSelector)
NS_INTERFACE_MAP_END_AGGREGATED(mNode)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsNodeSelectorTearoff)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsNodeSelectorTearoff)

NS_IMETHODIMP
nsNodeSelectorTearoff::QuerySelector(const nsAString& aSelector,
                                     nsIDOMElement **aReturn)
{
  nsresult rv;
  nsIContent* result = nsGenericElement::doQuerySelector(mNode, aSelector, &rv);
  return result ? CallQueryInterface(result, aReturn) : rv;
}

NS_IMETHODIMP
nsNodeSelectorTearoff::QuerySelectorAll(const nsAString& aSelector,
                                        nsIDOMNodeList **aReturn)
{
  return nsGenericElement::doQuerySelectorAll(mNode, aSelector, aReturn);
}



NS_IMPL_CYCLE_COLLECTION_1(nsTouchEventReceiverTearoff, mElement)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsTouchEventReceiverTearoff)
  NS_INTERFACE_MAP_ENTRY(nsITouchEventReceiver)
NS_INTERFACE_MAP_END_AGGREGATED(mElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsTouchEventReceiverTearoff)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsTouchEventReceiverTearoff)



NS_IMPL_CYCLE_COLLECTION_1(nsInlineEventHandlersTearoff, mElement)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsInlineEventHandlersTearoff)
  NS_INTERFACE_MAP_ENTRY(nsIInlineEventHandlers)
NS_INTERFACE_MAP_END_AGGREGATED(mElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsInlineEventHandlersTearoff)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsInlineEventHandlersTearoff)


nsGenericElement::nsDOMSlots::nsDOMSlots()
  : nsINode::nsSlots(),
    mDataset(nsnull),
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

void
nsGenericElement::nsDOMSlots::Traverse(nsCycleCollectionTraversalCallback &cb, bool aIsXUL)
{
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mSlots->mStyle");
  cb.NoteXPCOMChild(mStyle.get());

#ifdef MOZ_SMIL
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mSlots->mSMILOverrideStyle");
  cb.NoteXPCOMChild(mSMILOverrideStyle.get());
#endif 

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mSlots->mAttributeMap");
  cb.NoteXPCOMChild(mAttributeMap.get());

  if (aIsXUL) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mSlots->mControllers");
    cb.NoteXPCOMChild(mControllers);
  }

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mSlots->mChildrenList");
  cb.NoteXPCOMChild(NS_ISUPPORTS_CAST(nsIDOMNodeList*, mChildrenList));
}

void
nsGenericElement::nsDOMSlots::Unlink(bool aIsXUL)
{
  mStyle = nsnull;
#ifdef MOZ_SMIL
  mSMILOverrideStyle = nsnull;
#endif 
  if (mAttributeMap) {
    mAttributeMap->DropReference();
    mAttributeMap = nsnull;
  }
  if (aIsXUL)
    NS_IF_RELEASE(mControllers);
  mChildrenList = nsnull;
}

nsGenericElement::nsGenericElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : Element(aNodeInfo)
{
  NS_ABORT_IF_FALSE(mNodeInfo->NodeType() == nsIDOMNode::ELEMENT_NODE ||
                    (mNodeInfo->NodeType() ==
                       nsIDOMNode::DOCUMENT_FRAGMENT_NODE &&
                     mNodeInfo->Equals(nsGkAtoms::documentFragmentNodeName,
                                       kNameSpaceID_None)),
                    "Bad NodeType in aNodeInfo");

  
  
  SetFlags((nsIProgrammingLanguage::JAVASCRIPT << NODE_SCRIPT_TYPE_OFFSET));
  SetIsElement();
}

nsGenericElement::~nsGenericElement()
{
  NS_PRECONDITION(!IsInDoc(),
                  "Please remove this from the document properly");
  if (GetParent()) {
    NS_RELEASE(mParent);
  }
}

NS_IMETHODIMP
nsGenericElement::GetNodeName(nsAString& aNodeName)
{
  aNodeName = NodeName();
  return NS_OK;
}

NS_IMETHODIMP
nsGenericElement::GetLocalName(nsAString& aLocalName)
{
  aLocalName = LocalName();
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
  *aNodeType = NodeType();
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

nsresult
nsGenericElement::InternalIsSupported(nsISupports* aObject,
                                      const nsAString& aFeature,
                                      const nsAString& aVersion,
                                      bool* aReturn)
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
  } else if (PL_strcasecmp(f, "SVGEvents") == 0 ||
             PL_strcasecmp(f, "SVGZoomEvents") == 0 ||
             nsSVGFeatures::HaveFeature(aObject, aFeature)) {
    if (aVersion.IsEmpty() ||
        PL_strcmp(v, "1.0") == 0 ||
        PL_strcmp(v, "1.1") == 0) {
      *aReturn = PR_TRUE;
    }
  }
#ifdef MOZ_SMIL
  else if (NS_SMILEnabled() && PL_strcasecmp(f, "TimeControl") == 0) {
    if (aVersion.IsEmpty() || PL_strcmp(v, "1.0") == 0) {
      *aReturn = PR_TRUE;
    }
  }
#endif 

  return NS_OK;
}

NS_IMETHODIMP
nsGenericElement::IsSupported(const nsAString& aFeature,
                              const nsAString& aVersion,
                              bool* aReturn)
{
  return InternalIsSupported(this, aFeature, aVersion, aReturn);
}

NS_IMETHODIMP
nsGenericElement::HasAttributes(bool* aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  *aReturn = GetAttrCount() > 0;

  return NS_OK;
}

NS_IMETHODIMP
nsGenericElement::GetAttributes(nsIDOMNamedNodeMap** aAttributes)
{
  if (!IsElement()) {
    *aAttributes = nsnull;
    return NS_OK;
  }

  nsDOMSlots *slots = DOMSlots();

  if (!slots->mAttributeMap) {
    slots->mAttributeMap = new nsDOMAttributeMap(this);
    if (!slots->mAttributeMap->Init()) {
      slots->mAttributeMap = nsnull;
      return NS_ERROR_FAILURE;
    }
  }

  NS_ADDREF(*aAttributes = slots->mAttributeMap);

  return NS_OK;
}

nsresult
nsGenericElement::HasChildNodes(bool* aReturn)
{
  *aReturn = mAttrsAndChildren.ChildCount() > 0;

  return NS_OK;
}

NS_IMETHODIMP
nsGenericElement::GetTagName(nsAString& aTagName)
{
  aTagName = NodeName();
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

  nsIDocument* document = GetOwnerDoc();
  if (document) {
    document->WarnOnceAbout(nsIDocument::eGetAttributeNode);
  }

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

  nsIDocument* document = GetOwnerDoc();
  if (document) {
    document->WarnOnceAbout(nsIDocument::eSetAttributeNode);
  }

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

  nsIDocument* document = GetOwnerDoc();
  if (document) {
    document->WarnOnceAbout(nsIDocument::eRemoveAttributeNode);
  }

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
  nsContentList *list = NS_GetContentList(this, kNameSpaceID_Unknown, 
                                          aTagname).get();

  
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
                                         nsIDOMNode::ATTRIBUTE_NODE,
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

  nsIDocument* document = GetOwnerDoc();
  if (document) {
    document->WarnOnceAbout(nsIDocument::eGetAttributeNodeNS);
  }

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

  nsIDocument* document = GetOwnerDoc();
  if (document) {
    document->WarnOnceAbout(nsIDocument::eSetAttributeNodeNS);
  }

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

  NS_ASSERTION(nameSpaceId != kNameSpaceID_Unknown, "Unexpected namespace ID!");

  nsContentList *list = NS_GetContentList(this, nameSpaceId, aLocalName).get();

  
  *aReturn = list;
  return NS_OK;
}

nsresult
nsGenericElement::HasAttribute(const nsAString& aName, bool* aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  const nsAttrName* name = InternalGetExistingAttrNameFromQName(aName);
  *aReturn = (name != nsnull);

  return NS_OK;
}

nsresult
nsGenericElement::HasAttributeNS(const nsAString& aNamespaceURI,
                                 const nsAString& aLocalName,
                                 bool* aReturn)
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
    bool allowScripts = aBinding->AllowScripts();
#ifdef MOZ_XUL
    nsCOMPtr<nsIXULDocument> xulDoc = do_QueryInterface(aDocument);
#endif
    PRUint32 i;
    for (i = 0; i < inserts->Length(); ++i) {
      nsCOMPtr<nsIContent> insertRoot =
        inserts->ElementAt(i)->GetDefaultContent();
      if (insertRoot) {
        for (nsCOMPtr<nsIContent> child = insertRoot->GetFirstChild();
             child;
             child = child->GetNextSibling()) {
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
                             bool aCompileEventHandlers)
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
      nsDOMSlots *slots = DOMSlots();

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

  bool hadForceXBL = HasFlag(NODE_FORCE_XBL_BINDINGS);

  
  if (aParent) {
    if (!GetParent()) {
      NS_ADDREF(aParent);
    }
    mParent = aParent;

    if (aParent->HasFlag(NODE_FORCE_XBL_BINDINGS)) {
      SetFlags(NODE_FORCE_XBL_BINDINGS);
    }
  }
  else {
    mParent = aDocument;
  }
  SetParentIsContent(aParent);

  

  
  if (aDocument) {
    
    
    
    
    
    
    
    

    
    SetInDocument();

    
    UnsetFlags(NODE_FORCE_XBL_BINDINGS |
               
               NODE_NEEDS_FRAME | NODE_DESCENDANTS_NEED_FRAMES |
               
               ELEMENT_ALL_RESTYLE_FLAGS);
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
        bool allowScripts = contBinding->AllowScripts();
        for (nsCOMPtr<nsIContent> child = anonRoot->GetFirstChild();
             child;
             child = child->GetNextSibling()) {
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

  UpdateEditableState(PR_FALSE);

  
  for (nsIContent* child = GetFirstChild(); child;
       child = child->GetNextSibling()) {
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
nsGenericElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  NS_PRECONDITION(aDeep || (!GetCurrentDoc() && !GetBindingParent()),
                  "Shallow unbind won't clear document and binding parent on "
                  "kids!");
  
  nsIDocument *document =
    HasFlag(NODE_FORCE_XBL_BINDINGS) ? GetOwnerDoc() : GetCurrentDoc();

  if (aNullParent) {
    if (GetParent()) {
      NS_RELEASE(mParent);
    } else {
      mParent = nsnull;
    }
    SetParentIsContent(false);
  }
  ClearInDocument();

  if (document) {
    
    
    document->BindingManager()->RemovedFromDocument(this, document);

    document->ClearBoxObjectFor(this);
  }

  
  
  
  
  if (HasFlag(NODE_HAS_PROPERTIES)) {
    DeleteProperty(nsGkAtoms::transitionsOfBeforeProperty);
    DeleteProperty(nsGkAtoms::transitionsOfAfterProperty);
    DeleteProperty(nsGkAtoms::transitionsProperty);
    DeleteProperty(nsGkAtoms::animationsOfBeforeProperty);
    DeleteProperty(nsGkAtoms::animationsOfAfterProperty);
    DeleteProperty(nsGkAtoms::animationsProperty);
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

already_AddRefed<nsINodeList>
nsGenericElement::GetChildren(PRUint32 aFilter)
{
  nsRefPtr<nsSimpleContentList> list = new nsSimpleContentList(this);
  if (!list) {
    return nsnull;
  }

  nsIFrame *frame = GetPrimaryFrame();

  
  if (frame) {
    nsIFrame *beforeFrame = nsLayoutUtils::GetBeforeFrame(frame);
    if (beforeFrame) {
      list->AppendElement(beforeFrame->GetContent());
    }
  }

  
  
  
  
  nsINodeList *childList = nsnull;

  nsIDocument* document = GetOwnerDoc();
  if (document) {
    if (!(aFilter & eAllButXBL)) {
      childList = document->BindingManager()->GetXBLChildNodesFor(this);
      if (!childList) {
        childList = GetChildNodesList();
      }

    } else {
      childList = document->BindingManager()->GetContentListFor(this);
    }
  } else {
    childList = GetChildNodesList();
  }

  if (childList) {
    PRUint32 length = 0;
    childList->GetLength(&length);
    for (PRUint32 idx = 0; idx < length; idx++) {
      nsIContent* child = childList->GetNodeAt(idx);
      list->AppendElement(child);
    }
  }

  if (frame) {
    
    nsIAnonymousContentCreator* creator = do_QueryFrame(frame);
    if (creator) {
      creator->AppendAnonymousContentTo(*list, aFilter);
    }

    
    nsIFrame *afterFrame = nsLayoutUtils::GetAfterFrame(frame);
    if (afterFrame) {
      list->AppendElement(afterFrame->GetContent());
    }
  }

  nsINodeList* returnList = nsnull;
  list.forget(&returnList);
  return returnList;
}

static nsIContent*
FindNativeAnonymousSubtreeOwner(nsIContent* aContent)
{
  if (aContent->IsInNativeAnonymousSubtree()) {
    bool isNativeAnon = false;
    while (aContent && !isNativeAnon) {
      isNativeAnon = aContent->IsRootOfNativeAnonymousSubtree();
      aContent = aContent->GetParent();
    }
  }
  return aContent;
}

nsresult
nsIContent::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  
  aVisitor.mCanHandle = PR_TRUE;
  aVisitor.mMayHaveListenerManager = HasFlag(NODE_HAS_LISTENERMANAGER);

  
  
  bool isAnonForEvents = IsRootOfNativeAnonymousSubtree();
  if ((aVisitor.mEvent->message == NS_MOUSE_ENTER_SYNTH ||
       aVisitor.mEvent->message == NS_MOUSE_EXIT_SYNTH) &&
      
      
      
      ((this == aVisitor.mEvent->originalTarget &&
        !IsInNativeAnonymousSubtree()) || isAnonForEvents)) {
     nsCOMPtr<nsIContent> relatedTarget =
       do_QueryInterface(static_cast<nsMouseEvent*>
                                    (aVisitor.mEvent)->relatedTarget);
    if (relatedTarget &&
        relatedTarget->GetOwnerDoc() == GetOwnerDoc()) {

      
      
      
      
      
      if (isAnonForEvents || aVisitor.mRelatedTargetIsInAnon ||
          (aVisitor.mEvent->originalTarget == this &&
           (aVisitor.mRelatedTargetIsInAnon =
            relatedTarget->IsInNativeAnonymousSubtree()))) {
        nsIContent* anonOwner = FindNativeAnonymousSubtreeOwner(this);
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
              Tag()->ToString(ct);
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
                       : (IsInNativeAnonymousSubtree()
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

  nsIContent* parent = GetParent();
  
  
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

  
  
  if (HasFlag(NODE_MAY_BE_IN_BINDING_MNGR)) {
    nsIDocument* ownerDoc = GetOwnerDoc();
    if (ownerDoc) {
      nsIContent* insertionParent = ownerDoc->BindingManager()->
        GetInsertionParent(this);
      NS_ASSERTION(!(aVisitor.mEventTargetAtParent && insertionParent &&
                     aVisitor.mEventTargetAtParent != insertionParent),
                   "Retargeting and having insertion parent!");
      if (insertionParent) {
        parent = insertionParent;
      }
    }
  }

  if (parent) {
    aVisitor.mParentTarget = parent;
  } else {
    aVisitor.mParentTarget = GetCurrentDoc();
  }
  return NS_OK;
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
nsIDOMCSSStyleDeclaration*
nsGenericElement::GetSMILOverrideStyle()
{
  nsGenericElement::nsDOMSlots *slots = DOMSlots();

  if (!slots->mSMILOverrideStyle) {
    slots->mSMILOverrideStyle = new nsDOMCSSAttributeDeclaration(this, PR_TRUE);
  }

  return slots->mSMILOverrideStyle;
}

css::StyleRule*
nsGenericElement::GetSMILOverrideStyleRule()
{
  nsGenericElement::nsDOMSlots *slots = GetExistingDOMSlots();
  return slots ? slots->mSMILOverrideStyleRule.get() : nsnull;
}

nsresult
nsGenericElement::SetSMILOverrideStyleRule(css::StyleRule* aStyleRule,
                                           bool aNotify)
{
  nsGenericElement::nsDOMSlots *slots = DOMSlots();

  slots->mSMILOverrideStyleRule = aStyleRule;

  if (aNotify) {
    nsIDocument* doc = GetCurrentDoc();
    
    
    
    if (doc) {
      nsCOMPtr<nsIPresShell> shell = doc->GetShell();
      if (shell) {
        shell->RestyleForAnimation(this, eRestyle_Self);
      }
    }
  }

  return NS_OK;
}
#endif 

css::StyleRule*
nsGenericElement::GetInlineStyleRule()
{
  return nsnull;
}

NS_IMETHODIMP
nsGenericElement::SetInlineStyleRule(css::StyleRule* aStyleRule,
                                     bool aNotify)
{
  NS_NOTYETIMPLEMENTED("nsGenericElement::SetInlineStyleRule");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP_(bool)
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
nsGenericElement::GetClassAttributeName() const
{
  return nsnull;
}

bool
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
    nodeInfo = mNodeInfo->NodeInfoManager()->
      GetNodeInfo(name->Atom(), nsnull, kNameSpaceID_None,
                  nsIDOMNode::ATTRIBUTE_NODE).get();
  }
  else {
    NS_ADDREF(nodeInfo = name->NodeInfo());
  }

  return nodeInfo;
}

bool
nsGenericElement::IsLink(nsIURI** aURI) const
{
  *aURI = nsnull;
  return PR_FALSE;
}


bool
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

bool
nsGenericElement::IsNodeOfType(PRUint32 aFlags) const
{
  return !(aFlags & ~eCONTENT);
}



PRUint32
nsGenericElement::GetScriptTypeID() const
{
    PtrBits flags = GetFlags();

    return (flags >> NODE_SCRIPT_TYPE_OFFSET) & NODE_SCRIPT_TYPE_MASK;
}

NS_IMETHODIMP
nsGenericElement::SetScriptTypeID(PRUint32 aLang)
{
    if ((aLang & NODE_SCRIPT_TYPE_MASK) != aLang) {
        NS_ERROR("script ID too large!");
        return NS_ERROR_FAILURE;
    }
    

    UnsetFlags(NODE_SCRIPT_TYPE_MASK << NODE_SCRIPT_TYPE_OFFSET);
    SetFlags(aLang << NODE_SCRIPT_TYPE_OFFSET);
    return NS_OK;
}

nsresult
nsGenericElement::InsertChildAt(nsIContent* aKid,
                                PRUint32 aIndex,
                                bool aNotify)
{
  NS_PRECONDITION(aKid, "null ptr");

  return doInsertChildAt(aKid, aIndex, aNotify, mAttrsAndChildren);
}

static nsresult
AdoptNodeIntoOwnerDoc(nsINode *aParent, nsINode *aNode)
{
  NS_ASSERTION(!aNode->GetNodeParent(),
               "Should have removed from parent already");

  nsIDocument *doc = aParent->GetOwnerDoc();

  nsresult rv;
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aNode, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> adoptedNode;
  rv = domDoc->AdoptNode(node, getter_AddRefs(adoptedNode));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(aParent->GetOwnerDoc() == doc,
               "ownerDoc chainged while adopting");
  NS_ASSERTION(adoptedNode == node, "Uh, adopt node changed nodes?");
  NS_ASSERTION(aParent->HasSameOwnerDoc(aNode),
               "ownerDocument changed again after adopting!");

  return NS_OK;
}

nsresult
nsINode::doInsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                         bool aNotify, nsAttrAndChildArray& aChildArray)
{
  NS_PRECONDITION(!aKid->GetNodeParent(),
                  "Inserting node that already has parent");
  nsresult rv;

  
  
  nsMutationGuard::DidMutate();

  
  nsIDocument* doc = GetCurrentDoc();
  mozAutoDocUpdate updateBatch(doc, UPDATE_CONTENT_MODEL, aNotify);

  if (!HasSameOwnerDoc(aKid)) {
    rv = AdoptNodeIntoOwnerDoc(this, aKid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRUint32 childCount = aChildArray.ChildCount();
  NS_ENSURE_TRUE(aIndex <= childCount, NS_ERROR_ILLEGAL_VALUE);
  bool isAppend = (aIndex == childCount);

  rv = aChildArray.InsertChildAt(aKid, aIndex);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aIndex == 0) {
    mFirstChild = aKid;
  }

  nsIContent* parent =
    IsNodeOfType(eDOCUMENT) ? nsnull : static_cast<nsIContent*>(this);

  rv = aKid->BindToTree(doc, parent, nsnull, PR_TRUE);
  if (NS_FAILED(rv)) {
    if (GetFirstChild() == aKid) {
      mFirstChild = aKid->GetNextSibling();
    }
    aChildArray.RemoveChildAt(aIndex);
    aKid->UnbindFromTree();
    return rv;
  }

  NS_ASSERTION(aKid->GetNodeParent() == this,
               "Did we run script inappropriately?");

  if (aNotify) {
    
    
    if (parent && isAppend) {
      nsNodeUtils::ContentAppended(parent, aKid, aIndex);
    } else {
      nsNodeUtils::ContentInserted(this, aKid, aIndex);
    }

    if (nsContentUtils::HasMutationListeners(aKid,
          NS_EVENT_BITS_MUTATION_NODEINSERTED, this)) {
      nsMutationEvent mutation(PR_TRUE, NS_MUTATION_NODEINSERTED);
      mutation.mRelatedNode = do_QueryInterface(this);

      mozAutoSubtreeModified subtree(GetOwnerDoc(), this);
      (new nsPLDOMEvent(aKid, mutation))->RunDOMEventWhenSafe();
    }
  }

  return NS_OK;
}

nsresult
nsGenericElement::RemoveChildAt(PRUint32 aIndex, bool aNotify)
{
  nsCOMPtr<nsIContent> oldKid = mAttrsAndChildren.GetSafeChildAt(aIndex);
  NS_ASSERTION(oldKid == GetChildAt(aIndex), "Unexpected child in RemoveChildAt");

  if (oldKid) {
    return doRemoveChildAt(aIndex, aNotify, oldKid, mAttrsAndChildren);
  }

  return NS_OK;
}

nsresult
nsINode::doRemoveChildAt(PRUint32 aIndex, bool aNotify,
                         nsIContent* aKid, nsAttrAndChildArray& aChildArray)
{
  NS_PRECONDITION(aKid && aKid->GetNodeParent() == this &&
                  aKid == GetChildAt(aIndex) &&
                  IndexOf(aKid) == (PRInt32)aIndex, "Bogus aKid");

  nsMutationGuard::DidMutate();

  nsIDocument* doc = GetCurrentDoc();

  mozAutoDocUpdate updateBatch(doc, UPDATE_CONTENT_MODEL, aNotify);

  nsIContent* previousSibling = aKid->GetPreviousSibling();

  if (GetFirstChild() == aKid) {
    mFirstChild = aKid->GetNextSibling();
  }

  aChildArray.RemoveChildAt(aIndex);

  if (aNotify) {
    nsNodeUtils::ContentRemoved(this, aKid, aIndex, previousSibling);
  }

  aKid->UnbindFromTree();

  return NS_OK;
}

NS_IMETHODIMP
nsGenericElement::GetTextContent(nsAString &aTextContent)
{
  nsContentUtils::GetNodeTextContent(this, PR_TRUE, aTextContent);
  return NS_OK;
}

NS_IMETHODIMP
nsGenericElement::SetTextContent(const nsAString& aTextContent)
{
  return nsContentUtils::SetNodeTextContent(this, aTextContent, PR_FALSE);
}


nsresult
nsGenericElement::DispatchEvent(nsPresContext* aPresContext,
                                nsEvent* aEvent,
                                nsIContent* aTarget,
                                bool aFullDispatch,
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
                                     bool aFullDispatch,
                                     PRUint32 aFlags,
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
  PRUint16 inputSource = 0;
  if (aSourceEvent->eventStructType == NS_MOUSE_EVENT) {
    clickCount = static_cast<nsMouseEvent*>(aSourceEvent)->clickCount;
    pressure = static_cast<nsMouseEvent*>(aSourceEvent)->pressure;
    inputSource = static_cast<nsMouseEvent*>(aSourceEvent)->inputSource;
  } else if (aSourceEvent->eventStructType == NS_KEY_EVENT) {
    inputSource = nsIDOMMouseEvent::MOZ_SOURCE_KEYBOARD;
  }
  event.pressure = pressure;
  event.clickCount = clickCount;
  event.inputSource = inputSource;
  event.isShift = aSourceEvent->isShift;
  event.isControl = aSourceEvent->isControl;
  event.isAlt = aSourceEvent->isAlt;
  event.isMeta = aSourceEvent->isMeta;
  event.flags |= aFlags; 

  return DispatchEvent(aPresContext, &event, aTarget, aFullDispatch, aStatus);
}

nsIFrame*
nsGenericElement::GetPrimaryFrame(mozFlushType aType)
{
  nsIDocument* doc = GetCurrentDoc();
  if (!doc) {
    return nsnull;
  }

  
  
  doc->FlushPendingNotifications(aType);

  return GetPrimaryFrame();
}

void
nsGenericElement::DestroyContent()
{
  nsIDocument *document = GetOwnerDoc();
  if (document) {
    document->BindingManager()->RemovedFromDocument(this, document);
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








static
bool IsAllowedAsChild(nsIContent* aNewChild, nsINode* aParent,
                        bool aIsReplace, nsINode* aRefChild)
{
  NS_PRECONDITION(aNewChild, "Must have new child");
  NS_PRECONDITION(!aIsReplace || aRefChild,
                  "Must have ref content for replace");
  NS_PRECONDITION(aParent->IsNodeOfType(nsINode::eDOCUMENT) ||
                  aParent->IsNodeOfType(nsINode::eDOCUMENT_FRAGMENT) ||
                  aParent->IsElement(),
                  "Nodes that are not documents, document fragments or "
                  "elements can't be parents!");

  if (aParent && nsContentUtils::ContentIsDescendantOf(aParent, aNewChild)) {
    return PR_FALSE;
  }

  
  switch (aNewChild->NodeType()) {
  case nsIDOMNode::COMMENT_NODE :
  case nsIDOMNode::PROCESSING_INSTRUCTION_NODE :
    
    return PR_TRUE;
  case nsIDOMNode::TEXT_NODE :
  case nsIDOMNode::CDATA_SECTION_NODE :
  case nsIDOMNode::ENTITY_REFERENCE_NODE :
    
    return aParent != nsnull;
  case nsIDOMNode::ELEMENT_NODE :
    {
      if (!aParent->IsNodeOfType(nsINode::eDOCUMENT)) {
        
        return PR_TRUE;
      }

      Element* rootElement =
        static_cast<nsIDocument*>(aParent)->GetRootElement();
      if (rootElement) {
        
        
        return aIsReplace && rootElement == aRefChild;
      }

      
      
      if (!aRefChild) {
        
        return PR_TRUE;
      }

      
      nsCOMPtr<nsIDOMDocument> doc = do_QueryInterface(aParent);
      NS_ASSERTION(doc, "Shouldn't happen");
      nsCOMPtr<nsIDOMDocumentType> docType;
      doc->GetDoctype(getter_AddRefs(docType));
      nsCOMPtr<nsIContent> docTypeContent = do_QueryInterface(docType);
      
      if (!docTypeContent) {
        
        return PR_TRUE;
      }

      PRInt32 doctypeIndex = aParent->IndexOf(docTypeContent);
      PRInt32 insertIndex = aParent->IndexOf(aRefChild);

      
      
      
      return aIsReplace ? (insertIndex >= doctypeIndex) :
        insertIndex > doctypeIndex;
    }
  case nsIDOMNode::DOCUMENT_TYPE_NODE :
    {
      if (!aParent->IsNodeOfType(nsINode::eDOCUMENT)) {
        
        return PR_FALSE;
      }

      nsCOMPtr<nsIDOMDocument> doc = do_QueryInterface(aParent);
      NS_ASSERTION(doc, "Shouldn't happen");
      nsCOMPtr<nsIDOMDocumentType> docType;
      doc->GetDoctype(getter_AddRefs(docType));
      nsCOMPtr<nsIContent> docTypeContent = do_QueryInterface(docType);
      if (docTypeContent) {
        
        return aIsReplace && docTypeContent == aRefChild;
      }

      
      
      Element* rootElement =
        static_cast<nsIDocument*>(aParent)->GetRootElement();
      if (!rootElement) {
        
        return PR_TRUE;
      }

      if (!aRefChild) {
        
        return PR_FALSE;
      }

      PRInt32 rootIndex = aParent->IndexOf(rootElement);
      PRInt32 insertIndex = aParent->IndexOf(aRefChild);

      
      
      
      return insertIndex <= rootIndex;
    }
  case nsIDOMNode::DOCUMENT_FRAGMENT_NODE :
    {
      
      
      
      if (!aParent->IsNodeOfType(nsINode::eDOCUMENT)) {
        
        return PR_TRUE;
      }

      bool sawElement = false;
      for (nsIContent* child = aNewChild->GetFirstChild();
           child;
           child = child->GetNextSibling()) {
        if (child->IsElement()) {
          if (sawElement) {
            
            return PR_FALSE;
          }
          sawElement = PR_TRUE;
        }
        
        
        if (!IsAllowedAsChild(child, aParent, aIsReplace, aRefChild)) {
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

void
nsGenericElement::FireNodeInserted(nsIDocument* aDoc,
                                   nsINode* aParent,
                                   nsTArray<nsCOMPtr<nsIContent> >& aNodes)
{
  PRUint32 count = aNodes.Length();
  for (PRUint32 i = 0; i < count; ++i) {
    nsIContent* childContent = aNodes[i];

    if (nsContentUtils::HasMutationListeners(childContent,
          NS_EVENT_BITS_MUTATION_NODEINSERTED, aParent)) {
      nsMutationEvent mutation(PR_TRUE, NS_MUTATION_NODEINSERTED);
      mutation.mRelatedNode = do_QueryInterface(aParent);

      mozAutoSubtreeModified subtree(aDoc, aParent);
      (new nsPLDOMEvent(childContent, mutation))->RunDOMEventWhenSafe();
    }
  }
}

nsresult
nsINode::ReplaceOrInsertBefore(bool aReplace, nsINode* aNewChild,
                               nsINode* aRefChild)
{
  if (!aNewChild || (aReplace && !aRefChild)) {
    return NS_ERROR_NULL_POINTER;
  }

  if ((!IsNodeOfType(eDOCUMENT) &&
       !IsNodeOfType(eDOCUMENT_FRAGMENT) &&
       !IsElement()) ||
      !aNewChild->IsNodeOfType(eCONTENT)){
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  PRUint16 nodeType = aNewChild->NodeType();

  
  
  
  
  
  {
    
    
    
    
    
    if (aRefChild && aRefChild->GetNodeParent() != this) {
      return NS_ERROR_DOM_NOT_FOUND_ERR;
    }

    
    
    if (aReplace && aRefChild != aNewChild) {
      nsContentUtils::MaybeFireNodeRemoved(aRefChild, this, GetOwnerDoc());
    }

    
    
    nsINode* oldParent = aNewChild->GetNodeParent();
    if (oldParent) {
      nsContentUtils::MaybeFireNodeRemoved(aNewChild, oldParent,
                                           aNewChild->GetOwnerDoc());
    }

    
    
    if (nodeType == nsIDOMNode::DOCUMENT_FRAGMENT_NODE) {
      static_cast<nsGenericElement*>(aNewChild)->FireNodeRemovedForChildren();
    }
  }

  nsIDocument* doc = GetOwnerDoc();
  nsIContent* newContent = static_cast<nsIContent*>(aNewChild);
  PRInt32 insPos;

  mozAutoDocUpdate batch(GetCurrentDoc(), UPDATE_CONTENT_MODEL, PR_TRUE);

  
  if (aRefChild) {
    insPos = IndexOf(aRefChild);
    if (insPos < 0) {
      return NS_ERROR_DOM_NOT_FOUND_ERR;
    }
  }
  else {
    insPos = GetChildCount();
  }

  
  if (!IsAllowedAsChild(newContent, this, aReplace, aRefChild)) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  nsresult res;

  
  if (aReplace) {
    res = RemoveChildAt(insPos, PR_TRUE);
    NS_ENSURE_SUCCESS(res, res);
  }

  if (newContent->IsRootOfAnonymousSubtree()) {
    
    
    
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  
  nsCOMPtr<nsINode> oldParent = newContent->GetNodeParent();
  if (oldParent) {
    PRInt32 removeIndex = oldParent->IndexOf(newContent);
    if (removeIndex < 0) {
      
      NS_ERROR("How come our flags didn't catch this?");
      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }

    res = oldParent->RemoveChildAt(removeIndex, PR_TRUE);
    NS_ENSURE_SUCCESS(res, res);

    
    
    if (oldParent == this && removeIndex < insPos) {
      --insPos;
    }
  }

  
  
  
  
  
  if (!HasSameOwnerDoc(newContent)) {
    res = AdoptNodeIntoOwnerDoc(this, aNewChild);
    NS_ENSURE_SUCCESS(res, res);
  }

  




  if (nodeType == nsIDOMNode::DOCUMENT_FRAGMENT_NODE) {
    PRUint32 count = newContent->GetChildCount();

    if (!count) {
      return NS_OK;
    }

    
    
    nsAutoTArray<nsCOMPtr<nsIContent>, 50> fragChildren;
    fragChildren.SetCapacity(count);
    for (nsIContent* child = newContent->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
      NS_ASSERTION(child->GetCurrentDoc() == nsnull,
                   "How did we get a child with a current doc?");
      fragChildren.AppendElement(child);
    }

    
    for (PRUint32 i = count; i > 0;) {
      newContent->RemoveChildAt(--i, PR_TRUE);
    }

    bool appending =
      !IsNodeOfType(eDOCUMENT) && PRUint32(insPos) == GetChildCount();
    PRInt32 firstInsPos = insPos;
    nsIContent* firstInsertedContent = fragChildren[0];

    
    
    for (PRUint32 i = 0; i < count; ++i, ++insPos) {
      
      
      res = InsertChildAt(fragChildren[i], insPos, !appending);
      if (NS_FAILED(res)) {
        
        if (appending && i != 0) {
          nsNodeUtils::ContentAppended(static_cast<nsIContent*>(this),
                                       firstInsertedContent,
                                       firstInsPos);
        }
        return res;
      }
    }

    
    if (appending) {
      nsNodeUtils::ContentAppended(static_cast<nsIContent*>(this),
                                   firstInsertedContent, firstInsPos);
      
      if (nsContentUtils::
            HasMutationListeners(doc, NS_EVENT_BITS_MUTATION_NODEINSERTED)) {
        nsGenericElement::FireNodeInserted(doc, this, fragChildren);
      }
    }
  }
  else {
    

    
    
    
    

    res = InsertChildAt(newContent, insPos, PR_TRUE);
    NS_ENSURE_SUCCESS(res, res);
  }

  return NS_OK;
}

nsresult
nsINode::CompareDocumentPosition(nsIDOMNode* aOther, PRUint16* aReturn)
{
  nsCOMPtr<nsINode> other = do_QueryInterface(aOther);
  if (!other) {
    return NS_ERROR_NULL_POINTER;
  }
  *aReturn = CompareDocPosition(other);
  return NS_OK;
}

nsresult
nsINode::IsEqualNode(nsIDOMNode* aOther, bool* aReturn)
{
  nsCOMPtr<nsINode> other = do_QueryInterface(aOther);
  *aReturn = IsEqualTo(other);
  return NS_OK;
}

nsresult
nsINode::IsSameNode(nsIDOMNode* aOther, bool* aReturn)
{
  nsIDocument* owner = GetOwnerDoc();
  if (owner) {
    owner->WarnOnceAbout(nsIDocument::eIsSameNode);
  }
  nsCOMPtr<nsINode> other = do_QueryInterface(aOther);
  *aReturn = other == this;
  return NS_OK;
}





NS_IMPL_CYCLE_COLLECTION_CLASS(nsGenericElement)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsGenericElement)
  nsINode::Unlink(tmp);

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
      tmp->mFirstChild = nsnull;
    }
  }  

  
  {
    nsDOMSlots *slots = tmp->GetExistingDOMSlots();
    if (slots) {
      slots->Unlink(tmp->IsXUL());
    }
  }

  {
    nsIDocument *doc;
    if (!tmp->GetNodeParent() && (doc = tmp->GetOwnerDoc())) {
      doc->BindingManager()->RemovedFromDocument(tmp, doc);
    }
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsGenericElement)
  nsINode::Trace(tmp, aCallback, aClosure);
NS_IMPL_CYCLE_COLLECTION_TRACE_END

static const char* kNSURIs[] = {
  " ([none])",
  " (xmlns)",
  " (xml)",
  " (xhtml)",
  " (XLink)",
  " (XSLT)",
  " (XBL)",
  " (MathML)",
  " (RDF)",
  " (XUL)",
  " (SVG)",
  " (XML Events)"
};

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INTERNAL(nsGenericElement)
  if (NS_UNLIKELY(cb.WantDebugInfo())) {
    char name[72];
    PRUint32 nsid = tmp->GetNameSpaceID();
    nsAtomCString localName(tmp->NodeInfo()->NameAtom());
    if (nsid < NS_ARRAY_LENGTH(kNSURIs)) {
      PR_snprintf(name, sizeof(name), "nsGenericElement%s %s", kNSURIs[nsid],
                  localName.get());
    }
    else {
      PR_snprintf(name, sizeof(name), "nsGenericElement %s", localName.get());
    }
    cb.DescribeRefCountedNode(tmp->mRefCnt.get(), sizeof(nsGenericElement),
                              name);
  }
  else {
    NS_IMPL_CYCLE_COLLECTION_DESCRIBE(nsGenericElement, tmp->mRefCnt.get())
  }

  
  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS

  if (!nsINode::Traverse(tmp, cb)) {
    return NS_SUCCESS_INTERRUPTED_TRAVERSE;
  }

  nsIDocument* ownerDoc = tmp->GetOwnerDoc();
  if (ownerDoc) {
    ownerDoc->BindingManager()->Traverse(tmp, cb);
  }

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

  
  {
    nsDOMSlots *slots = tmp->GetExistingDOMSlots();
    if (slots) {
      slots->Traverse(cb, tmp->IsXUL());
    }
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_INTERFACE_MAP_BEGIN(nsGenericElement)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsGenericElement)
  NS_INTERFACE_MAP_ENTRY(nsIContent)
  NS_INTERFACE_MAP_ENTRY(nsINode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOMNSElement, new nsNSElementTearoff(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsISupportsWeakReference,
                                 new nsNodeSupportsWeakRefTearoff(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOMNodeSelector,
                                 new nsNodeSelectorTearoff(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOMXPathNSResolver,
                                 new nsNode3Tearoff(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsITouchEventReceiver,
                                 new nsTouchEventReceiverTearoff(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIInlineEventHandlers,
                                 new nsInlineEventHandlersTearoff(this))
  
  
  
  
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContent)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsGenericElement)
NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_DESTROY(nsGenericElement,
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
                                         bool aDefer)
{
  nsIDocument *ownerDoc = GetOwnerDoc();
  if (!ownerDoc || ownerDoc->IsLoadedAsData()) {
    
    
    return NS_OK;
  }

  NS_PRECONDITION(aEventName, "Must have event name!");
  bool defer = true;
  nsEventListenerManager* manager = GetEventListenerManagerForAttr(aEventName,
                                                                   &defer);
  if (!manager) {
    return NS_OK;
  }

  defer = defer && aDefer; 
  PRUint32 lang = GetScriptTypeID();
  manager->AddScriptEventListener(aEventName, aValue, lang, defer,
                                  !nsContentUtils::IsChromeDoc(ownerDoc));
  return NS_OK;
}




const nsAttrName*
nsGenericElement::InternalGetExistingAttrNameFromQName(const nsAString& aStr) const
{
  return mAttrsAndChildren.GetExistingAttrNameFromQName(aStr);
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

bool
nsGenericElement::MaybeCheckSameAttrVal(PRInt32 aNamespaceID, nsIAtom* aName,
                                        nsIAtom* aPrefix, const nsAString& aValue,
                                        bool aNotify, nsAutoString* aOldValue,
                                        PRUint8* aModType, bool* aHasListeners)
{
  bool modification = false;
  *aHasListeners = aNotify &&
    nsContentUtils::HasMutationListeners(this,
                                         NS_EVENT_BITS_MUTATION_ATTRMODIFIED,
                                         this);

  
  
  
  
  
  if (*aHasListeners || aNotify) {
    nsAttrInfo info(GetAttrInfo(aNamespaceID, aName));
    if (info.mValue) {
      
      
      bool valueMatches;
      if (*aHasListeners) {
        
        info.mValue->ToString(*aOldValue);
        valueMatches = aValue.Equals(*aOldValue);
      } else {
        NS_ABORT_IF_FALSE(aNotify,
                          "Either hasListeners or aNotify should be true.");
        valueMatches = info.mValue->Equals(aValue, eCaseMatters);
      }
      if (valueMatches && aPrefix == info.mName->GetPrefix()) {
        return PR_TRUE;
      }
      modification = PR_TRUE;
    }
  }
  *aModType = modification ?
    static_cast<PRUint8>(nsIDOMMutationEvent::MODIFICATION) :
    static_cast<PRUint8>(nsIDOMMutationEvent::ADDITION);
  return PR_FALSE;
}

nsresult
nsGenericElement::SetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                          nsIAtom* aPrefix, const nsAString& aValue,
                          bool aNotify)
{
  

  NS_ENSURE_ARG_POINTER(aName);
  NS_ASSERTION(aNamespaceID != kNameSpaceID_Unknown,
               "Don't call SetAttr with unknown namespace");

  if (!mAttrsAndChildren.CanFitMoreAttrs()) {
    return NS_ERROR_FAILURE;
  }

  PRUint8 modType;
  bool hasListeners;
  nsAutoString oldValue;

  if (MaybeCheckSameAttrVal(aNamespaceID, aName, aPrefix, aValue, aNotify,
                            &oldValue, &modType, &hasListeners)) {
    return NS_OK;
  }

  nsresult rv = BeforeSetAttr(aNamespaceID, aName, &aValue, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNotify) {
    nsNodeUtils::AttributeWillChange(this, aNamespaceID, aName, modType);
  }

  
  
  nsAutoScriptBlocker scriptBlocker;

  nsAttrValue attrValue;
  if (!ParseAttribute(aNamespaceID, aName, aValue, attrValue)) {
    attrValue.SetTo(aValue);
  }

  return SetAttrAndNotify(aNamespaceID, aName, aPrefix, oldValue,
                          attrValue, modType, hasListeners, aNotify,
                          &aValue);
}

nsresult
nsGenericElement::SetParsedAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                nsIAtom* aPrefix, nsAttrValue& aParsedValue,
                                bool aNotify)
{
  

  NS_ENSURE_ARG_POINTER(aName);
  NS_ASSERTION(aNamespaceID != kNameSpaceID_Unknown,
               "Don't call SetAttr with unknown namespace");

  if (!mAttrsAndChildren.CanFitMoreAttrs()) {
    return NS_ERROR_FAILURE;
  }

  nsAutoString value;
  aParsedValue.ToString(value);

  PRUint8 modType;
  bool hasListeners;
  nsAutoString oldValue;

  if (MaybeCheckSameAttrVal(aNamespaceID, aName, aPrefix, value, aNotify,
                            &oldValue, &modType, &hasListeners)) {
    return NS_OK;
  }

  nsresult rv = BeforeSetAttr(aNamespaceID, aName, &value, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNotify) {
    nsNodeUtils::AttributeWillChange(this, aNamespaceID, aName, modType);
  }

  return SetAttrAndNotify(aNamespaceID, aName, aPrefix, oldValue,
                          aParsedValue, modType, hasListeners, aNotify,
                          &value);
}

nsresult
nsGenericElement::SetAttrAndNotify(PRInt32 aNamespaceID,
                                   nsIAtom* aName,
                                   nsIAtom* aPrefix,
                                   const nsAString& aOldValue,
                                   nsAttrValue& aParsedValue,
                                   PRUint8 aModType,
                                   bool aFireMutation,
                                   bool aNotify,
                                   const nsAString* aValueForAfterSetAttr)
{
  nsresult rv;

  nsIDocument* document = GetCurrentDoc();
  mozAutoDocUpdate updateBatch(document, UPDATE_CONTENT_MODEL, aNotify);

  nsMutationGuard::DidMutate();

  if (aNamespaceID == kNameSpaceID_None) {
    
    
    if (!IsAttributeMapped(aName) ||
        !SetMappedAttribute(document, aName, aParsedValue, &rv)) {
      rv = mAttrsAndChildren.SetAndTakeAttr(aName, aParsedValue);
    }
  }
  else {
    nsCOMPtr<nsINodeInfo> ni;
    ni = mNodeInfo->NodeInfoManager()->GetNodeInfo(aName, aPrefix,
                                                   aNamespaceID,
                                                   nsIDOMNode::ATTRIBUTE_NODE);
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

  UpdateState(aNotify);

  if (aNotify) {
    nsNodeUtils::AttributeChanged(this, aNamespaceID, aName, aModType);
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
    nsMutationEvent mutation(PR_TRUE, NS_MUTATION_ATTRMODIFIED);

    nsCOMPtr<nsIDOMAttr> attrNode;
    nsAutoString ns;
    nsContentUtils::NameSpaceManager()->GetNameSpaceURI(aNamespaceID, ns);
    GetAttributeNodeNS(ns, nsDependentAtomString(aName),
                       getter_AddRefs(attrNode));
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
    mutation.mAttrChange = aModType;

    mozAutoSubtreeModified subtree(GetOwnerDoc(), this);
    (new nsPLDOMEvent(this, mutation))->RunDOMEventWhenSafe();
  }

  return NS_OK;
}

bool
nsGenericElement::ParseAttribute(PRInt32 aNamespaceID,
                                 nsIAtom* aAttribute,
                                 const nsAString& aValue,
                                 nsAttrValue& aResult)
{
  return PR_FALSE;
}

bool
nsGenericElement::SetMappedAttribute(nsIDocument* aDocument,
                                     nsIAtom* aName,
                                     nsAttrValue& aValue,
                                     nsresult* aRetval)
{
  *aRetval = NS_OK;
  return PR_FALSE;
}

nsEventListenerManager*
nsGenericElement::GetEventListenerManagerForAttr(nsIAtom* aAttrName,
                                                 bool* aDefer)
{
  *aDefer = PR_TRUE;
  return GetListenerManager(PR_TRUE);
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
  

bool
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

bool
nsGenericElement::HasAttr(PRInt32 aNameSpaceID, nsIAtom* aName) const
{
  NS_ASSERTION(nsnull != aName, "must have attribute name");
  NS_ASSERTION(aNameSpaceID != kNameSpaceID_Unknown,
               "must have a real namespace ID!");

  return mAttrsAndChildren.IndexOfAttr(aName, aNameSpaceID) >= 0;
}

bool
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

bool
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
                            bool aNotify)
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

  bool hasMutationListeners = aNotify &&
    nsContentUtils::HasMutationListeners(this,
                                         NS_EVENT_BITS_MUTATION_ATTRMODIFIED,
                                         this);

  
  nsCOMPtr<nsIDOMAttr> attrNode;
  if (hasMutationListeners) {
    nsAutoString ns;
    nsContentUtils::NameSpaceManager()->GetNameSpaceURI(aNameSpaceID, ns);
    GetAttributeNodeNS(ns, nsDependentAtomString(aName),
                       getter_AddRefs(attrNode));
  }

  
  nsDOMSlots *slots = GetExistingDOMSlots();
  if (slots && slots->mAttributeMap) {
    slots->mAttributeMap->DropAttribute(aNameSpaceID, aName);
  }

  
  
  nsMutationGuard::DidMutate();

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

  UpdateState(aNotify);

  if (aNotify) {
    nsNodeUtils::AttributeChanged(this, aNameSpaceID, aName,
                                  nsIDOMMutationEvent::REMOVAL);
  }

  rv = AfterSetAttr(aNameSpaceID, aName, nsnull, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasMutationListeners) {
    nsCOMPtr<nsIDOMEventTarget> node = do_QueryObject(this);
    nsMutationEvent mutation(PR_TRUE, NS_MUTATION_ATTRMODIFIED);

    mutation.mRelatedNode = attrNode;
    mutation.mAttrName = aName;

    nsAutoString value;
    oldValue.ToString(value);
    if (!value.IsEmpty())
      mutation.mPrevAttrValue = do_GetAtom(value);
    mutation.mAttrChange = nsIDOMMutationEvent::REMOVAL;

    mozAutoSubtreeModified subtree(GetOwnerDoc(), this);
    (new nsPLDOMEvent(this, mutation))->RunDOMEventWhenSafe();
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
                          bool aNotify)
{
  NS_ERROR("called nsGenericElement::SetText");

  return NS_ERROR_FAILURE;
}

nsresult
nsGenericElement::AppendText(const PRUnichar* aBuffer, PRUint32 aLength,
                             bool aNotify)
{
  NS_ERROR("called nsGenericElement::AppendText");

  return NS_ERROR_FAILURE;
}

bool
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

  fputs(NS_LossyConvertUTF16toASCII(mNodeInfo->QualifiedName()).get(), out);

  fprintf(out, "@%p", (void *)this);

  ListAttributes(out);

  fprintf(out, " state=[%llx]", State().GetInternalValue());
  fprintf(out, " flags=[%08x]", static_cast<unsigned int>(GetFlags()));
  fprintf(out, " primaryframe=%p", static_cast<void*>(GetPrimaryFrame()));
  fprintf(out, " refcount=%d<", mRefCnt.get());

  nsIContent* child = GetFirstChild();
  if (child) {
    fputs("\n", out);
    
    for (; child; child = child->GetNextSibling()) {
      child->List(out, aIndent + 1);
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
      PRUint32 length;
      anonymousChildren->GetLength(&length);
      if (length > 0) {
        for (indent = aIndent; --indent >= 0; ) fputs("  ", out);
        fputs("anonymous-children<\n", out);

        for (PRUint32 i = 0; i < length; ++i) {
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
      
      PRUint32 length;
      contentList->GetLength(&length);
      if (length > 0) {
        for (indent = aIndent; --indent >= 0; ) fputs("  ", out);
        fputs("content-list<\n", out);

        for (PRUint32 i = 0; i < length; ++i) {
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
                              bool aDumpAll) const
{
  PRInt32 indent;
  for (indent = aIndent; --indent >= 0; ) fputs("  ", out);

  const nsString& buf = mNodeInfo->QualifiedName();
  fputs("<", out);
  fputs(NS_LossyConvertUTF16toASCII(buf).get(), out);

  if(aDumpAll) ListAttributes(out);

  fputs(">", out);

  if(aIndent) fputs("\n", out);

  for (nsIContent* child = GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    PRInt32 indent = aIndent ? aIndent + 1 : 0;
    child->DumpContent(out, indent, aDumpAll);
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
  return new nsDOMSlots();
}

bool
nsGenericElement::CheckHandleEventForLinksPrecondition(nsEventChainVisitor& aVisitor,
                                                       nsIURI** aURI) const
{
  if (aVisitor.mEventStatus == nsEventStatus_eConsumeNoDefault ||
      (!NS_IS_TRUSTED_EVENT(aVisitor.mEvent) &&
       (aVisitor.mEvent->message != NS_MOUSE_CLICK) &&
       (aVisitor.mEvent->message != NS_KEY_PRESS) &&
       (aVisitor.mEvent->message != NS_UI_ACTIVATE)) ||
      !aVisitor.mPresContext ||
      (aVisitor.mEvent->flags & NS_EVENT_FLAG_PREVENT_MULTIPLE_ACTIONS)) {
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
    if (aVisitor.mEvent->eventStructType != NS_FOCUS_EVENT ||
        !static_cast<nsFocusEvent*>(aVisitor.mEvent)->isRefocus) {
      nsAutoString target;
      GetLinkTarget(target);
      nsContentUtils::TriggerLink(this, aVisitor.mPresContext, absURI, target,
                                  PR_FALSE, PR_TRUE, PR_TRUE);
      
      aVisitor.mEvent->flags |= NS_EVENT_FLAG_PREVENT_MULTIPLE_ACTIONS;
    }
    break;

  case NS_MOUSE_EXIT_SYNTH:
    aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
    
  case NS_BLUR_CONTENT:
    rv = LeaveLink(aVisitor.mPresContext);
    if (NS_SUCCEEDED(rv)) {
      aVisitor.mEvent->flags |= NS_EVENT_FLAG_PREVENT_MULTIPLE_ACTIONS;
    }
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
            aVisitor.mEvent->flags |= NS_EVENT_FLAG_PREVENT_MULTIPLE_ACTIONS;
            nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(this);
            fm->SetFocus(elem, nsIFocusManager::FLAG_BYMOUSE |
                               nsIFocusManager::FLAG_NOSCROLL);
          }

          nsEventStateManager::SetActiveManager(
            aVisitor.mPresContext->EventStateManager(), this);
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
        if (NS_SUCCEEDED(rv)) {
          aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
        }
      }
    }
    break;

  case NS_UI_ACTIVATE:
    {
      if (aVisitor.mEvent->originalTarget == this) {
        nsAutoString target;
        GetLinkTarget(target);
        nsContentUtils::TriggerLink(this, aVisitor.mPresContext, absURI, target,
                                    PR_TRUE, PR_TRUE, NS_IS_TRUSTED_EVENT(aVisitor.mEvent));
        aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
      }
    }
    break;

  case NS_KEY_PRESS:
    {
      if (aVisitor.mEvent->eventStructType == NS_KEY_EVENT) {
        nsKeyEvent* keyEvent = static_cast<nsKeyEvent*>(aVisitor.mEvent);
        if (keyEvent->keyCode == NS_VK_RETURN) {
          nsEventStatus status = nsEventStatus_eIgnore;
          rv = DispatchClickEvent(aVisitor.mPresContext, keyEvent, this,
                                  PR_FALSE, 0, &status);
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
nsGenericElement::FireNodeRemovedForChildren()
{
  nsIDocument* doc = GetOwnerDoc();
  
  if (!nsContentUtils::
        HasMutationListeners(doc, NS_EVENT_BITS_MUTATION_NODEREMOVED)) {
    return;
  }

  nsCOMPtr<nsIDocument> owningDoc = doc;

  nsCOMPtr<nsINode> child;
  for (child = GetFirstChild();
       child && child->GetNodeParent() == this;
       child = child->GetNextSibling()) {
    nsContentUtils::MaybeFireNodeRemoved(child, this, doc);
  }
}

void
nsGenericElement::GetLinkTarget(nsAString& aTarget)
{
  aTarget.Truncate();
}




static nsresult
ParseSelectorList(nsINode* aNode,
                  const nsAString& aSelectorString,
                  nsCSSSelectorList** aSelectorList)
{
  NS_ENSURE_ARG(aNode);

  nsIDocument* doc = aNode->GetOwnerDoc();
  NS_ENSURE_STATE(doc);

  nsCSSParser parser(doc->CSSLoader());

  nsCSSSelectorList* selectorList;
  nsresult rv = parser.ParseSelectorString(aSelectorString,
                                           doc->GetDocumentURI(),
                                           0, 
                                           &selectorList);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCSSSelectorList** slot = &selectorList;
  do {
    nsCSSSelectorList* cur = *slot;
    if (cur->mSelectors->IsPseudoElement()) {
      *slot = cur->mNext;
      cur->mNext = nsnull;
      delete cur;
    } else {
      slot = &cur->mNext;
    }
  } while (*slot);
  *aSelectorList = selectorList;

  return NS_OK;
}


nsIContent*
nsGenericElement::doQuerySelector(nsINode* aRoot, const nsAString& aSelector,
                                  nsresult *aResult)
{
  NS_PRECONDITION(aResult, "Null out param?");

  nsAutoPtr<nsCSSSelectorList> selectorList;
  *aResult = ParseSelectorList(aRoot, aSelector,
                               getter_Transfers(selectorList));
  NS_ENSURE_SUCCESS(*aResult, nsnull);

  TreeMatchContext matchingContext(PR_FALSE,
                                   nsRuleWalker::eRelevantLinkUnvisited,
                                   aRoot->GetOwnerDoc());
  for (nsIContent* cur = aRoot->GetFirstChild();
       cur;
       cur = cur->GetNextNode(aRoot)) {
    if (cur->IsElement() &&
        nsCSSRuleProcessor::SelectorListMatches(cur->AsElement(),
                                                matchingContext,
                                                selectorList)) {
      return cur;
    }
  }

  return nsnull;
}


nsresult
nsGenericElement::doQuerySelectorAll(nsINode* aRoot,
                                     const nsAString& aSelector,
                                     nsIDOMNodeList **aReturn)
{
  NS_PRECONDITION(aReturn, "Null out param?");

  nsSimpleContentList* contentList = new nsSimpleContentList(aRoot);
  NS_ENSURE_TRUE(contentList, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aReturn = contentList);
  
  nsAutoPtr<nsCSSSelectorList> selectorList;
  nsresult rv = ParseSelectorList(aRoot, aSelector,
                                  getter_Transfers(selectorList));
  NS_ENSURE_SUCCESS(rv, rv);

  TreeMatchContext matchingContext(PR_FALSE,
                                   nsRuleWalker::eRelevantLinkUnvisited,
                                   aRoot->GetOwnerDoc());
  for (nsIContent* cur = aRoot->GetFirstChild();
       cur;
       cur = cur->GetNextNode(aRoot)) {
    if (cur->IsElement() &&
        nsCSSRuleProcessor::SelectorListMatches(cur->AsElement(),
                                                matchingContext,
                                                selectorList)) {
      contentList->AppendElement(cur);
    }
  }
  return NS_OK;
}


bool
nsGenericElement::MozMatchesSelector(const nsAString& aSelector, nsresult* aResult)
{
  nsAutoPtr<nsCSSSelectorList> selectorList;
  bool matches = false;

  *aResult = ParseSelectorList(this, aSelector, getter_Transfers(selectorList));

  if (NS_SUCCEEDED(*aResult)) {
    TreeMatchContext matchingContext(PR_FALSE,
                                     nsRuleWalker::eRelevantLinkUnvisited,
                                     GetOwnerDoc());
    matches = nsCSSRuleProcessor::SelectorListMatches(this, matchingContext,
                                                      selectorList);
  }

  return matches;
}

NS_IMETHODIMP
nsNSElementTearoff::MozMatchesSelector(const nsAString& aSelector, bool* aReturn)
{
  NS_PRECONDITION(aReturn, "Null out param?");

  nsresult rv;
  *aReturn = mContent->MozMatchesSelector(aSelector, &rv);

  return rv;
}

PRInt64
nsINode::SizeOf() const
{
  PRInt64 size = sizeof(*this);

  nsEventListenerManager* elm =
    const_cast<nsINode*>(this)->GetListenerManager(PR_FALSE);
  if (elm) {
    size += elm->SizeOf();
  }

  return size;
}

PRInt64
nsGenericElement::SizeOf() const
{
  PRInt64 size = MemoryReporter::GetBasicSize<nsGenericElement, Element>(this);

  size -= sizeof(mAttrsAndChildren);
  size += mAttrsAndChildren.SizeOf();

  return size;
}

#define EVENT(name_, id_, type_, struct_)                                    \
  NS_IMETHODIMP nsINode::GetOn##name_(JSContext *cx, jsval *vp) {            \
    nsEventListenerManager *elm = GetListenerManager(PR_FALSE);              \
    if (elm) {                                                               \
      elm->GetJSEventListener(nsGkAtoms::on##name_, vp);                     \
    } else {                                                                 \
      *vp = JSVAL_NULL;                                                      \
    }                                                                        \
    return NS_OK;                                                            \
  }                                                                          \
  NS_IMETHODIMP nsINode::SetOn##name_(JSContext *cx, const jsval &v) {       \
    nsEventListenerManager *elm = GetListenerManager(PR_TRUE);               \
    if (!elm) {                                                              \
      return NS_ERROR_OUT_OF_MEMORY;                                         \
    }                                                                        \
                                                                             \
    JSObject *obj = GetWrapper();                                            \
    if (!obj) {                                                              \
      /* Just silently do nothing */                                         \
      return NS_OK;                                                          \
    }                                                                        \
    return elm->SetJSEventListenerToJsval(nsGkAtoms::on##name_, cx, obj, v); \
}
#define TOUCH_EVENT EVENT
#define DOCUMENT_ONLY_EVENT EVENT
#include "nsEventNameList.h"
#undef DOCUMENT_ONLY_EVENT
#undef TOUCH_EVENT
#undef EVENT

bool
nsINode::Contains(const nsINode* aOther) const
{
  if (aOther == this) {
    return PR_TRUE;
  }
  if (!aOther ||
      GetOwnerDoc() != aOther->GetOwnerDoc() ||
      IsInDoc() != aOther->IsInDoc() ||
      !(aOther->IsElement() ||
        aOther->IsNodeOfType(nsINode::eCONTENT)) ||
      !GetFirstChild()) {
    return PR_FALSE;
  }

  const nsIContent* other = static_cast<const nsIContent*>(aOther);
  if (this == GetOwnerDoc()) {
    
    
    
    return !other->IsInAnonymousSubtree();
  }

  if (!IsElement() && !IsNodeOfType(nsINode::eDOCUMENT_FRAGMENT)) {
    return PR_FALSE;
  }

  const nsIContent* thisContent = static_cast<const nsIContent*>(this);
  if (thisContent->GetBindingParent() != other->GetBindingParent()) {
    return PR_FALSE;
  }

  return nsContentUtils::ContentIsDescendantOf(other, this);
}

nsresult
nsINode::Contains(nsIDOMNode* aOther, bool* aReturn)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aOther);
  *aReturn = Contains(node);
  return NS_OK;
}
