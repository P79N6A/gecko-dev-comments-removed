





#ifndef nsMIMEInfoUnix_h_
#define nsMIMEInfoUnix_h_

#include "nsMIMEInfoImpl.h"

class nsMIMEInfoUnix : public nsMIMEInfoImpl
{
public:
  explicit nsMIMEInfoUnix(const char *aMIMEType = "") : nsMIMEInfoImpl(aMIMEType) {}
  explicit nsMIMEInfoUnix(const nsACString& aMIMEType) : nsMIMEInfoImpl(aMIMEType) {}
  nsMIMEInfoUnix(const nsACString& aType, HandlerClass aClass) :
    nsMIMEInfoImpl(aType, aClass) {}
  static bool HandlerExists(const char *aProtocolScheme);

protected:
  NS_IMETHOD GetHasDefaultHandler(bool *_retval);

  virtual nsresult LoadUriInternal(nsIURI *aURI);

  virtual nsresult LaunchDefaultWithFile(nsIFile *aFile);
#if defined(MOZ_ENABLE_CONTENTACTION)
  NS_IMETHOD GetPossibleApplicationHandlers(nsIMutableArray * *aPossibleAppHandlers);
#endif
};

#endif 
