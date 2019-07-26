




#ifndef mozilla_dom_Comment_h
#define mozilla_dom_Comment_h

#include "mozilla/Attributes.h"
#include "nsIDOMComment.h"
#include "nsGenericDOMDataNode.h"

namespace mozilla {
namespace dom {

class Comment MOZ_FINAL : public nsGenericDOMDataNode,
                          public nsIDOMComment
{
private:
  void Init()
  {
    NS_ABORT_IF_FALSE(mNodeInfo->NodeType() == nsIDOMNode::COMMENT_NODE,
                      "Bad NodeType in aNodeInfo");
  }

public:
  Comment(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericDOMDataNode(aNodeInfo)
  {
    Init();
  }

  Comment(nsNodeInfoManager* aNodeInfoManager)
    : nsGenericDOMDataNode(aNodeInfoManager->GetCommentNodeInfo())
  {
    Init();
  }

  virtual ~Comment();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)
  using nsGenericDOMDataNode::SetData; 

  
  

  
  virtual bool IsNodeOfType(uint32_t aFlags) const;

  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              bool aCloneText) const MOZ_OVERRIDE;

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }
#ifdef DEBUG
  virtual void List(FILE* out, int32_t aIndent) const MOZ_OVERRIDE;
  virtual void DumpContent(FILE* out = stdout, int32_t aIndent = 0,
                           bool aDumpAll = true) const MOZ_OVERRIDE
  {
    return;
  }
#endif

  static already_AddRefed<Comment>
  Constructor(const GlobalObject& aGlobal, const nsAString& aData,
              ErrorResult& aRv);

protected:
  virtual JSObject* WrapNode(JSContext *aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;
};

} 
} 

#endif 
