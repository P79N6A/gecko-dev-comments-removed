





































#ifndef NS_ISMILANIMATIONELEMENT_H_
#define NS_ISMILANIMATIONELEMENT_H_

#include "nsISupports.h"





#define NS_ISMILANIMATIONELEMENT_IID \
{ 0x5c891601, 0x47aa, 0x4230,        \
  { 0xb8, 0xdc, 0xb9, 0x26, 0xd1, 0xe7, 0xd7, 0xf4 } }

class nsISMILAttr;
class nsSMILAnimationFunction;
class nsSMILTimeContainer;
class nsSMILTimedElement;
class nsIAtom;
class nsAttrValue;

namespace mozilla {
namespace dom {
class Element;
} 
} 

enum nsSMILTargetAttrType {
  eSMILTargetAttrType_auto,
  eSMILTargetAttrType_CSS,
  eSMILTargetAttrType_XML
};

class nsISMILAnimationElement : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISMILANIMATIONELEMENT_IID)

  


  virtual const mozilla::dom::Element& AsElement() const = 0;

  


  virtual mozilla::dom::Element& AsElement() = 0;

  










  virtual const nsAttrValue* GetAnimAttr(nsIAtom* aName) const = 0;

  








  virtual PRBool GetAnimAttr(nsIAtom* aAttName, nsAString& aResult) const = 0;

  


  virtual PRBool HasAnimAttr(nsIAtom* aAttName) const = 0;

  


  virtual mozilla::dom::Element* GetTargetElementContent() = 0;

  


  virtual nsIAtom* GetTargetAttributeName() const = 0;

  


  virtual nsSMILTargetAttrType GetTargetAttributeType() const = 0;

  




  virtual nsSMILAnimationFunction& AnimationFunction() = 0;

  




  virtual nsSMILTimedElement& TimedElement() = 0;

  



  virtual nsSMILTimeContainer* GetTimeContainer() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISMILAnimationElement,
                              NS_ISMILANIMATIONELEMENT_IID)

#endif 
