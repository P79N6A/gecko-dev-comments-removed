




































#include "nsGenericElement.h"
#include "nsGkAtoms.h"
#include "nsUnicharUtils.h"
#include "nsXMLProcessingInstruction.h"
#include "nsParserUtils.h"
#include "nsContentCreatorFunctions.h"

nsresult
NS_NewXMLProcessingInstruction(nsIContent** aInstancePtrResult,
                               nsNodeInfoManager *aNodeInfoManager,
                               const nsAString& aTarget,
                               const nsAString& aData)
{
  NS_PRECONDITION(aNodeInfoManager, "Missing nodeinfo manager");

  if (aTarget.EqualsLiteral("xml-stylesheet")) {
    return NS_NewXMLStylesheetProcessingInstruction(aInstancePtrResult,
                                                    aNodeInfoManager, aData);
  }

  *aInstancePtrResult = nsnull;

  nsCOMPtr<nsINodeInfo> ni;
  ni = aNodeInfoManager->GetNodeInfo(nsGkAtoms::processingInstructionTagName,
                                     nsnull, kNameSpaceID_None);
  NS_ENSURE_TRUE(ni, NS_ERROR_OUT_OF_MEMORY);

  nsXMLProcessingInstruction *instance =
    new nsXMLProcessingInstruction(ni.forget(), aTarget, aData);
  if (!instance) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = instance);

  return NS_OK;
}

nsXMLProcessingInstruction::nsXMLProcessingInstruction(already_AddRefed<nsINodeInfo> aNodeInfo,
                                                       const nsAString& aTarget,
                                                       const nsAString& aData)
  : nsGenericDOMDataNode(aNodeInfo),
    mTarget(aTarget)
{
  SetTextInternal(0, mText.GetLength(),
                  aData.BeginReading(), aData.Length(),
                  PR_FALSE);  
}

nsXMLProcessingInstruction::~nsXMLProcessingInstruction()
{
}


DOMCI_NODE_DATA(ProcessingInstruction, nsXMLProcessingInstruction)


NS_INTERFACE_TABLE_HEAD(nsXMLProcessingInstruction)
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsXMLProcessingInstruction)
    NS_INTERFACE_TABLE_ENTRY(nsXMLProcessingInstruction, nsIDOMNode)
    NS_INTERFACE_TABLE_ENTRY(nsXMLProcessingInstruction,
                             nsIDOMProcessingInstruction)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ProcessingInstruction)
NS_INTERFACE_MAP_END_INHERITING(nsGenericDOMDataNode)


NS_IMPL_ADDREF_INHERITED(nsXMLProcessingInstruction, nsGenericDOMDataNode)
NS_IMPL_RELEASE_INHERITED(nsXMLProcessingInstruction, nsGenericDOMDataNode)


NS_IMETHODIMP
nsXMLProcessingInstruction::GetTarget(nsAString& aTarget)
{
  aTarget.Assign(mTarget);

  return NS_OK;
}

NS_IMETHODIMP
nsXMLProcessingInstruction::SetData(const nsAString& aData)
{
  return SetNodeValue(aData);
}

NS_IMETHODIMP
nsXMLProcessingInstruction::GetData(nsAString& aData)
{
  return nsGenericDOMDataNode::GetData(aData);
}

PRBool
nsXMLProcessingInstruction::GetAttrValue(nsIAtom *aName, nsAString& aValue)
{
  nsAutoString data;

  GetData(data);
  return nsParserUtils::GetQuotedAttributeValue(data, aName, aValue);
}

PRBool
nsXMLProcessingInstruction::IsNodeOfType(PRUint32 aFlags) const
{
  return !(aFlags & ~(eCONTENT | ePROCESSING_INSTRUCTION | eDATA_NODE));
}

NS_IMETHODIMP
nsXMLProcessingInstruction::GetNodeName(nsAString& aNodeName)
{
  aNodeName.Assign(mTarget);
  return NS_OK;
}

NS_IMETHODIMP
nsXMLProcessingInstruction::GetNodeValue(nsAString& aNodeValue)
{
  return nsGenericDOMDataNode::GetNodeValue(aNodeValue);
}

NS_IMETHODIMP
nsXMLProcessingInstruction::SetNodeValue(const nsAString& aNodeValue)
{
  return nsGenericDOMDataNode::SetNodeValue(aNodeValue);
}

NS_IMETHODIMP
nsXMLProcessingInstruction::GetNodeType(PRUint16* aNodeType)
{
  *aNodeType = (PRUint16)nsIDOMNode::PROCESSING_INSTRUCTION_NODE;
  return NS_OK;
}

nsGenericDOMDataNode*
nsXMLProcessingInstruction::CloneDataNode(nsINodeInfo *aNodeInfo,
                                          PRBool aCloneText) const
{
  nsAutoString data;
  nsGenericDOMDataNode::GetData(data);
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  return new nsXMLProcessingInstruction(ni.forget(), mTarget, data);
}

#ifdef DEBUG
void
nsXMLProcessingInstruction::List(FILE* out, PRInt32 aIndent) const
{
  PRInt32 index;
  for (index = aIndent; --index >= 0; ) fputs("  ", out);

  fprintf(out, "Processing instruction refcount=%d<", mRefCnt.get());

  nsAutoString tmp;
  ToCString(tmp, 0, mText.GetLength());
  tmp.Insert(mTarget.get(), 0);
  fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);

  fputs(">\n", out);
}

void
nsXMLProcessingInstruction::DumpContent(FILE* out, PRInt32 aIndent,
                                        PRBool aDumpAll) const
{
}
#endif
