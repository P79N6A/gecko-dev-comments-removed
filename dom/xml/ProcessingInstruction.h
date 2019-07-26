




#ifndef mozilla_dom_ProcessingInstruction_h
#define mozilla_dom_ProcessingInstruction_h

#include "mozilla/Attributes.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsGenericDOMDataNode.h"
#include "nsAString.h"

namespace mozilla {
namespace dom {

class ProcessingInstruction : public nsGenericDOMDataNode,
                              public nsIDOMProcessingInstruction
{
public:
  ProcessingInstruction(already_AddRefed<nsINodeInfo>&& aNodeInfo,
                        const nsAString& aData);
  virtual ~ProcessingInstruction();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)
  using nsGenericDOMDataNode::SetData; 

  
  NS_DECL_NSIDOMPROCESSINGINSTRUCTION

  
  virtual bool IsNodeOfType(uint32_t aFlags) const;

  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              bool aCloneText) const MOZ_OVERRIDE;

#ifdef DEBUG
  virtual void List(FILE* out, int32_t aIndent) const MOZ_OVERRIDE;
  virtual void DumpContent(FILE* out, int32_t aIndent, bool aDumpAll) const MOZ_OVERRIDE;
#endif

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

  
  void GetTarget(nsString& aTarget)
  {
    aTarget = NodeName();
  }
protected:
  









  bool GetAttrValue(nsIAtom *aName, nsAString& aValue);

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;
};

} 
} 




already_AddRefed<mozilla::dom::ProcessingInstruction>
NS_NewXMLProcessingInstruction(nsNodeInfoManager *aNodeInfoManager,
                               const nsAString& aTarget,
                               const nsAString& aData);

#endif 
