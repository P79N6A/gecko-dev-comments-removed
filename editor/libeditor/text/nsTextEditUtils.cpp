




































#include "nsTextEditUtils.h"

#include "mozilla/dom/Element.h"

#include "nsEditor.h"
#include "nsPlaintextEditor.h"
#include "nsEditProperty.h"

using namespace mozilla;







bool 
nsTextEditUtils::IsBody(nsIDOMNode *node)
{
  return nsEditor::NodeIsTypeString(node, NS_LITERAL_STRING("body"));
}






bool 
nsTextEditUtils::IsBreak(nsIDOMNode *node)
{
  return nsEditor::NodeIsTypeString(node, NS_LITERAL_STRING("br"));
}





bool 
nsTextEditUtils::IsMozBR(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null node passed to nsHTMLEditUtils::IsMozBR");
  return IsBreak(node) && HasMozAttr(node);
}


bool
nsTextEditUtils::IsMozBR(dom::Element* aNode)
{
  MOZ_ASSERT(aNode);
  return aNode->IsHTML(nsGkAtoms::br) &&
         aNode->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                            NS_LITERAL_STRING("_moz"), eIgnoreCase);
}






bool 
nsTextEditUtils::HasMozAttr(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null parent passed to nsHTMLEditUtils::HasMozAttr");
  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(node);
  if (elem)
  {
    nsAutoString typeAttrVal;
    nsresult res = elem->GetAttribute(NS_LITERAL_STRING("type"), typeAttrVal);
    if (NS_SUCCEEDED(res) && (typeAttrVal.LowerCaseEqualsLiteral("_moz")))
      return true;
  }
  return false;
}





nsAutoEditInitRulesTrigger::nsAutoEditInitRulesTrigger( nsPlaintextEditor *aEd, nsresult &aRes) : mEd(aEd), mRes(aRes)
{
    if (mEd) mEd->BeginEditorInit();
}

nsAutoEditInitRulesTrigger::~nsAutoEditInitRulesTrigger()
{
    if (mEd) mRes = mEd->EndEditorInit();
}

