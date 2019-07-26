




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
  virtual uint64_t NativeState();

  
  virtual bool IsWidget() const;
};




class RadioButtonAccessible : public LeafAccessible
{

public:
  RadioButtonAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t aIndex);

  
  virtual mozilla::a11y::role NativeRole();

  
  virtual uint8_t ActionCount();

  enum { eAction_Click = 0 };

  
  virtual bool IsWidget() const;
};

} 
} 

#endif

