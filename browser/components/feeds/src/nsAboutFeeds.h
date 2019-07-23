






































#ifndef nsAboutFeeds_h__
#define nsAboutFeeds_h__

#include "nsIAboutModule.h"

class nsAboutFeeds : public nsIAboutModule
{
public:
  NS_DECL_ISUPPORTS

  NS_DECL_NSIABOUTMODULE

  nsAboutFeeds() { }
  virtual ~nsAboutFeeds() { }
  
  static NS_METHOD
    Create(nsISupports* outer, REFNSIID iid, void** result);
protected:
};

#endif
