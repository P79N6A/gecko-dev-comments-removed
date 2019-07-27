



#ifndef mozilla_dom_HTMLOptionsCollection_h
#define mozilla_dom_HTMLOptionsCollection_h

#include "mozilla/Attributes.h"
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

class nsIDOMHTMLOptionElement;

namespace mozilla {
namespace dom {

class HTMLSelectElement;





class HTMLOptionsCollection MOZ_FINAL : public nsIHTMLCollection
                                      , public nsIDOMHTMLOptionsCollection
                                      , public nsWrapperCache
{
  typedef HTMLOptionElementOrHTMLOptGroupElement HTMLOptionOrOptGroupElement;
public:
  explicit HTMLOptionsCollection(HTMLSelectElement* aSelect);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  using nsWrapperCache::GetWrapperPreserveColor;
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
protected:
  virtual ~HTMLOptionsCollection();

  virtual JSObject* GetWrapperPreserveColorInternal() MOZ_OVERRIDE
  {
    return nsWrapperCache::GetWrapperPreserveColor();
  }
public:

  
  NS_DECL_NSIDOMHTMLOPTIONSCOLLECTION

  
  

  virtual Element* GetElementAt(uint32_t aIndex);
  virtual nsINode* GetParentObject() MOZ_OVERRIDE;

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

  HTMLOptionElement* GetNamedItem(const nsAString& aName)
  {
    bool dummy;
    return NamedGetter(aName, dummy);
  }
  HTMLOptionElement* NamedGetter(const nsAString& aName, bool& aFound);
  virtual Element*
  GetFirstNamedElement(const nsAString& aName, bool& aFound) MOZ_OVERRIDE
  {
    return NamedGetter(aName, aFound);
  }

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
  virtual void GetSupportedNames(unsigned aFlags,
                                 nsTArray<nsString>& aNames) MOZ_OVERRIDE;

private:
  

  nsTArray<nsRefPtr<mozilla::dom::HTMLOptionElement> > mElements;
  
  HTMLSelectElement* mSelect;
};

} 
} 

#endif 
