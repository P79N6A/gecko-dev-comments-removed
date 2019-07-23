






































#include "nsTextEquivUtils.h"

#include "nsAccessible.h"

#include "nsIDOMXULLabeledControlEl.h"

#include "nsArrayUtils.h"

#define NS_OK_NO_NAME_CLAUSE_HANDLED \
NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 0x24)




nsresult
nsTextEquivUtils::GetNameFromSubtree(nsIAccessible *aAccessible,
                                     nsAString& aName)
{
  aName.Truncate();

  if (gInitiatorAcc)
    return NS_OK;

  gInitiatorAcc = aAccessible;

  PRUint32 role = nsAccUtils::Role(aAccessible);
  PRUint32 nameRule = gRoleToNameRulesMap[role];

  if (nameRule == eFromSubtree) {
    nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(aAccessible));

    nsCOMPtr<nsIDOMNode> DOMNode;
    accessNode->GetDOMNode(getter_AddRefs(DOMNode));
    nsCOMPtr<nsIContent> content(do_QueryInterface(DOMNode));
    if (content) {
      nsAutoString name;
      AppendFromAccessibleChildren(aAccessible, &name);
      name.CompressWhitespace();
      if (!IsWhitespaceString(name))
        aName = name;
    }
  }

  gInitiatorAcc = nsnull;

  return NS_OK;
}

