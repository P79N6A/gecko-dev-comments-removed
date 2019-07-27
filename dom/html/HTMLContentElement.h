




#ifndef mozilla_dom_HTMLContentElement_h__
#define mozilla_dom_HTMLContentElement_h__

#include "nsINodeList.h"
#include "nsGenericHTMLElement.h"

struct nsCSSSelectorList;

namespace mozilla {
namespace dom {

class DistributedContentList;

class HTMLContentElement MOZ_FINAL : public nsGenericHTMLElement
{
public:
  explicit HTMLContentElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLContentElement, content)

  
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLContentElement,
                                           nsGenericHTMLElement)

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;

  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) MOZ_OVERRIDE;

  



  bool Match(nsIContent* aContent);
  bool IsInsertionPoint() const { return mIsInsertionPoint; }
  nsCOMArray<nsIContent>& MatchedNodes() { return mMatchedNodes; }
  void AppendMatchedNode(nsIContent* aContent);
  void RemoveMatchedNode(nsIContent* aContent);
  void InsertMatchedNode(uint32_t aIndex, nsIContent* aContent);
  void ClearMatchedNodes();

  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) MOZ_OVERRIDE;

  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify) MOZ_OVERRIDE;

  
  already_AddRefed<DistributedContentList> GetDistributedNodes();
  void GetSelect(nsAString& aSelect)
  {
    Element::GetAttr(kNameSpaceID_None, nsGkAtoms::select, aSelect);
  }
  void SetSelect(const nsAString& aSelect)
  {
    Element::SetAttr(kNameSpaceID_None, nsGkAtoms::select, aSelect, true);
  }

protected:
  virtual ~HTMLContentElement();

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

  






  void UpdateFallbackDistribution();

  



  nsCOMArray<nsIContent> mMatchedNodes;

  nsAutoPtr<nsCSSSelectorList> mSelectorList;
  bool mValidSelector;
  bool mIsInsertionPoint;
};

class DistributedContentList : public nsINodeList
{
public:
  explicit DistributedContentList(HTMLContentElement* aHostElement);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DistributedContentList)

  
  NS_DECL_NSIDOMNODELIST

  
  virtual nsIContent* Item(uint32_t aIndex) MOZ_OVERRIDE;
  virtual int32_t IndexOf(nsIContent* aContent) MOZ_OVERRIDE;
  virtual nsINode* GetParentObject() MOZ_OVERRIDE { return mParent; }
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;
protected:
  virtual ~DistributedContentList();
  nsRefPtr<HTMLContentElement> mParent;
  nsCOMArray<nsIContent> mDistributedNodes;
};

} 
} 

#endif 

