




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

class ShadowRoot : public DocumentFragment,
                   public nsStubMutationObserver
{
public:
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ShadowRoot,
                                           DocumentFragment)
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  ShadowRoot(nsIContent* aContent, already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~ShadowRoot();

  void AddToIdTable(Element* aElement, nsIAtom* aId);
  void RemoveFromIdTable(Element* aElement, nsIAtom* aId);
  static bool PrefEnabled();

  



  void DistributeSingleNode(nsIContent* aContent);

  



  void RemoveDistributedNode(nsIContent* aContent);

  



  void DistributeAllNodes();

  void AddInsertionPoint(HTMLContentElement* aInsertionPoint);
  void RemoveInsertionPoint(HTMLContentElement* aInsertionPoint);

  void SetInsertionPointChanged() { mInsertionPointChanged = true; }

  nsISupports* GetParentObject() const
  {
    return mHost;
  }

  nsIContent* GetHost() { return mHost; }

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  static ShadowRoot* FromNode(nsINode* aNode);

  
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
  nsCOMPtr<nsIContent> mHost;

  
  
  
  
  
  nsTArray<HTMLContentElement*> mInsertionPoints;

  nsTHashtable<nsIdentifierMapEntry> mIdentifierMap;

  
  
  
  
  bool mInsertionPointChanged;
};

} 
} 

#endif 

