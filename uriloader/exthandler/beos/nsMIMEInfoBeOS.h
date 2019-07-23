



































#ifndef nsMIMEInfoBeOS_h_
#define nsMIMEInfoBeOS_h_

#include "nsMIMEInfoImpl.h"

class nsMIMEInfoBeOS : public nsMIMEInfoImpl {
  public:
    nsMIMEInfoBeOS(const char* aType = "") : nsMIMEInfoImpl(aType) {}
    nsMIMEInfoBeOS(const nsACString& aMIMEType) : nsMIMEInfoImpl(aMIMEType) {}
    nsMIMEInfoBeOS(const nsACString& aType, HandlerClass aClass) :
      nsMIMEInfoImpl(aType, aClass) {}
    virtual ~nsMIMEInfoBeOS();

  protected:
    virtual NS_HIDDEN_(nsresult) LaunchDefaultWithFile(nsIFile* aFile);
};

#endif
