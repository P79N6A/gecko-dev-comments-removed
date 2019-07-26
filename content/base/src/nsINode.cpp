









#include "nsINode.h"

#include "jsapi.h"
#include "mozAutoDocUpdate.h"
#include "mozilla/CORSMode.h"
#include "mozilla/Likely.h"
#include "mozilla/Telemetry.h"
#include "mozilla/Util.h"
#include "nsAsyncDOMEvent.h"
#include "nsAttrValueOrString.h"
#include "nsBindingManager.h"
#include "nsCCUncollectableMarker.h"
#include "nsClientRect.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentList.h"
#include "nsContentUtils.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCycleCollector.h"
#include "nsDocument.h"
#include "nsDOMAttribute.h"
#include "nsDOMAttributeMap.h"
#include "nsDOMCID.h"
#include "nsDOMCSSAttrDeclaration.h"
#include "nsError.h"
#include "nsDOMMutationObserver.h"
#include "nsDOMString.h"
#include "nsDOMTokenList.h"
#include "nsEventDispatcher.h"
#include "nsEventListenerManager.h"
#include "nsEventStateManager.h"
#include "nsFocusManager.h"
#include "nsFrameManager.h"
#include "nsFrameSelection.h"
#include "mozilla/dom/Element.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsIAnonymousContentCreator.h"
#include "nsIAtom.h"
#include "nsIBaseWindow.h"
#include "nsICategoryManager.h"
#include "nsIContentIterator.h"
#include "nsIControllers.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMMutationEvent.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMUserDataHandler.h"
#include "nsIEditor.h"
#include "nsIEditorIMESupport.h"
#include "nsIFrame.h"
#include "nsIJSContextStack.h"
#include "nsILinkHandler.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsIPresShell.h"
#include "nsIScriptError.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScrollableFrame.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsView.h"
#include "nsViewManager.h"
#include "nsIWebNavigation.h"
#include "nsIWidget.h"
#include "nsLayoutStatics.h"
#include "nsLayoutUtils.h"
#include "nsMutationEvent.h"
#include "nsNetUtil.h"
#include "nsNodeInfoManager.h"
#include "nsNodeUtils.h"
#include "nsPIBoxObject.h"
#include "nsPIDOMWindow.h"
#include "nsPresContext.h"
#include "nsRuleProcessorData.h"
#include "nsString.h"
#include "nsStyleConsts.h"
#include "nsSVGFeatures.h"
#include "nsSVGUtils.h"
#include "nsTextNode.h"
#include "nsUnicharUtils.h"
#include "nsXBLBinding.h"
#include "nsXBLInsertionPoint.h"
#include "nsXBLPrototypeBinding.h"
#include "prprf.h"
#include "xpcpublic.h"
#include "nsCSSRuleProcessor.h"
#include "nsCSSParser.h"
#include "HTMLLegendElement.h"
#include "nsWrapperCacheInlines.h"
#include "WrapperFactory.h"
#include "DocumentType.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::dom;

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
  NS_ASSERTION(mSubtreeRoot == this, "Didn't restore state properly?");
}

void*
nsINode::GetProperty(uint16_t aCategory, nsIAtom *aPropertyName,
                     nsresult *aStatus) const
{
  return OwnerDoc()->PropertyTable(aCategory)->GetProperty(this, aPropertyName,
                                                           aStatus);
}

nsresult
nsINode::SetProperty(uint16_t aCategory, nsIAtom *aPropertyName, void *aValue,
                     NSPropertyDtorFunc aDtor, bool aTransfer,
                     void **aOldValue)
{
  nsresult rv = OwnerDoc()->PropertyTable(aCategory)->SetProperty(this,
                                                                  aPropertyName,
                                                                  aValue, aDtor,
                                                                  nullptr,
                                                                  aTransfer,
                                                                  aOldValue);
  if (NS_SUCCEEDED(rv)) {
    SetFlags(NODE_HAS_PROPERTIES);
  }

  return rv;
}

void
nsINode::DeleteProperty(uint16_t aCategory, nsIAtom *aPropertyName)
{
  OwnerDoc()->PropertyTable(aCategory)->DeleteProperty(this, aPropertyName);
}

void*
nsINode::UnsetProperty(uint16_t aCategory, nsIAtom *aPropertyName,
                       nsresult *aStatus)
{
  return OwnerDoc()->PropertyTable(aCategory)->UnsetProperty(this,
                                                             aPropertyName,
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
    
    return true;
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
    *aEditor = nullptr;
  for (nsINode* node = this; node; node = node->GetParentNode()) {
    if (!node->IsElement() ||
        !node->AsElement()->IsHTML())
      continue;

    nsCOMPtr<nsIEditor> editor =
      static_cast<nsGenericHTMLElement*>(node)->GetEditorInternal();
    if (!editor)
      continue;

    nsIContent* rootContent = GetEditorRootContent(editor);
    if (aEditor)
      editor.swap(*aEditor);
    return rootContent;
  }
  return nullptr;
}

static nsIContent* GetRootForContentSubtree(nsIContent* aContent)
{
  NS_ENSURE_TRUE(aContent, nullptr);
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
  NS_ENSURE_TRUE(aPresShell, nullptr);

  if (IsNodeOfType(eDOCUMENT))
    return static_cast<nsIDocument*>(this)->GetRootElement();
  if (!IsNodeOfType(eCONTENT))
    return nullptr;

  if (GetCurrentDoc() != aPresShell->GetDocument()) {
    return nullptr;
  }

  if (static_cast<nsIContent*>(this)->HasIndependentSelection()) {
    
    nsIContent* content = GetTextEditorRootContent();
    if (content)
      return content;
  }

  nsPresContext* presContext = aPresShell->GetPresContext();
  if (presContext) {
    nsIEditor* editor = nsContentUtils::GetHTMLEditor(presContext);
    if (editor) {
      
      nsIDocument* doc = GetCurrentDoc();
      if (!doc || doc->HasFlag(NODE_IS_EDITABLE) ||
          !HasFlag(NODE_IS_EDITABLE)) {
        nsIContent* editorRoot = GetEditorRootContent(editor);
        NS_ENSURE_TRUE(editorRoot, nullptr);
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
      NS_ENSURE_TRUE(doc, nullptr);
      content = doc->GetRootElement();
      if (!content)
        return nullptr;
    }
  }

  
  
  NS_ENSURE_TRUE(content, nullptr);
  return nsContentUtils::IsInSameAnonymousTree(this, content) ?
           content : GetRootForContentSubtree(static_cast<nsIContent*>(this));
}

nsINodeList*
nsINode::ChildNodes()
{
  nsSlots* slots = Slots();
  if (!slots->mChildNodes) {
    slots->mChildNodes = new nsChildContentList(this);
    if (slots->mChildNodes) {
      NS_ADDREF(slots->mChildNodes);
    }
  }

  return slots->mChildNodes;
}

void
nsINode::GetTextContentInternal(nsAString& aTextContent)
{
  SetDOMStringToNull(aTextContent);
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
  *aParentNode = nullptr;

  nsINode *parent = GetParentNode();

  return parent ? CallQueryInterface(parent, aParentNode) : NS_OK;
}

nsresult
nsINode::GetParentElement(nsIDOMElement** aParentElement)
{
  *aParentElement = nullptr;
  nsINode* parent = GetParentElement();
  return parent ? CallQueryInterface(parent, aParentElement) : NS_OK;
}

nsresult
nsINode::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
  NS_ADDREF(*aChildNodes = ChildNodes());

  return NS_OK;
}

nsresult
nsINode::GetFirstChild(nsIDOMNode** aNode)
{
  nsIContent* child = GetFirstChild();
  if (child) {
    return CallQueryInterface(child, aNode);
  }

  *aNode = nullptr;

  return NS_OK;
}

nsresult
nsINode::GetLastChild(nsIDOMNode** aNode)
{
  nsIContent* child = GetLastChild();
  if (child) {
    return CallQueryInterface(child, aNode);
  }

  *aNode = nullptr;

  return NS_OK;
}

nsresult
nsINode::GetPreviousSibling(nsIDOMNode** aPrevSibling)
{
  *aPrevSibling = nullptr;

  nsIContent *sibling = GetPreviousSibling();

  return sibling ? CallQueryInterface(sibling, aPrevSibling) : NS_OK;
}

nsresult
nsINode::GetNextSibling(nsIDOMNode** aNextSibling)
{
  *aNextSibling = nullptr;

  nsIContent *sibling = GetNextSibling();

  return sibling ? CallQueryInterface(sibling, aNextSibling) : NS_OK;
}

nsresult
nsINode::GetOwnerDocument(nsIDOMDocument** aOwnerDocument)
{
  *aOwnerDocument = nullptr;

  nsIDocument *ownerDoc = GetOwnerDocument();

  return ownerDoc ? CallQueryInterface(ownerDoc, aOwnerDocument) : NS_OK;
}

void
nsINode::GetNodeValueInternal(nsAString& aNodeValue)
{
  SetDOMStringToNull(aNodeValue);
}

nsINode*
nsINode::RemoveChild(nsINode& aOldChild, ErrorResult& aError)
{
  if (IsNodeOfType(eDATA_NODE)) {
    
    aError.Throw(NS_ERROR_DOM_NOT_FOUND_ERR);
    return nullptr;
  }

  if (aOldChild.GetParentNode() == this) {
    nsContentUtils::MaybeFireNodeRemoved(&aOldChild, this, OwnerDoc());
  }

  int32_t index = IndexOf(&aOldChild);
  if (index == -1) {
    
    aError.Throw(NS_ERROR_DOM_NOT_FOUND_ERR);
    return nullptr;
  }

  RemoveChildAt(index, true);
  return &aOldChild;
}

nsresult
nsINode::RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
  nsCOMPtr<nsINode> oldChild = do_QueryInterface(aOldChild);
  if (!oldChild) {
    return NS_ERROR_NULL_POINTER;
  }

  ErrorResult rv;
  RemoveChild(*oldChild, rv);
  if (!rv.Failed()) {
    NS_ADDREF(*aReturn = aOldChild);
  }
  return rv.ErrorCode();
}

