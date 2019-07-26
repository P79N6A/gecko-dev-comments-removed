





































#ifndef _nsFormControlAccessible_H_
#define _nsFormControlAccessible_H_

#include "nsBaseWidgetAccessible.h"

typedef nsLeafAccessible nsFormControlAccessible;




template<int Max>
class ProgressMeterAccessible: public nsFormControlAccessible
{
public:
  ProgressMeterAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
    nsFormControlAccessible(aContent, aDoc)
  {
  }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLEVALUE

  
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsWidget() const;
};




class nsRadioButtonAccessible : public nsFormControlAccessible
{

public:
  nsRadioButtonAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual mozilla::a11y::role NativeRole();

  
  virtual PRUint8 ActionCount();

  enum { eAction_Click = 0 };

  
  virtual bool IsWidget() const;
};


#endif

