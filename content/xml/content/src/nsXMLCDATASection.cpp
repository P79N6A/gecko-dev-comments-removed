




































#include "nsIDOMCDATASection.h"
#include "nsGenericDOMDataNode.h"
#include "nsGkAtoms.h"
#include "nsIDocument.h"
#include "nsContentUtils.h"


class nsXMLCDATASection : public nsGenericDOMDataNode,
                          public nsIDOMCDATASection
{
public:
  nsXMLCDATASection(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsXMLCDATASection();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMPL_NSIDOMNODE_USING_GENERIC_DOM_DATA

  
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)

  
  NS_FORWARD_NSIDOMTEXT(nsGenericDOMDataNode::)

  
  

  
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;

  virtual nsXPCClassInfo* GetClassInfo();
#ifdef DEBUG
  virtual void List(FILE* out, PRInt32 aIndent) const;
  virtual void DumpContent(FILE* out, PRInt32 aIndent,PRBool aDumpAll) const;
#endif
};

nsresult
NS_NewXMLCDATASection(nsIContent** aInstancePtrResult,
                      nsNodeInfoManager *aNodeInfoManager)
{
  NS_PRECONDITION(aNodeInfoManager, "Missing nodeinfo manager");

  *aInstancePtrResult = nsnull;

  nsCOMPtr<nsINodeInfo> ni;
  ni = aNodeInfoManager->GetNodeInfo(nsGkAtoms::cdataTagName,
                                     nsnull, kNameSpaceID_None);
  NS_ENSURE_TRUE(ni, NS_ERROR_OUT_OF_MEMORY);

  nsXMLCDATASection *instance = new nsXMLCDATASection(ni.forget());
  if (!instance) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = instance);

  return NS_OK;
}

nsXMLCDATASection::nsXMLCDATASection(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericDOMDataNode(aNodeInfo)
{
}

nsXMLCDATASection::~nsXMLCDATASection()
{
}


DOMCI_NODE_DATA(CDATASection, nsXMLCDATASection)


NS_INTERFACE_TABLE_HEAD(nsXMLCDATASection)
  NS_NODE_INTERFACE_TABLE4(nsXMLCDATASection, nsIDOMNode, nsIDOMCharacterData,
                           nsIDOMText, nsIDOMCDATASection)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CDATASection)
NS_INTERFACE_MAP_END_INHERITING(nsGenericDOMDataNode)

NS_IMPL_ADDREF_INHERITED(nsXMLCDATASection, nsGenericDOMDataNode)
NS_IMPL_RELEASE_INHERITED(nsXMLCDATASection, nsGenericDOMDataNode)


PRBool
nsXMLCDATASection::IsNodeOfType(PRUint32 aFlags) const
{
  return !(aFlags & ~(eCONTENT | eTEXT | eDATA_NODE));
}

NS_IMETHODIMP
nsXMLCDATASection::GetNodeName(nsAString& aNodeName)
{
  aNodeName.AssignLiteral("#cdata-section");
  return NS_OK;
}

NS_IMETHODIMP
nsXMLCDATASection::GetNodeValue(nsAString& aNodeValue)
{
  return nsGenericDOMDataNode::GetNodeValue(aNodeValue);
}

NS_IMETHODIMP
nsXMLCDATASection::SetNodeValue(const nsAString& aNodeValue)
{
  return nsGenericDOMDataNode::SetNodeValue(aNodeValue);
}

NS_IMETHODIMP
nsXMLCDATASection::GetNodeType(PRUint16* aNodeType)
{
  *aNodeType = (PRUint16)nsIDOMNode::CDATA_SECTION_NODE;
  return NS_OK;
}

nsGenericDOMDataNode*
nsXMLCDATASection::CloneDataNode(nsINodeInfo *aNodeInfo, PRBool aCloneText) const
{
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  nsXMLCDATASection *it = new nsXMLCDATASection(ni.forget());
  if (it && aCloneText) {
    it->mText = mText;
  }

  return it;
}

#ifdef DEBUG
void
nsXMLCDATASection::List(FILE* out, PRInt32 aIndent) const
{
  PRInt32 index;
  for (index = aIndent; --index >= 0; ) fputs("  ", out);

  fprintf(out, "CDATASection refcount=%d<", mRefCnt.get());

  nsAutoString tmp;
  ToCString(tmp, 0, mText.GetLength());
  fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);

  fputs(">\n", out);
}

void
nsXMLCDATASection::DumpContent(FILE* out, PRInt32 aIndent,
                               PRBool aDumpAll) const {
}
#endif
