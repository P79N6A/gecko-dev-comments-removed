




#ifndef nsTextNode_h
#define nsTextNode_h





#include "mozilla/Attributes.h"
#include "mozilla/dom/Text.h"
#include "nsIDOMText.h"
#include "nsDebug.h"

class nsNodeInfoManager;




class nsTextNode : public mozilla::dom::Text,
                   public nsIDOMText
{
private:
  void Init()
  {
    MOZ_ASSERT(mNodeInfo->NodeType() == nsIDOMNode::TEXT_NODE,
               "Bad NodeType in aNodeInfo");
  }

public:
  explicit nsTextNode(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : mozilla::dom::Text(aNodeInfo)
  {
    Init();
  }

  explicit nsTextNode(nsNodeInfoManager* aNodeInfoManager)
    : mozilla::dom::Text(aNodeInfoManager->GetTextNodeInfo())
  {
    Init();
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)
  using nsGenericDOMDataNode::SetData; 

  
  NS_FORWARD_NSIDOMTEXT(nsGenericDOMDataNode::)

  
  virtual bool IsNodeOfType(uint32_t aFlags) const override;

  virtual nsGenericDOMDataNode* CloneDataNode(mozilla::dom::NodeInfo *aNodeInfo,
                                              bool aCloneText) const override;

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) override;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) override;

  nsresult AppendTextForNormalize(const char16_t* aBuffer, uint32_t aLength,
                                  bool aNotify, nsIContent* aNextSibling);

  virtual nsIDOMNode* AsDOMNode() override { return this; }

#ifdef DEBUG
  virtual void List(FILE* out, int32_t aIndent) const override;
  virtual void DumpContent(FILE* out, int32_t aIndent, bool aDumpAll) const override;
#endif

protected:
  virtual ~nsTextNode();

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;
};

#endif 
