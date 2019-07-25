








#include "nsGenericDOMDataNode.h"
#include "nsIDOMText.h"




class nsTextNode : public nsGenericDOMDataNode,
                   public nsIDOMText
{
public:
  nsTextNode(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsTextNode();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericDOMDataNode::)

  
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)

  
  NS_FORWARD_NSIDOMTEXT(nsGenericDOMDataNode::)

  
  virtual bool IsNodeOfType(uint32_t aFlags) const;

  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              bool aCloneText) const;

  virtual nsXPCClassInfo* GetClassInfo();

  nsresult AppendTextForNormalize(const PRUnichar* aBuffer, uint32_t aLength,
                                  bool aNotify, nsIContent* aNextSibling);

  virtual nsIDOMNode* AsDOMNode() { return this; }

#ifdef DEBUG
  virtual void List(FILE* out, int32_t aIndent) const;
  virtual void DumpContent(FILE* out, int32_t aIndent, bool aDumpAll) const;
#endif
};
