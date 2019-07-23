



































#ifndef nsMIMEInfoOS2_h_
#define nsMIMEInfoOS2_h_

#include "nsMIMEInfoImpl.h"

#include "nsIPref.h" 
#include "nsNetCID.h"
#include "nsEscape.h"
static NS_DEFINE_CID(kStandardURLCID, NS_STANDARDURL_CID); 

#define INCL_DOS
#define INCL_DOSMISC
#define INCL_DOSERRORS
#define INCL_WINSHELLDATA
#include <os2.h>

#define MAXINIPARAMLENGTH 1024 // max length of OS/2 INI key for application parameters

#define LOG(args) PR_LOG(mLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(mLog, PR_LOG_DEBUG)

class nsMIMEInfoOS2 : public nsMIMEInfoImpl
{
  public:
    nsMIMEInfoOS2(const char* aType = "") : nsMIMEInfoImpl(aType) {}
    nsMIMEInfoOS2(const nsACString& aMIMEType) : nsMIMEInfoImpl(aMIMEType) {}
    nsMIMEInfoOS2(const nsACString& aType, HandlerClass aClass) :
      nsMIMEInfoImpl(aType, aClass) {}
    virtual ~nsMIMEInfoOS2();

    NS_IMETHOD LaunchWithURI(nsIURI* aURI);
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
