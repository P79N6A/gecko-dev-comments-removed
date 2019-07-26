





#ifndef nsWeakReference_h__
#define nsWeakReference_h__





#include "nsIWeakReferenceUtils.h"

class nsWeakReference;



#undef  IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY NS_COM_GLUE

class NS_COM_GLUE nsSupportsWeakReference : public nsISupportsWeakReference
  {
    public:
      nsSupportsWeakReference()
          : mProxy(0)
        {
          
        }

      NS_DECL_NSISUPPORTSWEAKREFERENCE

    protected:
      inline ~nsSupportsWeakReference();

    private:
      friend class nsWeakReference;

      void
      NoticeProxyDestruction()
          
        {
          mProxy = 0;
        }

      nsWeakReference* mProxy;

		protected:

			void ClearWeakReferences();
			bool HasWeakReferences() const {return mProxy != 0;}
  };

#undef  IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY

inline
nsSupportsWeakReference::~nsSupportsWeakReference()
  {
  	ClearWeakReferences();
  }

#endif
