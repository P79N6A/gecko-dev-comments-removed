




#ifndef mozilla_dom_ProcessingInstruction_h
#define mozilla_dom_ProcessingInstruction_h

#include "nsIDOMProcessingInstruction.h"
#include "nsGenericDOMDataNode.h"
#include "nsAString.h"

namespace mozilla {
namespace dom {

class ProcessingInstruction : public nsGenericDOMDataNode,
                              public nsIDOMProcessingInstruction
{
public:
  ProcessingInstruction(already_AddRefed<nsINodeInfo> aNodeInfo,
                        const nsAString& aData);
  virtual ~ProcessingInstruction();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)

  
  NS_DECL_NSIDOMPROCESSINGINSTRUCTION

  
  virtual bool IsNodeOfType(uint32_t aFlags) const;

  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              bool aCloneText) const;

#ifdef DEBUG
  virtual void List(FILE* out, int32_t aIndent) const;
  virtual void DumpContent(FILE* out, int32_t aIndent, bool aDumpAll) const;
#endif

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  









  bool GetAttrValue(nsIAtom *aName, nsAString& aValue);
};

} 
} 

#endif 
