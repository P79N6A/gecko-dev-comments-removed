



#ifndef mozilla_dom_HTMLOptionsCollection_h
#define mozilla_dom_HTMLOptionsCollection_h

#include "nsIHTMLCollection.h"
#include "nsIDOMHTMLOptionsCollection.h"
#include "nsWrapperCache.h"

#include "mozilla/dom/HTMLOptionElement.h"
#include "mozilla/dom/UnionTypes.h"
#include "mozilla/ErrorResult.h"
#include "nsCOMPtr.h"
#include "nsError.h"
#include "nsGenericHTMLElement.h"
#include "nsTArray.h"

class nsHTMLSelectElement;
class nsIDOMHTMLOptionElement;

namespace mozilla {
namespace dom {





class HTMLOptionsCollection : public nsIHTMLCollection
                            , public nsIDOMHTMLOptionsCollection
                            , public nsWrapperCache
{
  typedef HTMLOptionElementOrHTMLOptGroupElement HTMLOptionOrOptGroupElement;
public:
  HTMLOptionsCollection(nsHTMLSelectElement* aSelect);
  virtual ~HTMLOptionsCollection();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  virtual JSObject* WrapObject(JSContext* cx, JSObject* scope) MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMHTMLOPTIONSCOLLECTION

  
  

  virtual Element* GetElementAt(uint32_t aIndex);
  virtual nsINode* GetParentObject();

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(HTMLOptionsCollection,
                                                         nsIHTMLCollection)

  
  




  void InsertOptionAt(mozilla::dom::HTMLOptionElement* aOption, uint32_t aIndex)
  {
    mElements.InsertElementAt(aIndex, aOption);
  }

  



  void RemoveOptionAt(uint32_t aIndex)
  {
    mElements.RemoveElementAt(aIndex);
  }

  




  mozilla::dom::HTMLOptionElement* ItemAsOption(uint32_t aIndex)
  {
    return mElements.SafeElementAt(aIndex, nullptr);
  }

  


  void Clear()
  {
    mElements.Clear();
  }

  


  void AppendOption(mozilla::dom::HTMLOptionElement* aOption)
  {
    mElements.AppendElement(aOption);
  }

  


  void DropReference();

  









  nsresult GetOptionIndex(Element* aOption,
                          int32_t aStartIndex, bool aForward,
                          int32_t* aIndex);

  virtual JSObject* NamedItem(JSContext* aCx, const nsAString& aName,
                              ErrorResult& error);

  void Add(const HTMLOptionOrOptGroupElement& aElement,
           const Nullable<HTMLElementOrLong>& aBefore,
           ErrorResult& aError);
  void Remove(int32_t aIndex, ErrorResult& aError);
  int32_t GetSelectedIndex(ErrorResult& aError);
  void SetSelectedIndex(int32_t aSelectedIndex, ErrorResult& aError);
  void IndexedSetter(uint32_t aIndex, nsIDOMHTMLOptionElement* aOption,
                     ErrorResult& aError)
  {
    aError = SetOption(aIndex, aOption);
  }
  virtual void GetSupportedNames(nsTArray<nsString>& aNames);

private:
  

  nsTArray<nsRefPtr<mozilla::dom::HTMLOptionElement> > mElements;
  
  nsHTMLSelectElement* mSelect;
};

} 
} 

#endif 
