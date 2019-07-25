





































#ifndef _nsFormControlAccessible_H_
#define _nsFormControlAccessible_H_

#include "nsBaseWidgetAccessible.h"

typedef nsLeafAccessible nsFormControlAccessible;




class nsRadioButtonAccessible : public nsFormControlAccessible
{

public:
  nsRadioButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);

  enum { eAction_Click = 0 };
};


#endif

