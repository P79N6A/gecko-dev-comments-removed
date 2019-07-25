





































#include "nsUnicharUtils.h"

#include "nsHTMLEditor.h"
#include "nsHTMLEditRules.h"
#include "nsTextEditUtils.h"
#include "nsHTMLEditUtils.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMAttr.h"
#include "nsIDOMMouseEvent.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsISelectionController.h"
#include "nsIDocumentObserver.h"
#include "TypeInState.h"

#include "nsIEnumerator.h"
#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsAttrName.h"


NS_IMETHODIMP nsHTMLEditor::AddDefaultProperty(nsIAtom *aProperty, 
                                            const nsAString & aAttribute, 
                                            const nsAString & aValue)
{
  nsString outValue;
  PRInt32 index;
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
  PRInt32 index;
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
  PRUint32 j, defcon = mDefaultStyles.Length();
  for (j=0; j<defcon; j++)
  {
    delete mDefaultStyles[j];
  }
  mDefaultStyles.Clear();
  return NS_OK;
}




NS_IMETHODIMP nsHTMLEditor::SetCSSInlineProperty(nsIAtom *aProperty, 
                            const nsAString & aAttribute, 
                            const nsAString & aValue)
{
  bool useCSS;
  GetIsCSSEnabled(&useCSS);
  if (useCSS) {
    return SetInlineProperty(aProperty, aAttribute, aValue);
  }
  return NS_OK;
}

