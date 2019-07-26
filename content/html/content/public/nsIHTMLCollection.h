




#ifndef nsIHTMLCollection_h___
#define nsIHTMLCollection_h___

#include "nsIDOMHTMLCollection.h"
#include "nsWrapperCache.h"
#include "js/TypeDecls.h"

class nsINode;
class nsString;
template<class> class nsTArray;

namespace mozilla {
class ErrorResult;

namespace dom {
class Element;
} 
} 


#define NS_IHTMLCOLLECTION_IID \
{ 0x5643235d, 0x9a72, 0x4b6a, \
 { 0xa6, 0x0c, 0x64, 0x63, 0x72, 0xb7, 0x53, 0x4a } }




class nsIHTMLCollection : public nsIDOMHTMLCollection
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IHTMLCOLLECTION_IID)

  


  virtual nsINode* GetParentObject() = 0;

  using nsIDOMHTMLCollection::Item;
  using nsIDOMHTMLCollection::NamedItem;

  uint32_t Length()
  {
    uint32_t length;
    GetLength(&length);
    return length;
  }
  virtual mozilla::dom::Element* GetElementAt(uint32_t index) = 0;
  mozilla::dom::Element* Item(uint32_t index)
  {
    return GetElementAt(index);
  }
  mozilla::dom::Element* IndexedGetter(uint32_t index, bool& aFound)
  {
    mozilla::dom::Element* item = Item(index);
    aFound = !!item;
    return item;
  }
  mozilla::dom::Element* NamedItem(const nsAString& aName)
  {
    bool dummy;
    return NamedGetter(aName, dummy);
  }
  mozilla::dom::Element* NamedGetter(const nsAString& aName, bool& aFound)
  {
    return GetFirstNamedElement(aName, aFound);
  }
  virtual mozilla::dom::Element*
  GetFirstNamedElement(const nsAString& aName, bool& aFound) = 0;

  virtual void GetSupportedNames(nsTArray<nsString>& aNames) = 0;

  JSObject* GetWrapperPreserveColor()
  {
    nsWrapperCache* cache;
    CallQueryInterface(this, &cache);
    return cache->GetWrapperPreserveColor();
  }
  virtual JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> scope) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLCollection, NS_IHTMLCOLLECTION_IID)

#endif 
