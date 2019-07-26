




#ifndef mozilla_dom_HTMLFieldSetElement_h
#define mozilla_dom_HTMLFieldSetElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLFieldSetElement.h"
#include "nsIConstraintValidation.h"
#include "mozilla/dom/HTMLFormElement.h"
#include "mozilla/dom/ValidityState.h"

namespace mozilla {
namespace dom {

class HTMLFieldSetElement MOZ_FINAL : public nsGenericHTMLFormElement,
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

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor) MOZ_OVERRIDE;
  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify) MOZ_OVERRIDE;

  virtual nsresult InsertChildAt(nsIContent* aChild, uint32_t aIndex,
                                     bool aNotify) MOZ_OVERRIDE;
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify) MOZ_OVERRIDE;

  
  NS_IMETHOD_(uint32_t) GetType() const MOZ_OVERRIDE { return NS_FORM_FIELDSET; }
  NS_IMETHOD Reset() MOZ_OVERRIDE;
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission) MOZ_OVERRIDE;
  virtual bool IsDisabledForEvents(uint32_t aMessage) MOZ_OVERRIDE;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;
  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

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
  virtual JSObject* WrapNode(JSContext* aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

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

