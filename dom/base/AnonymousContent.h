





#ifndef mozilla_dom_AnonymousContent_h
#define mozilla_dom_AnonymousContent_h

#include "mozilla/dom/Element.h"
#include "nsCycleCollectionParticipant.h"
#include "nsICSSDeclaration.h"
#include "nsIDocument.h"

namespace mozilla {
namespace dom {

class Element;

class AnonymousContent MOZ_FINAL
{
public:
  
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(AnonymousContent)
  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(AnonymousContent)

  explicit AnonymousContent(Element* aContentNode);
  nsCOMPtr<Element> GetContentNode();
  JSObject* WrapObject(JSContext* aCx);

  
  void SetTextContentForElement(const nsAString& aElementId,
                                const nsAString& aText,
                                ErrorResult& aRv);

  void GetTextContentForElement(const nsAString& aElementId,
                                DOMString& aText,
                                ErrorResult& aRv);

  void SetAttributeForElement(const nsAString& aElementId,
                              const nsAString& aName,
                              const nsAString& aValue,
                              ErrorResult& aRv);

  void GetAttributeForElement(const nsAString& aElementId,
                              const nsAString& aName,
                              DOMString& aValue,
                              ErrorResult& aRv);

  void RemoveAttributeForElement(const nsAString& aElementId,
                                 const nsAString& aName,
                                 ErrorResult& aRv);

private:
  ~AnonymousContent();
  Element* GetElementById(const nsAString& aElementId);
  nsCOMPtr<Element> mContentNode;
};

} 
} 

#endif 