nsresult
nsTextEquivUtils::GetTextEquivFromIDRefs(nsIAccessible *aAccessible,
                                         nsIAtom *aIDRefsAttr,
                                         nsAString& aTextEquiv)
{
  aTextEquiv.Truncate();

  nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(aAccessible));

  nsCOMPtr<nsIDOMNode> DOMNode;
  accessNode->GetDOMNode(getter_AddRefs(DOMNode));

  nsCOMPtr<nsIContent> content = nsCoreUtils::GetRoleContent(DOMNode);
  if (!content)
    return NS_OK;

  nsCOMPtr<nsIArray> refElms;
  nsCoreUtils::GetElementsByIDRefsAttr(content, aIDRefsAttr,
                                       getter_AddRefs(refElms));

  if (!refElms)
    return NS_OK;

  PRUint32 count = 0;
  nsresult rv = refElms->GetLength(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> refContent;
  for (PRUint32 idx = 0; idx < count; idx++) {
    refContent = do_QueryElementAt(refElms, idx, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!aTextEquiv.IsEmpty())
      aTextEquiv += ' ';

    rv = AppendTextEquivFromContent(aAccessible, refContent, &aTextEquiv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
nsTextEquivUtils::AppendTextEquivFromContent(nsIAccessible *aInitiatorAcc,
                                             nsIContent *aContent,
                                             nsAString *aString)
{
  
  if (gInitiatorAcc)
    return NS_OK;

  gInitiatorAcc = aInitiatorAcc;

  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(aContent));
  nsCOMPtr<nsIPresShell> shell = nsCoreUtils::GetPresShellFor(DOMNode);
  if (!shell) {
    NS_ASSERTION(PR_TRUE, "There is no presshell!");
    gInitiatorAcc = nsnull;
    return NS_ERROR_UNEXPECTED;
  }

  
  
  
  nsIFrame *frame = shell->GetPrimaryFrameFor(aContent);
  PRBool isVisible = frame && frame->GetStyleVisibility()->IsVisible();

  nsresult rv;
  PRBool goThroughDOMSubtree = PR_TRUE;

  if (isVisible) {
    nsCOMPtr<nsIAccessible> accessible;
    rv = nsAccessNode::GetAccService()->
      GetAccessibleInShell(DOMNode, shell, getter_AddRefs(accessible));
    if (NS_SUCCEEDED(rv) && accessible) {
      rv = AppendFromAccessible(accessible, aString);
      goThroughDOMSubtree = PR_FALSE;
    }
  }

  if (goThroughDOMSubtree)
    rv = AppendFromDOMNode(aContent, aString);

  gInitiatorAcc = nsnull;
  return rv;
}

nsresult
nsTextEquivUtils::AppendTextEquivFromTextContent(nsIContent *aContent,
                                                 nsAString *aString)
{
  if (aContent->IsNodeOfType(nsINode::eTEXT)) {
    
    nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(aContent));
    
    PRBool isHTMLBlock = PR_FALSE;
    nsCOMPtr<nsIPresShell> shell = nsCoreUtils::GetPresShellFor(DOMNode);
    NS_ENSURE_STATE(shell);
    
    nsIContent *parentContent = aContent->GetParent();
    if (parentContent) {
      nsIFrame *frame = shell->GetPrimaryFrameFor(parentContent);
      if (frame) {
        
        
        
        const nsStyleDisplay* display = frame->GetStyleDisplay();
        if (display->IsBlockOutside() ||
            display->mDisplay == NS_STYLE_DISPLAY_TABLE_CELL) {
          isHTMLBlock = PR_TRUE;
          if (!aString->IsEmpty()) {
            aString->Append(PRUnichar(' '));
          }
        }
      }
    }
    
    if (aContent->TextLength() > 0) {
      nsIFrame *frame = shell->GetPrimaryFrameFor(aContent);
      if (frame) {
        nsresult rv = frame->GetRenderedText(aString);
        NS_ENSURE_SUCCESS(rv, rv);
      } else {
        
        aContent->AppendTextTo(*aString);
      }
      if (isHTMLBlock && !aString->IsEmpty()) {
        aString->Append(PRUnichar(' '));
      }
    }
    
    return NS_OK;
  }
  
  if (aContent->IsHTML() &&
      aContent->NodeInfo()->Equals(nsAccessibilityAtoms::br)) {
    aString->AppendLiteral("\r\n");
    return NS_OK;
  }
  
  return NS_OK_NO_NAME_CLAUSE_HANDLED;
}




nsCOMPtr<nsIAccessible> nsTextEquivUtils::gInitiatorAcc;

nsresult
nsTextEquivUtils::AppendFromAccessibleChildren(nsIAccessible *aAccessible,
                                               nsAString *aString)
{
  nsCOMPtr<nsIAccessible> accChild, accNextChild;
  aAccessible->GetFirstChild(getter_AddRefs(accChild));

  nsresult rv = NS_OK_NO_NAME_CLAUSE_HANDLED;
  while (accChild) {
    rv = AppendFromAccessible(accChild, aString);
    NS_ENSURE_SUCCESS(rv, rv);

    accChild->GetNextSibling(getter_AddRefs(accNextChild));
    accChild.swap(accNextChild);
  }

  return rv;
}

nsresult
nsTextEquivUtils::AppendFromAccessible(nsIAccessible *aAccessible,
                                       nsAString *aString)
{
  nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(aAccessible));

  nsCOMPtr<nsIDOMNode> DOMNode;
  accessNode->GetDOMNode(getter_AddRefs(DOMNode));
  nsCOMPtr<nsIContent> content(do_QueryInterface(DOMNode));
  NS_ASSERTION(content, "There is no content!");

  if (content) {
    nsresult rv = AppendTextEquivFromTextContent(content, aString);
    if (rv != NS_OK_NO_NAME_CLAUSE_HANDLED)
      return rv;
  }

  nsAutoString text;
  nsresult rv = aAccessible->GetName(text);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isEmptyTextEquiv = PR_TRUE;

  
  
  if (rv != NS_OK_NAME_FROM_TOOLTIP)
    isEmptyTextEquiv = !AppendString(aString, text);

  
  rv = AppendFromValue(aAccessible, aString);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rv != NS_OK_NO_NAME_CLAUSE_HANDLED)
    isEmptyTextEquiv = PR_FALSE;

  
  
  
  if (isEmptyTextEquiv) {
    PRUint32 role = nsAccUtils::Role(aAccessible);
    PRUint32 nameRule = gRoleToNameRulesMap[role];

    if (nameRule & eFromSubtreeIfRec) {
      rv = AppendFromAccessibleChildren(aAccessible, aString);
      NS_ENSURE_SUCCESS(rv, rv);

      if (rv != NS_OK_NO_NAME_CLAUSE_HANDLED)
        isEmptyTextEquiv = PR_FALSE;
    }
  }

  
  if (isEmptyTextEquiv && !text.IsEmpty()) {
    AppendString(aString, text);
    return NS_OK;
  }

  return rv;
}

nsresult
nsTextEquivUtils::AppendFromValue(nsIAccessible *aAccessible,
                                  nsAString *aString)
{
  PRUint32 role = nsAccUtils::Role(aAccessible);
  PRUint32 nameRule = gRoleToNameRulesMap[role];

  if (nameRule != eFromValue)
    return NS_OK_NO_NAME_CLAUSE_HANDLED;

  
  
  
  

  nsAutoString text;
  if (aAccessible != gInitiatorAcc) {
    nsresult rv = aAccessible->GetValue(text);
    NS_ENSURE_SUCCESS(rv, rv);

    return AppendString(aString, text) ?
      NS_OK : NS_OK_NO_NAME_CLAUSE_HANDLED;
  }

  nsRefPtr<nsAccessible> acc = nsAccUtils::QueryAccessible(aAccessible);
  nsCOMPtr<nsIDOMNode> node;
  acc->GetDOMNode(getter_AddRefs(node));
  NS_ENSURE_STATE(node);

  nsCOMPtr<nsIContent> content(do_QueryInterface(node));
  NS_ENSURE_STATE(content);

  nsCOMPtr<nsIContent> parent = content->GetParent();
  PRInt32 indexOf = parent->IndexOf(content);

  for (PRInt32 i = indexOf - 1; i >= 0; i--) {
    
    if (!parent->GetChildAt(i)->TextIsOnlyWhitespace()) {
      PRUint32 childCount = parent->GetChildCount();
      for (PRUint32 j = indexOf + 1; j < childCount; j++) {
        
        if (!parent->GetChildAt(j)->TextIsOnlyWhitespace()) {
          nsresult rv = aAccessible->GetValue(text);
          NS_ENSURE_SUCCESS(rv, rv);

          return AppendString(aString, text) ?
            NS_OK : NS_OK_NO_NAME_CLAUSE_HANDLED;
          break;
        }
      }
      break;
    }
  }

  return NS_OK_NO_NAME_CLAUSE_HANDLED;
}

