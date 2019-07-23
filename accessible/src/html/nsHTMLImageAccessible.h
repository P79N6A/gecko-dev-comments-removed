





































#ifndef _nsHTMLImageAccessible_H_
#define _nsHTMLImageAccessible_H_

#include "nsBaseWidgetAccessible.h"
#include "nsIAccessibleImage.h"






class nsHTMLImageAccessible : public nsLinkableAccessible,
                              public nsIAccessibleImage
{
public:
  nsHTMLImageAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetNumActions(PRUint8 *aNumActions);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  NS_DECL_NSIACCESSIBLEIMAGE

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

private:
  




  PRBool HasLongDesc();
  
  








  PRBool IsValidLongDescIndex(PRUint8 aIndex);
};

#endif

