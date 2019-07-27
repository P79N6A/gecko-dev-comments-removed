




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
    NS_ABORT_IF_FALSE(mNodeInfo->NodeType() == nsIDOMNode::TEXT_NODE,
                      "Bad NodeType in aNodeInfo");
  }

public:
  nsTextNode(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : mozilla::dom::Text(aNodeInfo)
  {
    Init();
  }

  nsTextNode(nsNodeInfoManager* aNodeInfoManager)
    : mozilla::dom::Text(aNodeInfoManager->GetTextNodeInfo())
  {
    Init();
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMCHARACTERDATA(nsGenericDOMDataNode::)
  using nsGenericDOMDataNode::SetData; 

  
  NS_FORWARD_NSIDOMTEXT(nsGenericDOMDataNode::)

  
  virtual bool IsNodeOfType(uint32_t aFlags) const;

  virtual nsGenericDOMDataNode* CloneDataNode(mozilla::dom::NodeInfo *aNodeInfo,
                                              bool aCloneText) const MOZ_OVERRIDE;

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) MOZ_OVERRIDE;

  nsresult AppendTextForNormalize(const char16_t* aBuffer, uint32_t aLength,
                                  bool aNotify, nsIContent* aNextSibling);

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

#ifdef DEBUG
  virtual void List(FILE* out, int32_t aIndent) const MOZ_OVERRIDE;
  virtual void DumpContent(FILE* out, int32_t aIndent, bool aDumpAll) const MOZ_OVERRIDE;
#endif

protected:
  virtual ~nsTextNode();

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;
};

#endif 
