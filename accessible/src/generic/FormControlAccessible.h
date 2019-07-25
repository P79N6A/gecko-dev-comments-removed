




#ifndef MOZILLA_A11Y_FormControlAccessible_H_
#define MOZILLA_A11Y_FormControlAccessible_H_

#include "BaseAccessibles.h"

namespace mozilla {
namespace a11y {




template<int Max>
class ProgressMeterAccessible : public LeafAccessible
{
public:
  ProgressMeterAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    LeafAccessible(aContent, aDoc)
  {
  }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLEVALUE

  
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsWidget() const;
};




class RadioButtonAccessible : public LeafAccessible
{

public:
  RadioButtonAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual mozilla::a11y::role NativeRole();

  
  virtual PRUint8 ActionCount();

  enum { eAction_Click = 0 };

  
  virtual bool IsWidget() const;
};

} 
} 

#endif

