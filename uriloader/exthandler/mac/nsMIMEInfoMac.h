



































#ifndef nsMIMEInfoMac_h_
#define nsMIMEInfoMac_h_

#include "nsMIMEInfoImpl.h"

class nsMIMEInfoMac : public nsMIMEInfoImpl {
  public:
    nsMIMEInfoMac(const char* aMIMEType = "") : nsMIMEInfoImpl(aMIMEType) {}
    nsMIMEInfoMac(const nsACString& aMIMEType) : nsMIMEInfoImpl(aMIMEType) {}

    NS_IMETHOD LaunchWithFile(nsIFile* aFile);

#ifdef DEBUG
  protected:
    virtual NS_HIDDEN_(nsresult) LaunchDefaultWithFile(nsIFile* aFile) {
      NS_NOTREACHED("do not call this method, use LaunchWithFile");
      return NS_ERROR_UNEXPECTED;
    }
#endif
};


#endif
