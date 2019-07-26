




#ifndef mozilla_dom_HTMLFieldSetElement_h
#define mozilla_dom_HTMLFieldSetElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLFieldSetElement.h"
#include "nsIConstraintValidation.h"
#include "nsHTMLFormElement.h"
#include "mozilla/dom/ValidityState.h"

namespace mozilla {
namespace dom {

class HTMLFieldSetElement : public nsGenericHTMLFormElement,
                            public nsIDOMHTMLFieldSetElement,
                            public nsIConstraintValidation
{
public:
  using nsGenericHTMLFormElement::GetForm;
  using nsIConstraintValidation::Validity;
  using nsIConstraintValidation::CheckValidity;
  using nsIConstraintValidation::GetValidationMessage;

  HTMLFieldSetElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLFieldSetElement();

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLFieldSetElement, fieldset)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLFIELDSETELEMENT

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify);

  virtual nsresult InsertChildAt(nsIContent* aChild, uint32_t aIndex,
                                     bool aNotify);
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify);

  
  NS_IMETHOD_(uint32_t) GetType() const { return NS_FORM_FIELDSET; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);
  virtual bool IsDisabledForEvents(uint32_t aMessage);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsIDOMNode* AsDOMNode() { return this; }

  const nsIContent* GetFirstLegend() const { return mFirstLegend; }

  void AddElement(nsGenericHTMLFormElement* aElement) {
    mDependentElements.AppendElement(aElement);
  }

  void RemoveElement(nsGenericHTMLFormElement* aElement) {
    mDependentElements.RemoveElement(aElement);
  }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLFieldSetElement,
                                           nsGenericHTMLFormElement)

  
  bool Disabled() const
  {
    return GetBoolAttr(nsGkAtoms::disabled);
  }
  void SetDisabled(bool aValue, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::disabled, aValue, aRv);
  }

  

  void SetName(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::name, aValue, aRv);
  }

  

  nsIHTMLCollection* Elements();

  

  

  

  

  

protected:
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

private:

  



  void NotifyElementsForFirstLegendChange(bool aNotify);

  
  static bool MatchListedElements(nsIContent* aContent, int32_t aNamespaceID,
                                    nsIAtom* aAtom, void* aData);

  
  nsRefPtr<nsContentList> mElements;

  
  nsTArray<nsGenericHTMLFormElement*> mDependentElements;

  nsIContent* mFirstLegend;
};

} 
} 

#endif 

