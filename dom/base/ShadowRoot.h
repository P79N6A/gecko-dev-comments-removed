




#ifndef mozilla_dom_shadowroot_h__
#define mozilla_dom_shadowroot_h__

#include "mozilla/dom/DocumentFragment.h"
#include "mozilla/dom/StyleSheetList.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsTHashtable.h"
#include "nsDocument.h"

class nsIAtom;
class nsIContent;
class nsXBLPrototypeBinding;

namespace mozilla {
namespace dom {

class Element;
class HTMLContentElement;
class HTMLShadowElement;
class ShadowRootStyleSheetList;

class ShadowRoot final : public DocumentFragment,
                         public nsStubMutationObserver
{
  friend class ShadowRootStyleSheetList;
public:
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ShadowRoot,
                                           DocumentFragment)
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  ShadowRoot(nsIContent* aContent, already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
             nsXBLPrototypeBinding* aProtoBinding);

  void AddToIdTable(Element* aElement, nsIAtom* aId);
  void RemoveFromIdTable(Element* aElement, nsIAtom* aId);
  void InsertSheet(CSSStyleSheet* aSheet, nsIContent* aLinkingContent);
  void RemoveSheet(CSSStyleSheet* aSheet);
  bool ApplyAuthorStyles();
  void SetApplyAuthorStyles(bool aApplyAuthorStyles);
  StyleSheetList* StyleSheets();
  HTMLShadowElement* GetShadowElement() { return mShadowElement; }

  



  void SetShadowElement(HTMLShadowElement* aShadowElement);

  








  void ChangePoolHost(nsIContent* aNewHost);

  



  void DistributeSingleNode(nsIContent* aContent);

  



  void RemoveDistributedNode(nsIContent* aContent);

  



  void DistributeAllNodes();

  void AddInsertionPoint(HTMLContentElement* aInsertionPoint);
  void RemoveInsertionPoint(HTMLContentElement* aInsertionPoint);

  void SetYoungerShadow(ShadowRoot* aYoungerShadow);
  ShadowRoot* GetYoungerShadowRoot() { return mYoungerShadow; }
  void SetInsertionPointChanged() { mInsertionPointChanged = true; }

  void SetAssociatedBinding(nsXBLBinding* aBinding) { mAssociatedBinding = aBinding; }

  nsISupports* GetParentObject() const { return mPoolHost; }

  nsIContent* GetPoolHost() { return mPoolHost; }
  nsTArray<HTMLShadowElement*>& ShadowDescendants() { return mShadowDescendants; }

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  static bool IsPooledNode(nsIContent* aChild, nsIContent* aContainer,
                           nsIContent* aHost);
  static ShadowRoot* FromNode(nsINode* aNode);
  static bool IsShadowInsertionPoint(nsIContent* aContent);

  static void RemoveDestInsertionPoint(nsIContent* aInsertionPoint,
                                       nsTArray<nsIContent*>& aDestInsertionPoints);

  
  Element* GetElementById(const nsAString& aElementId);
  already_AddRefed<nsContentList>
    GetElementsByTagName(const nsAString& aNamespaceURI);
  already_AddRefed<nsContentList>
    GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                           const nsAString& aLocalName);
  already_AddRefed<nsContentList>
    GetElementsByClassName(const nsAString& aClasses);
  void GetInnerHTML(nsAString& aInnerHTML);
  void SetInnerHTML(const nsAString& aInnerHTML, ErrorResult& aError);
  Element* Host();
  ShadowRoot* GetOlderShadowRoot() { return mOlderShadow; }
  void StyleSheetChanged();

  bool IsComposedDocParticipant() { return mIsComposedDocParticipant; }
  void SetIsComposedDocParticipant(bool aIsComposedDocParticipant)
  {
    mIsComposedDocParticipant = aIsComposedDocParticipant;
  }

  virtual void DestroyContent() override;
protected:
  virtual ~ShadowRoot();

  
  
  nsCOMPtr<nsIContent> mPoolHost;

  
  
  
  
  
  nsTArray<HTMLContentElement*> mInsertionPoints;

  
  
  nsTArray<HTMLShadowElement*> mShadowDescendants;

  nsTHashtable<nsIdentifierMapEntry> mIdentifierMap;
  nsXBLPrototypeBinding* mProtoBinding;

  
  
  
  nsRefPtr<nsXBLBinding> mAssociatedBinding;

  nsRefPtr<ShadowRootStyleSheetList> mStyleSheetList;

  
  HTMLShadowElement* mShadowElement;

  
  
  nsRefPtr<ShadowRoot> mOlderShadow;

  
  
  nsRefPtr<ShadowRoot> mYoungerShadow;

  
  
  
  
  bool mInsertionPointChanged;

  
  
  
  
  bool mIsComposedDocParticipant;
};

class ShadowRootStyleSheetList : public StyleSheetList
{
public:
  explicit ShadowRootStyleSheetList(ShadowRoot* aShadowRoot);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ShadowRootStyleSheetList, StyleSheetList)

  virtual nsINode* GetParentObject() const override
  {
    return mShadowRoot;
  }

  virtual uint32_t Length() override;
  virtual CSSStyleSheet* IndexedGetter(uint32_t aIndex, bool& aFound) override;

protected:
  virtual ~ShadowRootStyleSheetList();

  nsRefPtr<ShadowRoot> mShadowRoot;
};

} 
} 

#endif 

