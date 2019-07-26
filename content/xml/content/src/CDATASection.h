




#ifndef mozilla_dom_CDATASection_h
#define mozilla_dom_CDATASection_h

#include "nsIDOMCDATASection.h"
#include "mozilla/dom/Text.h"

namespace mozilla {
namespace dom {

class CDATASection : public Text,
                     public nsIDOMCDATASection
{
public:
  CDATASection(already_AddRefed<nsINodeInfo> aNodeInfo)
    : Text(aNodeInfo)
  {
    NS_ABORT_IF_FALSE(mNodeInfo->NodeType() == nsIDOMNode::CDATA_SECTION_NODE,
                      "Bad NodeType in aNodeInfo");
    SetIsDOMBinding();
  }
  virtual ~CDATASection();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)

  
  NS_FORWARD_NSIDOMTEXT(nsGenericDOMDataNode::)

  
  

  
  virtual bool IsNodeOfType(uint32_t aFlags) const;

  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              bool aCloneText) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }
#ifdef DEBUG
  virtual void List(FILE* out, int32_t aIndent) const;
  virtual void DumpContent(FILE* out, int32_t aIndent,bool aDumpAll) const;
#endif

protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;
};

} 
} 

#endif 
