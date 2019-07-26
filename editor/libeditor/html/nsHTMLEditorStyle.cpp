



#include "TypeInState.h"
#include "mozilla/Assertions.h"
#include "mozilla/Selection.h"
#include "mozilla/dom/Element.h"
#include "mozilla/mozalloc.h"
#include "nsAString.h"
#include "nsAttrName.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsCaseTreatment.h"
#include "nsComponentManagerUtils.h"
#include "nsDebug.h"
#include "nsEditProperty.h"
#include "nsEditRules.h"
#include "nsEditor.h"
#include "nsEditorUtils.h"
#include "nsError.h"
#include "nsGkAtoms.h"
#include "nsHTMLCSSUtils.h"
#include "nsHTMLEditUtils.h"
#include "nsHTMLEditor.h"
#include "nsIAtom.h"
#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMElement.h"
#include "nsIDOMNode.h"
#include "nsIDOMRange.h"
#include "nsIEditor.h"
#include "nsIEditorIMESupport.h"
#include "nsIEnumerator.h"
#include "nsINameSpaceManager.h"
#include "nsINode.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsISupportsImpl.h"
#include "nsLiteralString.h"
#include "nsReadableUtils.h"
#include "nsSelectionState.h"
#include "nsString.h"
#include "nsStringFwd.h"
#include "nsTArray.h"
#include "nsTextEditRules.h"
#include "nsTextEditUtils.h"
#include "nsUnicharUtils.h"
#include "nscore.h"

class nsISupports;

using namespace mozilla;

static bool
IsEmptyTextNode(nsHTMLEditor* aThis, nsINode* aNode)
{
  bool isEmptyTextNode = false;
  return nsEditor::IsTextNode(aNode) &&
         NS_SUCCEEDED(aThis->IsEmptyNode(aNode, &isEmptyTextNode)) &&
         isEmptyTextNode;
}

NS_IMETHODIMP nsHTMLEditor::AddDefaultProperty(nsIAtom *aProperty, 
                                            const nsAString & aAttribute, 
                                            const nsAString & aValue)
{
  nsString outValue;
  int32_t index;
  nsString attr(aAttribute);
  if (TypeInState::FindPropInList(aProperty, attr, &outValue, mDefaultStyles, index))
  {
    PropItem *item = mDefaultStyles[index];
    item->value = aValue;
  }
  else
  {
    nsString value(aValue);
    PropItem *propItem = new PropItem(aProperty, attr, value);
    mDefaultStyles.AppendElement(propItem);
  }
  return NS_OK;
}

NS_IMETHODIMP nsHTMLEditor::RemoveDefaultProperty(nsIAtom *aProperty, 
                                   const nsAString & aAttribute, 
                                   const nsAString & aValue)
{
  nsString outValue;
  int32_t index;
  nsString attr(aAttribute);
  if (TypeInState::FindPropInList(aProperty, attr, &outValue, mDefaultStyles, index))
  {
    delete mDefaultStyles[index];
    mDefaultStyles.RemoveElementAt(index);
  }
  return NS_OK;
}

