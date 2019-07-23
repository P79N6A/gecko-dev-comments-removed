





































#ifndef _nsTextAccessible_H_
#define _nsTextAccessible_H_

#include "nsBaseWidgetAccessible.h"




class nsTextAccessible : public nsLinkableAccessible
{
public:
  nsTextAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult AppendTextTo(nsAString& aText, PRUint32 aStartOffset,
                                PRUint32 aLength);

protected:

  
  virtual void CacheChildren();
};


#endif