NS_IMETHODIMP nsHTMLEditor::SetInlineProperty(nsIAtom *aProperty, 
                            const nsAString & aAttribute, 
                            const nsAString & aValue)
{
  if (!aProperty) { return NS_ERROR_NULL_POINTER; }
  if (!mRules) { return NS_ERROR_NOT_INITIALIZED; }
  ForceCompositionEnd();

  nsCOMPtr<nsISelection>selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));

  bool isCollapsed;
  selection->GetIsCollapsed(&isCollapsed);
  if (isCollapsed)
  {
    
    nsString tAttr(aAttribute);
    nsString tVal(aValue);
    return mTypeInState->SetProp(aProperty, tAttr, tVal);
  }
  
  nsAutoEditBatch batchIt(this);
  nsAutoRules beginRulesSniffing(this, kOpInsertElement, nsIEditor::eNext);
  nsAutoSelectionReset selectionResetter(selection, this);
  nsAutoTxnsConserveSelection dontSpazMySelection(this);
  
  bool cancel, handled;
  nsTextRulesInfo ruleInfo(nsTextEditRules::kSetTextProperty);
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(res, res);
  if (!cancel && !handled)
  {
    
    nsCOMPtr<nsIEnumerator> enumerator;
    res = selPriv->GetEnumerator(getter_AddRefs(enumerator));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(enumerator, NS_ERROR_FAILURE);

    
    enumerator->First(); 
    nsCOMPtr<nsISupports> currentItem;
    while ((NS_ENUMERATOR_FALSE == enumerator->IsDone()))
    {
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
        PRInt32 startOffset, endOffset;
        range->GetStartOffset(&startOffset);
        range->GetEndOffset(&endOffset);
        nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(startNode);
        res = SetInlinePropertyOnTextNode(nodeAsText, startOffset, endOffset, aProperty, &aAttribute, &aValue);
        NS_ENSURE_SUCCESS(res, res);
      }
      else
      {
        
        
        
        
        
        
        
        
        
        

        nsCOMPtr<nsIContentIterator> iter =
          do_CreateInstance("@mozilla.org/content/subtree-content-iterator;1", &res);
        NS_ENSURE_SUCCESS(res, res);
        NS_ENSURE_TRUE(iter, NS_ERROR_FAILURE);

        nsCOMArray<nsIDOMNode> arrayOfNodes;
        nsCOMPtr<nsIDOMNode> node;
        
        
        res = iter->Init(range);
        
        
        
        
        if (NS_SUCCEEDED(res))
        {
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
        }
        
        
        
        if (IsTextNode(startNode) && IsEditable(startNode))
        {
          nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(startNode);
          PRInt32 startOffset;
          PRUint32 textLen;
          range->GetStartOffset(&startOffset);
          nodeAsText->GetLength(&textLen);
          res = SetInlinePropertyOnTextNode(nodeAsText, startOffset, textLen, aProperty, &aAttribute, &aValue);
          NS_ENSURE_SUCCESS(res, res);
        }
        
        
        PRInt32 listCount = arrayOfNodes.Count();
        PRInt32 j;
        for (j = 0; j < listCount; j++)
        {
          node = arrayOfNodes[j];
          res = SetInlinePropertyOnNode(node, aProperty, &aAttribute, &aValue);
          NS_ENSURE_SUCCESS(res, res);
        }
        arrayOfNodes.Clear();
        
        
        
        
        if (IsTextNode(endNode) && IsEditable(endNode))
        {
          nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(endNode);
          PRInt32 endOffset;
          range->GetEndOffset(&endOffset);
          res = SetInlinePropertyOnTextNode(nodeAsText, 0, endOffset, aProperty, &aAttribute, &aValue);
          NS_ENSURE_SUCCESS(res, res);
        }
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



nsresult
nsHTMLEditor::SetInlinePropertyOnTextNode( nsIDOMCharacterData *aTextNode, 
                                            PRInt32 aStartOffset,
                                            PRInt32 aEndOffset,
                                            nsIAtom *aProperty, 
                                            const nsAString *aAttribute,
                                            const nsAString *aValue)
{
  NS_ENSURE_TRUE(aTextNode, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMNode> parent;
  nsresult res = aTextNode->GetParentNode(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(res, res);

  nsAutoString tagString;
  aProperty->ToString(tagString);
  if (!CanContainTag(parent, tagString)) return NS_OK;
  
  
  if (aStartOffset == aEndOffset) return NS_OK;
  
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aTextNode);
  
  
  bool bHasProp;
  bool useCSS;
  GetIsCSSEnabled(&useCSS);

  if (useCSS &&
      mHTMLCSSUtils->IsCSSEditableProperty(node, aProperty, aAttribute)) {
    
    
    nsAutoString value;
    if (aValue) value.Assign(*aValue);
    mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(node, aProperty, aAttribute,
                                                       bHasProp, value,
                                                       COMPUTED_STYLE_TYPE);
  }
  else
  {
    nsCOMPtr<nsIDOMNode> styleNode;
    IsTextPropertySetByContent(node, aProperty, aAttribute, aValue, bHasProp, getter_AddRefs(styleNode));
  }

  if (bHasProp) return NS_OK;
  
  
  PRUint32 textLen;
  aTextNode->GetLength(&textLen);
  
  nsCOMPtr<nsIDOMNode> tmp;
  if ( (PRUint32)aEndOffset != textLen )
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
  
  
  nsCOMPtr<nsIDOMNode> sibling;
  GetPriorHTMLSibling(node, address_of(sibling));
  if (sibling && NodeIsType(sibling, aProperty) &&         
      HasAttrVal(sibling, aAttribute, aValue) &&
      IsOnlyAttribute(sibling, aAttribute) )
  {
    
    res = MoveNode(node, sibling, -1);
    return res;
  }
  sibling = nsnull;
  GetNextHTMLSibling(node, address_of(sibling));
  if (sibling && NodeIsType(sibling, aProperty) &&         
      HasAttrVal(sibling, aAttribute, aValue) &&
      IsOnlyAttribute(sibling, aAttribute) )
  {
    
    res = MoveNode(node, sibling, 0);
    return res;
  }
  
  
  return SetInlinePropertyOnNode(node, aProperty, aAttribute, aValue);
}


nsresult
nsHTMLEditor::SetInlinePropertyOnNode( nsIDOMNode *aNode,
                                       nsIAtom *aProperty, 
                                       const nsAString *aAttribute,
                                       const nsAString *aValue)
{
  NS_ENSURE_TRUE(aNode && aProperty, NS_ERROR_NULL_POINTER);

  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> tmp;
  nsAutoString tag;
  aProperty->ToString(tag);
  ToLowerCase(tag);
  
  bool useCSS;
  GetIsCSSEnabled(&useCSS);

  if (useCSS)
  {
    
    if (mHTMLCSSUtils->IsCSSEditableProperty(aNode, aProperty, aAttribute))
    {
      
      
      nsCOMPtr<nsIDOMNode> tmp = aNode;
      if (IsTextNode(tmp))
      {
        
        
        InsertContainerAbove( aNode, 
                              address_of(tmp), 
                              NS_LITERAL_STRING("span"),
                              nsnull,
                              nsnull);
      }
      nsCOMPtr<nsIDOMElement>element;
      element = do_QueryInterface(tmp);
      
      
      res = RemoveStyleInside(tmp, aProperty, aAttribute, PR_TRUE);
      NS_ENSURE_SUCCESS(res, res);
      PRInt32 count;
      
      res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(element, aProperty, aAttribute, aValue, &count, PR_FALSE);
      NS_ENSURE_SUCCESS(res, res);

      nsCOMPtr<nsIDOMNode> nextSibling, previousSibling;
      GetNextHTMLSibling(tmp, address_of(nextSibling));
      GetPriorHTMLSibling(tmp, address_of(previousSibling));
      if (nextSibling || previousSibling)
      {
        nsCOMPtr<nsIDOMNode> mergeParent;
        res = tmp->GetParentNode(getter_AddRefs(mergeParent));
        NS_ENSURE_SUCCESS(res, res);
        if (previousSibling &&
            nsEditor::NodeIsType(previousSibling, nsEditProperty::span) &&
            NodesSameType(tmp, previousSibling))
        {
          res = JoinNodes(previousSibling, tmp, mergeParent);
          NS_ENSURE_SUCCESS(res, res);
        }
        if (nextSibling &&
            nsEditor::NodeIsType(nextSibling, nsEditProperty::span) &&
            NodesSameType(tmp, nextSibling))
        {
          res = JoinNodes(tmp, nextSibling, mergeParent);
        }
      }
      return res;
    }
  }
  
  
  bool bHasProp;
  nsCOMPtr<nsIDOMNode> styleNode;
  IsTextPropertySetByContent(aNode, aProperty, aAttribute, aValue, bHasProp, getter_AddRefs(styleNode));
  if (bHasProp) return NS_OK;

  
  if (NodeIsType(aNode, aProperty))
  {
    
    
    res = RemoveStyleInside(aNode, aProperty, aAttribute, PR_TRUE);
    NS_ENSURE_SUCCESS(res, res);
    nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(aNode);
    return SetAttribute(elem, *aAttribute, *aValue);
  }
  
  
  if (TagCanContain(tag, aNode))
  {
    nsCOMPtr<nsIDOMNode> priorNode, nextNode;
    
    GetPriorHTMLSibling(aNode, address_of(priorNode));
    GetNextHTMLSibling(aNode, address_of(nextNode));
    if (priorNode && NodeIsType(priorNode, aProperty) && 
        HasAttrVal(priorNode, aAttribute, aValue)     &&
        IsOnlyAttribute(priorNode, aAttribute) )
    {
      
      res = MoveNode(aNode, priorNode, -1);
    }
    else if (nextNode && NodeIsType(nextNode, aProperty) && 
             HasAttrVal(nextNode, aAttribute, aValue)    &&
             IsOnlyAttribute(priorNode, aAttribute) )
    {
      
      res = MoveNode(aNode, nextNode, 0);
    }
    else
    {
      
      res = InsertContainerAbove(aNode, address_of(tmp), tag, aAttribute, aValue);
    }
    NS_ENSURE_SUCCESS(res, res);
    return RemoveStyleInside(aNode, aProperty, aAttribute);
  }
  
  nsCOMPtr<nsIDOMNodeList> childNodes;
  res = aNode->GetChildNodes(getter_AddRefs(childNodes));
  NS_ENSURE_SUCCESS(res, res);
  if (childNodes)
  {
    PRInt32 j;
    PRUint32 childCount;
    childNodes->GetLength(&childCount);
    if (childCount)
    {
      nsCOMArray<nsIDOMNode> arrayOfNodes;
      nsCOMPtr<nsIDOMNode> node;
      
      
      for (j=0 ; j < (PRInt32)childCount; j++)
      {
        nsCOMPtr<nsIDOMNode> childNode;
        res = childNodes->Item(j, getter_AddRefs(childNode));
        if ((NS_SUCCEEDED(res)) && (childNode) && IsEditable(childNode))
        {
          arrayOfNodes.AppendObject(childNode);
        }
      }
      
      
      PRInt32 listCount = arrayOfNodes.Count();
      for (j = 0; j < listCount; j++)
      {
        node = arrayOfNodes[j];
        res = SetInlinePropertyOnNode(node, aProperty, aAttribute, aValue);
        NS_ENSURE_SUCCESS(res, res);
      }
      arrayOfNodes.Clear();
    }
  }
  return res;
}


nsresult nsHTMLEditor::SplitStyleAboveRange(nsIDOMRange *inRange, 
                                            nsIAtom *aProperty, 
                                            const nsAString *aAttribute)
{
  NS_ENSURE_TRUE(inRange, NS_ERROR_NULL_POINTER);
  nsresult res;
  nsCOMPtr<nsIDOMNode> startNode, endNode, origStartNode;
  PRInt32 startOffset, endOffset, origStartOffset;
  
  res = inRange->GetStartContainer(getter_AddRefs(startNode));
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->GetStartOffset(&startOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->GetEndContainer(getter_AddRefs(endNode));
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->GetEndOffset(&endOffset);
  NS_ENSURE_SUCCESS(res, res);
  
  origStartNode = startNode;
  origStartOffset = startOffset;
  
  
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
                                           PRInt32 *aOffset,
                                           nsIAtom *aProperty,          
                                           const nsAString *aAttribute,
                                           nsCOMPtr<nsIDOMNode> *outLeftNode,
                                           nsCOMPtr<nsIDOMNode> *outRightNode)
{
  NS_ENSURE_TRUE(aNode && *aNode && aOffset, NS_ERROR_NULL_POINTER);
  if (outLeftNode)  *outLeftNode  = nsnull;
  if (outRightNode) *outRightNode = nsnull;
  
  nsCOMPtr<nsIDOMNode> parent, tmp = *aNode;
  PRInt32 offset;

  bool useCSS;
  GetIsCSSEnabled(&useCSS);

  bool isSet;
  while (tmp && !IsBlockNode(tmp))
  {
    isSet = PR_FALSE;
    if (useCSS && mHTMLCSSUtils->IsCSSEditableProperty(tmp, aProperty, aAttribute)) {
      
      
      nsAutoString firstValue;
      mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(tmp, aProperty, aAttribute,
                                                         isSet, firstValue,
                                                         SPECIFIED_STYLE_TYPE);
    }
    if ( (aProperty && NodeIsType(tmp, aProperty)) ||   
         (aProperty == nsEditProperty::href && nsHTMLEditUtils::IsLink(tmp)) ||
                                                        
         (!aProperty && NodeIsProperty(tmp)) ||         
         isSet)                                         
    {
      
      nsresult rv = SplitNodeDeep(tmp, *aNode, *aOffset, &offset, PR_FALSE,
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

bool nsHTMLEditor::NodeIsProperty(nsIDOMNode *aNode)
{
  NS_ENSURE_TRUE(aNode, PR_FALSE);
  if (!IsContainer(aNode))  return PR_FALSE;
  if (!IsEditable(aNode))   return PR_FALSE;
  if (IsBlockNode(aNode))   return PR_FALSE;
  if (NodeIsType(aNode, nsEditProperty::a)) return PR_FALSE;
  return PR_TRUE;
}

nsresult nsHTMLEditor::ApplyDefaultProperties()
{
  nsresult res = NS_OK;
  PRUint32 j, defcon = mDefaultStyles.Length();
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
                                   bool aChildrenOnly)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);
  if (IsTextNode(aNode)) return NS_OK;
  nsresult res = NS_OK;

  
  nsCOMPtr<nsIDOMNode> child, tmp;
  aNode->GetFirstChild(getter_AddRefs(child));
  while (child)
  {
    
    child->GetNextSibling(getter_AddRefs(tmp));
    res = RemoveStyleInside(child, aProperty, aAttribute);
    NS_ENSURE_SUCCESS(res, res);
    child = tmp;
  }

  
  if ( !aChildrenOnly && 
        (aProperty && NodeIsType(aNode, aProperty) || 
        (aProperty == nsEditProperty::href && nsHTMLEditUtils::IsLink(aNode)) || 
        (aProperty == nsEditProperty::name && nsHTMLEditUtils::IsNamedAnchor(aNode))) || 
        (!aProperty && NodeIsProperty(aNode)))  
  {
    
    
    if (!aAttribute || aAttribute->IsEmpty())
    {
      NS_NAMED_LITERAL_STRING(styleAttr, "style");
      NS_NAMED_LITERAL_STRING(classAttr, "class");
      bool hasStyleAttr = HasAttr(aNode, &styleAttr);
      bool hasClassAtrr = HasAttr(aNode, &classAttr);
      if (aProperty &&
          (hasStyleAttr || hasClassAtrr)) {
        
        
        
        
        nsCOMPtr<nsIDOMNode> spanNode;
        res = InsertContainerAbove(aNode, address_of(spanNode),
                                   NS_LITERAL_STRING("span"));
        NS_ENSURE_SUCCESS(res, res);
        res = CloneAttribute(styleAttr, spanNode, aNode);
        NS_ENSURE_SUCCESS(res, res);
        res = CloneAttribute(classAttr, spanNode, aNode);
        NS_ENSURE_SUCCESS(res, res);
        if (hasStyleAttr)
        {
          
          
          nsAutoString propertyValue;
          mHTMLCSSUtils->RemoveCSSEquivalentToHTMLStyle(spanNode,
                                                        aProperty,
                                                        aAttribute,
                                                        &propertyValue,
                                                        PR_FALSE);
          
          nsCOMPtr<nsIDOMElement> element = do_QueryInterface(spanNode);
          res = RemoveElementIfNoStyleOrIdOrClass(element, nsEditProperty::span);
        }
      }
      res = RemoveContainer(aNode);
    }
    
    else
    {
      if (HasAttr(aNode, aAttribute))
      {
        
        
        if (IsOnlyAttribute(aNode, aAttribute))
        {
          res = RemoveContainer(aNode);
        }
        else
        {
          nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(aNode);
          NS_ENSURE_TRUE(elem, NS_ERROR_NULL_POINTER);
          res = RemoveAttribute(elem, *aAttribute);
        }
      }
    }
  }
  else {
    bool useCSS;
    GetIsCSSEnabled(&useCSS);

    if (!aChildrenOnly
        && useCSS && mHTMLCSSUtils->IsCSSEditableProperty(aNode, aProperty, aAttribute)) {
      
      
      nsAutoString propertyValue;
      bool isSet;
      mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(aNode, aProperty, aAttribute,
                                                         isSet, propertyValue,
                                                         SPECIFIED_STYLE_TYPE);
      if (isSet) {
        
        
        mHTMLCSSUtils->RemoveCSSEquivalentToHTMLStyle(aNode,
                                                      aProperty,
                                                      aAttribute,
                                                      &propertyValue,
                                                      PR_FALSE);
        
        
        nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aNode);
        res = RemoveElementIfNoStyleOrIdOrClass(element, nsEditProperty::span);
      }
    }
  }  
  if ( aProperty == nsEditProperty::font &&    
       (nsHTMLEditUtils::IsBig(aNode) || nsHTMLEditUtils::IsSmall(aNode)) &&
       aAttribute && aAttribute->LowerCaseEqualsLiteral("size"))       
  {
    res = RemoveContainer(aNode);  
  }
  return res;
}

bool nsHTMLEditor::IsOnlyAttribute(nsIDOMNode *aNode, 
                                     const nsAString *aAttribute)
{
  NS_ENSURE_TRUE(aNode && aAttribute, PR_FALSE);  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(content, PR_FALSE);  
  
  PRUint32 i, attrCount = content->GetAttrCount();
  for (i = 0; i < attrCount; ++i) {
    nsAutoString attrString;
    const nsAttrName* name = content->GetAttrNameAt(i);
    if (!name->NamespaceEquals(kNameSpaceID_None)) {
      return PR_FALSE;
    }
    name->LocalName()->ToString(attrString);
    
    
    if (!attrString.Equals(*aAttribute, nsCaseInsensitiveStringComparator()) &&
        !StringBeginsWith(attrString, NS_LITERAL_STRING("_moz"))) {
      return PR_FALSE;
    }
  }
  
  
  return PR_TRUE;
}

bool nsHTMLEditor::HasAttr(nsIDOMNode *aNode, 
                             const nsAString *aAttribute)
{
  NS_ENSURE_TRUE(aNode, PR_FALSE);
  if (!aAttribute || aAttribute->IsEmpty()) return PR_TRUE;  
  
  
  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(elem, PR_FALSE);
  
  
  nsCOMPtr<nsIDOMAttr> attNode;
  nsresult res = elem->GetAttributeNode(*aAttribute, getter_AddRefs(attNode));
  if ((NS_FAILED(res)) || !attNode) return PR_FALSE;
  return PR_TRUE;
}


bool nsHTMLEditor::HasAttrVal(nsIDOMNode *aNode, 
                                const nsAString *aAttribute, 
                                const nsAString *aValue)
{
  NS_ENSURE_TRUE(aNode, PR_FALSE);
  if (!aAttribute || aAttribute->IsEmpty()) return PR_TRUE;  
  
  
  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(elem, PR_FALSE);
  
  
  nsCOMPtr<nsIDOMAttr> attNode;
  nsresult res = elem->GetAttributeNode(*aAttribute, getter_AddRefs(attNode));
  if ((NS_FAILED(res)) || !attNode) return PR_FALSE;
  
  
  bool isSet;
  attNode->GetSpecified(&isSet);
  
  if (!isSet && (!aValue || aValue->IsEmpty())) return PR_TRUE; 
  
  
  nsAutoString attrVal;
  attNode->GetValue(attrVal);
  
  
  if (attrVal.Equals(*aValue,nsCaseInsensitiveStringComparator())) return PR_TRUE;
  return PR_FALSE;
}

nsresult nsHTMLEditor::PromoteRangeIfStartsOrEndsInNamedAnchor(nsIDOMRange *inRange)
{
  NS_ENSURE_TRUE(inRange, NS_ERROR_NULL_POINTER);
  nsresult res;
  nsCOMPtr<nsIDOMNode> startNode, endNode, parent, tmp;
  PRInt32 startOffset, endOffset, tmpOffset;
  
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
    res = GetNodeLocation(tmp, address_of(parent), &tmpOffset);
    NS_ENSURE_SUCCESS(res, res);
    tmp = parent;
  }
  NS_ENSURE_TRUE(tmp, NS_ERROR_NULL_POINTER);
  if (nsHTMLEditUtils::IsNamedAnchor(tmp))
  {
    res = GetNodeLocation(tmp, address_of(parent), &tmpOffset);
    NS_ENSURE_SUCCESS(res, res);
    startNode = parent;
    startOffset = tmpOffset;
  }

  tmp = endNode;
  while ( tmp && 
          !nsTextEditUtils::IsBody(tmp) &&
          !nsHTMLEditUtils::IsNamedAnchor(tmp))
  {
    res = GetNodeLocation(tmp, address_of(parent), &tmpOffset);
    NS_ENSURE_SUCCESS(res, res);
    tmp = parent;
  }
  NS_ENSURE_TRUE(tmp, NS_ERROR_NULL_POINTER);
  if (nsHTMLEditUtils::IsNamedAnchor(tmp))
  {
    res = GetNodeLocation(tmp, address_of(parent), &tmpOffset);
    NS_ENSURE_SUCCESS(res, res);
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
  PRInt32 startOffset, endOffset;
  
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
    res = GetNodeLocation(startNode, address_of(parent), &startOffset);
    NS_ENSURE_SUCCESS(res, res);
    startNode = parent;
  }
  NS_ENSURE_TRUE(startNode, NS_ERROR_NULL_POINTER);
  
  while ( endNode && 
          !nsTextEditUtils::IsBody(endNode) && 
          IsEditable(endNode) &&
          IsAtEndOfNode(endNode, endOffset) )
  {
    res = GetNodeLocation(endNode, address_of(parent), &endOffset);
    NS_ENSURE_SUCCESS(res, res);
    endNode = parent;
    endOffset++;  
  }
  NS_ENSURE_TRUE(endNode, NS_ERROR_NULL_POINTER);
  
  res = inRange->SetStart(startNode, startOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = inRange->SetEnd(endNode, endOffset);
  return res;
}

bool nsHTMLEditor::IsAtFrontOfNode(nsIDOMNode *aNode, PRInt32 aOffset)
{
  NS_ENSURE_TRUE(aNode, PR_FALSE);  
  if (!aOffset) {
    return PR_TRUE;
  }

  if (IsTextNode(aNode))
  {
    return PR_FALSE;
  }
  else
  {
    nsCOMPtr<nsIDOMNode> firstNode;
    GetFirstEditableChild(aNode, address_of(firstNode));
    NS_ENSURE_TRUE(firstNode, PR_TRUE); 
    PRInt32 offset;
    nsEditor::GetChildOffset(firstNode, aNode, offset);
    if (offset < aOffset) return PR_FALSE;
    return PR_TRUE;
  }
}

bool nsHTMLEditor::IsAtEndOfNode(nsIDOMNode *aNode, PRInt32 aOffset)
{
  NS_ENSURE_TRUE(aNode, PR_FALSE);  
  PRUint32 len;
  GetLengthOfDOMNode(aNode, len);
  if (aOffset == (PRInt32)len) return PR_TRUE;
  
  if (IsTextNode(aNode))
  {
    return PR_FALSE;
  }
  else
  {
    nsCOMPtr<nsIDOMNode> lastNode;
    GetLastEditableChild(aNode, address_of(lastNode));
    NS_ENSURE_TRUE(lastNode, PR_TRUE); 
    PRInt32 offset;
    nsEditor::GetChildOffset(lastNode, aNode, offset);
    if (offset < aOffset) return PR_TRUE;
    return PR_FALSE;
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
  *aAny=PR_FALSE;
  *aAll=PR_TRUE;
  *aFirst=PR_FALSE;
  bool first=true;

  bool useCSS;
  GetIsCSSEnabled(&useCSS);

  nsCOMPtr<nsISelection>selection;
  result = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));

  bool isCollapsed;
  selection->GetIsCollapsed(&isCollapsed);
  nsCOMPtr<nsIDOMNode> collapsedNode;
  nsCOMPtr<nsIEnumerator> enumerator;
  result = selPriv->GetEnumerator(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_NULL_POINTER);

  enumerator->First(); 
  nsCOMPtr<nsISupports> currentItem;
  result = enumerator->CurrentItem(getter_AddRefs(currentItem));
  
  
  if ((NS_SUCCEEDED(result)) && currentItem)
  {
    bool firstNodeInRange = true; 
    nsCOMPtr<nsIDOMRange> range(do_QueryInterface(currentItem));

    if (isCollapsed)
    {
      range->GetStartContainer(getter_AddRefs(collapsedNode));
      NS_ENSURE_TRUE(collapsedNode, NS_ERROR_FAILURE);
      bool isSet, theSetting;
      if (aAttribute)
      {
        nsString tString(*aAttribute); 
        nsString tOutString;
        nsString *tPassString=nsnull;
        if (outValue)
            tPassString = &tOutString;
        mTypeInState->GetTypingState(isSet, theSetting, aProperty, tString, &tOutString);
        if (outValue)
          outValue->Assign(tOutString);
      }
      else
        mTypeInState->GetTypingState(isSet, theSetting, aProperty);
      if (isSet) 
      {
        *aFirst = *aAny = *aAll = theSetting;
        return NS_OK;
      }
      if (!useCSS) {
        nsCOMPtr<nsIDOMNode> resultNode;
        IsTextPropertySetByContent(collapsedNode, aProperty, aAttribute, aValue,
                                   isSet, getter_AddRefs(resultNode), outValue);
        *aFirst = *aAny = *aAll = isSet;
        
        if (!isSet && aCheckDefaults) 
        {
          
          
          PRInt32 index;
          if (aAttribute &&
              TypeInState::FindPropInList(aProperty, *aAttribute, outValue, mDefaultStyles, index))
          {
            *aFirst = *aAny = *aAll = PR_TRUE;
            if (outValue)
              outValue->Assign(mDefaultStyles[index]->value);
          }
        }
        return NS_OK;
      }
    }

    
    nsCOMPtr<nsIContentIterator> iter =
            do_CreateInstance("@mozilla.org/content/post-content-iterator;1");
    NS_ENSURE_TRUE(iter, NS_ERROR_NULL_POINTER);

    iter->Init(range);
    nsAutoString firstValue, theValue;

    nsCOMPtr<nsIDOMNode> endNode;
    PRInt32 endOffset;
    result = range->GetEndContainer(getter_AddRefs(endNode));
    NS_ENSURE_SUCCESS(result, result);
    result = range->GetEndOffset(&endOffset);
    NS_ENSURE_SUCCESS(result, result);
    while (!iter->IsDone())
    {
      nsCOMPtr<nsIContent> content = do_QueryInterface(iter->GetCurrentNode());

      nsCOMPtr<nsIDOMNode> node = do_QueryInterface(content);

      if (node && nsTextEditUtils::IsBody(node))
        break;

      nsCOMPtr<nsIDOMCharacterData>text;
      text = do_QueryInterface(content);
      
      bool skipNode = false;
      
      
      if (text && !IsEditable(text))
      {
        skipNode = PR_TRUE;
      }
      else if (text)
      {
        if (!isCollapsed && first && firstNodeInRange)
        {
          firstNodeInRange = PR_FALSE;
          PRInt32 startOffset;
          range->GetStartOffset(&startOffset);
          PRUint32 count;
          text->GetLength(&count);
          if (startOffset==(PRInt32)count) 
          {
            skipNode = PR_TRUE;
          }
        }
        else if (node == endNode && !endOffset)
        {
          skipNode = PR_TRUE;
        }
      }
      else if (content->IsElement())
      { 
        skipNode = PR_TRUE;
      }
      if (!skipNode)
      {
        if (node)
        {
          bool isSet = false;
          nsCOMPtr<nsIDOMNode>resultNode;
          if (first)
          {
            if (useCSS &&
                mHTMLCSSUtils->IsCSSEditableProperty(node, aProperty, aAttribute)) {
              
              
              if (aValue) firstValue.Assign(*aValue);
              mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(node, aProperty, aAttribute,
                                                                 isSet, firstValue,
                                                                 COMPUTED_STYLE_TYPE);
            }
            else {
              IsTextPropertySetByContent(node, aProperty, aAttribute, aValue, isSet,
                                         getter_AddRefs(resultNode), &firstValue);
            }
            *aFirst = isSet;
            first = PR_FALSE;
            if (outValue) *outValue = firstValue;
          }
          else
          {
            if (useCSS &&
                mHTMLCSSUtils->IsCSSEditableProperty(node, aProperty, aAttribute)) {
              
              
              if (aValue) theValue.Assign(*aValue);
              mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(node, aProperty, aAttribute,
                                                                 isSet, theValue,
                                                                 COMPUTED_STYLE_TYPE);
            }
            else {
              IsTextPropertySetByContent(node, aProperty, aAttribute, aValue, isSet,
                                         getter_AddRefs(resultNode), &theValue);
            }
            if (firstValue != theValue)
              *aAll = PR_FALSE;
          }
          
          if (isSet) {
            *aAny = PR_TRUE;
          }
          else {
            *aAll = PR_FALSE;
          }
        }
      }

      iter->Next();
    }
  }
  if (!*aAny) 
  { 
    *aAll = PR_FALSE;
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
  const nsAString *att = nsnull;
  if (!aAttribute.IsEmpty())
    att = &aAttribute;
  const nsAString *val = nsnull;
  if (!aValue.IsEmpty())
    val = &aValue;
  return GetInlinePropertyBase( aProperty, att, val, aFirst, aAny, aAll, nsnull);
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
  const nsAString *att = nsnull;
  if (!aAttribute.IsEmpty())
    att = &aAttribute;
  const nsAString *val = nsnull;
  if (!aValue.IsEmpty())
    val = &aValue;
  return GetInlinePropertyBase( aProperty, att, val, aFirst, aAny, aAll, &outValue);
}


NS_IMETHODIMP nsHTMLEditor::RemoveAllInlineProperties()
{
  nsAutoEditBatch batchIt(this);
  nsAutoRules beginRulesSniffing(this, kOpResetTextProperties, nsIEditor::eNext);

  nsresult res = RemoveInlinePropertyImpl(nsnull, nsnull);
  NS_ENSURE_SUCCESS(res, res);
  return ApplyDefaultProperties();
}

NS_IMETHODIMP nsHTMLEditor::RemoveInlineProperty(nsIAtom *aProperty, const nsAString &aAttribute)
{
  return RemoveInlinePropertyImpl(aProperty, &aAttribute);
}

nsresult nsHTMLEditor::RemoveInlinePropertyImpl(nsIAtom *aProperty, const nsAString *aAttribute)
{
  NS_ENSURE_TRUE(mRules, NS_ERROR_NOT_INITIALIZED);
  ForceCompositionEnd();

  nsresult res;
  nsCOMPtr<nsISelection>selection;
  res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));

  bool isCollapsed;
  selection->GetIsCollapsed(&isCollapsed);

  bool useCSS;
  GetIsCSSEnabled(&useCSS);

  if (isCollapsed)
  {
    

    
    if (aProperty == nsEditProperty::href ||
        aProperty == nsEditProperty::name)
      aProperty = nsEditProperty::a;

    if (aProperty) return mTypeInState->ClearProp(aProperty, nsAutoString(*aAttribute));
    else return mTypeInState->ClearAllProps();
  }
  nsAutoEditBatch batchIt(this);
  nsAutoRules beginRulesSniffing(this, kOpRemoveTextProperty, nsIEditor::eNext);
  nsAutoSelectionReset selectionResetter(selection, this);
  nsAutoTxnsConserveSelection dontSpazMySelection(this);
  
  bool cancel, handled;
  nsTextRulesInfo ruleInfo(nsTextEditRules::kRemoveTextProperty);
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  NS_ENSURE_SUCCESS(res, res);
  if (!cancel && !handled)
  {
    
    nsCOMPtr<nsIEnumerator> enumerator;
    res = selPriv->GetEnumerator(getter_AddRefs(enumerator));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(enumerator, NS_ERROR_FAILURE);

    
    enumerator->First(); 
    nsCOMPtr<nsISupports> currentItem;
    while ((NS_ENUMERATOR_FALSE == enumerator->IsDone()))
    {
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
                                                    aProperty,
                                                    aAttribute,
                                                    isSet ,
                                                    cssValue,
                                                    COMPUTED_STYLE_TYPE);
          if (isSet) {
            
            
            
            
            nsAutoString value; value.AssignLiteral("-moz-editor-invert-value");
            PRInt32 startOffset, endOffset;
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
        
        
        PRInt32 listCount = arrayOfNodes.Count();
        PRInt32 j;
        for (j = 0; j < listCount; j++)
        {
          node = arrayOfNodes[j];
          res = RemoveStyleInside(node, aProperty, aAttribute);
          NS_ENSURE_SUCCESS(res, res);
          if (useCSS && mHTMLCSSUtils->IsCSSEditableProperty(node, aProperty, aAttribute)) {
            
            
            nsAutoString cssValue;
            bool isSet = false;
            mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(node,
                                                               aProperty,
                                                               aAttribute,
                                                               isSet ,
                                                               cssValue,
                                                               COMPUTED_STYLE_TYPE);
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
nsHTMLEditor::RelativeFontChange( PRInt32 aSizeChange)
{
  
  if ( !( (aSizeChange==1) || (aSizeChange==-1) ) )
    return NS_ERROR_ILLEGAL_VALUE;
  
  ForceCompositionEnd();

  
  nsCOMPtr<nsISelection>selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));  
  
  bool bCollapsed;
  res = selection->GetIsCollapsed(&bCollapsed);
  NS_ENSURE_SUCCESS(res, res);
  
  
  if (bCollapsed)
  {
    nsCOMPtr<nsIAtom> atom;
    if (aSizeChange==1) atom = nsEditProperty::big;
    else                atom = nsEditProperty::small;

    
    PRInt32 offset;
    nsCOMPtr<nsIDOMNode> selectedNode;
    res = GetStartNodeAndOffset(selection, getter_AddRefs(selectedNode), &offset);
    if (IsTextNode(selectedNode)) {
      nsCOMPtr<nsIDOMNode> parent;
      res = selectedNode->GetParentNode(getter_AddRefs(parent));
      NS_ENSURE_SUCCESS(res, res);
      selectedNode = parent;
    }
    nsAutoString tag;
    atom->ToString(tag);
    if (!CanContainTag(selectedNode, tag)) return NS_OK;

    
    return mTypeInState->SetProp(atom, EmptyString(), EmptyString());
  }
  
  
  nsAutoEditBatch batchIt(this);
  nsAutoRules beginRulesSniffing(this, kOpSetTextProperty, nsIEditor::eNext);
  nsAutoSelectionReset selectionResetter(selection, this);
  nsAutoTxnsConserveSelection dontSpazMySelection(this);

  
  nsCOMPtr<nsIEnumerator> enumerator;
  res = selPriv->GetEnumerator(getter_AddRefs(enumerator));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_FAILURE);

  
  enumerator->First(); 
  nsCOMPtr<nsISupports> currentItem;
  while ((NS_ENUMERATOR_FALSE == enumerator->IsDone()))
  {
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
      PRInt32 startOffset, endOffset;
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

      nsCOMArray<nsIDOMNode> arrayOfNodes;
      nsCOMPtr<nsIDOMNode> node;
      
      
      res = iter->Init(range);
      if (NS_SUCCEEDED(res))
      {
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
        
        
        PRInt32 listCount = arrayOfNodes.Count();
        PRInt32 j;
        for (j = 0; j < listCount; j++)
        {
          node = arrayOfNodes[j];
          res = RelativeFontChangeOnNode(aSizeChange, node);
          NS_ENSURE_SUCCESS(res, res);
        }
        arrayOfNodes.Clear();
      }
      
      
      
      if (IsTextNode(startNode) && IsEditable(startNode))
      {
        nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(startNode);
        PRInt32 startOffset;
        PRUint32 textLen;
        range->GetStartOffset(&startOffset);
        nodeAsText->GetLength(&textLen);
        res = RelativeFontChangeOnTextNode(aSizeChange, nodeAsText, startOffset, textLen);
        NS_ENSURE_SUCCESS(res, res);
      }
      if (IsTextNode(endNode) && IsEditable(endNode))
      {
        nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(endNode);
        PRInt32 endOffset;
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
nsHTMLEditor::RelativeFontChangeOnTextNode( PRInt32 aSizeChange, 
                                            nsIDOMCharacterData *aTextNode, 
                                            PRInt32 aStartOffset,
                                            PRInt32 aEndOffset)
{
  
  if ( !( (aSizeChange==1) || (aSizeChange==-1) ) )
    return NS_ERROR_ILLEGAL_VALUE;
  NS_ENSURE_TRUE(aTextNode, NS_ERROR_NULL_POINTER);
  
  
  if (aStartOffset == aEndOffset) return NS_OK;
  
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> parent;
  res = aTextNode->GetParentNode(getter_AddRefs(parent));
  NS_ENSURE_SUCCESS(res, res);
  if (!CanContainTag(parent, NS_LITERAL_STRING("big"))) return NS_OK;

  nsCOMPtr<nsIDOMNode> tmp, node = do_QueryInterface(aTextNode);

  
  PRUint32 textLen;
  aTextNode->GetLength(&textLen);
  
  
  if (aEndOffset == -1) aEndOffset = textLen;
  
  if ( (PRUint32)aEndOffset != textLen )
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
  sibling = nsnull;
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
nsHTMLEditor::RelativeFontChangeHelper( PRInt32 aSizeChange, 
                                        nsIDOMNode *aNode)
{
  




  
  
  if ( !( (aSizeChange==1) || (aSizeChange==-1) ) )
    return NS_ERROR_ILLEGAL_VALUE;
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  nsresult res = NS_OK;
  nsAutoString tag;
  if (aSizeChange == 1) tag.AssignLiteral("big");
  else tag.AssignLiteral("small");
  nsCOMPtr<nsIDOMNodeList> childNodes;
  PRInt32 j;
  PRUint32 childCount;
  nsCOMPtr<nsIDOMNode> childNode;
  
  
  NS_NAMED_LITERAL_STRING(attr, "size");
  if (NodeIsType(aNode, nsEditProperty::font) && HasAttr(aNode, &attr))
  {
    
    res = aNode->GetChildNodes(getter_AddRefs(childNodes));
    NS_ENSURE_SUCCESS(res, res);
    if (childNodes)
    {
      childNodes->GetLength(&childCount);
      for (j=childCount-1; j>=0; j--)
      {
        res = childNodes->Item(j, getter_AddRefs(childNode));
        if ((NS_SUCCEEDED(res)) && (childNode))
        {
          res = RelativeFontChangeOnNode(aSizeChange, childNode);
          NS_ENSURE_SUCCESS(res, res);
        }
      }
    }
  }

  childNodes = nsnull;
  
  res = aNode->GetChildNodes(getter_AddRefs(childNodes));
  NS_ENSURE_SUCCESS(res, res);
  if (childNodes)
  {
    childNodes->GetLength(&childCount);
    for (j=childCount-1; j>=0; j--)
    {
      res = childNodes->Item(j, getter_AddRefs(childNode));
      if ((NS_SUCCEEDED(res)) && (childNode))
      {
        res = RelativeFontChangeHelper(aSizeChange, childNode);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
  }

  return res;
}


nsresult
nsHTMLEditor::RelativeFontChangeOnNode( PRInt32 aSizeChange, 
                                        nsIDOMNode *aNode)
{
  
  if ( !( (aSizeChange==1) || (aSizeChange==-1) ) )
    return NS_ERROR_ILLEGAL_VALUE;
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> tmp;
  nsAutoString tag;
  if (aSizeChange == 1) tag.AssignLiteral("big");
  else tag.AssignLiteral("small");
  
  
  if ( ((aSizeChange == 1) && nsHTMLEditUtils::IsSmall(aNode)) || 
       ((aSizeChange == -1) &&  nsHTMLEditUtils::IsBig(aNode)) )
  {
    
    res = RelativeFontChangeHelper(aSizeChange, aNode);
    NS_ENSURE_SUCCESS(res, res);
    
    res = RemoveContainer(aNode);
    return res;
  }
  
  if (TagCanContain(tag, aNode))
  {
    
    res = RelativeFontChangeHelper(aSizeChange, aNode);
    NS_ENSURE_SUCCESS(res, res);
    
    
    
    nsCOMPtr<nsIDOMNode> sibling;
    GetPriorHTMLSibling(aNode, address_of(sibling));
    if (sibling && nsEditor::NodeIsType(sibling, (aSizeChange==1 ? nsEditProperty::big : nsEditProperty::small)))
    {
      
      res = MoveNode(aNode, sibling, -1);
      return res;
    }
    sibling = nsnull;
    GetNextHTMLSibling(aNode, address_of(sibling));
    if (sibling && nsEditor::NodeIsType(sibling, (aSizeChange==1 ? nsEditProperty::big : nsEditProperty::small)))
    {
      
      res = MoveNode(aNode, sibling, 0);
      return res;
    }
    
    res = InsertContainerAbove(aNode, address_of(tmp), tag);
    return res;
  }
  
  
  
  
  nsCOMPtr<nsIDOMNodeList> childNodes;
  res = aNode->GetChildNodes(getter_AddRefs(childNodes));
  NS_ENSURE_SUCCESS(res, res);
  if (childNodes)
  {
    PRInt32 j;
    PRUint32 childCount;
    childNodes->GetLength(&childCount);
    for (j=childCount-1; j>=0; j--)
    {
      nsCOMPtr<nsIDOMNode> childNode;
      res = childNodes->Item(j, getter_AddRefs(childNode));
      if ((NS_SUCCEEDED(res)) && (childNode))
      {
        res = RelativeFontChangeOnNode(aSizeChange, childNode);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::GetFontFaceState(bool *aMixed, nsAString &outFace)
{
  NS_ENSURE_TRUE(aMixed, NS_ERROR_FAILURE);
  *aMixed = PR_TRUE;
  outFace.Truncate();

  nsresult res;
  bool first, any, all;
  
  NS_NAMED_LITERAL_STRING(attr, "face");
  res = GetInlinePropertyBase(nsEditProperty::font, &attr, nsnull, &first, &any, &all, &outFace);
  NS_ENSURE_SUCCESS(res, res);
  if (any && !all) return res; 
  if (all)
  {
    *aMixed = PR_FALSE;
    return res;
  }
  
  
  res = GetInlinePropertyBase(nsEditProperty::tt, nsnull, nsnull, &first, &any, &all,nsnull);
  NS_ENSURE_SUCCESS(res, res);
  if (any && !all) return res; 
  if (all)
  {
    *aMixed = PR_FALSE;
    nsEditProperty::tt->ToString(outFace);
  }
  
  if (!any)
  {
    
    outFace.Truncate();
    *aMixed = PR_FALSE;
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::GetFontColorState(bool *aMixed, nsAString &aOutColor)
{
  NS_ENSURE_TRUE(aMixed, NS_ERROR_NULL_POINTER);
  *aMixed = PR_TRUE;
  aOutColor.Truncate();
  
  nsresult res;
  NS_NAMED_LITERAL_STRING(colorStr, "color");
  bool first, any, all;
  
  res = GetInlinePropertyBase(nsEditProperty::font, &colorStr, nsnull, &first, &any, &all, &aOutColor);
  NS_ENSURE_SUCCESS(res, res);
  if (any && !all) return res; 
  if (all)
  {
    *aMixed = PR_FALSE;
    return res;
  }
  
  if (!any)
  {
    
    aOutColor.Truncate();
    *aMixed = PR_FALSE;
  }
  return res;
}




nsresult
nsHTMLEditor::GetIsCSSEnabled(bool *aIsCSSEnabled)
{
  *aIsCSSEnabled = PR_FALSE;
  if (mCSSAware) {
    
    if (mHTMLCSSUtils) {
      *aIsCSSEnabled = mHTMLCSSUtils->IsCSSPrefChecked();
    }
  }
  return NS_OK;
}

nsresult
nsHTMLEditor::HasStyleOrIdOrClass(nsIDOMElement * aElement, bool *aHasStyleOrIdOrClass)
{
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMNode> node  = do_QueryInterface(aElement);


  
  
  nsAutoString styleVal;
  bool isStyleSet;
  *aHasStyleOrIdOrClass = PR_TRUE;
  nsresult res = GetAttributeValue(aElement,  NS_LITERAL_STRING("style"), styleVal, &isStyleSet);
  NS_ENSURE_SUCCESS(res, res);
  if (!isStyleSet || styleVal.IsEmpty()) {
    res = mHTMLCSSUtils->HasClassOrID(aElement, *aHasStyleOrIdOrClass);
    NS_ENSURE_SUCCESS(res, res);
  }
  return res;
}

nsresult
nsHTMLEditor::RemoveElementIfNoStyleOrIdOrClass(nsIDOMElement * aElement, nsIAtom * aTag)
{
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMNode> node  = do_QueryInterface(aElement);

  
  if (!NodeIsType(node, aTag)) {
    return NS_OK;
  }
  bool hasStyleOrIdOrClass;
  nsresult res = HasStyleOrIdOrClass(aElement, &hasStyleOrIdOrClass);
  if (!hasStyleOrIdOrClass) {
    res = RemoveContainer(node);
  }
  return res;
}
