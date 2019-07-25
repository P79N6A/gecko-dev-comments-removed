





































#ifndef _nsTextAccessible_H_
#define _nsTextAccessible_H_

#include "nsBaseWidgetAccessible.h"




class nsTextAccessible : public nsLinkableAccessible
{
public:
  nsTextAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual PRUint32 NativeRole();
  virtual nsresult AppendTextTo(nsAString& aText, PRUint32 aStartOffset,
                                PRUint32 aLength);

protected:

  
  virtual void CacheChildren();
};


#endif

