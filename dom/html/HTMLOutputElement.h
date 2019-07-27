




#ifndef mozilla_dom_HTMLOutputElement_h
#define mozilla_dom_HTMLOutputElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsStubMutationObserver.h"
#include "nsIConstraintValidation.h"

namespace mozilla {
namespace dom {

class HTMLOutputElement MOZ_FINAL : public nsGenericHTMLFormElement,
                                    public nsStubMutationObserver,
                                    public nsIConstraintValidation
{
public:
  using nsIConstraintValidation::GetValidationMessage;

  explicit HTMLOutputElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                             FromParser aFromParser = NOT_FROM_PARSER);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD_(uint32_t) GetType() const { return NS_FORM_OUTPUT; }
  NS_IMETHOD Reset() MOZ_OVERRIDE;
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission) MOZ_OVERRIDE;

  virtual bool IsDisabled() const MOZ_OVERRIDE { return false; }

  nsresult Clone(mozilla::dom::NodeInfo* aNodeInfo, nsINode** aResult) const MOZ_OVERRIDE;

  bool ParseAttribute(int32_t aNamespaceID, nsIAtom* aAttribute,
                        const nsAString& aValue, nsAttrValue& aResult) MOZ_OVERRIDE;

  virtual void DoneAddingChildren(bool aHaveNotified) MOZ_OVERRIDE;

  EventStates IntrinsicState() const MOZ_OVERRIDE;

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               bool aCompileEventHandlers) MOZ_OVERRIDE;

  
  
  void DescendantsChanged();

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLOutputElement,
                                           nsGenericHTMLFormElement)

  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;

  
  nsDOMSettableTokenList* HtmlFor();
  
  void GetName(nsAString& aName)
  {
    GetHTMLAttr(nsGkAtoms::name, aName);
  }

  void SetName(const nsAString& aName, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::name, aName, aRv);
  }

  void GetType(nsAString& aType)
  {
    aType.AssignLiteral("output");
  }

  void GetDefaultValue(nsAString& aDefaultValue)
  {
    aDefaultValue = mDefaultValue;
  }

  void SetDefaultValue(const nsAString& aDefaultValue, ErrorResult& aRv);

  void GetValue(nsAString& aValue);
  void SetValue(const nsAString& aValue, ErrorResult& aRv);

  
  
  
  
  void SetCustomValidity(const nsAString& aError);

protected:
  virtual ~HTMLOutputElement();

  enum ValueModeFlag {
    eModeDefault,
    eModeValue
  };

  ValueModeFlag                     mValueModeFlag;
  bool                              mIsDoneAddingChildren;
  nsString                          mDefaultValue;
  nsRefPtr<nsDOMSettableTokenList>  mTokenList;
};

} 
} 

#endif 