void
nsINode::Normalize()
{
  
  nsAutoTArray<nsCOMPtr<nsIContent>, 50> nodes;

  bool canMerge = false;
  for (nsIContent* node = this->GetFirstChild();
       node;
       node = node->GetNextNode(this)) {
    if (node->NodeType() != nsIDOMNode::TEXT_NODE) {
      canMerge = false;
      continue;
    }

    if (canMerge || node->TextLength() == 0) {
      
      
      nodes.AppendElement(node);
    }
    else {
      canMerge = true;
    }

    
    
    canMerge = canMerge && !!node->GetNextSibling();
  }

  if (nodes.IsEmpty()) {
    return;
  }

  
  nsIDocument* doc = OwnerDoc();

  
  mozAutoSubtreeModified subtree(doc, nullptr);

  
  
  bool hasRemoveListeners = nsContentUtils::
      HasMutationListeners(doc, NS_EVENT_BITS_MUTATION_NODEREMOVED);
  if (hasRemoveListeners) {
    for (uint32_t i = 0; i < nodes.Length(); ++i) {
      nsContentUtils::MaybeFireNodeRemoved(nodes[i], nodes[i]->GetParentNode(),
                                           doc);
    }
  }

  mozAutoDocUpdate batch(doc, UPDATE_CONTENT_MODEL, true);

  
  nsAutoString tmpStr;
  for (uint32_t i = 0; i < nodes.Length(); ++i) {
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
          t->AppendTextForNormalize(text->Get2b(), text->GetLength(), true, node);
        }
        else {
          tmpStr.Truncate();
          text->AppendTo(tmpStr);
          t->AppendTextForNormalize(tmpStr.get(), tmpStr.Length(), true, node);
        }
      }
    }

    
    nsCOMPtr<nsINode> parent = node->GetParentNode();
    NS_ASSERTION(parent || hasRemoveListeners,
                 "Should always have a parent unless "
                 "mutation events messed us up");
    if (parent) {
      parent->RemoveChildAt(parent->IndexOf(node), true);
    }
  }
}

void
nsINode::GetBaseURI(nsAString &aURI) const
{
  nsCOMPtr<nsIURI> baseURI = GetBaseURI();

  nsAutoCString spec;
  if (baseURI) {
    baseURI->GetSpec(spec);
  }

  CopyUTF8toUTF16(spec, aURI);
}

void
nsINode::LookupPrefix(const nsAString& aNamespaceURI, nsAString& aPrefix)
{
  Element *element = GetNameSpaceElement();
  if (element) {
    
  
    
    
    
    for (nsIContent* content = element; content;
         content = content->GetParent()) {
      uint32_t attrCount = content->GetAttrCount();
  
      for (uint32_t i = 0; i < attrCount; ++i) {
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
          return;
        }
      }
    }
  }

  SetDOMStringToNull(aPrefix);
}

static nsresult
SetUserDataProperty(uint16_t aCategory, nsINode *aNode, nsIAtom *aKey,
                    nsISupports* aValue, void** aOldValue)
{
  nsresult rv = aNode->SetProperty(aCategory, aKey, aValue,
                                   nsPropertyTable::SupportsDtorFunc, true,
                                   aOldValue);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NS_ADDREF(aValue);

  return NS_OK;
}