nsresult
nsTextEquivUtils::AppendFromDOMChildren(nsIContent *aContent,
                                        nsAString *aString)
{
  PRUint32 childCount = aContent->GetChildCount();
  for (PRUint32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsCOMPtr<nsIContent> childContent = aContent->GetChildAt(childIdx);

    nsresult rv = AppendFromDOMNode(childContent, aString);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
nsTextEquivUtils::AppendFromDOMNode(nsIContent *aContent, nsAString *aString)
{
  nsresult rv = AppendTextEquivFromTextContent(aContent, aString);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rv != NS_OK_NO_NAME_CLAUSE_HANDLED)
    return NS_OK;

  if (aContent->IsXUL()) {
    nsAutoString textEquivalent;
    nsCOMPtr<nsIDOMXULLabeledControlElement> labeledEl =
      do_QueryInterface(aContent);

    if (labeledEl) {
      labeledEl->GetLabel(textEquivalent);
    } else {
      if (aContent->NodeInfo()->Equals(nsAccessibilityAtoms::label,
                                       kNameSpaceID_XUL))
        aContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::value,
                          textEquivalent);

      if (textEquivalent.IsEmpty())
        aContent->GetAttr(kNameSpaceID_None,
                          nsAccessibilityAtoms::tooltiptext, textEquivalent);
    }

    AppendString(aString, textEquivalent);
  }

  return AppendFromDOMChildren(aContent, aString);
}

PRBool
nsTextEquivUtils::AppendString(nsAString *aString,
                               const nsAString& aTextEquivalent)
{
  
  if (aTextEquivalent.IsEmpty())
    return PR_FALSE;

  if (!aString->IsEmpty())
    aString->Append(PRUnichar(' '));

  aString->Append(aTextEquivalent);
  return PR_TRUE;
}

PRBool
nsTextEquivUtils::IsWhitespaceString(const nsSubstring& aString)
{
  nsSubstring::const_char_iterator iterBegin, iterEnd;

  aString.BeginReading(iterBegin);
  aString.EndReading(iterEnd);

  while (iterBegin != iterEnd && IsWhitespace(*iterBegin))
    ++iterBegin;

  return iterBegin == iterEnd;
}

PRBool
nsTextEquivUtils::IsWhitespace(PRUnichar aChar)
{
  return aChar == ' ' || aChar == '\n' ||
    aChar == '\r' || aChar == '\t' || aChar == 0xa0;
}




PRUint32 nsTextEquivUtils::gRoleToNameRulesMap[] =
{
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eFromSubtree,      
  eFromSubtree,      
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eFromSubtree,      
  eFromSubtree,      
  eFromSubtree,      
  eFromSubtree,      
  eFromSubtreeIfRec, 
  eFromSubtree,      
  eFromSubtree,      
  eNoRule,           
  eFromSubtreeIfRec, 
  eFromSubtree,      
  eNoRule,           
  eFromSubtree,      
  eFromSubtree,      
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eFromSubtree,      
  eFromSubtree,      
  eFromSubtree,      
  eFromValue,        
  eNoRule,           
  eFromValue,        
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eFromSubtree,      
  eFromSubtree,      
  eFromSubtree,      
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eFromSubtree,      
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eFromSubtreeIfRec, 
  eNoRule,           
  eFromSubtree,      
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eFromSubtree,      
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eFromSubtree,      
  eFromSubtree,      
  eFromSubtree,      
  eNoRule,           
  eFromSubtreeIfRec, 
  eFromSubtree,      
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eFromSubtreeIfRec, 
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eFromValue,        
  eNoRule,           
  eNoRule,           
  eFromSubtreeIfRec, 
  eNoRule,           
  eFromSubtreeIfRec, 
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eNoRule,           
  eFromSubtree,      
  eNoRule,           
  eNoRule,           
  eFromSubtree,      
  eNoRule,           
  eFromSubtree,      
  eFromSubtree,      
  eNoRule,           
  eNoRule,           
  eFromSubtree       
};
