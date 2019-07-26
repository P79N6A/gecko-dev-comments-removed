




#ifndef mozilla_dom_shadowroot_h__
#define mozilla_dom_shadowroot_h__

#include "mozilla/dom/DocumentFragment.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsTHashtable.h"
#include "nsDocument.h"

class nsIAtom;
class nsIContent;
class nsIDocument;
class nsINodeInfo;
class nsPIDOMWindow;
class nsXBLPrototypeBinding;
class nsTagNameMapEntry;

namespace mozilla {
namespace dom {

class Element;
class HTMLContentElement;
class HTMLShadowElement;
class ShadowRootStyleSheetList;

class ShadowRoot : public DocumentFragment,
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

  ShadowRoot(nsIContent* aContent, already_AddRefed<nsINodeInfo> aNodeInfo,
             nsXBLPrototypeBinding* aProtoBinding);
  virtual ~ShadowRoot();

  void AddToIdTable(Element* aElement, nsIAtom* aId);
  void RemoveFromIdTable(Element* aElement, nsIAtom* aId);
  void InsertSheet(nsCSSStyleSheet* aSheet, nsIContent* aLinkingContent);
  void RemoveSheet(nsCSSStyleSheet* aSheet);
  bool ApplyAuthorStyles();
  void SetApplyAuthorStyles(bool aApplyAuthorStyles);
  nsIDOMStyleSheetList* StyleSheets();
  HTMLShadowElement* GetShadowElement() { return mShadowElement; }

  



  void SetShadowElement(HTMLShadowElement* aShadowElement);

  








  void ChangePoolHost(nsIContent* aNewHost);

  



  void DistributeSingleNode(nsIContent* aContent);

  



  void RemoveDistributedNode(nsIContent* aContent);

  



  void DistributeAllNodes();

  void AddInsertionPoint(HTMLContentElement* aInsertionPoint);
  void RemoveInsertionPoint(HTMLContentElement* aInsertionPoint);

  void SetYoungerShadow(ShadowRoot* aYoungerShadow);
  ShadowRoot* GetOlderShadow() { return mOlderShadow; }
  ShadowRoot* GetYoungerShadow() { return mYoungerShadow; }
  void SetInsertionPointChanged() { mInsertionPointChanged = true; }

  void SetAssociatedBinding(nsXBLBinding* aBinding) { mAssociatedBinding = aBinding; }

  nsISupports* GetParentObject() const { return mPoolHost; }

  nsIContent* GetPoolHost() { return mPoolHost; }
  nsTArray<HTMLShadowElement*>& ShadowDescendants() { return mShadowDescendants; }

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  static bool IsPooledNode(nsIContent* aChild, nsIContent* aContainer,
                           nsIContent* aHost);
  static ShadowRoot* FromNode(nsINode* aNode);
  static bool IsShadowInsertionPoint(nsIContent* aContent);

  
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
protected:
  void Restyle();

  
  
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
};

class ShadowRootStyleSheetList : public nsIDOMStyleSheetList
{
public:
  ShadowRootStyleSheetList(ShadowRoot* aShadowRoot);
  virtual ~ShadowRootStyleSheetList();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(ShadowRootStyleSheetList)

  
  NS_DECL_NSIDOMSTYLESHEETLIST

protected:
  nsRefPtr<ShadowRoot> mShadowRoot;
};

} 
} 

#endif 

