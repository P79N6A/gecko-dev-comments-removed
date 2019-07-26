




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

class ShadowRoot : public DocumentFragment
{
public:
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ShadowRoot,
                                           DocumentFragment)
  NS_DECL_ISUPPORTS_INHERITED

  ShadowRoot(nsIContent* aContent, already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~ShadowRoot();

  void AddToIdTable(Element* aElement, nsIAtom* aId);
  void RemoveFromIdTable(Element* aElement, nsIAtom* aId);
  static bool PrefEnabled();

  nsISupports* GetParentObject() const
  {
    return mHost;
  }

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
protected:
  nsCOMPtr<nsIContent> mHost;
  nsTHashtable<nsIdentifierMapEntry> mIdentifierMap;
};

} 
} 

#endif 

