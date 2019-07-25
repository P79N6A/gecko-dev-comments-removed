








































#include "nsGenericDOMDataNode.h"
#include "nsIDOMText.h"

#include "nsIAttribute.h"
#include "nsIDocument.h"
#include "nsThreadUtils.h"
#include "nsDOMMemoryReporter.h"




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

  
  NS_DECL_AND_IMPL_DOM_MEMORY_REPORTER_SIZEOF(nsTextNode, nsGenericDOMDataNode)

  
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;

  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              PRBool aCloneText) const;

  nsresult BindToAttribute(nsIAttribute* aAttr);
  nsresult UnbindFromAttribute();

  virtual nsXPCClassInfo* GetClassInfo();

  nsresult AppendTextForNormalize(const PRUnichar* aBuffer, PRUint32 aLength,
                                  PRBool aNotify, nsIContent* aNextSibling);

#ifdef DEBUG
  virtual void List(FILE* out, PRInt32 aIndent) const;
  virtual void DumpContent(FILE* out, PRInt32 aIndent, PRBool aDumpAll) const;
#endif
};
