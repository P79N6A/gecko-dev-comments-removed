




































#include "nsTextEditUtils.h"

#include "nsEditor.h"
#include "nsPlaintextEditor.h"
#include "nsEditProperty.h"







PRBool 
nsTextEditUtils::IsBody(nsIDOMNode *node)
{
  return nsEditor::NodeIsTypeString(node, NS_LITERAL_STRING("body"));
}






PRBool 
nsTextEditUtils::IsBreak(nsIDOMNode *node)
{
  return nsEditor::NodeIsTypeString(node, NS_LITERAL_STRING("br"));
}





PRBool 
nsTextEditUtils::IsMozBR(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null node passed to nsHTMLEditUtils::IsMozBR");
  if (IsBreak(node) && HasMozAttr(node)) return PR_TRUE;
  return PR_FALSE;
}







PRBool 
nsTextEditUtils::HasMozAttr(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null parent passed to nsHTMLEditUtils::HasMozAttr");
  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(node);
  if (elem)
  {
    nsAutoString typeAttrVal;
    nsresult res = elem->GetAttribute(NS_LITERAL_STRING("type"), typeAttrVal);
    if (NS_SUCCEEDED(res) && (typeAttrVal.LowerCaseEqualsLiteral("_moz")))
      return PR_TRUE;
  }
  return PR_FALSE;
}





PRBool 
nsTextEditUtils::InBody(nsIDOMNode *node, nsIEditor *editor)
{
  if (!node)
    return PR_FALSE;

  nsCOMPtr<nsIDOMElement> rootElement;
  editor->GetRootElement(getter_AddRefs(rootElement));

  nsCOMPtr<nsIDOMNode> tmp;
  nsCOMPtr<nsIDOMNode> p = node;
  while (p != rootElement)
  {
    if (NS_FAILED(p->GetParentNode(getter_AddRefs(tmp))) || !tmp)
      return PR_FALSE;
    p = tmp;
  }
  return PR_TRUE;
}




nsAutoEditInitRulesTrigger::nsAutoEditInitRulesTrigger( nsPlaintextEditor *aEd, nsresult &aRes) : mEd(aEd), mRes(aRes)
{
    if (mEd) mEd->BeginEditorInit();
}

nsAutoEditInitRulesTrigger::~nsAutoEditInitRulesTrigger()
{
    if (mEd) mRes = mEd->EndEditorInit();
}

