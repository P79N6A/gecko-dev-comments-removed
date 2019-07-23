




































#ifndef __nsHTMLFormControlAccessibleWrap_h__
#define __nsHTMLFormControlAccessibleWrap_h__

#include "nsHTMLFormControlAccessible.h"

class nsHTMLRadioButtonAccessibleWrap : public nsHTMLRadioButtonAccessible
{
public:
  nsHTMLRadioButtonAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference *aShell);
  virtual ~nsHTMLRadioButtonAccessibleWrap() {}
  NS_IMETHOD GetDescription(nsAString& aName);
};

#endif
