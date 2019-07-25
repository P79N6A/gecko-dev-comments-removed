




#ifndef _nsHTMLWin32ObjectAccessible_H_
#define _nsHTMLWin32ObjectAccessible_H_

#include "BaseAccessibles.h"

struct IAccessible;

class nsHTMLWin32ObjectOwnerAccessible : public AccessibleWrap
{
public:
  
  
  
  
  
  
  nsHTMLWin32ObjectOwnerAccessible(nsIContent* aContent,
                                   DocAccessible* aDoc, void* aHwnd);
  virtual ~nsHTMLWin32ObjectOwnerAccessible() {}

  
  virtual void Shutdown();

  
  virtual mozilla::a11y::role NativeRole();
  virtual bool NativelyUnavailable() const;

protected:

  
  virtual void CacheChildren();

  void* mHwnd;
  nsRefPtr<Accessible> mNativeAccessible;
};










class nsHTMLWin32ObjectAccessible : public mozilla::a11y::LeafAccessible
{
public:

  nsHTMLWin32ObjectAccessible(void* aHwnd);
  virtual ~nsHTMLWin32ObjectAccessible() {}

  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD GetNativeInterface(void** aNativeAccessible) MOZ_OVERRIDE;

protected:
  void* mHwnd;
};

#endif  
