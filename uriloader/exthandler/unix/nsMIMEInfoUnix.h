






































#ifndef nsMIMEInfoUnix_h_
#define nsMIMEInfoUnix_h_

#include "nsMIMEInfoImpl.h"

class nsMIMEInfoUnix : public nsMIMEInfoImpl
{
public:
  nsMIMEInfoUnix(const char *aMIMEType = "") : nsMIMEInfoImpl(aMIMEType) {}
  nsMIMEInfoUnix(const nsACString& aMIMEType) : nsMIMEInfoImpl(aMIMEType) {}
  nsMIMEInfoUnix(const nsACString& aType, HandlerClass aClass) :
    nsMIMEInfoImpl(aType, aClass) {}
  static bool HandlerExists(const char *aProtocolScheme);

protected:
  NS_IMETHOD GetHasDefaultHandler(bool *_retval);

  virtual NS_HIDDEN_(nsresult) LoadUriInternal(nsIURI *aURI);

  virtual NS_HIDDEN_(nsresult) LaunchDefaultWithFile(nsIFile *aFile);
#if (MOZ_PLATFORM_MAEMO == 5) && defined (MOZ_ENABLE_GNOMEVFS)
  nsresult LaunchDefaultWithDBus(const char *aFilePath);
  NS_IMETHOD GetPossibleApplicationHandlers(nsIMutableArray * *aPossibleAppHandlers);
#endif
#if defined(MOZ_ENABLE_CONTENTACTION)
  NS_IMETHOD GetPossibleApplicationHandlers(nsIMutableArray * *aPossibleAppHandlers);
#endif
};

#endif 