NS_IMETHODIMP nsHTMLEditor::RemoveAllDefaultProperties()
{
  uint32_t j, defcon = mDefaultStyles.Length();
  for (j=0; j<defcon; j++)
  {
    delete mDefaultStyles[j];
  }
  mDefaultStyles.Clear();
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLEditor::SetInlineProperty(nsIAtom *aProperty,
                                const nsAString& aAttribute,
                                const nsAString& aValue)
{
  if (!aProperty) {
    return NS_ERROR_NULL_POINTER;
  }
  if (!mRules) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  ForceCompositionEnd();

  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  if (selection->Collapsed()) {
    
    
    mTypeInState->SetProp(aProperty, aAttribute, aValue);
    return NS_OK;
  }

  nsAutoEditBatch batchIt(this);
  nsAutoRules beginRulesSniffing(this, EditAction::insertElement, nsIEditor::eNext);
  nsAutoSelectionReset selectionResetter(selection, this);
  nsAutoTxnsConserveSelection dontSpazMySelection(this);

  bool cancel, handled;
  nsTextRulesInfo ruleInfo(EditAction::setTextProperty);
  nsresult res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(res, res);
  if (!cancel && !handled) {
    
    nsCOMPtr<nsIEnumerator> enumerator;
    res = selection->GetEnumerator(getter_AddRefs(enumerator));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(enumerator, NS_ERROR_FAILURE);

    
    nsCOMPtr<nsISupports> currentItem;
    for (enumerator->First();
         static_cast<nsresult>(NS_ENUMERATOR_FALSE) == enumerator->IsDone();
         enumerator->Next()) {
      res = enumerator->CurrentItem(getter_AddRefs(currentItem));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(currentItem, NS_ERROR_FAILURE);

      nsCOMPtr<nsIDOMRange> range(do_QueryInterface(currentItem));

      
      
      res = PromoteInlineRange(range);
      NS_ENSURE_SUCCESS(res, res);

      
      nsCOMPtr<nsIDOMNode> startNode, endNode;
      res = range->GetStartContainer(getter_AddRefs(startNode));
      NS_ENSURE_SUCCESS(res, res);
      res = range->GetEndContainer(getter_AddRefs(endNode));
      NS_ENSURE_SUCCESS(res, res);
      if (startNode == endNode && IsTextNode(startNode)) {
        int32_t startOffset, endOffset;
        range->GetStartOffset(&startOffset);
        range->GetEndOffset(&endOffset);
        nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(startNode);
        res = SetInlinePropertyOnTextNode(nodeAsText, startOffset, endOffset,
                                          aProperty, &aAttribute, &aValue);
        NS_ENSURE_SUCCESS(res, res);
        continue;
      }

      
      
      
      
      

      
      
      
      

      nsCOMPtr<nsIContentIterator> iter =
        do_CreateInstance("@mozilla.org/content/subtree-content-iterator;1", &res);
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(iter, NS_ERROR_FAILURE);

      nsCOMArray<nsIDOMNode> arrayOfNodes;

      
      res = iter->Init(range);
      
      
      
      if (NS_SUCCEEDED(res)) {
        nsCOMPtr<nsIDOMNode> node;
        for (; !iter->IsDone(); iter->Next()) {
          node = do_QueryInterface(iter->GetCurrentNode());
          NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

          if (IsEditable(node)) {
            arrayOfNodes.AppendObject(node);
          }
        }
      }
      
      
      
      if (IsTextNode(startNode) && IsEditable(startNode)) {
        nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(startNode);
        int32_t startOffset;
        uint32_t textLen;
        range->GetStartOffset(&startOffset);
        nodeAsText->GetLength(&textLen);
        res = SetInlinePropertyOnTextNode(nodeAsText, startOffset, textLen,
                                          aProperty, &aAttribute, &aValue);
        NS_ENSURE_SUCCESS(res, res);
      }

      
      int32_t listCount = arrayOfNodes.Count();
      int32_t j;
      for (j = 0; j < listCount; j++) {
        res = SetInlinePropertyOnNode(arrayOfNodes[j], aProperty,
                                      &aAttribute, &aValue);
        NS_ENSURE_SUCCESS(res, res);
      }

      
      
      
      if (IsTextNode(endNode) && IsEditable(endNode)) {
        nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(endNode);
        int32_t endOffset;
        range->GetEndOffset(&endOffset);
        res = SetInlinePropertyOnTextNode(nodeAsText, 0, endOffset,
                                          aProperty, &aAttribute, &aValue);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
  }
  if (!cancel) {
    
    return mRules->DidDoAction(selection, &ruleInfo, res);
  }
  return NS_OK;
}





bool
nsHTMLEditor::IsSimpleModifiableNode(nsIContent* aContent,
                                     nsIAtom* aProperty,
                                     const nsAString* aAttribute,
                                     const nsAString* aValue)
{
  
  MOZ_ASSERT(aProperty);
  MOZ_ASSERT_IF(aAttribute, aValue);

  nsCOMPtr<dom::Element> element = do_QueryInterface(aContent);
  if (!element) {
    return false;
  }

  
  if (element->IsHTML(aProperty) && !element->GetAttrCount() &&
      (!aAttribute || aAttribute->IsEmpty())) {
    return true;
  }

  
  if (!element->GetAttrCount() &&
      ((aProperty == nsGkAtoms::b && element->IsHTML(nsGkAtoms::strong)) ||
       (aProperty == nsGkAtoms::i && element->IsHTML(nsGkAtoms::em)) ||
       (aProperty == nsGkAtoms::strike && element->IsHTML(nsGkAtoms::s)))) {
    return true;
  }

  
  if (aAttribute && !aAttribute->IsEmpty()) {
    nsCOMPtr<nsIAtom> atom = do_GetAtom(*aAttribute);
    MOZ_ASSERT(atom);

    nsString attrValue;
    if (element->IsHTML(aProperty) && IsOnlyAttribute(element, *aAttribute) &&
        element->GetAttr(kNameSpaceID_None, atom, attrValue) &&
        attrValue.Equals(*aValue, nsCaseInsensitiveStringComparator())) {
      
      
      
      return true;
    }
  }

  
  
  
  if (!mHTMLCSSUtils->IsCSSEditableProperty(element, aProperty, aAttribute) ||
      !element->IsHTML(nsGkAtoms::span) || element->GetAttrCount() != 1 ||
      !element->HasAttr(kNameSpaceID_None, nsGkAtoms::style)) {
    return false;
  }

  
  
  
  
  nsCOMPtr<dom::Element> newSpan;
  nsresult res = CreateHTMLContent(NS_LITERAL_STRING("span"),
                                   getter_AddRefs(newSpan));
  NS_ASSERTION(NS_SUCCEEDED(res), "CreateHTMLContent failed");
  NS_ENSURE_SUCCESS(res, false);
  mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(newSpan, aProperty,
                                             aAttribute, aValue,
                                              true);

  return mHTMLCSSUtils->ElementsSameStyle(newSpan, element);
}


nsresult
nsHTMLEditor::SetInlinePropertyOnTextNode( nsIDOMCharacterData *aTextNode, 
                                            int32_t aStartOffset,
                                            int32_t aEndOffset,
                                            nsIAtom *aProperty, 
                                            const nsAString *aAttribute,
                                            const nsAString *aValue)
{
  MOZ_ASSERT(aValue);
  NS_ENSURE_TRUE(aTextNode, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMNode> parent;
  nsresult res = aTextNode->GetParentNode(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(res, res);

  if (!CanContainTag(parent, aProperty)) {
    return NS_OK;
  }
  
  
  if (aStartOffset == aEndOffset) return NS_OK;
  
  nsCOMPtr<nsIDOMNode> node = aTextNode;
  
  
  bool bHasProp;
  if (mHTMLCSSUtils->IsCSSEditableProperty(node, aProperty, aAttribute)) {
    
    
    nsAutoString value(*aValue);
    mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(node, aProperty, aAttribute,
                                                       bHasProp, value,
                                                       nsHTMLCSSUtils::eComputed);
  } else {
    IsTextPropertySetByContent(node, aProperty, aAttribute, aValue, bHasProp);
  }

  if (bHasProp) return NS_OK;
  
  
  uint32_t textLen;
  aTextNode->GetLength(&textLen);

  if (uint32_t(aEndOffset) != textLen) {
    
    nsCOMPtr<nsIDOMNode> tmp;
    res = SplitNode(node, aEndOffset, getter_AddRefs(tmp));
    NS_ENSURE_SUCCESS(res, res);
    node = tmp;  
  }

  if (aStartOffset) {
    
    nsCOMPtr<nsIDOMNode> tmp;
    res = SplitNode(node, aStartOffset, getter_AddRefs(tmp));
    NS_ENSURE_SUCCESS(res, res);
  }

  nsCOMPtr<nsIContent> content = do_QueryInterface(node);
  NS_ENSURE_STATE(content);

  if (aAttribute) {
    
    nsIContent* sibling = GetPriorHTMLSibling(content);
    if (IsSimpleModifiableNode(sibling, aProperty, aAttribute, aValue)) {
      
      return MoveNode(node, sibling->AsDOMNode(), -1);
    }
    sibling = GetNextHTMLSibling(content);
    if (IsSimpleModifiableNode(sibling, aProperty, aAttribute, aValue)) {
      
      return MoveNode(node, sibling->AsDOMNode(), 0);
    }
  }
  
  
  return SetInlinePropertyOnNode(node, aProperty, aAttribute, aValue);
}


nsresult
nsHTMLEditor::SetInlinePropertyOnNodeImpl(nsIContent* aNode,
                                          nsIAtom* aProperty,
                                          const nsAString* aAttribute,
                                          const nsAString* aValue)
{
  MOZ_ASSERT(aNode && aProperty);
  MOZ_ASSERT(aValue);

  
  
  if (!TagCanContain(nsGkAtoms::span, aNode->AsDOMNode())) {
    if (aNode->HasChildren()) {
      nsCOMArray<nsIContent> arrayOfNodes;

      
      for (nsIContent* child = aNode->GetFirstChild();
           child;
           child = child->GetNextSibling()) {
        if (IsEditable(child) && !IsEmptyTextNode(this, child)) {
          arrayOfNodes.AppendObject(child);
        }
      }

      
      int32_t listCount = arrayOfNodes.Count();
      for (int32_t j = 0; j < listCount; ++j) {
        nsresult rv = SetInlinePropertyOnNode(arrayOfNodes[j], aProperty,
                                              aAttribute, aValue);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    return NS_OK;
  }

  
  nsresult res;
  nsCOMPtr<nsIContent> previousSibling = GetPriorHTMLSibling(aNode);
  nsCOMPtr<nsIContent> nextSibling = GetNextHTMLSibling(aNode);
  if (IsSimpleModifiableNode(previousSibling, aProperty, aAttribute, aValue)) {
    res = MoveNode(aNode, previousSibling, -1);
    NS_ENSURE_SUCCESS(res, res);
    if (IsSimpleModifiableNode(nextSibling, aProperty, aAttribute, aValue)) {
      res = JoinNodes(previousSibling, nextSibling);
      NS_ENSURE_SUCCESS(res, res);
    }
    return NS_OK;
  }
  if (IsSimpleModifiableNode(nextSibling, aProperty, aAttribute, aValue)) {
    res = MoveNode(aNode, nextSibling, 0);
    NS_ENSURE_SUCCESS(res, res);
    return NS_OK;
  }

  
  if (mHTMLCSSUtils->IsCSSEditableProperty(aNode, aProperty, aAttribute)) {
    if (mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(
          aNode, aProperty, aAttribute, *aValue, nsHTMLCSSUtils::eComputed)) {
      return NS_OK;
    }
  } else if (IsTextPropertySetByContent(aNode, aProperty,
                                        aAttribute, aValue)) {
    return NS_OK;
  }

  bool useCSS = (IsCSSEnabled() &&
                 mHTMLCSSUtils->IsCSSEditableProperty(aNode, aProperty, aAttribute)) ||
                
                aAttribute->EqualsLiteral("bgcolor");

  if (useCSS) {
    nsCOMPtr<dom::Element> tmp;
    
    
    if (aNode->IsElement() && aNode->AsElement()->IsHTML(nsGkAtoms::span) &&
        !aNode->AsElement()->GetAttrCount()) {
      tmp = aNode->AsElement();
    } else {
      res = InsertContainerAbove(aNode, getter_AddRefs(tmp),
                                 NS_LITERAL_STRING("span"),
                                 nullptr, nullptr);
      NS_ENSURE_SUCCESS(res, res);
    }

    
    int32_t count;
    res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(tmp->AsDOMNode(),
                                                     aProperty, aAttribute,
                                                     aValue, &count, false);
    NS_ENSURE_SUCCESS(res, res);
    return NS_OK;
  }

  
  if (aNode->Tag() == aProperty) {
    
    nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(aNode);
    return SetAttribute(elem, *aAttribute, *aValue);
  }

  
  nsAutoString tag;
  aProperty->ToString(tag);
  ToLowerCase(tag);
  nsCOMPtr<nsIDOMNode> tmp;
  return InsertContainerAbove(aNode->AsDOMNode(), address_of(tmp), tag,
                              aAttribute, aValue);
}


nsresult
nsHTMLEditor::SetInlinePropertyOnNode(nsIDOMNode *aNode,
                                      nsIAtom *aProperty,
                                      const nsAString *aAttribute,
                                      const nsAString *aValue)
{
  
  
  
  
  
  NS_ENSURE_TRUE(aNode && aProperty, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIContent> node = do_QueryInterface(aNode);
  NS_ENSURE_STATE(node);

  return SetInlinePropertyOnNode(node, aProperty, aAttribute, aValue);
}

nsresult
nsHTMLEditor::SetInlinePropertyOnNode(nsIContent* aNode,
                                      nsIAtom* aProperty,
                                      const nsAString* aAttribute,
                                      const nsAString* aValue)
{
  MOZ_ASSERT(aNode);
  MOZ_ASSERT(aProperty);

  nsCOMPtr<nsIContent> previousSibling = aNode->GetPreviousSibling(),
                       nextSibling = aNode->GetNextSibling();
  nsCOMPtr<nsINode> parent = aNode->GetParentNode();
  NS_ENSURE_STATE(parent);

  nsresult res = RemoveStyleInside(aNode->AsDOMNode(), aProperty, aAttribute);
  NS_ENSURE_SUCCESS(res, res);

  if (aNode->GetParentNode()) {
    
    return SetInlinePropertyOnNodeImpl(aNode, aProperty,
                                       aAttribute, aValue);
  }

  
  
  
  if ((previousSibling && previousSibling->GetParentNode() != parent) ||
      (nextSibling && nextSibling->GetParentNode() != parent)) {
    return NS_ERROR_UNEXPECTED;
  }
  nsCOMArray<nsIContent> nodesToSet;
  nsCOMPtr<nsIContent> cur = previousSibling
    ? previousSibling->GetNextSibling() : parent->GetFirstChild();
  while (cur && cur != nextSibling) {
    if (IsEditable(cur)) {
      nodesToSet.AppendObject(cur);
    }
    cur = cur->GetNextSibling();
  }

  int32_t nodesToSetCount = nodesToSet.Count();
  for (int32_t k = 0; k < nodesToSetCount; k++) {
    res = SetInlinePropertyOnNodeImpl(nodesToSet[k], aProperty,
                                      aAttribute, aValue);
    NS_ENSURE_SUCCESS(res, res);
  }

  return NS_OK;
}


nsresult nsHTMLEditor::SplitStyleAboveRange(nsIDOMRange *inRange, 
                                            nsIAtom *aProperty, 
                                            const nsAString *aAttribute)
{
  NS_ENSURE_TRUE(inRange, NS_ERROR_NULL_POINTER);
  nsresult res;
  nsCOMPtr<nsIDOMNode> startNode, endNode, origStartNode;
  int32_t startOffset, endOffset;

  res = inRange->GetStartContainer(getter_AddRefs(startNode));
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->GetStartOffset(&startOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->GetEndContainer(getter_AddRefs(endNode));
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->GetEndOffset(&endOffset);
  NS_ENSURE_SUCCESS(res, res);

  origStartNode = startNode;

  
  {
    nsAutoTrackDOMPoint tracker(mRangeUpdater, address_of(endNode), &endOffset);
    res = SplitStyleAbovePoint(address_of(startNode), &startOffset, aProperty, aAttribute);
    NS_ENSURE_SUCCESS(res, res);
  }

  
  res = SplitStyleAbovePoint(address_of(endNode), &endOffset, aProperty, aAttribute);
  NS_ENSURE_SUCCESS(res, res);

  
  res = inRange->SetStart(startNode, startOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->SetEnd(endNode, endOffset);
  return res;
}

nsresult nsHTMLEditor::SplitStyleAbovePoint(nsCOMPtr<nsIDOMNode> *aNode,
                                           int32_t *aOffset,
                                           nsIAtom *aProperty,          
                                           const nsAString *aAttribute,
                                           nsCOMPtr<nsIDOMNode> *outLeftNode,
                                           nsCOMPtr<nsIDOMNode> *outRightNode)
{
  NS_ENSURE_TRUE(aNode && *aNode && aOffset, NS_ERROR_NULL_POINTER);
  if (outLeftNode)  *outLeftNode  = nullptr;
  if (outRightNode) *outRightNode = nullptr;
  
  nsCOMPtr<nsIDOMNode> parent, tmp = *aNode;
  int32_t offset;

  bool useCSS = IsCSSEnabled();

  bool isSet;
  while (tmp && !IsBlockNode(tmp))
  {
    isSet = false;
    if (useCSS && mHTMLCSSUtils->IsCSSEditableProperty(tmp, aProperty, aAttribute)) {
      
      
      nsAutoString firstValue;
      mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(tmp, aProperty,
        aAttribute, isSet, firstValue, nsHTMLCSSUtils::eSpecified);
    }
    if ( (aProperty && NodeIsType(tmp, aProperty)) ||   
         (aProperty == nsEditProperty::href && nsHTMLEditUtils::IsLink(tmp)) ||
                                                        
         (!aProperty && NodeIsProperty(tmp)) ||         
         isSet)                                         
    {
      
      nsresult rv = SplitNodeDeep(tmp, *aNode, *aOffset, &offset, false,
                                  outLeftNode, outRightNode);
      NS_ENSURE_SUCCESS(rv, rv);
      
      tmp->GetParentNode(getter_AddRefs(*aNode));
      *aOffset = offset;
    }
    tmp->GetParentNode(getter_AddRefs(parent));
    tmp = parent;
  }
  return NS_OK;
}

nsresult
nsHTMLEditor::ClearStyle(nsCOMPtr<nsIDOMNode>* aNode, int32_t* aOffset,
                         nsIAtom* aProperty, const nsAString* aAttribute)
{
  nsCOMPtr<nsIDOMNode> leftNode, rightNode, tmp;
  nsresult res = SplitStyleAbovePoint(aNode, aOffset, aProperty, aAttribute,
                                      address_of(leftNode),
                                      address_of(rightNode));
  NS_ENSURE_SUCCESS(res, res);
  if (leftNode) {
    bool bIsEmptyNode;
    IsEmptyNode(leftNode, &bIsEmptyNode, false, true);
    if (bIsEmptyNode) {
      
      res = DeleteNode(leftNode);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  if (rightNode) {
    nsCOMPtr<nsIDOMNode> secondSplitParent = GetLeftmostChild(rightNode);
    
    if (!secondSplitParent) {
      secondSplitParent = rightNode;
    }
    nsCOMPtr<nsIDOMNode> savedBR;
    if (!IsContainer(secondSplitParent)) {
      if (nsTextEditUtils::IsBreak(secondSplitParent)) {
        savedBR = secondSplitParent;
      }

      secondSplitParent->GetParentNode(getter_AddRefs(tmp));
      secondSplitParent = tmp;
    }
    *aOffset = 0;
    res = SplitStyleAbovePoint(address_of(secondSplitParent),
                               aOffset, aProperty, aAttribute,
                               address_of(leftNode), address_of(rightNode));
    NS_ENSURE_SUCCESS(res, res);
    
    NS_ENSURE_TRUE(leftNode, NS_ERROR_FAILURE);
    nsCOMPtr<nsIDOMNode> newSelParent = GetLeftmostChild(leftNode);
    if (!newSelParent) {
      newSelParent = leftNode;
    }
    
    
    
    if (savedBR) {
      res = MoveNode(savedBR, newSelParent, 0);
      NS_ENSURE_SUCCESS(res, res);
    }
    bool bIsEmptyNode;
    IsEmptyNode(rightNode, &bIsEmptyNode, false, true);
    if (bIsEmptyNode) {
      
      res = DeleteNode(rightNode);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    int32_t newSelOffset = 0;
    {
      
      
      
      
      
      nsAutoTrackDOMPoint tracker(mRangeUpdater,
                                  address_of(newSelParent), &newSelOffset);
      res = RemoveStyleInside(leftNode, aProperty, aAttribute);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    *aNode = newSelParent;
    *aOffset = newSelOffset;
  }

  return NS_OK;
}

bool nsHTMLEditor::NodeIsProperty(nsIDOMNode *aNode)
{
  NS_ENSURE_TRUE(aNode, false);
  if (!IsContainer(aNode))  return false;
  if (!IsEditable(aNode))   return false;
  if (IsBlockNode(aNode))   return false;
  if (NodeIsType(aNode, nsEditProperty::a)) return false;
  return true;
}

nsresult nsHTMLEditor::ApplyDefaultProperties()
{
  nsresult res = NS_OK;
  uint32_t j, defcon = mDefaultStyles.Length();
  for (j=0; j<defcon; j++)
  {
    PropItem *propItem = mDefaultStyles[j];
    NS_ENSURE_TRUE(propItem, NS_ERROR_NULL_POINTER);
    res = SetInlineProperty(propItem->tag, propItem->attr, propItem->value);
    NS_ENSURE_SUCCESS(res, res);
  }
  return res;
}

nsresult nsHTMLEditor::RemoveStyleInside(nsIDOMNode *aNode, 
                                         
                                         nsIAtom *aProperty,
                                         const nsAString *aAttribute,
                                         const bool aChildrenOnly)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);
  if (IsTextNode(aNode)) {
    return NS_OK;
  }
  nsresult res;

  
  nsCOMPtr<nsIDOMNode> child, tmp;
  aNode->GetFirstChild(getter_AddRefs(child));
  while (child) {
    
    child->GetNextSibling(getter_AddRefs(tmp));
    res = RemoveStyleInside(child, aProperty, aAttribute);
    NS_ENSURE_SUCCESS(res, res);
    child = tmp;
  }

  
  if (!aChildrenOnly &&
    (
      
      (aProperty && NodeIsType(aNode, aProperty)) ||
      
      (aProperty == nsEditProperty::href && nsHTMLEditUtils::IsLink(aNode)) ||
      
      (aProperty == nsEditProperty::name && nsHTMLEditUtils::IsNamedAnchor(aNode)) ||
      
      (!aProperty && NodeIsProperty(aNode))
    )
  ) {
    
    
    if (!aAttribute || aAttribute->IsEmpty()) {
      NS_NAMED_LITERAL_STRING(styleAttr, "style");
      NS_NAMED_LITERAL_STRING(classAttr, "class");
      bool hasStyleAttr = HasAttr(aNode, &styleAttr);
      bool hasClassAttr = HasAttr(aNode, &classAttr);
      if (aProperty && (hasStyleAttr || hasClassAttr)) {
        
        
        
        
        nsCOMPtr<nsIDOMNode> spanNode;
        res = InsertContainerAbove(aNode, address_of(spanNode),
                                   NS_LITERAL_STRING("span"));
        NS_ENSURE_SUCCESS(res, res);
        res = CloneAttribute(styleAttr, spanNode, aNode);
        NS_ENSURE_SUCCESS(res, res);
        res = CloneAttribute(classAttr, spanNode, aNode);
        NS_ENSURE_SUCCESS(res, res);
      }
      res = RemoveContainer(aNode);
      NS_ENSURE_SUCCESS(res, res);
    } else {
      
      if (HasAttr(aNode, aAttribute)) {
        
        
        if (IsOnlyAttribute(aNode, aAttribute)) {
          res = RemoveContainer(aNode);
        } else {
          nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(aNode);
          NS_ENSURE_TRUE(elem, NS_ERROR_NULL_POINTER);
          res = RemoveAttribute(elem, *aAttribute);
        }
        NS_ENSURE_SUCCESS(res, res);
      }
    }
  }

  if (!aChildrenOnly &&
      mHTMLCSSUtils->IsCSSEditableProperty(aNode, aProperty, aAttribute)) {
    
    
    
    nsAutoString propertyValue;
    bool isSet;
    mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(aNode, aProperty,
      aAttribute, isSet, propertyValue, nsHTMLCSSUtils::eSpecified);
    if (isSet) {
      
      
      mHTMLCSSUtils->RemoveCSSEquivalentToHTMLStyle(aNode,
                                                    aProperty,
                                                    aAttribute,
                                                    &propertyValue,
                                                    false);
      
      
      RemoveElementIfNoStyleOrIdOrClass(aNode);
    }
  }

  if (!aChildrenOnly &&
    (
      (aProperty == nsEditProperty::font) &&    
      (nsHTMLEditUtils::IsBig(aNode) || nsHTMLEditUtils::IsSmall(aNode)) &&
      (aAttribute && aAttribute->LowerCaseEqualsLiteral("size"))
    )
  ) {
    return RemoveContainer(aNode);  
  }
  return NS_OK;
}

bool nsHTMLEditor::IsOnlyAttribute(nsIDOMNode *aNode, 
                                     const nsAString *aAttribute)
{
  NS_ENSURE_TRUE(aNode && aAttribute, false);  

  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(content, false);  

  return IsOnlyAttribute(content, *aAttribute);
}

bool
nsHTMLEditor::IsOnlyAttribute(const nsIContent* aContent,
                              const nsAString& aAttribute)
{
  MOZ_ASSERT(aContent);

  uint32_t attrCount = aContent->GetAttrCount();
  for (uint32_t i = 0; i < attrCount; ++i) {
    const nsAttrName* name = aContent->GetAttrNameAt(i);
    if (!name->NamespaceEquals(kNameSpaceID_None)) {
      return false;
    }

    nsAutoString attrString;
    name->LocalName()->ToString(attrString);
    
    
    if (!attrString.Equals(aAttribute, nsCaseInsensitiveStringComparator()) &&
        !StringBeginsWith(attrString, NS_LITERAL_STRING("_moz"))) {
      return false;
    }
  }
  
  
  return true;
}

bool nsHTMLEditor::HasAttr(nsIDOMNode* aNode,
                           const nsAString* aAttribute)
{
  NS_ENSURE_TRUE(aNode, false);
  if (!aAttribute || aAttribute->IsEmpty()) {
    
    return true;
  }

  
  nsCOMPtr<dom::Element> element = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(element, false);

  nsCOMPtr<nsIAtom> atom = do_GetAtom(*aAttribute);
  NS_ENSURE_TRUE(atom, false);

  return element->HasAttr(kNameSpaceID_None, atom);
}


nsresult nsHTMLEditor::PromoteRangeIfStartsOrEndsInNamedAnchor(nsIDOMRange *inRange)
{
  NS_ENSURE_TRUE(inRange, NS_ERROR_NULL_POINTER);
  nsresult res;
  nsCOMPtr<nsIDOMNode> startNode, endNode, parent, tmp;
  int32_t startOffset, endOffset, tmpOffset;
  
  res = inRange->GetStartContainer(getter_AddRefs(startNode));
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->GetStartOffset(&startOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->GetEndContainer(getter_AddRefs(endNode));
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->GetEndOffset(&endOffset);
  NS_ENSURE_SUCCESS(res, res);

  tmp = startNode;
  while ( tmp && 
          !nsTextEditUtils::IsBody(tmp) &&
          !nsHTMLEditUtils::IsNamedAnchor(tmp))
  {
    parent = GetNodeLocation(tmp, &tmpOffset);
    tmp = parent;
  }
  NS_ENSURE_TRUE(tmp, NS_ERROR_NULL_POINTER);
  if (nsHTMLEditUtils::IsNamedAnchor(tmp))
  {
    parent = GetNodeLocation(tmp, &tmpOffset);
    startNode = parent;
    startOffset = tmpOffset;
  }

  tmp = endNode;
  while ( tmp && 
          !nsTextEditUtils::IsBody(tmp) &&
          !nsHTMLEditUtils::IsNamedAnchor(tmp))
  {
    parent = GetNodeLocation(tmp, &tmpOffset);
    tmp = parent;
  }
  NS_ENSURE_TRUE(tmp, NS_ERROR_NULL_POINTER);
  if (nsHTMLEditUtils::IsNamedAnchor(tmp))
  {
    parent = GetNodeLocation(tmp, &tmpOffset);
    endNode = parent;
    endOffset = tmpOffset + 1;
  }

  res = inRange->SetStart(startNode, startOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->SetEnd(endNode, endOffset);
  return res;
}

nsresult nsHTMLEditor::PromoteInlineRange(nsIDOMRange *inRange)
{
  NS_ENSURE_TRUE(inRange, NS_ERROR_NULL_POINTER);
  nsresult res;
  nsCOMPtr<nsIDOMNode> startNode, endNode, parent;
  int32_t startOffset, endOffset;
  
  res = inRange->GetStartContainer(getter_AddRefs(startNode));
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->GetStartOffset(&startOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->GetEndContainer(getter_AddRefs(endNode));
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->GetEndOffset(&endOffset);
  NS_ENSURE_SUCCESS(res, res);
  
  while ( startNode && 
          !nsTextEditUtils::IsBody(startNode) && 
          IsEditable(startNode) &&
          IsAtFrontOfNode(startNode, startOffset) )
  {
    parent = GetNodeLocation(startNode, &startOffset);
    startNode = parent;
  }
  NS_ENSURE_TRUE(startNode, NS_ERROR_NULL_POINTER);
  
  while ( endNode && 
          !nsTextEditUtils::IsBody(endNode) && 
          IsEditable(endNode) &&
          IsAtEndOfNode(endNode, endOffset) )
  {
    parent = GetNodeLocation(endNode, &endOffset);
    endNode = parent;
    endOffset++;  
  }
  NS_ENSURE_TRUE(endNode, NS_ERROR_NULL_POINTER);
  
  res = inRange->SetStart(startNode, startOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->SetEnd(endNode, endOffset);
  return res;
}

bool nsHTMLEditor::IsAtFrontOfNode(nsIDOMNode *aNode, int32_t aOffset)
{
  NS_ENSURE_TRUE(aNode, false);  
  if (!aOffset) {
    return true;
  }

  if (IsTextNode(aNode))
  {
    return false;
  }
  else
  {
    nsCOMPtr<nsIDOMNode> firstNode;
    GetFirstEditableChild(aNode, address_of(firstNode));
    NS_ENSURE_TRUE(firstNode, true); 
    int32_t offset = GetChildOffset(firstNode, aNode);
    if (offset < aOffset) return false;
    return true;
  }
}

bool nsHTMLEditor::IsAtEndOfNode(nsIDOMNode *aNode, int32_t aOffset)
{
  NS_ENSURE_TRUE(aNode, false);  
  uint32_t len;
  GetLengthOfDOMNode(aNode, len);
  if (aOffset == (int32_t)len) return true;
  
  if (IsTextNode(aNode))
  {
    return false;
  }
  else
  {
    nsCOMPtr<nsIDOMNode> lastNode;
    GetLastEditableChild(aNode, address_of(lastNode));
    NS_ENSURE_TRUE(lastNode, true); 
    int32_t offset = GetChildOffset(lastNode, aNode);
    if (offset < aOffset) return true;
    return false;
  }
}


nsresult
nsHTMLEditor::GetInlinePropertyBase(nsIAtom *aProperty, 
                                    const nsAString *aAttribute,
                                    const nsAString *aValue,
                                    bool *aFirst, 
                                    bool *aAny, 
                                    bool *aAll,
                                    nsAString *outValue,
                                    bool aCheckDefaults)
{
  NS_ENSURE_TRUE(aProperty, NS_ERROR_NULL_POINTER);

  nsresult result;
  *aAny = false;
  *aAll = true;
  *aFirst = false;
  bool first = true;

  nsCOMPtr<nsISelection> selection;
  result = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));

  bool isCollapsed = selection->Collapsed();
  nsCOMPtr<nsIDOMNode> collapsedNode;
  nsCOMPtr<nsIEnumerator> enumerator;
  result = selPriv->GetEnumerator(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_NULL_POINTER);

  enumerator->First(); 
  nsCOMPtr<nsISupports> currentItem;
  result = enumerator->CurrentItem(getter_AddRefs(currentItem));
  
  
  if (NS_SUCCEEDED(result) && currentItem) {
    bool firstNodeInRange = true; 
    nsCOMPtr<nsIDOMRange> range(do_QueryInterface(currentItem));

    if (isCollapsed) {
      range->GetStartContainer(getter_AddRefs(collapsedNode));
      NS_ENSURE_TRUE(collapsedNode, NS_ERROR_FAILURE);
      bool isSet, theSetting;
      nsString tOutString;
      if (aAttribute) {
        nsString tString(*aAttribute);
        mTypeInState->GetTypingState(isSet, theSetting, aProperty, tString,
                                     &tOutString);
        if (outValue) {
          outValue->Assign(tOutString);
        }
      } else {
        mTypeInState->GetTypingState(isSet, theSetting, aProperty);
      }
      if (isSet) {
        *aFirst = *aAny = *aAll = theSetting;
        return NS_OK;
      }

      if (mHTMLCSSUtils->IsCSSEditableProperty(collapsedNode, aProperty, aAttribute)) {
        mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(
          collapsedNode, aProperty, aAttribute, isSet, tOutString,
          nsHTMLCSSUtils::eComputed);
        if (outValue) {
          outValue->Assign(tOutString);
        }
        *aFirst = *aAny = *aAll = isSet;
        return NS_OK;
      }

      IsTextPropertySetByContent(collapsedNode, aProperty, aAttribute, aValue,
                                 isSet, outValue);
      *aFirst = *aAny = *aAll = isSet;

      if (!isSet && aCheckDefaults) {
        
        
        
        int32_t index;
        if (aAttribute && TypeInState::FindPropInList(aProperty, *aAttribute,
                                                      outValue, mDefaultStyles,
                                                      index)) {
          *aFirst = *aAny = *aAll = true;
          if (outValue) {
            outValue->Assign(mDefaultStyles[index]->value);
          }
        }
      }
      return NS_OK;
    }

    
    nsCOMPtr<nsIContentIterator> iter =
            do_CreateInstance("@mozilla.org/content/post-content-iterator;1");
    NS_ENSURE_TRUE(iter, NS_ERROR_NULL_POINTER);

    nsAutoString firstValue, theValue;

    nsCOMPtr<nsIDOMNode> endNode;
    int32_t endOffset;
    result = range->GetEndContainer(getter_AddRefs(endNode));
    NS_ENSURE_SUCCESS(result, result);
    result = range->GetEndOffset(&endOffset);
    NS_ENSURE_SUCCESS(result, result);

    for (iter->Init(range); !iter->IsDone(); iter->Next()) {
      if (!iter->GetCurrentNode()->IsContent()) {
        continue;
      }
      nsCOMPtr<nsIContent> content = iter->GetCurrentNode()->AsContent();
      nsCOMPtr<nsIDOMNode> node = content->AsDOMNode();

      if (nsTextEditUtils::IsBody(node)) {
        break;
      }

      nsCOMPtr<nsIDOMCharacterData> text;
      text = do_QueryInterface(content);
      
      
      if (text && (!IsEditable(text) || IsEmptyTextNode(this, content))) {
        continue;
      }
      if (text) {
        if (!isCollapsed && first && firstNodeInRange) {
          firstNodeInRange = false;
          int32_t startOffset;
          range->GetStartOffset(&startOffset);
          uint32_t count;
          text->GetLength(&count);
          if (startOffset == (int32_t)count) {
            continue;
          }
        } else if (node == endNode && !endOffset) {
          continue;
        }
      } else if (content->IsElement()) {
        
        continue;
      }

      bool isSet = false;
      if (first) {
        if (mHTMLCSSUtils->IsCSSEditableProperty(node, aProperty, aAttribute)){
          
          
          
          if (aValue) {
            firstValue.Assign(*aValue);
          }
          mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(node, aProperty,
            aAttribute, isSet, firstValue, nsHTMLCSSUtils::eComputed);
        } else {
          IsTextPropertySetByContent(node, aProperty, aAttribute, aValue, isSet,
                                     &firstValue);
        }
        *aFirst = isSet;
        first = false;
        if (outValue) {
          *outValue = firstValue;
        }
      } else {
        if (mHTMLCSSUtils->IsCSSEditableProperty(node, aProperty, aAttribute)){
          
          
          if (aValue) {
            theValue.Assign(*aValue);
          }
          mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(node, aProperty,
            aAttribute, isSet, theValue, nsHTMLCSSUtils::eComputed);
        } else {
          IsTextPropertySetByContent(node, aProperty, aAttribute, aValue, isSet,
                                     &theValue);
        }
        if (firstValue != theValue) {
          *aAll = false;
        }
      }

      if (isSet) {
        *aAny = true;
      } else {
        *aAll = false;
      }
    }
  }
  if (!*aAny) {
    
    
    *aAll = false;
  }
  return result;
}


NS_IMETHODIMP nsHTMLEditor::GetInlineProperty(nsIAtom *aProperty, 
                                              const nsAString &aAttribute, 
                                              const nsAString &aValue,
                                              bool *aFirst, 
                                              bool *aAny, 
                                              bool *aAll)
{
  NS_ENSURE_TRUE(aProperty && aFirst && aAny && aAll, NS_ERROR_NULL_POINTER);
  const nsAString *att = nullptr;
  if (!aAttribute.IsEmpty())
    att = &aAttribute;
  const nsAString *val = nullptr;
  if (!aValue.IsEmpty())
    val = &aValue;
  return GetInlinePropertyBase( aProperty, att, val, aFirst, aAny, aAll, nullptr);
}


NS_IMETHODIMP nsHTMLEditor::GetInlinePropertyWithAttrValue(nsIAtom *aProperty, 
                                              const nsAString &aAttribute, 
                                              const nsAString &aValue,
                                              bool *aFirst, 
                                              bool *aAny, 
                                              bool *aAll,
                                              nsAString &outValue)
{
  NS_ENSURE_TRUE(aProperty && aFirst && aAny && aAll, NS_ERROR_NULL_POINTER);
  const nsAString *att = nullptr;
  if (!aAttribute.IsEmpty())
    att = &aAttribute;
  const nsAString *val = nullptr;
  if (!aValue.IsEmpty())
    val = &aValue;
  return GetInlinePropertyBase( aProperty, att, val, aFirst, aAny, aAll, &outValue);
}


NS_IMETHODIMP nsHTMLEditor::RemoveAllInlineProperties()
{
  nsAutoEditBatch batchIt(this);
  nsAutoRules beginRulesSniffing(this, EditAction::resetTextProperties, nsIEditor::eNext);

  nsresult res = RemoveInlinePropertyImpl(nullptr, nullptr);
  NS_ENSURE_SUCCESS(res, res);
  return ApplyDefaultProperties();
}

NS_IMETHODIMP nsHTMLEditor::RemoveInlineProperty(nsIAtom *aProperty, const nsAString &aAttribute)
{
  return RemoveInlinePropertyImpl(aProperty, &aAttribute);
}

nsresult nsHTMLEditor::RemoveInlinePropertyImpl(nsIAtom *aProperty, const nsAString *aAttribute)
{
  MOZ_ASSERT_IF(aProperty, aAttribute);
  NS_ENSURE_TRUE(mRules, NS_ERROR_NOT_INITIALIZED);
  ForceCompositionEnd();

  nsresult res;
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);

  bool useCSS = IsCSSEnabled();
  if (selection->Collapsed()) {
    

    
    if (aProperty == nsEditProperty::href ||
        aProperty == nsEditProperty::name)
      aProperty = nsEditProperty::a;

    if (aProperty) {
      mTypeInState->ClearProp(aProperty, *aAttribute);
    } else {
      mTypeInState->ClearAllProps();
    }
    return NS_OK;
  }

  nsAutoEditBatch batchIt(this);
  nsAutoRules beginRulesSniffing(this, EditAction::removeTextProperty, nsIEditor::eNext);
  nsAutoSelectionReset selectionResetter(selection, this);
  nsAutoTxnsConserveSelection dontSpazMySelection(this);
  
  bool cancel, handled;
  nsTextRulesInfo ruleInfo(EditAction::removeTextProperty);
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(res, res);
  if (!cancel && !handled)
  {
    
    nsCOMPtr<nsIEnumerator> enumerator;
    res = selection->GetEnumerator(getter_AddRefs(enumerator));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(enumerator, NS_ERROR_FAILURE);

    
    enumerator->First(); 
    nsCOMPtr<nsISupports> currentItem;
    while (static_cast<nsresult>(NS_ENUMERATOR_FALSE) == enumerator->IsDone()) {
      res = enumerator->CurrentItem(getter_AddRefs(currentItem));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(currentItem, NS_ERROR_FAILURE);
      
      nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );

      if (aProperty == nsEditProperty::name)
      {
        
        
        res = PromoteRangeIfStartsOrEndsInNamedAnchor(range);
      }
      else {
        
        res = PromoteInlineRange(range);
      }
      NS_ENSURE_SUCCESS(res, res);

      
      
      res = SplitStyleAboveRange(range, aProperty, aAttribute);
      NS_ENSURE_SUCCESS(res, res);

      
      nsCOMPtr<nsIDOMNode> startNode, endNode;
      res = range->GetStartContainer(getter_AddRefs(startNode));
      NS_ENSURE_SUCCESS(res, res);
      res = range->GetEndContainer(getter_AddRefs(endNode));
      NS_ENSURE_SUCCESS(res, res);
      if ((startNode == endNode) && IsTextNode(startNode))
      {
        
        if (useCSS && mHTMLCSSUtils->IsCSSEditableProperty(startNode, aProperty, aAttribute)) {
          
          
          nsAutoString cssValue;
          bool isSet = false;
          mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(startNode,
            aProperty, aAttribute, isSet , cssValue,
            nsHTMLCSSUtils::eComputed);
          if (isSet) {
            
            
            
            
            nsAutoString value; value.AssignLiteral("-moz-editor-invert-value");
            int32_t startOffset, endOffset;
            range->GetStartOffset(&startOffset);
            range->GetEndOffset(&endOffset);
            nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(startNode);
            if (mHTMLCSSUtils->IsCSSInvertable(aProperty, aAttribute)) {
              SetInlinePropertyOnTextNode(nodeAsText, startOffset, endOffset, aProperty, aAttribute, &value);
            }
          }
        }
      }
      else
      {
        
        nsCOMPtr<nsIContentIterator> iter =
          do_CreateInstance("@mozilla.org/content/subtree-content-iterator;1", &res);
        NS_ENSURE_SUCCESS(res, res);
        NS_ENSURE_TRUE(iter, NS_ERROR_FAILURE);

        nsCOMArray<nsIDOMNode> arrayOfNodes;
        nsCOMPtr<nsIDOMNode> node;
        
        
        iter->Init(range);
        while (!iter->IsDone())
        {
          node = do_QueryInterface(iter->GetCurrentNode());
          NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

          if (IsEditable(node))
          { 
            arrayOfNodes.AppendObject(node);
          }

          iter->Next();
        }
        
        
        int32_t listCount = arrayOfNodes.Count();
        int32_t j;
        for (j = 0; j < listCount; j++)
        {
          node = arrayOfNodes[j];
          res = RemoveStyleInside(node, aProperty, aAttribute);
          NS_ENSURE_SUCCESS(res, res);
          if (useCSS && mHTMLCSSUtils->IsCSSEditableProperty(node, aProperty, aAttribute)) {
            
            
            nsAutoString cssValue;
            bool isSet = false;
            mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(node, aProperty,
              aAttribute, isSet , cssValue, nsHTMLCSSUtils::eComputed);
            if (isSet) {
              
              
              
              
              if (mHTMLCSSUtils->IsCSSInvertable(aProperty, aAttribute)) {
                nsAutoString value; value.AssignLiteral("-moz-editor-invert-value");
                SetInlinePropertyOnNode(node, aProperty, aAttribute, &value);
              }
            }
          }
        }
        arrayOfNodes.Clear();
      }
      enumerator->Next();
    }
  }
  if (!cancel)
  {
    
    res = mRules->DidDoAction(selection, &ruleInfo, res);
  }
  return res;
}

NS_IMETHODIMP nsHTMLEditor::IncreaseFontSize()
{
  return RelativeFontChange(1);
}

NS_IMETHODIMP nsHTMLEditor::DecreaseFontSize()
{
  return RelativeFontChange(-1);
}

nsresult
nsHTMLEditor::RelativeFontChange( int32_t aSizeChange)
{
  
  if ( !( (aSizeChange==1) || (aSizeChange==-1) ) )
    return NS_ERROR_ILLEGAL_VALUE;
  
  ForceCompositionEnd();

  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);
  
  
  if (selection->Collapsed()) {
    nsCOMPtr<nsIAtom> atom;
    if (aSizeChange==1) atom = nsEditProperty::big;
    else                atom = nsEditProperty::small;

    
    int32_t offset;
    nsCOMPtr<nsIDOMNode> selectedNode;
    GetStartNodeAndOffset(selection, getter_AddRefs(selectedNode), &offset);
    NS_ENSURE_TRUE(selectedNode, NS_OK);
    if (IsTextNode(selectedNode)) {
      nsCOMPtr<nsIDOMNode> parent;
      nsresult res = selectedNode->GetParentNode(getter_AddRefs(parent));
      NS_ENSURE_SUCCESS(res, res);
      selectedNode = parent;
    }
    if (!CanContainTag(selectedNode, atom)) {
      return NS_OK;
    }

    
    mTypeInState->SetProp(atom, EmptyString(), EmptyString());
    return NS_OK;
  }
  
  
  nsAutoEditBatch batchIt(this);
  nsAutoRules beginRulesSniffing(this, EditAction::setTextProperty, nsIEditor::eNext);
  nsAutoSelectionReset selectionResetter(selection, this);
  nsAutoTxnsConserveSelection dontSpazMySelection(this);

  
  nsCOMPtr<nsIEnumerator> enumerator;
  nsresult res = selection->GetEnumerator(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_FAILURE);

  
  enumerator->First(); 
  nsCOMPtr<nsISupports> currentItem;
  while (static_cast<nsresult>(NS_ENUMERATOR_FALSE) == enumerator->IsDone()) {
    res = enumerator->CurrentItem(getter_AddRefs(currentItem));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(currentItem, NS_ERROR_FAILURE);
    
    nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );

    
    res = PromoteInlineRange(range);
    NS_ENSURE_SUCCESS(res, res);
    
    
    nsCOMPtr<nsIDOMNode> startNode, endNode;
    res = range->GetStartContainer(getter_AddRefs(startNode));
    NS_ENSURE_SUCCESS(res, res);
    res = range->GetEndContainer(getter_AddRefs(endNode));
    NS_ENSURE_SUCCESS(res, res);
    if ((startNode == endNode) && IsTextNode(startNode))
    {
      int32_t startOffset, endOffset;
      range->GetStartOffset(&startOffset);
      range->GetEndOffset(&endOffset);
      nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(startNode);
      res = RelativeFontChangeOnTextNode(aSizeChange, nodeAsText, startOffset, endOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
    else
    {
      
      
      
      
      
      
      
      
      
      

      nsCOMPtr<nsIContentIterator> iter =
        do_CreateInstance("@mozilla.org/content/subtree-content-iterator;1", &res);
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(iter, NS_ERROR_FAILURE);

      
      res = iter->Init(range);
      if (NS_SUCCEEDED(res)) {
        nsCOMArray<nsIContent> arrayOfNodes;
        while (!iter->IsDone()) {
          NS_ENSURE_TRUE(iter->GetCurrentNode()->IsContent(), NS_ERROR_FAILURE);
          nsCOMPtr<nsIContent> node = iter->GetCurrentNode()->AsContent();

          if (IsEditable(node)) {
            arrayOfNodes.AppendObject(node);
          }

          iter->Next();
        }
        
        
        int32_t listCount = arrayOfNodes.Count();
        for (int32_t j = 0; j < listCount; ++j) {
          nsIContent* node = arrayOfNodes[j];
          res = RelativeFontChangeOnNode(aSizeChange, node);
          NS_ENSURE_SUCCESS(res, res);
        }
        arrayOfNodes.Clear();
      }
      
      
      
      if (IsTextNode(startNode) && IsEditable(startNode))
      {
        nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(startNode);
        int32_t startOffset;
        uint32_t textLen;
        range->GetStartOffset(&startOffset);
        nodeAsText->GetLength(&textLen);
        res = RelativeFontChangeOnTextNode(aSizeChange, nodeAsText, startOffset, textLen);
        NS_ENSURE_SUCCESS(res, res);
      }
      if (IsTextNode(endNode) && IsEditable(endNode))
      {
        nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(endNode);
        int32_t endOffset;
        range->GetEndOffset(&endOffset);
        res = RelativeFontChangeOnTextNode(aSizeChange, nodeAsText, 0, endOffset);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
    enumerator->Next();
  }
  
  return res;  
}

nsresult
nsHTMLEditor::RelativeFontChangeOnTextNode( int32_t aSizeChange, 
                                            nsIDOMCharacterData *aTextNode, 
                                            int32_t aStartOffset,
                                            int32_t aEndOffset)
{
  
  if ( !( (aSizeChange==1) || (aSizeChange==-1) ) )
    return NS_ERROR_ILLEGAL_VALUE;
  NS_ENSURE_TRUE(aTextNode, NS_ERROR_NULL_POINTER);
  
  
  if (aStartOffset == aEndOffset) return NS_OK;
  
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> parent;
  res = aTextNode->GetParentNode(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(res, res);
  if (!CanContainTag(parent, nsGkAtoms::big)) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> tmp, node = do_QueryInterface(aTextNode);

  
  uint32_t textLen;
  aTextNode->GetLength(&textLen);
  
  
  if (aEndOffset == -1) aEndOffset = textLen;
  
  if ( (uint32_t)aEndOffset != textLen )
  {
    
    res = SplitNode(node, aEndOffset, getter_AddRefs(tmp));
    NS_ENSURE_SUCCESS(res, res);
    node = tmp;  
  }
  if ( aStartOffset )
  {
    
    res = SplitNode(node, aStartOffset, getter_AddRefs(tmp));
    NS_ENSURE_SUCCESS(res, res);
  }

  NS_NAMED_LITERAL_STRING(bigSize, "big");
  NS_NAMED_LITERAL_STRING(smallSize, "small");
  const nsAString& nodeType = (aSizeChange==1) ? static_cast<const nsAString&>(bigSize) : static_cast<const nsAString&>(smallSize);
  
  nsCOMPtr<nsIDOMNode> sibling;
  GetPriorHTMLSibling(node, address_of(sibling));
  if (sibling && NodeIsType(sibling, (aSizeChange==1) ? nsEditProperty::big : nsEditProperty::small))
  {
    
    res = MoveNode(node, sibling, -1);
    return res;
  }
  sibling = nullptr;
  GetNextHTMLSibling(node, address_of(sibling));
  if (sibling && NodeIsType(sibling, (aSizeChange==1) ? nsEditProperty::big : nsEditProperty::small))
  {
    
    res = MoveNode(node, sibling, 0);
    return res;
  }
  
  
  res = InsertContainerAbove(node, address_of(tmp), nodeType);
  return res;
}


nsresult
nsHTMLEditor::RelativeFontChangeHelper(int32_t aSizeChange, nsINode* aNode)
{
  MOZ_ASSERT(aNode);

  




  
  
  if (aSizeChange != 1 && aSizeChange != -1) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  
  if (aNode->IsElement() && aNode->AsElement()->IsHTML(nsGkAtoms::font) &&
      aNode->AsElement()->HasAttr(kNameSpaceID_None, nsGkAtoms::size)) {
    
    for (uint32_t i = aNode->GetChildCount(); i--; ) {
      nsresult rv = RelativeFontChangeOnNode(aSizeChange, aNode->GetChildAt(i));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    return NS_OK;
  }

  
  for (uint32_t i = aNode->GetChildCount(); i--; ) {
    nsresult rv = RelativeFontChangeHelper(aSizeChange, aNode->GetChildAt(i));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


nsresult
nsHTMLEditor::RelativeFontChangeOnNode(int32_t aSizeChange, nsINode* aNode)
{
  MOZ_ASSERT(aNode);
  
  if (aSizeChange != 1 && aSizeChange != -1) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  nsIAtom* atom;
  if (aSizeChange == 1) {
    atom = nsGkAtoms::big;
  } else {
    atom = nsGkAtoms::small;
  }
  
  
  if (aNode->IsElement() &&
      ((aSizeChange == 1 && aNode->AsElement()->IsHTML(nsGkAtoms::small)) ||
       (aSizeChange == -1 && aNode->AsElement()->IsHTML(nsGkAtoms::big)))) {
    
    nsresult rv = RelativeFontChangeHelper(aSizeChange, aNode);
    NS_ENSURE_SUCCESS(rv, rv);
    
    return RemoveContainer(aNode);
  }

  
  if (TagCanContain(atom, aNode->AsDOMNode())) {
    
    nsresult rv = RelativeFontChangeHelper(aSizeChange, aNode);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    nsIContent* sibling = GetPriorHTMLSibling(aNode);
    if (sibling && sibling->IsHTML(atom)) {
      
      return MoveNode(aNode->AsDOMNode(), sibling->AsDOMNode(), -1);
    }

    sibling = GetNextHTMLSibling(aNode);
    if (sibling && sibling->IsHTML(atom)) {
      
      return MoveNode(aNode->AsDOMNode(), sibling->AsDOMNode(), 0);
    }

    
    nsCOMPtr<nsIDOMNode> tmp;
    return InsertContainerAbove(aNode->AsDOMNode(), address_of(tmp),
                                nsAtomString(atom));
  }

  
  
  
  
  for (uint32_t i = aNode->GetChildCount(); i--; ) {
    nsresult rv = RelativeFontChangeOnNode(aSizeChange, aNode->GetChildAt(i));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLEditor::GetFontFaceState(bool *aMixed, nsAString &outFace)
{
  NS_ENSURE_TRUE(aMixed, NS_ERROR_FAILURE);
  *aMixed = true;
  outFace.Truncate();

  nsresult res;
  bool first, any, all;
  
  NS_NAMED_LITERAL_STRING(attr, "face");
  res = GetInlinePropertyBase(nsEditProperty::font, &attr, nullptr, &first, &any, &all, &outFace);
  NS_ENSURE_SUCCESS(res, res);
  if (any && !all) return res; 
  if (all)
  {
    *aMixed = false;
    return res;
  }
  
  
  res = GetInlinePropertyBase(nsEditProperty::tt, nullptr, nullptr, &first, &any, &all,nullptr);
  NS_ENSURE_SUCCESS(res, res);
  if (any && !all) return res; 
  if (all)
  {
    *aMixed = false;
    nsEditProperty::tt->ToString(outFace);
  }
  
  if (!any)
  {
    
    outFace.Truncate();
    *aMixed = false;
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::GetFontColorState(bool *aMixed, nsAString &aOutColor)
{
  NS_ENSURE_TRUE(aMixed, NS_ERROR_NULL_POINTER);
  *aMixed = true;
  aOutColor.Truncate();
  
  nsresult res;
  NS_NAMED_LITERAL_STRING(colorStr, "color");
  bool first, any, all;
  
  res = GetInlinePropertyBase(nsEditProperty::font, &colorStr, nullptr, &first, &any, &all, &aOutColor);
  NS_ENSURE_SUCCESS(res, res);
  if (any && !all) return res; 
  if (all)
  {
    *aMixed = false;
    return res;
  }
  
  if (!any)
  {
    
    aOutColor.Truncate();
    *aMixed = false;
  }
  return res;
}




nsresult
nsHTMLEditor::GetIsCSSEnabled(bool *aIsCSSEnabled)
{
  *aIsCSSEnabled = IsCSSEnabled();
  return NS_OK;
}

static bool
HasNonEmptyAttribute(dom::Element* aElement, nsIAtom* aName)
{
  MOZ_ASSERT(aElement);

  nsAutoString value;
  return aElement->GetAttr(kNameSpaceID_None, aName, value) && !value.IsEmpty();
}

bool
nsHTMLEditor::HasStyleOrIdOrClass(dom::Element* aElement)
{
  MOZ_ASSERT(aElement);

  
  
  return HasNonEmptyAttribute(aElement, nsGkAtoms::style) ||
         HasNonEmptyAttribute(aElement, nsGkAtoms::_class) ||
         HasNonEmptyAttribute(aElement, nsGkAtoms::id);
}

nsresult
nsHTMLEditor::RemoveElementIfNoStyleOrIdOrClass(nsIDOMNode* aElement)
{
  nsCOMPtr<dom::Element> element = do_QueryInterface(aElement);
  NS_ENSURE_TRUE(element, NS_ERROR_NULL_POINTER);

  
  if ((!element->IsHTML(nsGkAtoms::span) &&
       !element->IsHTML(nsGkAtoms::font)) ||
      HasStyleOrIdOrClass(element)) {
    return NS_OK;
  }

  return RemoveContainer(element);
}
