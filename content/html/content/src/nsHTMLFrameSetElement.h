






































#ifndef nsHTMLFrameSetElement_h
#define nsHTMLFrameSetElement_h

#include "nsISupports.h"
#include "nsIDOMHTMLFrameSetElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsIHTMLDocument.h"
#include "nsIDocument.h"





enum nsFramesetUnit {
  eFramesetUnit_Fixed = 0,
  eFramesetUnit_Percent,
  eFramesetUnit_Relative
};





struct nsFramesetSpec {
  nsFramesetUnit mUnit;
  nscoord        mValue;
};





#define NS_MAX_FRAMESET_SPEC_COUNT 16000



class nsHTMLFrameSetElement : public nsGenericHTMLElement,
                              public nsIDOMHTMLFrameSetElement
{
public:
  nsHTMLFrameSetElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLFrameSetElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLFRAMESETELEMENT

  
  
#define EVENT(name_, id_, type_, struct_)
#define FORWARDED_EVENT(name_, id_, type_, struct_)                     \
    NS_IMETHOD GetOn##name_(JSContext *cx, jsval *vp);            \
    NS_IMETHOD SetOn##name_(JSContext *cx, const jsval &v);
#include "nsEventNameList.h"
#undef FORWARDED_EVENT
#undef EVENT

  
  
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);

   






  nsresult GetRowSpec(PRInt32 *aNumValues, const nsFramesetSpec** aSpecs);
   






  nsresult GetColSpec(PRInt32 *aNumValues, const nsFramesetSpec** aSpecs);


  virtual bool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsXPCClassInfo* GetClassInfo();
  static nsHTMLFrameSetElement* FromContent(nsIContent *aContent)
  {
    if (aContent->IsHTML(nsGkAtoms::frameset))
      return static_cast<nsHTMLFrameSetElement*>(aContent);
    return nsnull;
  }

private:
  nsresult ParseRowCol(const nsAString& aValue,
                       PRInt32&         aNumSpecs,
                       nsFramesetSpec** aSpecs);

  


  PRInt32          mNumRows;
  


  PRInt32          mNumCols;
  



  nsChangeHint      mCurrentRowColHint;
  


  nsAutoArrayPtr<nsFramesetSpec>  mRowSpecs; 
  


  nsAutoArrayPtr<nsFramesetSpec>  mColSpecs; 
};

#endif 
