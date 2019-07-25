





































#ifndef _nsXULColorPickerAccessible_H_
#define _nsXULColorPickerAccessible_H_


#include "nsAccessibleWrap.h"




class nsXULColorPickerTileAccessible : public nsAccessibleWrap
{
public:
  nsXULColorPickerTileAccessible(nsIContent *aContent,
                                 nsIWeakReference *aShell);

  
  NS_IMETHOD GetValue(nsAString& _retval);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};





class nsXULColorPickerAccessible : public nsXULColorPickerTileAccessible
{
public:
  nsXULColorPickerAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual nsresult Init();

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

protected:

  
  virtual void CacheChildren();
};

#endif  
