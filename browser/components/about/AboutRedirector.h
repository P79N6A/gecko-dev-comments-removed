






































#ifndef AboutRedirector_h__
#define AboutRedirector_h__

#include "nsIAboutModule.h"

namespace mozilla {
namespace browser {

class AboutRedirector : public nsIAboutModule
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIABOUTMODULE
 
  AboutRedirector() {}
  virtual ~AboutRedirector() {}

  static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
};

} 
} 

#endif
