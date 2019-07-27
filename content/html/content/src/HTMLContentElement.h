




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
  HTMLContentElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLContentElement, content)

  
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLContentElement,
                                           nsGenericHTMLElement)

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);

  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);

  



  bool Match(nsIContent* aContent);
  bool IsInsertionPoint() const { return mIsInsertionPoint; }
  nsCOMArray<nsIContent>& MatchedNodes() { return mMatchedNodes; }
  void AppendMatchedNode(nsIContent* aContent);
  void RemoveMatchedNode(nsIContent* aContent);
  void InsertMatchedNode(uint32_t aIndex, nsIContent* aContent);
  void ClearMatchedNodes();

  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);

  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify);

  
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

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

  






  void UpdateFallbackDistribution();

  



  nsCOMArray<nsIContent> mMatchedNodes;

  nsAutoPtr<nsCSSSelectorList> mSelectorList;
  bool mValidSelector;
  bool mIsInsertionPoint;
};

class DistributedContentList : public nsINodeList
{
public:
  DistributedContentList(HTMLContentElement* aHostElement);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DistributedContentList)

  
  NS_DECL_NSIDOMNODELIST

  
  virtual nsIContent* Item(uint32_t aIndex);
  virtual int32_t IndexOf(nsIContent* aContent);
  virtual nsINode* GetParentObject() { return mParent; }
  virtual uint32_t Length() const;
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
protected:
  virtual ~DistributedContentList();
  nsRefPtr<HTMLContentElement> mParent;
  nsCOMArray<nsIContent> mDistributedNodes;
};

} 
} 

#endif 

