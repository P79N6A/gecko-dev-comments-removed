




































#include "nsUnicharUtils.h"

#include "nsHTMLEditor.h"
#include "nsHTMLEditRules.h"
#include "nsTextEditUtils.h"
#include "nsHTMLEditUtils.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMAttr.h"
#include "nsIDOMKeyListener.h" 
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseEvent.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsISelectionController.h"
#include "nsICSSLoader.h"
#include "nsICSSStyleSheet.h"
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
    PropItem *item = (PropItem*)mDefaultStyles[index];
    item->value = aValue;
  }
  else
  {
    nsString value(aValue);
    PropItem *propItem = new PropItem(aProperty, attr, value);
    mDefaultStyles.AppendElement((void*)propItem);
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
    PropItem *item = (PropItem*)mDefaultStyles[index];
    if (item) delete item;
    mDefaultStyles.RemoveElementAt(index);
  }
  return NS_OK;
}

NS_IMETHODIMP nsHTMLEditor::RemoveAllDefaultProperties()
{
  PRInt32 j, defcon = mDefaultStyles.Count();
  for (j=0; j<defcon; j++)
  {
    PropItem *item = (PropItem*)mDefaultStyles[j];
    if (item) delete item;
  }
  mDefaultStyles.Clear();
  return NS_OK;
}




