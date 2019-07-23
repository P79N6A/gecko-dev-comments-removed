



































#ifndef nsMIMEInfoOS2_h_
#define nsMIMEInfoOS2_h_

#include "nsMIMEInfoImpl.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsNetCID.h"
#include "nsEscape.h"

#define INCL_DOS
#define INCL_DOSMISC
#define INCL_DOSERRORS
#define INCL_WINSHELLDATA
#include <os2.h>

class nsMIMEInfoOS2 : public nsMIMEInfoImpl
{
  public:
    nsMIMEInfoOS2(const char* aType = "") : nsMIMEInfoImpl(aType) {}
    nsMIMEInfoOS2(const nsACString& aMIMEType) : nsMIMEInfoImpl(aMIMEType) {}
    nsMIMEInfoOS2(const nsACString& aType, HandlerClass aClass) :
      nsMIMEInfoImpl(aType, aClass) {}
    virtual ~nsMIMEInfoOS2();

    NS_IMETHOD LaunchWithURI(nsIURI* aURI,
                             nsIInterfaceRequestor* aWindowContext);
  protected:
    virtual NS_HIDDEN_(nsresult) LoadUriInternal(nsIURI *aURI);
#ifdef DEBUG
    virtual NS_HIDDEN_(nsresult) LaunchDefaultWithFile(nsIFile* aFile) {
      NS_NOTREACHED("Do not call this, use LaunchWithFile");
      return NS_ERROR_UNEXPECTED;
    }
#endif
};

#endif
