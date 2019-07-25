





































#ifndef _nsFormControlAccessible_H_
#define _nsFormControlAccessible_H_

#include "nsBaseWidgetAccessible.h"

typedef nsLeafAccessible nsFormControlAccessible;




template<int Max>
class ProgressMeterAccessible: public nsFormControlAccessible
{
public:
  ProgressMeterAccessible(nsIContent* aContent, nsIWeakReference* aShell) :
    nsFormControlAccessible(aContent, aShell)
  {
  }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLEVALUE

  
  NS_IMETHOD GetValue(nsAString &aValue);

  
  virtual PRUint32 NativeRole();
};




class nsRadioButtonAccessible : public nsFormControlAccessible
{

public:
  nsRadioButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual PRUint32 NativeRole();

  enum { eAction_Click = 0 };
};


#endif

