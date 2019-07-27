






#include "nsTextEquivUtils.h"

#include "Accessible-inl.h"
#include "AccIterator.h"
#include "nsCoreUtils.h"
#include "nsIDOMXULLabeledControlEl.h"

using namespace mozilla::a11y;






static Accessible* sInitiatorAcc = nullptr;




nsresult
nsTextEquivUtils::GetNameFromSubtree(Accessible* aAccessible,
                                     nsAString& aName)
{
  aName.Truncate();

  if (sInitiatorAcc)
    return NS_OK;

  sInitiatorAcc = aAccessible;
  if (GetRoleRule(aAccessible->Role()) == eNameFromSubtreeRule) {
    
    if (aAccessible->IsContent()) {
      nsAutoString name;
      AppendFromAccessibleChildren(aAccessible, &name);
      name.CompressWhitespace();
      if (!nsCoreUtils::IsWhitespaceString(name))
        aName = name;
    }
  }

  sInitiatorAcc = nullptr;

  return NS_OK;
}

nsresult
nsTextEquivUtils::GetTextEquivFromIDRefs(Accessible* aAccessible,
                                         nsIAtom *aIDRefsAttr,
                                         nsAString& aTextEquiv)
{
  aTextEquiv.Truncate();

  nsIContent* content = aAccessible->GetContent();
  if (!content)
    return NS_OK;

  nsIContent* refContent = nullptr;
  IDRefsIterator iter(aAccessible->Document(), content, aIDRefsAttr);
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
nsTextEquivUtils::AppendTextEquivFromContent(Accessible* aInitiatorAcc,
                                             nsIContent *aContent,
                                             nsAString *aString)
{
  
  if (sInitiatorAcc)
    return NS_OK;

  sInitiatorAcc = aInitiatorAcc;

  
  
  
  nsIFrame *frame = aContent->GetPrimaryFrame();
  bool isVisible = frame && frame->StyleVisibility()->IsVisible();

  nsresult rv = NS_ERROR_FAILURE;
  bool goThroughDOMSubtree = true;

  if (isVisible) {
    Accessible* accessible =
      sInitiatorAcc->Document()->GetAccessible(aContent);
    if (accessible) {
      rv = AppendFromAccessible(accessible, aString);
      goThroughDOMSubtree = false;
    }
  }

  if (goThroughDOMSubtree)
    rv = AppendFromDOMNode(aContent, aString);

  sInitiatorAcc = nullptr;
  return rv;
}

nsresult
nsTextEquivUtils::AppendTextEquivFromTextContent(nsIContent *aContent,
                                                 nsAString *aString)
{
  if (aContent->IsNodeOfType(nsINode::eTEXT)) {
    bool isHTMLBlock = false;

    nsIContent *parentContent = aContent->GetFlattenedTreeParent();
    if (parentContent) {
      nsIFrame *frame = parentContent->GetPrimaryFrame();
      if (frame) {
        
        
        
        const nsStyleDisplay* display = frame->StyleDisplay();
        if (display->IsBlockOutsideStyle() ||
            display->mDisplay == NS_STYLE_DISPLAY_TABLE_CELL) {
          isHTMLBlock = true;
          if (!aString->IsEmpty()) {
            aString->Append(char16_t(' '));
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
        aString->Append(char16_t(' '));
      }
    }
    
    return NS_OK;
  }
  
  if (aContent->IsHTMLElement() &&
      aContent->NodeInfo()->Equals(nsGkAtoms::br)) {
    aString->AppendLiteral("\r\n");
    return NS_OK;
  }
  
  return NS_OK_NO_NAME_CLAUSE_HANDLED;
}




nsresult
nsTextEquivUtils::AppendFromAccessibleChildren(Accessible* aAccessible,
                                               nsAString *aString)
{
  nsresult rv = NS_OK_NO_NAME_CLAUSE_HANDLED;

  uint32_t childCount = aAccessible->ChildCount();
  for (uint32_t childIdx = 0; childIdx < childCount; childIdx++) {
    Accessible* child = aAccessible->GetChildAt(childIdx);
    rv = AppendFromAccessible(child, aString);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}

nsresult
nsTextEquivUtils::AppendFromAccessible(Accessible* aAccessible,
                                       nsAString *aString)
{
  
  if (aAccessible->IsContent()) {
    nsresult rv = AppendTextEquivFromTextContent(aAccessible->GetContent(),
                                                 aString);
    if (rv != NS_OK_NO_NAME_CLAUSE_HANDLED)
      return rv;
  }

  bool isEmptyTextEquiv = true;

  
  
  nsAutoString text;
  if (aAccessible->Name(text) != eNameFromTooltip)
    isEmptyTextEquiv = !AppendString(aString, text);

  
  nsresult rv = AppendFromValue(aAccessible, aString);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rv != NS_OK_NO_NAME_CLAUSE_HANDLED)
    isEmptyTextEquiv = false;

  
  
  
  if (isEmptyTextEquiv) {
    uint32_t nameRule = GetRoleRule(aAccessible->Role());
    if (nameRule & eNameFromSubtreeIfReqRule) {
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
nsTextEquivUtils::AppendFromValue(Accessible* aAccessible,
                                  nsAString *aString)
{
  if (GetRoleRule(aAccessible->Role()) != eNameFromValueRule)
    return NS_OK_NO_NAME_CLAUSE_HANDLED;

  
  
  
  

  nsAutoString text;
  if (aAccessible != sInitiatorAcc) {
    aAccessible->Value(text);

    return AppendString(aString, text) ?
      NS_OK : NS_OK_NO_NAME_CLAUSE_HANDLED;
  }

  
  if (aAccessible->IsDoc())
    return NS_ERROR_UNEXPECTED;

  nsIContent *content = aAccessible->GetContent();

  for (nsIContent* childContent = content->GetPreviousSibling(); childContent;
       childContent = childContent->GetPreviousSibling()) {
    
    if (!childContent->TextIsOnlyWhitespace()) {
      for (nsIContent* siblingContent = content->GetNextSibling(); siblingContent;
           siblingContent = siblingContent->GetNextSibling()) {
        
        if (!siblingContent->TextIsOnlyWhitespace()) {
          aAccessible->Value(text);

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

  if (aContent->IsXULElement()) {
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

  
  if (!aString->IsEmpty() && !nsCoreUtils::IsWhitespace(aString->Last()))
    aString->Append(char16_t(' '));

  aString->Append(aTextEquivalent);

  if (!nsCoreUtils::IsWhitespace(aString->Last()))
    aString->Append(char16_t(' '));

  return true;
}

uint32_t
nsTextEquivUtils::GetRoleRule(role aRole)
{
#define ROLE(geckoRole, stringRole, atkRole, \
             macRole, msaaRole, ia2Role, nameRule) \
  case roles::geckoRole: \
    return nameRule;

  switch (aRole) {
#include "RoleMap.h"
    default:
      MOZ_CRASH("Unknown role.");
  }

#undef ROLE
}
