




#ifndef mozilla_a11y_HTMLWin32ObjectAccessible_h_
#define mozilla_a11y_HTMLWin32ObjectAccessible_h_

#include "BaseAccessibles.h"

struct IAccessible;

namespace mozilla {
namespace a11y {

class HTMLWin32ObjectOwnerAccessible : public AccessibleWrap
{
public:
  
  
  
  
  
  
  HTMLWin32ObjectOwnerAccessible(nsIContent* aContent,
                                   DocAccessible* aDoc, void* aHwnd);
  virtual ~HTMLWin32ObjectOwnerAccessible() {}

  
  virtual void Shutdown();

  
  virtual mozilla::a11y::role NativeRole();
  virtual bool NativelyUnavailable() const;

protected:

  
  virtual void CacheChildren();

  void* mHwnd;
  nsRefPtr<Accessible> mNativeAccessible;
};










class HTMLWin32ObjectAccessible : public DummyAccessible
{
public:
  HTMLWin32ObjectAccessible(void* aHwnd);
  virtual ~HTMLWin32ObjectAccessible() {}

  NS_IMETHOD GetNativeInterface(void** aNativeAccessible) MOZ_OVERRIDE;

protected:
  void* mHwnd;
};

} 
} 

#endif
