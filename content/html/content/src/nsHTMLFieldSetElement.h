




#ifndef nsHTMLFieldSetElement_h___
#define nsHTMLFieldSetElement_h___

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLFieldSetElement.h"
#include "nsIConstraintValidation.h"


class nsHTMLFieldSetElement : public nsGenericHTMLFormElement,
                              public nsIDOMHTMLFieldSetElement,
                              public nsIConstraintValidation
{
public:
  using nsIConstraintValidation::GetValidationMessage;

  nsHTMLFieldSetElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLFieldSetElement();

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(nsHTMLFieldSetElement, fieldset)

  
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
  virtual nsXPCClassInfo* GetClassInfo();
  virtual nsIDOMNode* AsDOMNode() { return this; }

  const nsIContent* GetFirstLegend() const { return mFirstLegend; }

  void AddElement(nsGenericHTMLFormElement* aElement) {
    mDependentElements.AppendElement(aElement);
  }

  void RemoveElement(nsGenericHTMLFormElement* aElement) {
    mDependentElements.RemoveElement(aElement);
  }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHTMLFieldSetElement,
                                           nsGenericHTMLFormElement)
private:

  



  void NotifyElementsForFirstLegendChange(bool aNotify);

  
  static bool MatchListedElements(nsIContent* aContent, int32_t aNamespaceID,
                                    nsIAtom* aAtom, void* aData);

  
  nsRefPtr<nsContentList> mElements;

  
  nsTArray<nsGenericHTMLFormElement*> mDependentElements;

  nsIContent* mFirstLegend;
};

#endif 

