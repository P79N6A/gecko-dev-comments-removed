



































#ifndef nsMIMEInfoOS2_h_
#define nsMIMEInfoOS2_h_

#include "nsMIMEInfoImpl.h"

class nsMIMEInfoOS2 : public nsMIMEInfoImpl
{
  public:
    nsMIMEInfoOS2(const char* aType = "") : nsMIMEInfoImpl(aType) {}
    nsMIMEInfoOS2(const nsACString& aMIMEType) : nsMIMEInfoImpl(aMIMEType) {}
    nsMIMEInfoOS2(const nsACString& aType, HandlerClass aClass) :
      nsMIMEInfoImpl(aType, aClass) {}
    virtual ~nsMIMEInfoOS2();

    NS_IMETHOD LaunchWithURI(nsIURI* aURI);

#ifdef DEBUG
  protected:
    virtual NS_HIDDEN_(nsresult) LaunchDefaultWithFile(nsIFile* aFile) {
      NS_NOTREACHED("Do not call this, use LaunchWithFile");
      return NS_ERROR_UNEXPECTED;
    }
#endif
};

#endif
