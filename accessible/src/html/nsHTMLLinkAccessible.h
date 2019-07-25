






































#ifndef _nsHTMLLinkAccessible_H_
#define _nsHTMLLinkAccessible_H_

#include "nsHyperTextAccessibleWrap.h"

class nsHTMLLinkAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLLinkAccessible(nsIContent *aContent, nsIWeakReference *aShell);
 
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetValue(nsAString& aValue);

  NS_IMETHOD GetNumActions(PRUint8 *aNumActions);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsHyperLink();
  virtual already_AddRefed<nsIURI> GetAnchorURI(PRUint32 aAnchorIndex);

protected:
  enum { eAction_Jump = 0 };

  


  PRBool IsLinked();
};

#endif  
