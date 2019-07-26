





#ifndef mozilla_dom_HTMLObjectElement_h
#define mozilla_dom_HTMLObjectElement_h

#include "nsGenericHTMLElement.h"
#include "nsObjectLoadingContent.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIConstraintValidation.h"
#include "nsIDOMGetSVGDocument.h"

namespace mozilla {
namespace dom {

class HTMLObjectElement MOZ_FINAL : public nsGenericHTMLFormElement
                                  , public nsObjectLoadingContent
                                  , public nsIDOMHTMLObjectElement
                                  , public nsIConstraintValidation
                                  , public nsIDOMGetSVGDocument
{
public:
  using nsIConstraintValidation::GetValidationMessage;

  HTMLObjectElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                    FromParser aFromParser = NOT_FROM_PARSER);
  virtual ~HTMLObjectElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  virtual int32_t TabIndexDefault() MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMHTMLOBJECTELEMENT

  
  NS_DECL_NSIDOMGETSVGDOCUMENT

  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom *aName,
                           nsIAtom *aPrefix, const nsAString &aValue,
                           bool aNotify);
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify);

  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, int32_t *aTabIndex);
  virtual IMEState GetDesiredIMEState();

  
  NS_IMETHOD_(uint32_t) GetType() const
  {
    return NS_FORM_OBJECT;
  }

  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission *aFormSubmission);

  virtual bool IsDisabled() const { return false; }

  virtual void DoneAddingChildren(bool aHaveNotified);
  virtual bool IsDoneAddingChildren();

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom *aAttribute,
                                const nsAString &aValue,
                                nsAttrValue &aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom *aAttribute) const;
  virtual nsEventStates IntrinsicState() const;
  virtual void DestroyContent();

  
  virtual uint32_t GetCapabilities() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  nsresult CopyInnerTo(Element* aDest);

  void StartObjectLoad() { StartObjectLoad(true); }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLObjectElement,
                                           nsGenericHTMLFormElement)

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
private:
  


  NS_HIDDEN_(void) StartObjectLoad(bool aNotify);

  



  bool IsFocusableForTabIndex();
  
  virtual void GetItemValueText(nsAString& text);
  virtual void SetItemValueText(const nsAString& text);

  bool mIsDoneAddingChildren;
};

} 
} 

#endif 
