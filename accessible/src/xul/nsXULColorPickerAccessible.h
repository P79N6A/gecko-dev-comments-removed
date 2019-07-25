





































#ifndef _nsXULColorPickerAccessible_H_
#define _nsXULColorPickerAccessible_H_


#include "nsAccessibleWrap.h"




class nsXULColorPickerTileAccessible : public nsAccessibleWrap
{
public:
  nsXULColorPickerTileAccessible(nsIContent *aContent,
                                 nsIWeakReference *aShell);

  
  NS_IMETHOD GetValue(nsAString& _retval);

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();
};





class nsXULColorPickerAccessible : public nsXULColorPickerTileAccessible
{
public:
  nsXULColorPickerAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();

protected:

  
  virtual void CacheChildren();
};

#endif  
