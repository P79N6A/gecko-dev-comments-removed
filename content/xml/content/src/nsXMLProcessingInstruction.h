




#ifndef nsIXMLProcessingInstruction_h___
#define nsIXMLProcessingInstruction_h___

#include "nsIDOMProcessingInstruction.h"
#include "nsGenericDOMDataNode.h"
#include "nsAString.h"


class nsXMLProcessingInstruction : public nsGenericDOMDataNode,
                                   public nsIDOMProcessingInstruction
{
public:
  nsXMLProcessingInstruction(already_AddRefed<nsINodeInfo> aNodeInfo,
                             const nsAString& aData);
  virtual ~nsXMLProcessingInstruction();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericDOMDataNode::)

  
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)

  
  NS_DECL_NSIDOMPROCESSINGINSTRUCTION

  
  virtual bool IsNodeOfType(PRUint32 aFlags) const;

  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              bool aCloneText) const;

#ifdef DEBUG
  virtual void List(FILE* out, PRInt32 aIndent) const;
  virtual void DumpContent(FILE* out, PRInt32 aIndent, bool aDumpAll) const;
#endif

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  









  bool GetAttrValue(nsIAtom *aName, nsAString& aValue);
};

#endif 
