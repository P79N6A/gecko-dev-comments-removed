




#ifndef nsIHTMLCollection_h___
#define nsIHTMLCollection_h___

#include "nsIDOMHTMLCollection.h"
#include "nsWrapperCache.h"

struct JSContext;
struct JSObject;
class nsGenericElement;
class nsINode;
namespace mozilla {
class ErrorResult;
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
  virtual nsGenericElement* GetElementAt(uint32_t index) = 0;
  nsGenericElement* Item(uint32_t index)
  {
    return GetElementAt(index);
  }
  nsGenericElement* IndexedGetter(uint32_t index, bool& aFound)
  {
    nsGenericElement* item = Item(index);
    aFound = !!item;
    return item;
  }
  virtual JSObject* NamedItem(JSContext* cx, const nsAString& name,
                              mozilla::ErrorResult& error) = 0;
  JSObject* NamedGetter(JSContext* cx, const nsAString& name,
                        bool& found, mozilla::ErrorResult& error)
  {
    JSObject* namedItem = NamedItem(cx, name, error);
    found = !!namedItem;
    return namedItem;
  }
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIHTMLCollection, NS_IHTMLCOLLECTION_IID)

#endif 
