





#ifndef mozilla_dom_HTMLFormControlsCollection_h
#define mozilla_dom_HTMLFormControlsCollection_h

#include "nsIHTMLCollection.h"
#include "nsWrapperCache.h"
#include "nsTArray.h"
#include "nsInterfaceHashtable.h"

class nsGenericHTMLFormElement;
class nsIFormControl;

namespace mozilla {
namespace dom {
class HTMLFormElement;
class HTMLImageElement;

class HTMLFormControlsCollection : public nsIHTMLCollection
                                 , public nsWrapperCache
{
public:
  HTMLFormControlsCollection(HTMLFormElement* aForm);
  virtual ~HTMLFormControlsCollection();

  void DropFormReference();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMHTMLCOLLECTION

  virtual Element* GetElementAt(uint32_t index);
  virtual nsINode* GetParentObject() MOZ_OVERRIDE;

  virtual JSObject* NamedItem(JSContext* aCx, const nsAString& aName,
                              ErrorResult& aError);
  virtual void GetSupportedNames(nsTArray<nsString>& aNames);

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

  
  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

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
