





































#ifndef NS_ISMILANIMATIONELEMENT_H_
#define NS_ISMILANIMATIONELEMENT_H_

#include "nsISupports.h"
#include "nsIContent.h"

class nsISMILAttr;





#define NS_ISMILANIMATIONELEMENT_IID \
{ 0x70ac6eed, 0x0dba, 0x4c11, { 0xa6, 0xc5, 0x15, 0x73, 0xbc, 0x2f, 0x1a, 0xd8 } }

class nsSMILAnimationFunction;
class nsSMILTimeContainer;
class nsSMILTimedElement;

enum nsSMILTargetAttrType {
  eSMILTargetAttrType_auto,
  eSMILTargetAttrType_CSS,
  eSMILTargetAttrType_XML
};

class nsISMILAnimationElement : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISMILANIMATIONELEMENT_IID)

  


  virtual const nsIContent& Content() const = 0;

  


  virtual nsIContent& Content() = 0;

  










  virtual const nsAttrValue* GetAnimAttr(nsIAtom* aName) const = 0;

  








  PRBool GetAnimAttr(nsIAtom* aAttName, nsAString &aResult) const
  {
    return Content().GetAttr(kNameSpaceID_None, aAttName, aResult);
  }

  


  PRBool HasAnimAttr(nsIAtom* aAttName) const
  {
    return Content().HasAttr(kNameSpaceID_None, aAttName);
  }

  


  virtual nsIContent* GetTargetElementContent() = 0;

  


  virtual nsIAtom* GetTargetAttributeName() const = 0;

  


  virtual nsSMILTargetAttrType GetTargetAttributeType() const = 0;

  




  virtual nsSMILAnimationFunction& AnimationFunction() = 0;

  




  virtual nsSMILTimedElement& TimedElement() = 0;

  



  virtual nsSMILTimeContainer* GetTimeContainer() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISMILAnimationElement,
                              NS_ISMILANIMATIONELEMENT_IID)

#endif 
