



#ifndef nsMIMEInfoMac_h_
#define nsMIMEInfoMac_h_

#include "nsMIMEInfoImpl.h"

class nsMIMEInfoMac : public nsMIMEInfoImpl {
  public:
    explicit nsMIMEInfoMac(const char* aMIMEType = "") : nsMIMEInfoImpl(aMIMEType) {}
    explicit nsMIMEInfoMac(const nsACString& aMIMEType) : nsMIMEInfoImpl(aMIMEType) {}
    nsMIMEInfoMac(const nsACString& aType, HandlerClass aClass) :
      nsMIMEInfoImpl(aType, aClass) {}

    NS_IMETHOD LaunchWithFile(nsIFile* aFile);
  protected:
    virtual nsresult LoadUriInternal(nsIURI *aURI);
#ifdef DEBUG
    virtual nsresult LaunchDefaultWithFile(nsIFile* aFile) {
      NS_NOTREACHED("do not call this method, use LaunchWithFile");
      return NS_ERROR_UNEXPECTED;
    }
#endif
    static nsresult OpenApplicationWithURI(nsIFile *aApplication, 
                                                       const nsCString& aURI);
                                                       
    NS_IMETHOD GetDefaultDescription(nsAString& aDefaultDescription);
    
};


#endif
