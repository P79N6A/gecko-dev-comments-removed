



































#ifndef nsMIMEInfoMac_h_
#define nsMIMEInfoMac_h_

#include "nsMIMEInfoImpl.h"

class nsMIMEInfoMac : public nsMIMEInfoImpl {
  public:
    nsMIMEInfoMac(const char* aMIMEType = "") : nsMIMEInfoImpl(aMIMEType) {}
    nsMIMEInfoMac(const nsACString& aMIMEType) : nsMIMEInfoImpl(aMIMEType) {}
    nsMIMEInfoMac(const nsACString& aType, HandlerClass aClass) :
      nsMIMEInfoImpl(aType, aClass) {}

    NS_IMETHOD LaunchWithURI(nsIURI* aURI);
    NS_IMETHOD GetHasDefaultHandler(PRBool *_retval);
  protected:
    virtual NS_HIDDEN_(nsresult) LoadUriInternal(nsIURI *aURI);
#ifdef DEBUG
    virtual NS_HIDDEN_(nsresult) LaunchDefaultWithFile(nsIFile* aFile) {
      NS_NOTREACHED("do not call this method, use LaunchWithFile");
      return NS_ERROR_UNEXPECTED;
    }
#endif
};


#endif
