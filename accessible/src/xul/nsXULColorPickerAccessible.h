





































#ifndef _nsXULColorPickerAccessible_H_
#define _nsXULColorPickerAccessible_H_


#include "nsFormControlAccessible.h"

class nsXULColorPickerTileAccessible : public nsFormControlAccessible
{
public:
  nsXULColorPickerTileAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetName(nsAString& _retval);
  NS_IMETHOD GetValue(nsAString& _retval);
};

class nsXULColorPickerAccessible : public nsXULColorPickerTileAccessible
{
public:
  nsXULColorPickerAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetRole(PRUint32 *_retval);
};

#endif  
