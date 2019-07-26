




#ifndef nsTextNode_h
#define nsTextNode_h





#include "nsGenericDOMDataNode.h"
#include "nsIDOMText.h"
#include "nsDebug.h"




class nsTextNode : public nsGenericDOMDataNode,
                   public nsIDOMText
{
public:
  nsTextNode(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericDOMDataNode(aNodeInfo)
  {
    NS_ABORT_IF_FALSE(mNodeInfo->NodeType() == nsIDOMNode::TEXT_NODE,
                      "Bad NodeType in aNodeInfo");
    SetIsDOMBinding();
  }

  virtual ~nsTextNode();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)

  
  NS_FORWARD_NSIDOMTEXT(nsGenericDOMDataNode::)

  
  virtual bool IsNodeOfType(uint32_t aFlags) const;

  virtual nsGenericDOMDataNode* CloneDataNode(nsINodeInfo *aNodeInfo,
                                              bool aCloneText) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);

  nsresult AppendTextForNormalize(const PRUnichar* aBuffer, uint32_t aLength,
                                  bool aNotify, nsIContent* aNextSibling);

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  already_AddRefed<nsTextNode> SplitText(uint32_t aOffset,
                                         mozilla::ErrorResult& rv);
  void GetWholeText(nsAString& aWholeText, mozilla::ErrorResult& rv)
  {
    rv = GetWholeText(aWholeText);
  }

#ifdef DEBUG
  virtual void List(FILE* out, int32_t aIndent) const;
  virtual void DumpContent(FILE* out, int32_t aIndent, bool aDumpAll) const;
#endif

protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope,
                             bool *aTriedToWrap) MOZ_OVERRIDE;
};

#endif 