NS_IMETHODIMP nsHTMLEditor::SetCSSInlineProperty(nsIAtom *aProperty, 
                            const nsAString & aAttribute, 
                            const nsAString & aValue)
{
  PRBool useCSS;
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
  if (NS_FAILED(res)) return res;
  if (!selection) return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));

  PRBool isCollapsed;
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
  
  PRBool cancel, handled;
  nsTextRulesInfo ruleInfo(nsTextEditRules::kSetTextProperty);
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (NS_FAILED(res)) return res;
  if (!cancel && !handled)
  {
    
    nsCOMPtr<nsIEnumerator> enumerator;
    res = selPriv->GetEnumerator(getter_AddRefs(enumerator));
    if (NS_FAILED(res)) return res;
    if (!enumerator)    return NS_ERROR_FAILURE;

    
    enumerator->First(); 
    nsCOMPtr<nsISupports> currentItem;
    while ((NS_ENUMERATOR_FALSE == enumerator->IsDone()))
    {
      res = enumerator->CurrentItem(getter_AddRefs(currentItem));
      if (NS_FAILED(res)) return res;
      if (!currentItem)   return NS_ERROR_FAILURE;
      
      nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );

      
      res = PromoteInlineRange(range);
      if (NS_FAILED(res)) return res;
      
      
      nsCOMPtr<nsIDOMNode> startNode, endNode;
      res = range->GetStartContainer(getter_AddRefs(startNode));
      if (NS_FAILED(res)) return res;
      res = range->GetEndContainer(getter_AddRefs(endNode));
      if (NS_FAILED(res)) return res;
      if ((startNode == endNode) && IsTextNode(startNode))
      {
        PRInt32 startOffset, endOffset;
        range->GetStartOffset(&startOffset);
        range->GetEndOffset(&endOffset);
        nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(startNode);
        res = SetInlinePropertyOnTextNode(nodeAsText, startOffset, endOffset, aProperty, &aAttribute, &aValue);
        if (NS_FAILED(res)) return res;
      }
      else
      {
        
        
        
        
        
        
        
        
        
        

        nsCOMPtr<nsIContentIterator> iter =
          do_CreateInstance("@mozilla.org/content/subtree-content-iterator;1", &res);
        if (NS_FAILED(res)) return res;
        if (!iter)          return NS_ERROR_FAILURE;

        nsCOMArray<nsIDOMNode> arrayOfNodes;
        nsCOMPtr<nsIDOMNode> node;
        
        
        res = iter->Init(range);
        
        
        
        
        if (NS_SUCCEEDED(res))
        {
          while (!iter->IsDone())
          {
            node = do_QueryInterface(iter->GetCurrentNode());
            if (!node)
              return NS_ERROR_FAILURE;

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
          if (NS_FAILED(res)) return res;
        }
        
        
        PRInt32 listCount = arrayOfNodes.Count();
        PRInt32 j;
        for (j = 0; j < listCount; j++)
        {
          node = arrayOfNodes[j];
          res = SetInlinePropertyOnNode(node, aProperty, &aAttribute, &aValue);
          if (NS_FAILED(res)) return res;
        }
        arrayOfNodes.Clear();
        
        
        
        
        if (IsTextNode(endNode) && IsEditable(endNode))
        {
          nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(endNode);
          PRInt32 endOffset;
          range->GetEndOffset(&endOffset);
          res = SetInlinePropertyOnTextNode(nodeAsText, 0, endOffset, aProperty, &aAttribute, &aValue);
          if (NS_FAILED(res)) return res;
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
  if (!aTextNode) return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMNode> parent;
  nsresult res = aTextNode->GetParentNode(getter_AddRefs(parent));
  if (NS_FAILED(res)) return res;

  nsAutoString tagString;
  aProperty->ToString(tagString);
  if (!CanContainTag(parent, tagString)) return NS_OK;
  
  
  if (aStartOffset == aEndOffset) return NS_OK;
  
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aTextNode);
  
  
  PRBool bHasProp;
  PRBool useCSS;
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
    if (NS_FAILED(res)) return res;
    node = tmp;  
  }
  if ( aStartOffset )
  {
    
    res = SplitNode(node, aStartOffset, getter_AddRefs(tmp));
    if (NS_FAILED(res)) return res;
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
  if (!aNode || !aProperty) return NS_ERROR_NULL_POINTER;

  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> tmp;
  nsAutoString tag;
  aProperty->ToString(tag);
  ToLowerCase(tag);
  
  PRBool useCSS;
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
      if (NS_FAILED(res)) return res;
      PRInt32 count;
      
      res = mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(element, aProperty, aAttribute, aValue, &count, PR_FALSE);
      if (NS_FAILED(res)) return res;

      nsCOMPtr<nsIDOMNode> nextSibling, previousSibling;
      GetNextHTMLSibling(tmp, address_of(nextSibling));
      GetPriorHTMLSibling(tmp, address_of(previousSibling));
      if (nextSibling || previousSibling)
      {
        nsCOMPtr<nsIDOMNode> mergeParent;
        res = tmp->GetParentNode(getter_AddRefs(mergeParent));
        if (NS_FAILED(res)) return res;
        if (previousSibling &&
            nsEditor::NodeIsType(previousSibling, nsEditProperty::span) &&
            NodesSameType(tmp, previousSibling))
        {
          res = JoinNodes(previousSibling, tmp, mergeParent);
          if (NS_FAILED(res)) return res;
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
  
  
  PRBool bHasProp;
  nsCOMPtr<nsIDOMNode> styleNode;
  IsTextPropertySetByContent(aNode, aProperty, aAttribute, aValue, bHasProp, getter_AddRefs(styleNode));
  if (bHasProp) return NS_OK;

  
  if (NodeIsType(aNode, aProperty))
  {
    
    
    res = RemoveStyleInside(aNode, aProperty, aAttribute, PR_TRUE);
    if (NS_FAILED(res)) return res;
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
    if (NS_FAILED(res)) return res;
    return RemoveStyleInside(aNode, aProperty, aAttribute);
  }
  
  nsCOMPtr<nsIDOMNodeList> childNodes;
  res = aNode->GetChildNodes(getter_AddRefs(childNodes));
  if (NS_FAILED(res)) return res;
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
        if (NS_FAILED(res)) return res;
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
  if (!inRange) return NS_ERROR_NULL_POINTER;
  nsresult res;
  nsCOMPtr<nsIDOMNode> startNode, endNode, origStartNode;
  PRInt32 startOffset, endOffset, origStartOffset;
  
  res = inRange->GetStartContainer(getter_AddRefs(startNode));
  if (NS_FAILED(res)) return res;
  res = inRange->GetStartOffset(&startOffset);
  if (NS_FAILED(res)) return res;
  res = inRange->GetEndContainer(getter_AddRefs(endNode));
  if (NS_FAILED(res)) return res;
  res = inRange->GetEndOffset(&endOffset);
  if (NS_FAILED(res)) return res;
  
  origStartNode = startNode;
  origStartOffset = startOffset;
  
  
  {
    nsAutoTrackDOMPoint tracker(mRangeUpdater, address_of(endNode), &endOffset);
    res = SplitStyleAbovePoint(address_of(startNode), &startOffset, aProperty, aAttribute);
    if (NS_FAILED(res)) return res;
  }

  
  res = SplitStyleAbovePoint(address_of(endNode), &endOffset, aProperty, aAttribute);
  if (NS_FAILED(res)) return res;
  
  
  res = inRange->SetStart(startNode, startOffset);
  if (NS_FAILED(res)) return res;
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
  if (!aNode || !*aNode || !aOffset) return NS_ERROR_NULL_POINTER;
  if (outLeftNode)  *outLeftNode  = nsnull;
  if (outRightNode) *outRightNode = nsnull;
  
  nsCOMPtr<nsIDOMNode> parent, tmp = *aNode;
  PRInt32 offset;

  PRBool useCSS;
  GetIsCSSEnabled(&useCSS);

  PRBool isSet;
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
      
      SplitNodeDeep(tmp, *aNode, *aOffset, &offset, PR_FALSE, outLeftNode, outRightNode);
      
      tmp->GetParentNode(getter_AddRefs(*aNode));
      *aOffset = offset;
    }
    tmp->GetParentNode(getter_AddRefs(parent));
    tmp = parent;
  }
  return NS_OK;
}

PRBool nsHTMLEditor::NodeIsProperty(nsIDOMNode *aNode)
{
  if (!aNode)               return PR_FALSE;
  if (!IsContainer(aNode))  return PR_FALSE;
  if (!IsEditable(aNode))   return PR_FALSE;
  if (IsBlockNode(aNode))   return PR_FALSE;
  if (NodeIsType(aNode, nsEditProperty::a)) return PR_FALSE;
  return PR_TRUE;
}

nsresult nsHTMLEditor::ApplyDefaultProperties()
{
  nsresult res = NS_OK;
  PRInt32 j, defcon = mDefaultStyles.Count();
  for (j=0; j<defcon; j++)
  {
    PropItem *propItem = (PropItem*)mDefaultStyles[j];
    if (!propItem) 
      return NS_ERROR_NULL_POINTER;
    res = SetInlineProperty(propItem->tag, propItem->attr, propItem->value);
    if (NS_FAILED(res)) return res;
  }
  return res;
}

nsresult nsHTMLEditor::RemoveStyleInside(nsIDOMNode *aNode, 
                                   nsIAtom *aProperty,   
                                   const nsAString *aAttribute, 
                                   PRBool aChildrenOnly)
{
  if (!aNode) return NS_ERROR_NULL_POINTER;
  if (IsTextNode(aNode)) return NS_OK;
  nsresult res = NS_OK;

  
  nsCOMPtr<nsIDOMNode> child, tmp;
  aNode->GetFirstChild(getter_AddRefs(child));
  while (child)
  {
    
    child->GetNextSibling(getter_AddRefs(tmp));
    res = RemoveStyleInside(child, aProperty, aAttribute);
    if (NS_FAILED(res)) return res;
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
      PRBool hasStyleAttr = HasAttr(aNode, &styleAttr);
      PRBool hasClassAtrr = HasAttr(aNode, &classAttr);
      if (aProperty &&
          (hasStyleAttr || hasClassAtrr)) {
        
        
        
        
        nsCOMPtr<nsIDOMNode> spanNode;
        res = InsertContainerAbove(aNode, address_of(spanNode),
                                   NS_LITERAL_STRING("span"));
        if (NS_FAILED(res))
          return res;
        res = CloneAttribute(styleAttr, spanNode, aNode);
        if (NS_FAILED(res))
          return res;
        res = CloneAttribute(classAttr, spanNode, aNode);
        if (NS_FAILED(res))
          return res;
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
          if (!elem) return NS_ERROR_NULL_POINTER;
          res = RemoveAttribute(elem, *aAttribute);
        }
      }
    }
  }
  else {
    PRBool useCSS;
    GetIsCSSEnabled(&useCSS);

    if (!aChildrenOnly
        && useCSS && mHTMLCSSUtils->IsCSSEditableProperty(aNode, aProperty, aAttribute)) {
      
      
      nsAutoString propertyValue;
      PRBool isSet;
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
       aAttribute->LowerCaseEqualsLiteral("size"))       
  {
    res = RemoveContainer(aNode);  
  }
  return res;
}

PRBool nsHTMLEditor::IsOnlyAttribute(nsIDOMNode *aNode, 
                                     const nsAString *aAttribute)
{
  if (!aNode || !aAttribute) return PR_FALSE;  
  nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
  if (!content) return PR_FALSE;  
  
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

PRBool nsHTMLEditor::HasAttr(nsIDOMNode *aNode, 
                             const nsAString *aAttribute)
{
  if (!aNode) return PR_FALSE;
  if (!aAttribute || aAttribute->IsEmpty()) return PR_TRUE;  
  
  
  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(aNode);
  if (!elem) return PR_FALSE;
  
  
  nsCOMPtr<nsIDOMAttr> attNode;
  nsresult res = elem->GetAttributeNode(*aAttribute, getter_AddRefs(attNode));
  if ((NS_FAILED(res)) || !attNode) return PR_FALSE;
  return PR_TRUE;
}


PRBool nsHTMLEditor::HasAttrVal(nsIDOMNode *aNode, 
                                const nsAString *aAttribute, 
                                const nsAString *aValue)
{
  if (!aNode) return PR_FALSE;
  if (!aAttribute || aAttribute->IsEmpty()) return PR_TRUE;  
  
  
  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(aNode);
  if (!elem) return PR_FALSE;
  
  
  nsCOMPtr<nsIDOMAttr> attNode;
  nsresult res = elem->GetAttributeNode(*aAttribute, getter_AddRefs(attNode));
  if ((NS_FAILED(res)) || !attNode) return PR_FALSE;
  
  
  PRBool isSet;
  attNode->GetSpecified(&isSet);
  
  if (!isSet && (!aValue || aValue->IsEmpty())) return PR_TRUE; 
  
  
  nsAutoString attrVal;
  attNode->GetValue(attrVal);
  
  
  if (attrVal.Equals(*aValue,nsCaseInsensitiveStringComparator())) return PR_TRUE;
  return PR_FALSE;
}

nsresult nsHTMLEditor::PromoteRangeIfStartsOrEndsInNamedAnchor(nsIDOMRange *inRange)
{
  if (!inRange) return NS_ERROR_NULL_POINTER;
  nsresult res;
  nsCOMPtr<nsIDOMNode> startNode, endNode, parent, tmp;
  PRInt32 startOffset, endOffset, tmpOffset;
  
  res = inRange->GetStartContainer(getter_AddRefs(startNode));
  if (NS_FAILED(res)) return res;
  res = inRange->GetStartOffset(&startOffset);
  if (NS_FAILED(res)) return res;
  res = inRange->GetEndContainer(getter_AddRefs(endNode));
  if (NS_FAILED(res)) return res;
  res = inRange->GetEndOffset(&endOffset);
  if (NS_FAILED(res)) return res;

  tmp = startNode;
  while ( tmp && 
          !nsTextEditUtils::IsBody(tmp) &&
          !nsHTMLEditUtils::IsNamedAnchor(tmp))
  {
    res = GetNodeLocation(tmp, address_of(parent), &tmpOffset);
    if (NS_FAILED(res)) return res;
    tmp = parent;
  }
  if (!tmp) return NS_ERROR_NULL_POINTER;
  if (nsHTMLEditUtils::IsNamedAnchor(tmp))
  {
    res = GetNodeLocation(tmp, address_of(parent), &tmpOffset);
    if (NS_FAILED(res)) return res;
    startNode = parent;
    startOffset = tmpOffset;
  }

  tmp = endNode;
  while ( tmp && 
          !nsTextEditUtils::IsBody(tmp) &&
          !nsHTMLEditUtils::IsNamedAnchor(tmp))
  {
    res = GetNodeLocation(tmp, address_of(parent), &tmpOffset);
    if (NS_FAILED(res)) return res;
    tmp = parent;
  }
  if (!tmp) return NS_ERROR_NULL_POINTER;
  if (nsHTMLEditUtils::IsNamedAnchor(tmp))
  {
    res = GetNodeLocation(tmp, address_of(parent), &tmpOffset);
    if (NS_FAILED(res)) return res;
    endNode = parent;
    endOffset = tmpOffset + 1;
  }

  res = inRange->SetStart(startNode, startOffset);
  if (NS_FAILED(res)) return res;
  res = inRange->SetEnd(endNode, endOffset);
  return res;
}

nsresult nsHTMLEditor::PromoteInlineRange(nsIDOMRange *inRange)
{
  if (!inRange) return NS_ERROR_NULL_POINTER;
  nsresult res;
  nsCOMPtr<nsIDOMNode> startNode, endNode, parent;
  PRInt32 startOffset, endOffset;
  
  res = inRange->GetStartContainer(getter_AddRefs(startNode));
  if (NS_FAILED(res)) return res;
  res = inRange->GetStartOffset(&startOffset);
  if (NS_FAILED(res)) return res;
  res = inRange->GetEndContainer(getter_AddRefs(endNode));
  if (NS_FAILED(res)) return res;
  res = inRange->GetEndOffset(&endOffset);
  if (NS_FAILED(res)) return res;
  
  while ( startNode && 
          !nsTextEditUtils::IsBody(startNode) && 
          IsAtFrontOfNode(startNode, startOffset) )
  {
    res = GetNodeLocation(startNode, address_of(parent), &startOffset);
    if (NS_FAILED(res)) return res;
    startNode = parent;
  }
  if (!startNode) return NS_ERROR_NULL_POINTER;
  
  while ( endNode && 
          !nsTextEditUtils::IsBody(endNode) && 
          IsAtEndOfNode(endNode, endOffset) )
  {
    res = GetNodeLocation(endNode, address_of(parent), &endOffset);
    if (NS_FAILED(res)) return res;
    endNode = parent;
    endOffset++;  
  }
  if (!endNode) return NS_ERROR_NULL_POINTER;
  
  res = inRange->SetStart(startNode, startOffset);
  if (NS_FAILED(res)) return res;
  res = inRange->SetEnd(endNode, endOffset);
  return res;
}

PRBool nsHTMLEditor::IsAtFrontOfNode(nsIDOMNode *aNode, PRInt32 aOffset)
{
  if (!aNode) return PR_FALSE;  
  if (!aOffset) return PR_TRUE;
  
  if (IsTextNode(aNode))
  {
    return PR_FALSE;
  }
  else
  {
    nsCOMPtr<nsIDOMNode> firstNode;
    GetFirstEditableChild(aNode, address_of(firstNode));
    if (!firstNode) return PR_TRUE; 
    PRInt32 offset;
    nsEditor::GetChildOffset(firstNode, aNode, offset);
    if (offset < aOffset) return PR_FALSE;
    return PR_TRUE;
  }
}

PRBool nsHTMLEditor::IsAtEndOfNode(nsIDOMNode *aNode, PRInt32 aOffset)
{
  if (!aNode) return PR_FALSE;  
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
    if (!lastNode) return PR_TRUE; 
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
                             PRBool *aFirst, 
                             PRBool *aAny, 
                             PRBool *aAll,
                             nsAString *outValue,
                             PRBool aCheckDefaults)
{
  if (!aProperty)
    return NS_ERROR_NULL_POINTER;

  nsresult result;
  *aAny=PR_FALSE;
  *aAll=PR_TRUE;
  *aFirst=PR_FALSE;
  PRBool first=PR_TRUE;

  PRBool useCSS;
  GetIsCSSEnabled(&useCSS);

  nsCOMPtr<nsISelection>selection;
  result = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(result)) return result;
  if (!selection) return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));

  PRBool isCollapsed;
  selection->GetIsCollapsed(&isCollapsed);
  nsCOMPtr<nsIDOMNode> collapsedNode;
  nsCOMPtr<nsIEnumerator> enumerator;
  result = selPriv->GetEnumerator(getter_AddRefs(enumerator));
  if (NS_FAILED(result)) return result;
  if (!enumerator) return NS_ERROR_NULL_POINTER;

  enumerator->First(); 
  nsCOMPtr<nsISupports> currentItem;
  result = enumerator->CurrentItem(getter_AddRefs(currentItem));
  
  
  if ((NS_SUCCEEDED(result)) && currentItem)
  {
    PRBool firstNodeInRange = PR_TRUE; 
    nsCOMPtr<nsIDOMRange> range(do_QueryInterface(currentItem));

    if (isCollapsed)
    {
      range->GetStartContainer(getter_AddRefs(collapsedNode));
      if (!collapsedNode) return NS_ERROR_FAILURE;
      PRBool isSet, theSetting;
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
          if (TypeInState::FindPropInList(aProperty, *aAttribute, outValue, mDefaultStyles, index))
          {
            *aFirst = *aAny = *aAll = PR_TRUE;
            if (outValue)
              outValue->Assign(((PropItem*)mDefaultStyles[index])->value);
          }
        }
        return NS_OK;
      }
    }

    
    nsCOMPtr<nsIContentIterator> iter =
            do_CreateInstance("@mozilla.org/content/post-content-iterator;1");
    if (!iter) return NS_ERROR_NULL_POINTER;

    iter->Init(range);
    nsAutoString firstValue, theValue;

    nsCOMPtr<nsIDOMNode> endNode;
    PRInt32 endOffset;
    result = range->GetEndContainer(getter_AddRefs(endNode));
    if (NS_FAILED(result)) return result;
    result = range->GetEndOffset(&endOffset);
    if (NS_FAILED(result)) return result;
    while (!iter->IsDone())
    {
      nsIContent *content = iter->GetCurrentNode();

      nsCOMPtr<nsIDOMNode> node = do_QueryInterface(content);

      if (node && nsTextEditUtils::IsBody(node))
        break;

      nsCOMPtr<nsIDOMCharacterData>text;
      text = do_QueryInterface(content);
      
      PRBool skipNode = PR_FALSE;
      
      
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
      else if (content->IsNodeOfType(nsINode::eELEMENT))
      { 
        skipNode = PR_TRUE;
      }
      if (!skipNode)
      {
        if (node)
        {
          PRBool isSet = PR_FALSE;
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
                                              PRBool *aFirst, 
                                              PRBool *aAny, 
                                              PRBool *aAll)
{
  if (!aProperty || !aFirst || !aAny || !aAll)
    return NS_ERROR_NULL_POINTER;
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
                                              PRBool *aFirst, 
                                              PRBool *aAny, 
                                              PRBool *aAll,
                                              nsAString &outValue)
{
  if (!aProperty || !aFirst || !aAny || !aAll)
    return NS_ERROR_NULL_POINTER;
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
  if (NS_FAILED(res)) return res;
  return ApplyDefaultProperties();
}

NS_IMETHODIMP nsHTMLEditor::RemoveInlineProperty(nsIAtom *aProperty, const nsAString &aAttribute)
{
  return RemoveInlinePropertyImpl(aProperty, &aAttribute);
}

nsresult nsHTMLEditor::RemoveInlinePropertyImpl(nsIAtom *aProperty, const nsAString *aAttribute)
{
  if (!mRules)    return NS_ERROR_NOT_INITIALIZED;
  ForceCompositionEnd();

  nsresult res;
  nsCOMPtr<nsISelection>selection;
  res = GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  if (!selection) return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));

  PRBool isCollapsed;
  selection->GetIsCollapsed(&isCollapsed);

  PRBool useCSS;
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
  
  PRBool cancel, handled;
  nsTextRulesInfo ruleInfo(nsTextEditRules::kRemoveTextProperty);
  res = mRules->WillDoAction(selection, &ruleInfo, &cancel, &handled);
  if (NS_FAILED(res)) return res;
  if (!cancel && !handled)
  {
    
    nsCOMPtr<nsIEnumerator> enumerator;
    res = selPriv->GetEnumerator(getter_AddRefs(enumerator));
    if (NS_FAILED(res)) return res;
    if (!enumerator)    return NS_ERROR_FAILURE;

    
    enumerator->First(); 
    nsCOMPtr<nsISupports> currentItem;
    while ((NS_ENUMERATOR_FALSE == enumerator->IsDone()))
    {
      res = enumerator->CurrentItem(getter_AddRefs(currentItem));
      if (NS_FAILED(res)) return res;
      if (!currentItem)   return NS_ERROR_FAILURE;
      
      nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );

      if (aProperty == nsEditProperty::name)
      {
        
        
        res = PromoteRangeIfStartsOrEndsInNamedAnchor(range);
      }
      else {
        
        res = PromoteInlineRange(range);
      }
      if (NS_FAILED(res)) return res;

      
      
      res = SplitStyleAboveRange(range, aProperty, aAttribute);
      if (NS_FAILED(res)) return res;

      
      nsCOMPtr<nsIDOMNode> startNode, endNode;
      res = range->GetStartContainer(getter_AddRefs(startNode));
      if (NS_FAILED(res)) return res;
      res = range->GetEndContainer(getter_AddRefs(endNode));
      if (NS_FAILED(res)) return res;
      if ((startNode == endNode) && IsTextNode(startNode))
      {
        
        if (useCSS && mHTMLCSSUtils->IsCSSEditableProperty(startNode, aProperty, aAttribute)) {
          
          
          nsAutoString cssValue;
          PRBool isSet = PR_FALSE;
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
        if (NS_FAILED(res)) return res;
        if (!iter)          return NS_ERROR_FAILURE;

        nsCOMArray<nsIDOMNode> arrayOfNodes;
        nsCOMPtr<nsIDOMNode> node;
        
        
        iter->Init(range);
        while (!iter->IsDone())
        {
          node = do_QueryInterface(iter->GetCurrentNode());
          if (!node)
            return NS_ERROR_FAILURE;

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
          if (NS_FAILED(res)) return res;
          if (useCSS && mHTMLCSSUtils->IsCSSEditableProperty(node, aProperty, aAttribute)) {
            
            
            nsAutoString cssValue;
            PRBool isSet = PR_FALSE;
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
  if (NS_FAILED(res)) return res;
  if (!selection) return NS_ERROR_FAILURE;
  nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(selection));  
  
  PRBool bCollapsed;
  res = selection->GetIsCollapsed(&bCollapsed);
  if (NS_FAILED(res)) return res;
  
  
  if (bCollapsed)
  {
    nsCOMPtr<nsIAtom> atom;
    if (aSizeChange==1) atom = nsEditProperty::big;
    else                atom = nsEditProperty::small;

    
    PRInt32 offset;
    nsCOMPtr<nsIDOMNode> selectedNode;
    res = GetStartNodeAndOffset(selection, address_of(selectedNode), &offset);
    if (IsTextNode(selectedNode)) {
      nsCOMPtr<nsIDOMNode> parent;
      res = selectedNode->GetParentNode(getter_AddRefs(parent));
      if (NS_FAILED(res)) return res;
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
  if (NS_FAILED(res)) return res;
  if (!enumerator)    return NS_ERROR_FAILURE;

  
  enumerator->First(); 
  nsCOMPtr<nsISupports> currentItem;
  while ((NS_ENUMERATOR_FALSE == enumerator->IsDone()))
  {
    res = enumerator->CurrentItem(getter_AddRefs(currentItem));
    if (NS_FAILED(res)) return res;
    if (!currentItem)   return NS_ERROR_FAILURE;
    
    nsCOMPtr<nsIDOMRange> range( do_QueryInterface(currentItem) );

    
    res = PromoteInlineRange(range);
    if (NS_FAILED(res)) return res;
    
    
    nsCOMPtr<nsIDOMNode> startNode, endNode;
    res = range->GetStartContainer(getter_AddRefs(startNode));
    if (NS_FAILED(res)) return res;
    res = range->GetEndContainer(getter_AddRefs(endNode));
    if (NS_FAILED(res)) return res;
    if ((startNode == endNode) && IsTextNode(startNode))
    {
      PRInt32 startOffset, endOffset;
      range->GetStartOffset(&startOffset);
      range->GetEndOffset(&endOffset);
      nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(startNode);
      res = RelativeFontChangeOnTextNode(aSizeChange, nodeAsText, startOffset, endOffset);
      if (NS_FAILED(res)) return res;
    }
    else
    {
      
      
      
      
      
      
      
      
      
      

      nsCOMPtr<nsIContentIterator> iter =
        do_CreateInstance("@mozilla.org/content/subtree-content-iterator;1", &res);
      if (NS_FAILED(res)) return res;
      if (!iter)          return NS_ERROR_FAILURE;

      nsCOMArray<nsIDOMNode> arrayOfNodes;
      nsCOMPtr<nsIDOMNode> node;
      
      
      res = iter->Init(range);
      if (NS_SUCCEEDED(res))
      {
        while (!iter->IsDone())
        {
          node = do_QueryInterface(iter->GetCurrentNode());
          if (!node)
            return NS_ERROR_FAILURE;

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
          if (NS_FAILED(res)) return res;
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
        if (NS_FAILED(res)) return res;
      }
      if (IsTextNode(endNode) && IsEditable(endNode))
      {
        nsCOMPtr<nsIDOMCharacterData> nodeAsText = do_QueryInterface(endNode);
        PRInt32 endOffset;
        range->GetEndOffset(&endOffset);
        res = RelativeFontChangeOnTextNode(aSizeChange, nodeAsText, 0, endOffset);
        if (NS_FAILED(res)) return res;
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
  if (!aTextNode) return NS_ERROR_NULL_POINTER;
  
  
  if (aStartOffset == aEndOffset) return NS_OK;
  
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> parent;
  res = aTextNode->GetParentNode(getter_AddRefs(parent));
  if (NS_FAILED(res)) return res;
  if (!CanContainTag(parent, NS_LITERAL_STRING("big"))) return NS_OK;

  nsCOMPtr<nsIDOMNode> tmp, node = do_QueryInterface(aTextNode);

  
  PRUint32 textLen;
  aTextNode->GetLength(&textLen);
  
  
  if (aEndOffset == -1) aEndOffset = textLen;
  
  if ( (PRUint32)aEndOffset != textLen )
  {
    
    res = SplitNode(node, aEndOffset, getter_AddRefs(tmp));
    if (NS_FAILED(res)) return res;
    node = tmp;  
  }
  if ( aStartOffset )
  {
    
    res = SplitNode(node, aStartOffset, getter_AddRefs(tmp));
    if (NS_FAILED(res)) return res;
  }

  NS_NAMED_LITERAL_STRING(bigSize, "big");
  NS_NAMED_LITERAL_STRING(smallSize, "small");
  const nsAString& nodeType = (aSizeChange==1) ? NS_STATIC_CAST(const nsAString&, bigSize) : NS_STATIC_CAST(const nsAString&, smallSize);
  
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
  if (!aNode) return NS_ERROR_NULL_POINTER;

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
    if (NS_FAILED(res)) return res;
    if (childNodes)
    {
      childNodes->GetLength(&childCount);
      for (j=childCount-1; j>=0; j--)
      {
        res = childNodes->Item(j, getter_AddRefs(childNode));
        if ((NS_SUCCEEDED(res)) && (childNode))
        {
          res = RelativeFontChangeOnNode(aSizeChange, childNode);
          if (NS_FAILED(res)) return res;
        }
      }
    }
  }

  childNodes = nsnull;
  
  res = aNode->GetChildNodes(getter_AddRefs(childNodes));
  if (NS_FAILED(res)) return res;
  if (childNodes)
  {
    childNodes->GetLength(&childCount);
    for (j=childCount-1; j>=0; j--)
    {
      res = childNodes->Item(j, getter_AddRefs(childNode));
      if ((NS_SUCCEEDED(res)) && (childNode))
      {
        res = RelativeFontChangeHelper(aSizeChange, childNode);
        if (NS_FAILED(res)) return res;
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
  if (!aNode) return NS_ERROR_NULL_POINTER;

  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> tmp;
  nsAutoString tag;
  if (aSizeChange == 1) tag.AssignLiteral("big");
  else tag.AssignLiteral("small");
  
  
  if ( ((aSizeChange == 1) && nsHTMLEditUtils::IsSmall(aNode)) || 
       ((aSizeChange == -1) &&  nsHTMLEditUtils::IsBig(aNode)) )
  {
    
    res = RelativeFontChangeHelper(aSizeChange, aNode);
    if (NS_FAILED(res)) return res;
    
    res = RemoveContainer(aNode);
    return res;
  }
  
  if (TagCanContain(tag, aNode))
  {
    
    res = RelativeFontChangeHelper(aSizeChange, aNode);
    if (NS_FAILED(res)) return res;
    
    
    
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
  if (NS_FAILED(res)) return res;
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
        if (NS_FAILED(res)) return res;
      }
    }
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::GetFontFaceState(PRBool *aMixed, nsAString &outFace)
{
  if (!aMixed)
      return NS_ERROR_FAILURE;
  *aMixed = PR_TRUE;
  outFace.Truncate();

  nsresult res;
  PRBool first, any, all;
  
  NS_NAMED_LITERAL_STRING(attr, "face");
  res = GetInlinePropertyBase(nsEditProperty::font, &attr, nsnull, &first, &any, &all, &outFace);
  if (NS_FAILED(res)) return res;
  if (any && !all) return res; 
  if (all)
  {
    *aMixed = PR_FALSE;
    return res;
  }
  
  
  res = GetInlinePropertyBase(nsEditProperty::tt, nsnull, nsnull, &first, &any, &all,nsnull);
  if (NS_FAILED(res)) return res;
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
nsHTMLEditor::GetFontColorState(PRBool *aMixed, nsAString &aOutColor)
{
  if (!aMixed)
      return NS_ERROR_NULL_POINTER;
  *aMixed = PR_TRUE;
  aOutColor.Truncate();
  
  nsresult res;
  NS_NAMED_LITERAL_STRING(colorStr, "color");
  PRBool first, any, all;
  
  res = GetInlinePropertyBase(nsEditProperty::font, &colorStr, nsnull, &first, &any, &all, &aOutColor);
  if (NS_FAILED(res)) return res;
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
nsHTMLEditor::GetIsCSSEnabled(PRBool *aIsCSSEnabled)
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
nsHTMLEditor::HasStyleOrIdOrClass(nsIDOMElement * aElement, PRBool *aHasStyleOrIdOrClass)
{
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMNode> node  = do_QueryInterface(aElement);


  
  
  nsAutoString styleVal;
  PRBool isStyleSet;
  *aHasStyleOrIdOrClass = PR_TRUE;
  nsresult res = GetAttributeValue(aElement,  NS_LITERAL_STRING("style"), styleVal, &isStyleSet);
  if (NS_FAILED(res)) return res;
  if (!isStyleSet || styleVal.IsEmpty()) {
    res = mHTMLCSSUtils->HasClassOrID(aElement, *aHasStyleOrIdOrClass);
    if (NS_FAILED(res)) return res;
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
  PRBool hasStyleOrIdOrClass;
  nsresult res = HasStyleOrIdOrClass(aElement, &hasStyleOrIdOrClass);
  if (!hasStyleOrIdOrClass) {
    res = RemoveContainer(node);
  }
  return res;
}
