




#ifndef MOZILLA_A11Y_FormControlAccessible_H_
#define MOZILLA_A11Y_FormControlAccessible_H_

#include "nsBaseWidgetAccessible.h"

namespace mozilla {
namespace a11y {




template<int Max>
class ProgressMeterAccessible : public nsLeafAccessible
{
public:
  ProgressMeterAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
    nsLeafAccessible(aContent, aDoc)
  {
  }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLEVALUE

  
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsWidget() const;
};




class RadioButtonAccessible : public nsLeafAccessible
{

public:
  RadioButtonAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
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

