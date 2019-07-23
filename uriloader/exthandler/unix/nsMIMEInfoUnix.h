






































#ifndef nsMIMEInfoUnix_h_
#define nsMIMEInfoUnix_h_

#include "nsMIMEInfoImpl.h"

class nsMIMEInfoUnix : public nsMIMEInfoBase
{
protected:
  virtual NS_HIDDEN_(nsresult) LoadUriInternal(nsIURI *aURI);
};

#endif 