nsresult
nsINode::SetUserData(const nsAString &aKey, nsIVariant *aData,
                     nsIDOMUserDataHandler *aHandler, nsIVariant **aResult)
{
  *aResult = nullptr;

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

JS::Value
nsINode::SetUserData(JSContext* aCx, const nsAString& aKey, JS::Value aData,
                     nsIDOMUserDataHandler* aHandler, ErrorResult& aError)
{
  nsCOMPtr<nsIVariant> data;
  aError = nsContentUtils::XPConnect()->JSValToVariant(aCx, &aData,
                                                       getter_AddRefs(data));
  if (aError.Failed()) {
    return JS::UndefinedValue();
  }

  nsCOMPtr<nsIVariant> oldData;
  aError = SetUserData(aKey, data, aHandler, getter_AddRefs(oldData));
  if (aError.Failed()) {
    return JS::UndefinedValue();
  }

  if (!oldData) {
    return JS::NullValue();
  }

  JS::Value result;
  JSAutoCompartment ac(aCx, GetWrapper());
  aError = nsContentUtils::XPConnect()->VariantToJS(aCx, GetWrapper(), oldData,
                                                    &result);
  return result;
}

JS::Value
nsINode::GetUserData(JSContext* aCx, const nsAString& aKey, ErrorResult& aError)
{
  nsIVariant* data = GetUserData(aKey);
  if (!data) {
    return JS::NullValue();
  }

  JS::Value result;
  JSAutoCompartment ac(aCx, GetWrapper());
  aError = nsContentUtils::XPConnect()->VariantToJS(aCx, GetWrapper(), data,
                                                    &result);
  return result;
}

uint16_t
nsINode::CompareDocumentPosition(nsINode& aOtherNode) const
{
  if (this == &aOtherNode) {
    return 0;
  }

  nsAutoTArray<const nsINode*, 32> parents1, parents2;

  const nsINode *node1 = &aOtherNode, *node2 = this;

  
  const nsIAttribute* attr1 = nullptr;
  if (node1->IsNodeOfType(nsINode::eATTRIBUTE)) {
    attr1 = static_cast<const nsIAttribute*>(node1);
    const nsIContent* elem = attr1->GetContent();
    
    
    if (elem) {
      node1 = elem;
      parents1.AppendElement(attr1);
    }
  }
  if (node2->IsNodeOfType(nsINode::eATTRIBUTE)) {
    const nsIAttribute* attr2 = static_cast<const nsIAttribute*>(node2);
    const nsIContent* elem = attr2->GetContent();
    if (elem == node1 && attr1) {
      
      

      uint32_t i;
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
      parents2.AppendElement(attr2);
    }
  }

  
  
  
  

  
  do {
    parents1.AppendElement(node1);
    node1 = node1->GetParentNode();
  } while (node1);
  do {
    parents2.AppendElement(node2);
    node2 = node2->GetParentNode();
  } while (node2);

  
  uint32_t pos1 = parents1.Length();
  uint32_t pos2 = parents2.Length();
  const nsINode* top1 = parents1.ElementAt(--pos1);
  const nsINode* top2 = parents2.ElementAt(--pos2);
  if (top1 != top2) {
    return top1 < top2 ?
      (nsIDOMNode::DOCUMENT_POSITION_PRECEDING |
       nsIDOMNode::DOCUMENT_POSITION_DISCONNECTED |
       nsIDOMNode::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC) :
      (nsIDOMNode::DOCUMENT_POSITION_FOLLOWING |
       nsIDOMNode::DOCUMENT_POSITION_DISCONNECTED |
       nsIDOMNode::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC);
  }

  
  const nsINode* parent = top1;
  uint32_t len;
  for (len = std::min(pos1, pos2); len > 0; --len) {
    const nsINode* child1 = parents1.ElementAt(--pos1);
    const nsINode* child2 = parents2.ElementAt(--pos2);
    if (child1 != child2) {
      
      
      
      return parent->IndexOf(child1) < parent->IndexOf(child2) ?
        static_cast<uint16_t>(nsIDOMNode::DOCUMENT_POSITION_PRECEDING) :
        static_cast<uint16_t>(nsIDOMNode::DOCUMENT_POSITION_FOLLOWING);
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
nsINode::IsEqualNode(nsINode* aOther)
{
  if (!aOther) {
    return false;
  }

  nsAutoString string1, string2;

  nsINode* node1 = this;
  nsINode* node2 = aOther;
  do {
    uint16_t nodeType = node1->NodeType();
    if (nodeType != node2->NodeType()) {
      return false;
    }

    nsINodeInfo* nodeInfo1 = node1->mNodeInfo;
    nsINodeInfo* nodeInfo2 = node2->mNodeInfo;
    if (!nodeInfo1->Equals(nodeInfo2) ||
        nodeInfo1->GetExtraName() != nodeInfo2->GetExtraName()) {
      return false;
    }

    switch(nodeType) {
      case nsIDOMNode::ELEMENT_NODE:
      {
        
        
        Element* element1 = node1->AsElement();
        Element* element2 = node2->AsElement();
        uint32_t attrCount = element1->GetAttrCount();
        if (attrCount != element2->GetAttrCount()) {
          return false;
        }

        
        for (uint32_t i = 0; i < attrCount; ++i) {
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
            return false;
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
          return false;
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
        node1->GetNodeValue(string1);
        node2->GetNodeValue(string2);
        
        
        
        
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
          return false;
        }
    
        
        docType1->GetSystemId(string1);
        docType2->GetSystemId(string2);
        if (!string1.Equals(string2)) {
          return false;
        }
    
        
        docType1->GetInternalSubset(string1);
        docType2->GetInternalSubset(string2);
        if (!string1.Equals(string2)) {
          return false;
        }

        break;
      }
      default:
        NS_ABORT_IF_FALSE(false, "Unknown node type");
    }

    nsINode* nextNode = node1->GetFirstChild();
    if (nextNode) {
      node1 = nextNode;
      node2 = node2->GetFirstChild();
    }
    else {
      if (node2->GetFirstChild()) {
        
        return false;
      }

      
      while (1) {
        if (node1 == this) {
          NS_ASSERTION(node2 == aOther, "Should have reached the start node "
                                        "for both trees at the same time");
          return true;
        }

        nextNode = node1->GetNextSibling();
        if (nextNode) {
          node1 = nextNode;
          node2 = node2->GetNextSibling();
          break;
        }

        if (node2->GetNextSibling()) {
          
          return false;
        }
        
        node1 = node1->GetParentNode();
        node2 = node2->GetParentNode();
        NS_ASSERTION(node1 && node2, "no parent while walking subtree");
      }
    }
  } while(node2);

  return false;
}

void
nsINode::LookupNamespaceURI(const nsAString& aNamespacePrefix,
                            nsAString& aNamespaceURI)
{
  Element *element = GetNameSpaceElement();
  if (!element ||
      NS_FAILED(element->LookupNamespaceURIInternal(aNamespacePrefix,
                                                    aNamespaceURI))) {
    SetDOMStringToNull(aNamespaceURI);
  }
}

NS_IMPL_DOMTARGET_DEFAULTS(nsINode)

NS_IMETHODIMP
nsINode::AddEventListener(const nsAString& aType,
                          nsIDOMEventListener *aListener,
                          bool aUseCapture,
                          bool aWantsUntrusted,
                          uint8_t aOptionalArgc)
{
  NS_ASSERTION(!aWantsUntrusted || aOptionalArgc > 1,
               "Won't check if this is chrome, you want to set "
               "aWantsUntrusted to false or make the aWantsUntrusted "
               "explicit by making aOptionalArgc non-zero.");

  if (!aWantsUntrusted &&
      (aOptionalArgc < 2 &&
       !nsContentUtils::IsChromeDoc(OwnerDoc()))) {
    aWantsUntrusted = true;
  }

  nsEventListenerManager* listener_manager = GetListenerManager(true);
  NS_ENSURE_STATE(listener_manager);
  listener_manager->AddEventListener(aType, aListener, aUseCapture,
                                     aWantsUntrusted);
  return NS_OK;
}

NS_IMETHODIMP
nsINode::AddSystemEventListener(const nsAString& aType,
                                nsIDOMEventListener *aListener,
                                bool aUseCapture,
                                bool aWantsUntrusted,
                                uint8_t aOptionalArgc)
{
  NS_ASSERTION(!aWantsUntrusted || aOptionalArgc > 1,
               "Won't check if this is chrome, you want to set "
               "aWantsUntrusted to false or make the aWantsUntrusted "
               "explicit by making aOptionalArgc non-zero.");

  if (!aWantsUntrusted &&
      (aOptionalArgc < 2 &&
       !nsContentUtils::IsChromeDoc(OwnerDoc()))) {
    aWantsUntrusted = true;
  }

  return NS_AddSystemEventListener(this, aType, aListener, aUseCapture,
                                   aWantsUntrusted);
}

NS_IMETHODIMP
nsINode::RemoveEventListener(const nsAString& aType,
                             nsIDOMEventListener* aListener,
                             bool aUseCapture)
{
  nsEventListenerManager* elm = GetListenerManager(false);
  if (elm) {
    elm->RemoveEventListener(aType, aListener, aUseCapture);
  }
  return NS_OK;
}

NS_IMPL_REMOVE_SYSTEM_EVENT_LISTENER(nsINode)

nsresult
nsINode::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  
  NS_ABORT();
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsINode::DispatchEvent(nsIDOMEvent *aEvent, bool* aRetVal)
{
  
  
  nsCOMPtr<nsIDocument> document = OwnerDoc();

  
  if (!document) {
    *aRetVal = true;
    return NS_OK;
  }

  
  nsIPresShell *shell = document->GetShell();
  nsRefPtr<nsPresContext> context;
  if (shell) {
    context = shell->GetPresContext();
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  nsresult rv =
    nsEventDispatcher::DispatchDOMEvent(this, nullptr, aEvent, context,
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
nsINode::UnoptimizableCCNode() const
{
  const uintptr_t problematicFlags = (NODE_IS_ANONYMOUS |
                                      NODE_IS_IN_ANONYMOUS_SUBTREE |
                                      NODE_IS_NATIVE_ANONYMOUS_ROOT |
                                      NODE_MAY_BE_IN_BINDING_MNGR |
                                      NODE_IS_INSERTION_PARENT);
  return HasFlag(problematicFlags) ||
         NodeType() == nsIDOMNode::ATTRIBUTE_NODE ||
         
         (IsElement() &&
          AsElement()->IsInNamespace(kNameSpaceID_XBL));
}


bool
nsINode::Traverse(nsINode *tmp, nsCycleCollectionTraversalCallback &cb)
{
  if (MOZ_LIKELY(!cb.WantAllTraces())) {
    nsIDocument *currentDoc = tmp->GetCurrentDoc();
    if (currentDoc &&
        nsCCUncollectableMarker::InGeneration(currentDoc->GetMarkedCCGeneration())) {
      return false;
    }

    if (nsCCUncollectableMarker::sGeneration) {
      
      if (tmp->IsBlack() || tmp->InCCBlackTree()) {
        return false;
      }

      if (!tmp->UnoptimizableCCNode()) {
        
        if ((currentDoc && currentDoc->IsBlack())) {
          return false;
        }
        
        
        nsIContent* parent = tmp->GetParent();
        if (parent && !parent->UnoptimizableCCNode() && parent->IsBlack()) {
          NS_ABORT_IF_FALSE(parent->IndexOf(tmp) >= 0, "Parent doesn't own us?");
          return false;
        }
      }
    }
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNodeInfo)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(GetParent())

  nsSlots *slots = tmp->GetExistingSlots();
  if (slots) {
    slots->Traverse(cb);
  }

  if (tmp->HasProperties()) {
    nsNodeUtils::TraverseUserData(tmp, cb);
    nsCOMArray<nsISupports>* objects =
      static_cast<nsCOMArray<nsISupports>*>(tmp->GetProperty(nsGkAtoms::keepobjectsalive));
    if (objects) {
      for (int32_t i = 0; i < objects->Count(); ++i) {
         cb.NoteXPCOMChild(objects->ObjectAt(i));
      }
    }
  }

  if (tmp->NodeType() != nsIDOMNode::DOCUMENT_NODE &&
      tmp->HasFlag(NODE_HAS_LISTENERMANAGER)) {
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

  if (tmp->NodeType() != nsIDOMNode::DOCUMENT_NODE &&
      tmp->HasFlag(NODE_HAS_LISTENERMANAGER)) {
    nsContentUtils::RemoveListenerManager(tmp);
    tmp->UnsetFlags(NODE_HAS_LISTENERMANAGER);
  }

  if (tmp->HasProperties()) {
    nsNodeUtils::UnlinkUserData(tmp);
    tmp->DeleteProperty(nsGkAtoms::keepobjectsalive);
  }
}

static void
ReleaseURI(void*, 
           nsIAtom*, 
           void* aPropertyValue,
           void* )
{
  nsIURI* uri = static_cast<nsIURI*>(aPropertyValue);
  NS_RELEASE(uri);
}

nsresult
nsINode::SetExplicitBaseURI(nsIURI* aURI)
{
  nsresult rv = SetProperty(nsGkAtoms::baseURIProperty, aURI, ReleaseURI);
  if (NS_SUCCEEDED(rv)) {
    SetHasExplicitBaseURI();
    NS_ADDREF(aURI);
  }
  return rv;
}

static nsresult
AdoptNodeIntoOwnerDoc(nsINode *aParent, nsINode *aNode)
{
  NS_ASSERTION(!aNode->GetParentNode(),
               "Should have removed from parent already");

  nsIDocument *doc = aParent->OwnerDoc();

  nsresult rv;
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aNode, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> adoptedNode;
  rv = domDoc->AdoptNode(node, getter_AddRefs(adoptedNode));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(aParent->OwnerDoc() == doc,
               "ownerDoc chainged while adopting");
  NS_ASSERTION(adoptedNode == node, "Uh, adopt node changed nodes?");
  NS_ASSERTION(aParent->HasSameOwnerDoc(aNode),
               "ownerDocument changed again after adopting!");

  return NS_OK;
}

nsresult
nsINode::doInsertChildAt(nsIContent* aKid, uint32_t aIndex,
                         bool aNotify, nsAttrAndChildArray& aChildArray)
{
  NS_PRECONDITION(!aKid->GetParentNode(),
                  "Inserting node that already has parent");
  nsresult rv;

  
  
  nsMutationGuard::DidMutate();

  
  nsIDocument* doc = GetCurrentDoc();
  mozAutoDocUpdate updateBatch(doc, UPDATE_CONTENT_MODEL, aNotify);

  if (!HasSameOwnerDoc(aKid)) {
    rv = AdoptNodeIntoOwnerDoc(this, aKid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  uint32_t childCount = aChildArray.ChildCount();
  NS_ENSURE_TRUE(aIndex <= childCount, NS_ERROR_ILLEGAL_VALUE);
  bool isAppend = (aIndex == childCount);

  rv = aChildArray.InsertChildAt(aKid, aIndex);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aIndex == 0) {
    mFirstChild = aKid;
  }

  nsIContent* parent =
    IsNodeOfType(eDOCUMENT) ? nullptr : static_cast<nsIContent*>(this);

  rv = aKid->BindToTree(doc, parent,
                        parent ? parent->GetBindingParent() : nullptr,
                        true);
  if (NS_FAILED(rv)) {
    if (GetFirstChild() == aKid) {
      mFirstChild = aKid->GetNextSibling();
    }
    aChildArray.RemoveChildAt(aIndex);
    aKid->UnbindFromTree();
    return rv;
  }

  NS_ASSERTION(aKid->GetParentNode() == this,
               "Did we run script inappropriately?");

  if (aNotify) {
    
    
    if (parent && isAppend) {
      nsNodeUtils::ContentAppended(parent, aKid, aIndex);
    } else {
      nsNodeUtils::ContentInserted(this, aKid, aIndex);
    }

    if (nsContentUtils::HasMutationListeners(aKid,
          NS_EVENT_BITS_MUTATION_NODEINSERTED, this)) {
      nsMutationEvent mutation(true, NS_MUTATION_NODEINSERTED);
      mutation.mRelatedNode = do_QueryInterface(this);

      mozAutoSubtreeModified subtree(OwnerDoc(), this);
      (new nsAsyncDOMEvent(aKid, mutation))->RunDOMEventWhenSafe();
    }
  }

  return NS_OK;
}

void
nsINode::doRemoveChildAt(uint32_t aIndex, bool aNotify,
                         nsIContent* aKid, nsAttrAndChildArray& aChildArray)
{
  NS_PRECONDITION(aKid && aKid->GetParentNode() == this &&
                  aKid == GetChildAt(aIndex) &&
                  IndexOf(aKid) == (int32_t)aIndex, "Bogus aKid");

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
}




static
bool IsAllowedAsChild(nsIContent* aNewChild, nsINode* aParent,
                      bool aIsReplace, nsINode* aRefChild)
{
  MOZ_ASSERT(aNewChild, "Must have new child");
  MOZ_ASSERT_IF(aIsReplace, aRefChild);
  MOZ_ASSERT(aParent);
  MOZ_ASSERT(aParent->IsNodeOfType(nsINode::eDOCUMENT) ||
             aParent->IsNodeOfType(nsINode::eDOCUMENT_FRAGMENT) ||
             aParent->IsElement(),
             "Nodes that are not documents, document fragments or elements "
             "can't be parents!");

  
  
  
  
  if (aNewChild == aParent ||
      (aNewChild->GetFirstChild() &&
       nsContentUtils::ContentIsDescendantOf(aParent, aNewChild))) {
    return false;
  }

  
  switch (aNewChild->NodeType()) {
  case nsIDOMNode::COMMENT_NODE :
  case nsIDOMNode::PROCESSING_INSTRUCTION_NODE :
    
    return true;
  case nsIDOMNode::TEXT_NODE :
  case nsIDOMNode::CDATA_SECTION_NODE :
  case nsIDOMNode::ENTITY_REFERENCE_NODE :
    
    return aParent->NodeType() != nsIDOMNode::DOCUMENT_NODE;
  case nsIDOMNode::ELEMENT_NODE :
    {
      if (!aParent->IsNodeOfType(nsINode::eDOCUMENT)) {
        
        return true;
      }

      nsIDocument* parentDocument = static_cast<nsIDocument*>(aParent);
      Element* rootElement = parentDocument->GetRootElement();
      if (rootElement) {
        
        
        return aIsReplace && rootElement == aRefChild;
      }

      
      
      if (!aRefChild) {
        
        return true;
      }

      nsIContent* docTypeContent = parentDocument->GetDoctype();
      if (!docTypeContent) {
        
        return true;
      }

      int32_t doctypeIndex = aParent->IndexOf(docTypeContent);
      int32_t insertIndex = aParent->IndexOf(aRefChild);

      
      
      
      return aIsReplace ? (insertIndex >= doctypeIndex) :
        insertIndex > doctypeIndex;
    }
  case nsIDOMNode::DOCUMENT_TYPE_NODE :
    {
      if (!aParent->IsNodeOfType(nsINode::eDOCUMENT)) {
        
        return false;
      }

      nsIDocument* parentDocument = static_cast<nsIDocument*>(aParent);
      nsIContent* docTypeContent = parentDocument->GetDoctype();
      if (docTypeContent) {
        
        return aIsReplace && docTypeContent == aRefChild;
      }

      
      
      Element* rootElement = parentDocument->GetRootElement();
      if (!rootElement) {
        
        return true;
      }

      if (!aRefChild) {
        
        return false;
      }

      int32_t rootIndex = aParent->IndexOf(rootElement);
      int32_t insertIndex = aParent->IndexOf(aRefChild);

      
      
      
      return insertIndex <= rootIndex;
    }
  case nsIDOMNode::DOCUMENT_FRAGMENT_NODE :
    {
      
      
      
      
      
      if (!aParent->IsNodeOfType(nsINode::eDOCUMENT)) {
        
        return true;
      }

      bool sawElement = false;
      for (nsIContent* child = aNewChild->GetFirstChild();
           child;
           child = child->GetNextSibling()) {
        if (child->IsElement()) {
          if (sawElement) {
            
            return false;
          }
          sawElement = true;
        }
        
        
        if (!IsAllowedAsChild(child, aParent, aIsReplace, aRefChild)) {
          return false;
        }
      }

      
      return true;
    }
  default:
    


    break;
  }

  return false;
}

nsINode*
nsINode::ReplaceOrInsertBefore(bool aReplace, nsINode* aNewChild,
                               nsINode* aRefChild, ErrorResult& aError)
{
  
  
  
  
  
  MOZ_ASSERT_IF(aReplace, aRefChild);

  if ((!IsNodeOfType(eDOCUMENT) &&
       !IsNodeOfType(eDOCUMENT_FRAGMENT) &&
       !IsElement()) ||
      !aNewChild->IsNodeOfType(eCONTENT)) {
    aError.Throw(NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);
    return nullptr;
  }

  uint16_t nodeType = aNewChild->NodeType();

  
  
  
  
  
  {
    
    
    
    
    
    if (aRefChild && aRefChild->GetParentNode() != this) {
      aError.Throw(NS_ERROR_DOM_NOT_FOUND_ERR);
      return nullptr;
    }

    
    
    if (aReplace && aRefChild != aNewChild) {
      nsContentUtils::MaybeFireNodeRemoved(aRefChild, this, OwnerDoc());
    }

    
    
    nsINode* oldParent = aNewChild->GetParentNode();
    if (oldParent) {
      nsContentUtils::MaybeFireNodeRemoved(aNewChild, oldParent,
                                           aNewChild->OwnerDoc());
    }

    
    
    if (nodeType == nsIDOMNode::DOCUMENT_FRAGMENT_NODE) {
      static_cast<FragmentOrElement*>(aNewChild)->FireNodeRemovedForChildren();
    }
    
    if (aRefChild && aRefChild->GetParentNode() != this) {
      aError.Throw(NS_ERROR_DOM_NOT_FOUND_ERR);
      return nullptr;
    }
  }

  nsIDocument* doc = OwnerDoc();
  nsIContent* newContent = static_cast<nsIContent*>(aNewChild);
  if (newContent->IsRootOfAnonymousSubtree()) {
    
    
    
    aError.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return nullptr;
  }

  
  if (!IsAllowedAsChild(newContent, this, aReplace, aRefChild)) {
    aError.Throw(NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);
    return nullptr;
  }

  
  nsINode* nodeToInsertBefore;
  if (aReplace) {
    nodeToInsertBefore = aRefChild->GetNextSibling();
  } else {
    nodeToInsertBefore = aRefChild;
  }
  if (nodeToInsertBefore == aNewChild) {
    
    
    nodeToInsertBefore = nodeToInsertBefore->GetNextSibling();
  }

  Maybe<nsAutoTArray<nsCOMPtr<nsIContent>, 50> > fragChildren;

  
  nsCOMPtr<nsINode> oldParent = newContent->GetParentNode();
  if (oldParent) {
    int32_t removeIndex = oldParent->IndexOf(newContent);
    if (removeIndex < 0) {
      
      NS_ERROR("How come our flags didn't catch this?");
      aError.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
      return nullptr;
    }

    
    nsCOMPtr<nsINode> kungFuDeathGrip = nodeToInsertBefore;

    
    nsMutationGuard guard;

    
    
    {
      mozAutoDocUpdate batch(newContent->GetCurrentDoc(),
                             UPDATE_CONTENT_MODEL, true);
      nsAutoMutationBatch mb(oldParent, true, true);
      oldParent->RemoveChildAt(removeIndex, true);
      if (nsAutoMutationBatch::GetCurrentBatch() == &mb) {
        mb.RemovalDone();
        mb.SetPrevSibling(oldParent->GetChildAt(removeIndex - 1));
        mb.SetNextSibling(oldParent->GetChildAt(removeIndex));
      }
    }

    
    if (guard.Mutated(1)) {
      
      
      
      
      if (nodeToInsertBefore && nodeToInsertBefore->GetParent() != this) {
        aError.Throw(NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);
        return nullptr;
      }

      
      if (newContent->GetParent()) {
        aError.Throw(NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);
        return nullptr;
      }

      
      if (aNewChild == aRefChild) {
        
        
        if (!IsAllowedAsChild(newContent, this, false, nodeToInsertBefore)) {
          aError.Throw(NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);
          return nullptr;
        }
      } else {
        if ((aRefChild && aRefChild->GetParent() != this) ||
            !IsAllowedAsChild(newContent, this, aReplace, aRefChild)) {
          aError.Throw(NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);
          return nullptr;
        }
        
        if (aReplace) {
          nodeToInsertBefore = aRefChild->GetNextSibling();
        } else {
          nodeToInsertBefore = aRefChild;
        }
      }
    }
  } else if (nodeType == nsIDOMNode::DOCUMENT_FRAGMENT_NODE) {
    
    
    
    
    uint32_t count = newContent->GetChildCount();

    fragChildren.construct();

    
    
    fragChildren.ref().SetCapacity(count);
    for (nsIContent* child = newContent->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
      NS_ASSERTION(child->GetCurrentDoc() == nullptr,
                   "How did we get a child with a current doc?");
      fragChildren.ref().AppendElement(child);
    }

    
    nsCOMPtr<nsINode> kungFuDeathGrip = nodeToInsertBefore;

    nsMutationGuard guard;

    
    
    {
      mozAutoDocUpdate batch(newContent->GetCurrentDoc(),
                             UPDATE_CONTENT_MODEL, true);
      nsAutoMutationBatch mb(newContent, false, true);

      for (uint32_t i = count; i > 0;) {
        newContent->RemoveChildAt(--i, true);
      }
    }

    
    if (guard.Mutated(count)) {
      
      
      
      
      if (nodeToInsertBefore && nodeToInsertBefore->GetParent() != this) {
        aError.Throw(NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);
        return nullptr;
      }

      
      for (uint32_t i = 0; i < count; ++i) {
        if (fragChildren.ref().ElementAt(i)->GetParent()) {
          aError.Throw(NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);
          return nullptr;
        }
      }

      
      
      
      

      
      if (aRefChild && aRefChild->GetParent() != this) {
        aError.Throw(NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);
        return nullptr;
      }

      
      if (aReplace) {
        nodeToInsertBefore = aRefChild->GetNextSibling();
      } else {
        nodeToInsertBefore = aRefChild;
      }      

      
      
      
      
      if (IsNodeOfType(nsINode::eDOCUMENT)) {
        bool sawElement = false;
        for (uint32_t i = 0; i < count; ++i) {
          nsIContent* child = fragChildren.ref().ElementAt(i);
          if (child->IsElement()) {
            if (sawElement) {
              
              aError.Throw(NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);
              return nullptr;
            }
            sawElement = true;
          }
          if (!IsAllowedAsChild(child, this, aReplace, aRefChild)) {
            aError.Throw(NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);
            return nullptr;
          }
        }
      }
    }
  }

  mozAutoDocUpdate batch(GetCurrentDoc(), UPDATE_CONTENT_MODEL, true);
  nsAutoMutationBatch mb;

  
  
  
  
  int32_t insPos;
  if (nodeToInsertBefore) {
    insPos = IndexOf(nodeToInsertBefore);
    if (insPos < 0) {
      
      aError.Throw(NS_ERROR_DOM_NOT_FOUND_ERR);
      return nullptr;
    }
  }
  else {
    insPos = GetChildCount();
  }

  
  if (aReplace && aRefChild != aNewChild) {
    mb.Init(this, true, true);

    
    
    NS_ASSERTION(aRefChild->GetNextSibling() == nodeToInsertBefore,
                 "Unexpected nodeToInsertBefore");

    
    
    NS_ASSERTION(insPos >= 1, "insPos too small");
    RemoveChildAt(insPos-1, true);
    --insPos;
  }

  
  
  
  
  
  if (!HasSameOwnerDoc(newContent)) {
    aError = AdoptNodeIntoOwnerDoc(this, aNewChild);
    if (aError.Failed()) {
      return nullptr;
    }
  }

  




  nsINode* result = aReplace ? aRefChild : aNewChild;
  if (nodeType == nsIDOMNode::DOCUMENT_FRAGMENT_NODE) {
    if (!aReplace) {
      mb.Init(this, true, true);
    }
    nsAutoMutationBatch* mutationBatch = nsAutoMutationBatch::GetCurrentBatch();
    if (mutationBatch) {
      mutationBatch->RemovalDone();
      mutationBatch->SetPrevSibling(GetChildAt(insPos - 1));
      mutationBatch->SetNextSibling(GetChildAt(insPos));
    }

    uint32_t count = fragChildren.ref().Length();
    if (!count) {
      return result;
    }

    bool appending =
      !IsNodeOfType(eDOCUMENT) && uint32_t(insPos) == GetChildCount();
    int32_t firstInsPos = insPos;
    nsIContent* firstInsertedContent = fragChildren.ref().ElementAt(0);

    
    
    for (uint32_t i = 0; i < count; ++i, ++insPos) {
      
      
      aError = InsertChildAt(fragChildren.ref().ElementAt(i), insPos,
                             !appending);
      if (aError.Failed()) {
        
        if (appending && i != 0) {
          nsNodeUtils::ContentAppended(static_cast<nsIContent*>(this),
                                       firstInsertedContent,
                                       firstInsPos);
        }
        return nullptr;
      }
    }

    if (mutationBatch && !appending) {
      mutationBatch->NodesAdded();
    }

    
    if (appending) {
      nsNodeUtils::ContentAppended(static_cast<nsIContent*>(this),
                                   firstInsertedContent, firstInsPos);
      if (mutationBatch) {
        mutationBatch->NodesAdded();
      }
      
      if (nsContentUtils::
            HasMutationListeners(doc, NS_EVENT_BITS_MUTATION_NODEINSERTED)) {
        Element::FireNodeInserted(doc, this, fragChildren.ref());
      }
    }
  }
  else {
    

    
    
    
    

    if (nsAutoMutationBatch::GetCurrentBatch() == &mb) {
      mb.RemovalDone();
      mb.SetPrevSibling(GetChildAt(insPos - 1));
      mb.SetNextSibling(GetChildAt(insPos));
    }
    aError = InsertChildAt(newContent, insPos, true);
    if (aError.Failed()) {
      return nullptr;
    }
  }

  return result;
}

nsresult
nsINode::ReplaceOrInsertBefore(bool aReplace, nsIDOMNode *aNewChild,
                               nsIDOMNode *aRefChild, nsIDOMNode **aReturn)
{
  nsCOMPtr<nsINode> newChild = do_QueryInterface(aNewChild);
  if (!newChild) {
    return NS_ERROR_NULL_POINTER;
  }

  if (aReplace && !aRefChild) {
    return NS_ERROR_NULL_POINTER;
  }

  nsCOMPtr<nsINode> refChild = do_QueryInterface(aRefChild);
  if (aRefChild && !refChild) {
    return NS_NOINTERFACE;
  }

  ErrorResult rv;
  nsINode* result = ReplaceOrInsertBefore(aReplace, newChild, refChild, rv);
  if (result) {
    NS_ADDREF(*aReturn = result->AsDOMNode());
  }
  return rv.ErrorCode();
}

nsresult
nsINode::CompareDocumentPosition(nsIDOMNode* aOther, uint16_t* aReturn)
{
  nsCOMPtr<nsINode> other = do_QueryInterface(aOther);
  NS_ENSURE_ARG(other);
  *aReturn = CompareDocumentPosition(*other);
  return NS_OK;
}

nsresult
nsINode::IsEqualNode(nsIDOMNode* aOther, bool* aReturn)
{
  nsCOMPtr<nsINode> other = do_QueryInterface(aOther);
  *aReturn = IsEqualNode(other);
  return NS_OK;
}

static void
nsCOMArrayDeleter(void* aObject, nsIAtom* aPropertyName,
                  void* aPropertyValue, void* aData)
{
  nsCOMArray<nsISupports>* objects =
    static_cast<nsCOMArray<nsISupports>*>(aPropertyValue);
  delete objects;
}

void
nsINode::BindObject(nsISupports* aObject)
{
  nsCOMArray<nsISupports>* objects =
    static_cast<nsCOMArray<nsISupports>*>(GetProperty(nsGkAtoms::keepobjectsalive));
  if (!objects) {
    objects = new nsCOMArray<nsISupports>();
    SetProperty(nsGkAtoms::keepobjectsalive, objects, nsCOMArrayDeleter, true);
  }
  objects->AppendObject(aObject);
}

void
nsINode::UnbindObject(nsISupports* aObject)
{
  nsCOMArray<nsISupports>* objects =
    static_cast<nsCOMArray<nsISupports>*>(GetProperty(nsGkAtoms::keepobjectsalive));
  if (objects) {
    objects->RemoveObject(aObject);
  }
}

size_t
nsINode::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  size_t n = 0;
  nsEventListenerManager* elm =
    const_cast<nsINode*>(this)->GetListenerManager(false);
  if (elm) {
    n += elm->SizeOfIncludingThis(aMallocSizeOf);
  }

  
  
  
  
  
  
  
  
  return n;
}

#define EVENT(name_, id_, type_, struct_)                                    \
  EventHandlerNonNull* nsINode::GetOn##name_() {                             \
    nsEventListenerManager *elm = GetListenerManager(false);                 \
    return elm ? elm->GetEventHandler(nsGkAtoms::on##name_) : nullptr;       \
  }                                                                          \
  void nsINode::SetOn##name_(EventHandlerNonNull* handler,                   \
                             ErrorResult& error) {                           \
    nsEventListenerManager *elm = GetListenerManager(true);                  \
    if (elm) {                                                               \
      error = elm->SetEventHandler(nsGkAtoms::on##name_, handler);           \
    } else {                                                                 \
      error.Throw(NS_ERROR_OUT_OF_MEMORY);                                   \
    }                                                                        \
  }                                                                          \
  NS_IMETHODIMP nsINode::GetOn##name_(JSContext *cx, jsval *vp) {            \
    EventHandlerNonNull* h = GetOn##name_();                                 \
    vp->setObjectOrNull(h ? h->Callable() : nullptr);                        \
    return NS_OK;                                                            \
  }                                                                          \
  NS_IMETHODIMP nsINode::SetOn##name_(JSContext *cx, const jsval &v) {       \
    JSObject *obj = GetWrapper();                                            \
    if (!obj) {                                                              \
      /* Just silently do nothing */                                         \
      return NS_OK;                                                          \
    }                                                                        \
    nsRefPtr<EventHandlerNonNull> handler;                                   \
    JSObject *callable;                                                      \
    if (v.isObject() &&                                                      \
        JS_ObjectIsCallable(cx, callable = &v.toObject())) {                 \
      bool ok;                                                               \
      handler = new EventHandlerNonNull(cx, obj, callable, &ok);             \
      if (!ok) {                                                             \
        return NS_ERROR_OUT_OF_MEMORY;                                       \
      }                                                                      \
    }                                                                        \
    ErrorResult rv;                                                          \
    SetOn##name_(handler, rv);                                               \
    return rv.ErrorCode();                                                   \
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
    return true;
  }
  if (!aOther ||
      OwnerDoc() != aOther->OwnerDoc() ||
      IsInDoc() != aOther->IsInDoc() ||
      !(aOther->IsElement() ||
        aOther->IsNodeOfType(nsINode::eCONTENT)) ||
      !GetFirstChild()) {
    return false;
  }

  const nsIContent* other = static_cast<const nsIContent*>(aOther);
  if (this == OwnerDoc()) {
    
    
    
    return !other->IsInAnonymousSubtree();
  }

  if (!IsElement() && !IsNodeOfType(nsINode::eDOCUMENT_FRAGMENT)) {
    return false;
  }

  const nsIContent* thisContent = static_cast<const nsIContent*>(this);
  if (thisContent->GetBindingParent() != other->GetBindingParent()) {
    return false;
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

uint32_t
nsINode::Length() const
{
  switch (NodeType()) {
  case nsIDOMNode::DOCUMENT_TYPE_NODE:
    return 0;

  case nsIDOMNode::TEXT_NODE:
  case nsIDOMNode::CDATA_SECTION_NODE:
  case nsIDOMNode::PROCESSING_INSTRUCTION_NODE:
  case nsIDOMNode::COMMENT_NODE:
    MOZ_ASSERT(IsNodeOfType(eCONTENT));
    return static_cast<const nsIContent*>(this)->TextLength();

  default:
    return GetChildCount();
  }
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
  ErrorResult rv;
  nsIContent* result = mNode->QuerySelector(aSelector, rv);
  return result ? CallQueryInterface(result, aReturn) : rv.ErrorCode();
}

NS_IMETHODIMP
nsNodeSelectorTearoff::QuerySelectorAll(const nsAString& aSelector,
                                        nsIDOMNodeList **aReturn)
{
  ErrorResult rv;
  *aReturn = mNode->QuerySelectorAll(aSelector, rv).get();
  return rv.ErrorCode();
}




static nsresult
ParseSelectorList(nsINode* aNode,
                  const nsAString& aSelectorString,
                  nsCSSSelectorList** aSelectorList)
{
  NS_ENSURE_ARG(aNode);

  nsIDocument* doc = aNode->OwnerDoc();
  nsCSSParser parser(doc->CSSLoader());

  nsCSSSelectorList* selectorList;
  nsresult rv = parser.ParseSelectorString(aSelectorString,
                                           doc->GetDocumentURI(),
                                           0, 
                                           &selectorList);
  if (NS_FAILED(rv)) {
    
    
    
    return rv;
  }

  
  nsCSSSelectorList** slot = &selectorList;
  do {
    nsCSSSelectorList* cur = *slot;
    if (cur->mSelectors->IsPseudoElement()) {
      *slot = cur->mNext;
      cur->mNext = nullptr;
      delete cur;
    } else {
      slot = &cur->mNext;
    }
  } while (*slot);
  *aSelectorList = selectorList;

  return NS_OK;
}

static void
AddScopeElements(TreeMatchContext& aMatchContext,
                 nsINode* aMatchContextNode)
{
  if (aMatchContextNode->IsElement()) {
    aMatchContext.SetHasSpecifiedScope();
    aMatchContext.AddScopeElement(aMatchContextNode->AsElement());
  }
}




template<bool onlyFirstMatch, class T>
inline static nsresult
FindMatchingElements(nsINode* aRoot, const nsAString& aSelector, T &aList)
{
  nsAutoPtr<nsCSSSelectorList> selectorList;
  nsresult rv = ParseSelectorList(aRoot, aSelector,
                                  getter_Transfers(selectorList));
  if (NS_FAILED(rv)) {
    
    
    
    return rv;
  }
  NS_ENSURE_TRUE(selectorList, NS_OK);

  NS_ASSERTION(selectorList->mSelectors,
               "How can we not have any selectors?");

  nsIDocument* doc = aRoot->OwnerDoc();
  TreeMatchContext matchingContext(false, nsRuleWalker::eRelevantLinkUnvisited,
                                   doc, TreeMatchContext::eNeverMatchVisited);
  doc->FlushPendingLinkUpdates();
  AddScopeElements(matchingContext, aRoot);

  
  
  
  
  
  NS_ASSERTION(aRoot->IsElement() || aRoot->IsNodeOfType(nsINode::eDOCUMENT) ||
               !aRoot->IsInDoc(),
               "The optimization below to check ContentIsDescendantOf only for "
               "elements depends on aRoot being either an element or a "
               "document if it's in the document.");
  if (aRoot->IsInDoc() &&
      doc->GetCompatibilityMode() != eCompatibility_NavQuirks &&
      !selectorList->mNext &&
      selectorList->mSelectors->mIDList) {
    nsIAtom* id = selectorList->mSelectors->mIDList->mAtom;
    const nsSmallVoidArray* elements =
      doc->GetAllElementsForId(nsDependentAtomString(id));

    
    
    if (elements) {
      for (int32_t i = 0; i < elements->Count(); ++i) {
        Element *element = static_cast<Element*>(elements->ElementAt(i));
        if (!aRoot->IsElement() ||
            (element != aRoot &&
             nsContentUtils::ContentIsDescendantOf(element, aRoot))) {
          
          
          if (nsCSSRuleProcessor::SelectorListMatches(element, matchingContext,
                                                      selectorList)) {
            aList.AppendElement(element);
            if (onlyFirstMatch) {
              return NS_OK;
            }
          }
        }
      }
    }

    
    
    return NS_OK;
  }

  for (nsIContent* cur = aRoot->GetFirstChild();
       cur;
       cur = cur->GetNextNode(aRoot)) {
    if (cur->IsElement() &&
        nsCSSRuleProcessor::SelectorListMatches(cur->AsElement(),
                                                matchingContext,
                                                selectorList)) {
      aList.AppendElement(cur->AsElement());
      if (onlyFirstMatch) {
        return NS_OK;
      }
    }
  }

  return NS_OK;
}

struct ElementHolder {
  ElementHolder() : mElement(nullptr) {}
  void AppendElement(Element* aElement) {
    NS_ABORT_IF_FALSE(!mElement, "Should only get one element");
    mElement = aElement;
  }
  Element* mElement;
};

Element*
nsINode::QuerySelector(const nsAString& aSelector, ErrorResult& aResult)
{
  ElementHolder holder;
  aResult = FindMatchingElements<true>(this, aSelector, holder);

  return holder.mElement;
}

already_AddRefed<nsINodeList>
nsINode::QuerySelectorAll(const nsAString& aSelector, ErrorResult& aResult)
{
  nsRefPtr<nsSimpleContentList> contentList = new nsSimpleContentList(this);

  aResult = FindMatchingElements<false>(this, aSelector, *contentList);

  return contentList.forget();
}

JSObject*
nsINode::WrapObject(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  MOZ_ASSERT(IsDOMBinding());

  
  
  
  
  
  
  
  
  
  bool hasHadScriptHandlingObject = false;
  if (!OwnerDoc()->GetScriptHandlingObject(hasHadScriptHandlingObject) &&
      !hasHadScriptHandlingObject &&
      !nsContentUtils::IsCallerChrome()) {
    Throw<true>(aCx, NS_ERROR_UNEXPECTED);
    *aTriedToWrap = true;
    return nullptr;
  }

  JSObject* obj = WrapNode(aCx, aScope, aTriedToWrap);
  if (obj && ChromeOnlyAccess() &&
      !nsContentUtils::IsSystemPrincipal(NodePrincipal()))
  {
    
    JSAutoCompartment ac(aCx, obj);
    JSObject* wrapper = xpc::WrapperFactory::WrapSOWObject(aCx, obj);
    if (!wrapper) {
      ClearWrapper();
      return nullptr;
    }
    dom::SetSystemOnlyWrapper(obj, this, *wrapper);
  }
  return obj;
}

bool
nsINode::IsSupported(const nsAString& aFeature, const nsAString& aVersion)
{
  return nsContentUtils::InternalIsSupported(this, aFeature, aVersion);
}

already_AddRefed<nsINode>
nsINode::CloneNode(bool aDeep, ErrorResult& aError)
{
  bool callUserDataHandlers = NodeType() != nsIDOMNode::DOCUMENT_NODE ||
                              !static_cast<nsIDocument*>(this)->CreatingStaticClone();

  nsCOMPtr<nsINode> result;
  aError = nsNodeUtils::CloneNodeImpl(this, aDeep, callUserDataHandlers,
                                      getter_AddRefs(result));
  return result.forget();
}

nsDOMAttributeMap*
nsINode::GetAttributes()
{
  if (!IsElement()) {
    return nullptr;
  }
  return AsElement()->GetAttributes();
}

nsresult
nsINode::GetAttributes(nsIDOMNamedNodeMap** aAttributes)
{
  if (!IsElement()) {
    *aAttributes = nullptr;
    return NS_OK;
  }
  return CallQueryInterface(GetAttributes(), aAttributes);
}
