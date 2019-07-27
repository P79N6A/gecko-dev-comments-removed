





#ifndef nsWeakReference_h__
#define nsWeakReference_h__





#include "nsIWeakReferenceUtils.h"

class nsWeakReference;

class nsSupportsWeakReference : public nsISupportsWeakReference
{
public:
  nsSupportsWeakReference() : mProxy(0) {}

  NS_DECL_NSISUPPORTSWEAKREFERENCE

protected:
  inline ~nsSupportsWeakReference();

private:
  friend class nsWeakReference;

  
  void NoticeProxyDestruction() { mProxy = 0; }

  nsWeakReference* mProxy;

protected:

  void ClearWeakReferences();
  bool HasWeakReferences() const { return mProxy != 0; }
};

inline
nsSupportsWeakReference::~nsSupportsWeakReference()
{
  ClearWeakReferences();
}

#endif
