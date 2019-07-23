








































#include "nsIDOMComment.h"
#include "nsGenericDOMDataNode.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"

class nsCommentNode : public nsGenericDOMDataNode,
                      public nsIDOMComment
{
public:
  nsCommentNode(nsINodeInfo *aNodeInfo);
  virtual ~nsCommentNode();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMPL_NSIDOMNODE_USING_GENERIC_DOM_DATA

  
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)

  
  

  
  virtual PRBool MayHaveFrame() const;
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;

#ifdef DEBUG
  virtual void List(FILE* out, PRInt32 aIndent) const;
  virtual void DumpContent(FILE* out = stdout, PRInt32 aIndent = 0,
                           PRBool aDumpAll = PR_TRUE) const
  {
    return;
  }
#endif
};

nsresult
NS_NewCommentNode(nsIContent** aInstancePtrResult,
                  nsNodeInfoManager *aNodeInfoManager)
{
  NS_PRECONDITION(aNodeInfoManager, "Missing nodeinfo manager");

  *aInstancePtrResult = nsnull;

  nsCOMPtr<nsINodeInfo> ni = aNodeInfoManager->GetCommentNodeInfo();
  NS_ENSURE_TRUE(ni, NS_ERROR_OUT_OF_MEMORY);

  nsCommentNode *instance = new nsCommentNode(ni);
  if (!instance) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = instance);

  return NS_OK;
}

nsCommentNode::nsCommentNode(nsINodeInfo *aNodeInfo)
  : nsGenericDOMDataNode(aNodeInfo)
{
}

nsCommentNode::~nsCommentNode()
{
}



NS_INTERFACE_MAP_BEGIN(nsCommentNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCharacterData)
  NS_INTERFACE_MAP_ENTRY(nsIDOMComment)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(Comment)
NS_INTERFACE_MAP_END_INHERITING(nsGenericDOMDataNode)


NS_IMPL_ADDREF_INHERITED(nsCommentNode, nsGenericDOMDataNode)
NS_IMPL_RELEASE_INHERITED(nsCommentNode, nsGenericDOMDataNode)



PRBool
nsCommentNode::MayHaveFrame() const
{
  return PR_FALSE;
}

PRBool
nsCommentNode::IsNodeOfType(PRUint32 aFlags) const
{
  return !(aFlags & ~(eCONTENT | eCOMMENT | eDATA_NODE));
}

NS_IMETHODIMP
nsCommentNode::GetNodeName(nsAString& aNodeName)
{
  aNodeName.AssignLiteral("#comment");
  return NS_OK;
}

NS_IMETHODIMP
nsCommentNode::GetNodeValue(nsAString& aNodeValue)
{
  return nsGenericDOMDataNode::GetNodeValue(aNodeValue);
}

NS_IMETHODIMP
nsCommentNode::SetNodeValue(const nsAString& aNodeValue)
{
  return nsGenericDOMDataNode::SetNodeValue(aNodeValue);
}

NS_IMETHODIMP
nsCommentNode::GetNodeType(PRUint16* aNodeType)
{
  *aNodeType = (PRUint16)nsIDOMNode::COMMENT_NODE;
  return NS_OK;
}

nsGenericDOMDataNode*
nsCommentNode::CloneDataNode(nsINodeInfo *aNodeInfo, PRBool aCloneText) const
{
  nsCommentNode *it = new nsCommentNode(aNodeInfo);
  if (it && aCloneText) {
    it->mText = mText;
  }

  return it;
}

#ifdef DEBUG
void
nsCommentNode::List(FILE* out, PRInt32 aIndent) const
{
  PRInt32 indx;
  for (indx = aIndent; --indx >= 0; ) fputs("  ", out);

  fprintf(out, "Comment@%p refcount=%d<!--", this, mRefCnt.get());

  nsAutoString tmp;
  ToCString(tmp, 0, mText.GetLength());
  fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);

  fputs("-->\n", out);
}
#endif
