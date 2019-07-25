





































#ifndef nsIXMLProcessingInstruction_h___
#define nsIXMLProcessingInstruction_h___

#include "nsIDOMProcessingInstruction.h"
#include "nsIDocument.h"
#include "nsGenericDOMDataNode.h"
#include "nsAString.h"


class nsXMLProcessingInstruction : public nsGenericDOMDataNode,
                                   public nsIDOMProcessingInstruction
{
public:
  nsXMLProcessingInstruction(already_AddRefed<nsINodeInfo> aNodeInfo,
                             const nsAString& aTarget,
                             const nsAString& aData);
  virtual ~nsXMLProcessingInstruction();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericDOMDataNode::)

  
  NS_DECL_NSIDOMPROCESSINGINSTRUCTION

  
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;
  virtual PRUint16 NodeType();
  virtual void NodeName(nsAString& aNodeName);

  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              PRBool aCloneText) const;

#ifdef DEBUG
  virtual void List(FILE* out, PRInt32 aIndent) const;
  virtual void DumpContent(FILE* out, PRInt32 aIndent, PRBool aDumpAll) const;
#endif

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  









  PRBool GetAttrValue(nsIAtom *aName, nsAString& aValue);

  nsString mTarget;
};

#endif 
