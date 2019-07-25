





































#ifndef _nsHTMLWin32ObjectAccessible_H_
#define _nsHTMLWin32ObjectAccessible_H_

#include "nsBaseWidgetAccessible.h"

struct IAccessible;

class nsHTMLWin32ObjectOwnerAccessible : public nsAccessibleWrap
{
public:
  
  
  
  
  
  
  nsHTMLWin32ObjectOwnerAccessible(nsIContent* aContent,
                                   nsDocAccessible* aDoc, void* aHwnd);
  virtual ~nsHTMLWin32ObjectOwnerAccessible() {}

  
  virtual void Shutdown();

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

protected:

  
  virtual void CacheChildren();

  void* mHwnd;
  nsRefPtr<nsAccessible> mNativeAccessible;
};










class nsHTMLWin32ObjectAccessible : public nsLeafAccessible
{
public:

  nsHTMLWin32ObjectAccessible(void *aHwnd);
  virtual ~nsHTMLWin32ObjectAccessible() {}

  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD GetNativeInterface(void** aNativeAccessible) MOZ_OVERRIDE;

protected:
  void* mHwnd;
};

#endif  
