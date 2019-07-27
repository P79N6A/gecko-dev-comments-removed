





#ifndef mozilla_dom_HTMLFormControlsCollection_h
#define mozilla_dom_HTMLFormControlsCollection_h

#include "mozilla/dom/Element.h" 
#include "nsIHTMLCollection.h"
#include "nsInterfaceHashtable.h"
#include "nsTArray.h"
#include "nsWrapperCache.h"

class nsGenericHTMLFormElement;
class nsIFormControl;

namespace mozilla {
namespace dom {
class HTMLFormElement;
class HTMLImageElement;
class OwningRadioNodeListOrElement;
template<typename> struct Nullable;

class HTMLFormControlsCollection : public nsIHTMLCollection
                                 , public nsWrapperCache
{
public:
  explicit HTMLFormControlsCollection(HTMLFormElement* aForm);

  void DropFormReference();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMHTMLCOLLECTION

  virtual Element* GetElementAt(uint32_t index);
  virtual nsINode* GetParentObject() MOZ_OVERRIDE;

  virtual Element*
  GetFirstNamedElement(const nsAString& aName, bool& aFound) MOZ_OVERRIDE;

  void
  NamedGetter(const nsAString& aName,
              bool& aFound,
              Nullable<OwningRadioNodeListOrElement>& aResult);
  void
  NamedItem(const nsAString& aName,
            Nullable<OwningRadioNodeListOrElement>& aResult)
  {
    bool dummy;
    NamedGetter(aName, dummy, aResult);
  }
  virtual void GetSupportedNames(unsigned aFlags,
                                 nsTArray<nsString>& aNames) MOZ_OVERRIDE;

  nsresult AddElementToTable(nsGenericHTMLFormElement* aChild,
                             const nsAString& aName);
  nsresult AddImageElementToTable(HTMLImageElement* aChild,
                                  const nsAString& aName);
  nsresult RemoveElementFromTable(nsGenericHTMLFormElement* aChild,
                                  const nsAString& aName);
  nsresult IndexOfControl(nsIFormControl* aControl,
                          int32_t* aIndex);

  nsISupports* NamedItemInternal(const nsAString& aName, bool aFlushContent);
  
  








  nsresult GetSortedControls(nsTArray<nsGenericHTMLFormElement*>& aControls) const;

  
  using nsWrapperCache::GetWrapperPreserveColor;
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
protected:
  virtual ~HTMLFormControlsCollection();
  virtual JSObject* GetWrapperPreserveColorInternal() MOZ_OVERRIDE
  {
    return nsWrapperCache::GetWrapperPreserveColor();
  }
public:

  static bool ShouldBeInElements(nsIFormControl* aFormControl);

  HTMLFormElement* mForm;  

  nsTArray<nsGenericHTMLFormElement*> mElements;  

  
  
  
  

  nsTArray<nsGenericHTMLFormElement*> mNotInElements; 

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(HTMLFormControlsCollection)

protected:
  
  void Clear();

  
  void FlushPendingNotifications();
  
  
  
  
  
  

  nsInterfaceHashtable<nsStringHashKey,nsISupports> mNameLookupTable;
};

} 
} 

#endif 
