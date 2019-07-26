




#ifndef mozilla_dom_Comment_h
#define mozilla_dom_Comment_h

#include "nsIDOMComment.h"
#include "nsGenericDOMDataNode.h"

namespace mozilla {
namespace dom {

class Comment : public nsGenericDOMDataNode,
                public nsIDOMComment
{
public:
  Comment(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericDOMDataNode(aNodeInfo)
  {
    NS_ABORT_IF_FALSE(mNodeInfo->NodeType() == nsIDOMNode::COMMENT_NODE,
                      "Bad NodeType in aNodeInfo");
    SetIsDOMBinding();
  }
  virtual ~Comment();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)

  
  

  
  virtual bool IsNodeOfType(uint32_t aFlags) const;

  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              bool aCloneText) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }
#ifdef DEBUG
  virtual void List(FILE* out, int32_t aIndent) const;
  virtual void DumpContent(FILE* out = stdout, int32_t aIndent = 0,
                           bool aDumpAll = true) const
  {
    return;
  }
#endif

protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;
};

} 
} 

#endif 
