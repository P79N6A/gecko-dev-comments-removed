





































#ifndef _nsXULColorPickerAccessible_H_
#define _nsXULColorPickerAccessible_H_


#include "nsFormControlAccessible.h"

class nsXULColorPickerTileAccessible : public nsFormControlAccessible
{
public:
  nsXULColorPickerTileAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetValue(nsAString& _retval);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsXULColorPickerAccessible : public nsXULColorPickerTileAccessible
{
public:
  nsXULColorPickerAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};

#endif  
