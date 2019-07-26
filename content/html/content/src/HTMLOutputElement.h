




#ifndef mozilla_dom_HTMLOutputElement_h
#define mozilla_dom_HTMLOutputElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLOutputElement.h"
#include "nsStubMutationObserver.h"
#include "nsIConstraintValidation.h"

namespace mozilla {
namespace dom {

class HTMLOutputElement : public nsGenericHTMLFormElement,
                          public nsIDOMHTMLOutputElement,
                          public nsStubMutationObserver,
                          public nsIConstraintValidation
{
public:
  using nsIConstraintValidation::GetValidationMessage;

  HTMLOutputElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLOutputElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLOUTPUTELEMENT

  
  NS_IMETHOD_(uint32_t) GetType() const { return NS_FORM_OUTPUT; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);

  virtual bool IsDisabled() const { return false; }

  nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;

  bool ParseAttribute(int32_t aNamespaceID, nsIAtom* aAttribute,
                        const nsAString& aValue, nsAttrValue& aResult);

  nsEventStates IntrinsicState() const;

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               bool aCompileEventHandlers);

  
  
  void DescendantsChanged();

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLOutputElement,
                                           nsGenericHTMLFormElement)

  virtual nsIDOMNode* AsDOMNode() { return this; }
  virtual JSObject*
  WrapNode(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap) MOZ_OVERRIDE;

  
  nsDOMSettableTokenList* HtmlFor();
  
  using nsGenericHTMLFormElement::GetForm;
  
  void SetName(const nsAString& aName, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::name, aName, aRv);
  }

  
  
  void SetDefaultValue(const nsAString& aDefaultValue, ErrorResult& aRv)
  {
    aRv = SetDefaultValue(aDefaultValue);
  }
  
  void SetValue(const nsAString& aValue, ErrorResult& aRv)
  {
    aRv = SetValue(aValue);
  }

  
  
  
  
  using nsIConstraintValidation::CheckValidity;
  

protected:
  enum ValueModeFlag {
    eModeDefault,
    eModeValue
  };

  ValueModeFlag                     mValueModeFlag;
  nsString                          mDefaultValue;
  nsRefPtr<nsDOMSettableTokenList>  mTokenList;
};

} 
} 

#endif
