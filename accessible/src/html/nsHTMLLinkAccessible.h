






































#ifndef _nsHTMLLinkAccessible_H_
#define _nsHTMLLinkAccessible_H_

#include "nsHyperTextAccessibleWrap.h"

class nsHTMLLinkAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLLinkAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
 
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetValue(nsAString& aValue);

  NS_IMETHOD GetNumActions(PRUint8 *aNumActions);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  NS_IMETHOD GetURI(PRInt32 aIndex, nsIURI **aURI);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

protected:
  enum { eAction_Jump = 0 };

  


  PRBool IsLinked();
};

#endif  
