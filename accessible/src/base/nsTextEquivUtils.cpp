






































#include "nsTextEquivUtils.h"

#include "AccIterator.h"
#include "nsAccessibilityService.h"
#include "nsAccessible.h"
#include "nsAccUtils.h"

#include "nsIDOMXULLabeledControlEl.h"

#include "nsArrayUtils.h"

#define NS_OK_NO_NAME_CLAUSE_HANDLED \
NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 0x24)




nsresult
nsTextEquivUtils::GetNameFromSubtree(nsAccessible *aAccessible,
                                     nsAString& aName)
{
  aName.Truncate();

  if (gInitiatorAcc)
    return NS_OK;

  gInitiatorAcc = aAccessible;

  PRUint32 nameRule = gRoleToNameRulesMap[aAccessible->Role()];
  if (nameRule == eFromSubtree) {
    
    if (aAccessible->IsContent()) {
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
nsTextEquivUtils::GetTextEquivFromIDRefs(nsAccessible *aAccessible,
                                         nsIAtom *aIDRefsAttr,
                                         nsAString& aTextEquiv)
{
  aTextEquiv.Truncate();

  nsIContent* content = aAccessible->GetContent();
  if (!content)
    return NS_OK;

  nsIContent* refContent = nsnull;
  IDRefsIterator iter(content, aIDRefsAttr);
  while ((refContent = iter.NextElem())) {
    if (!aTextEquiv.IsEmpty())
      aTextEquiv += ' ';

    nsresult rv = AppendTextEquivFromContent(aAccessible, refContent,
                                             &aTextEquiv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
nsTextEquivUtils::AppendTextEquivFromContent(nsAccessible *aInitiatorAcc,
                                             nsIContent *aContent,
                                             nsAString *aString)
{
  
  if (gInitiatorAcc)
    return NS_OK;

  gInitiatorAcc = aInitiatorAcc;

  nsIPresShell* shell = nsCoreUtils::GetPresShellFor(aContent);
  if (!shell) {
    NS_ASSERTION(true, "There is no presshell!");
    gInitiatorAcc = nsnull;
    return NS_ERROR_UNEXPECTED;
  }

  
  
  
  nsIFrame *frame = aContent->GetPrimaryFrame();
  bool isVisible = frame && frame->GetStyleVisibility()->IsVisible();

  nsresult rv = NS_ERROR_FAILURE;
  bool goThroughDOMSubtree = true;

  if (isVisible) {
    nsAccessible* accessible =
      GetAccService()->GetAccessible(aContent, shell);
    if (accessible) {
      rv = AppendFromAccessible(accessible, aString);
      goThroughDOMSubtree = false;
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
    bool isHTMLBlock = false;

    nsIContent *parentContent = aContent->GetParent();
    if (parentContent) {
      nsIFrame *frame = parentContent->GetPrimaryFrame();
      if (frame) {
        
        
        
        const nsStyleDisplay* display = frame->GetStyleDisplay();
        if (display->IsBlockOutside() ||
            display->mDisplay == NS_STYLE_DISPLAY_TABLE_CELL) {
          isHTMLBlock = true;
          if (!aString->IsEmpty()) {
            aString->Append(PRUnichar(' '));
          }
        }
      }
    }
    
    if (aContent->TextLength() > 0) {
      nsIFrame *frame = aContent->GetPrimaryFrame();
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
      aContent->NodeInfo()->Equals(nsGkAtoms::br)) {
    aString->AppendLiteral("\r\n");
    return NS_OK;
  }
  
  return NS_OK_NO_NAME_CLAUSE_HANDLED;
}




nsRefPtr<nsAccessible> nsTextEquivUtils::gInitiatorAcc;

nsresult
nsTextEquivUtils::AppendFromAccessibleChildren(nsAccessible *aAccessible,
                                               nsAString *aString)
{
  nsresult rv = NS_OK_NO_NAME_CLAUSE_HANDLED;

  PRInt32 childCount = aAccessible->GetChildCount();
  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsAccessible *child = aAccessible->GetChildAt(childIdx);
    rv = AppendFromAccessible(child, aString);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}

nsresult
nsTextEquivUtils::AppendFromAccessible(nsAccessible *aAccessible,
                                       nsAString *aString)
{
  
  if (aAccessible->IsContent()) {
    nsresult rv = AppendTextEquivFromTextContent(aAccessible->GetContent(),
                                                 aString);
    if (rv != NS_OK_NO_NAME_CLAUSE_HANDLED)
      return rv;
  }

  nsAutoString text;
  nsresult rv = aAccessible->GetName(text);
  NS_ENSURE_SUCCESS(rv, rv);

  bool isEmptyTextEquiv = true;

  
  
  if (rv != NS_OK_NAME_FROM_TOOLTIP)
    isEmptyTextEquiv = !AppendString(aString, text);

  
  rv = AppendFromValue(aAccessible, aString);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rv != NS_OK_NO_NAME_CLAUSE_HANDLED)
    isEmptyTextEquiv = false;

  
  
  
  if (isEmptyTextEquiv) {
    PRUint32 nameRule = gRoleToNameRulesMap[aAccessible->Role()];
    if (nameRule & eFromSubtreeIfRec) {
      rv = AppendFromAccessibleChildren(aAccessible, aString);
      NS_ENSURE_SUCCESS(rv, rv);

      if (rv != NS_OK_NO_NAME_CLAUSE_HANDLED)
        isEmptyTextEquiv = false;
    }
  }

  
  if (isEmptyTextEquiv && !text.IsEmpty()) {
    AppendString(aString, text);
    return NS_OK;
  }

  return rv;
}

nsresult
nsTextEquivUtils::AppendFromValue(nsAccessible *aAccessible,
                                  nsAString *aString)
{
  PRUint32 nameRule = gRoleToNameRulesMap[aAccessible->Role()];
  if (nameRule != eFromValue)
    return NS_OK_NO_NAME_CLAUSE_HANDLED;

  
  
  
  

  nsAutoString text;
  if (aAccessible != gInitiatorAcc) {
    nsresult rv = aAccessible->GetValue(text);
    NS_ENSURE_SUCCESS(rv, rv);

    return AppendString(aString, text) ?
      NS_OK : NS_OK_NO_NAME_CLAUSE_HANDLED;
  }

  
  if (aAccessible->IsDocumentNode())
    return NS_ERROR_UNEXPECTED;

  nsIContent *content = aAccessible->GetContent();

  for (nsIContent* childContent = content->GetPreviousSibling(); childContent;
       childContent = childContent->GetPreviousSibling()) {
    
    if (!childContent->TextIsOnlyWhitespace()) {
      for (nsIContent* siblingContent = content->GetNextSibling(); siblingContent;
           siblingContent = siblingContent->GetNextSibling()) {
        
        if (!siblingContent->TextIsOnlyWhitespace()) {
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
  for (nsIContent* childContent = aContent->GetFirstChild(); childContent;
       childContent = childContent->GetNextSibling()) {
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
      if (aContent->NodeInfo()->Equals(nsGkAtoms::label,
                                       kNameSpaceID_XUL))
        aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value,
                          textEquivalent);

      if (textEquivalent.IsEmpty())
        aContent->GetAttr(kNameSpaceID_None,
                          nsGkAtoms::tooltiptext, textEquivalent);
    }

    AppendString(aString, textEquivalent);
  }

  return AppendFromDOMChildren(aContent, aString);
}

bool
nsTextEquivUtils::AppendString(nsAString *aString,
                               const nsAString& aTextEquivalent)
{
  
  if (aTextEquivalent.IsEmpty())
    return false;

  if (!aString->IsEmpty())
    aString->Append(PRUnichar(' '));

  aString->Append(aTextEquivalent);
  return true;
}

bool
nsTextEquivUtils::IsWhitespaceString(const nsSubstring& aString)
{
  nsSubstring::const_char_iterator iterBegin, iterEnd;

  aString.BeginReading(iterBegin);
  aString.EndReading(iterEnd);

  while (iterBegin != iterEnd && IsWhitespace(*iterBegin))
    ++iterBegin;

  return iterBegin == iterEnd;
}

bool
nsTextEquivUtils::IsWhitespace(PRUnichar aChar)
{
  return aChar == ' ' || aChar == '\n' ||
    aChar == '\r' || aChar == '\t' || aChar == 0xa0;
}




PRUint32 nsTextEquivUtils::gRoleToNameRulesMap[] =
{
  eFromSubtreeIfRec, 
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
  eFromSubtreeIfRec, 
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
