





































#ifndef _nsTextAccessible_H_
#define _nsTextAccessible_H_

#include "nsBaseWidgetAccessible.h"

 




class nsTextAccessible : public nsLinkableAccessible
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  nsTextAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetFirstChild(nsIAccessible **_retval);
  NS_IMETHOD GetLastChild(nsIAccessible **_retval);
  NS_IMETHOD GetChildCount(PRInt32 *_retval);

  
  NS_IMETHOD AppendTextTo(nsAString& aText, PRUint32 aStartOffset, PRUint32 aLength);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};


#endif

