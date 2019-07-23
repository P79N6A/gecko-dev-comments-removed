




































#include "nsIURI.h"
#include "nsCOMPtr.h"

class nsMIMEInfoBase;

class nsGNOMERegistry
{
 public:
  static void Startup();

  static PRBool HandlerExists(const char *aProtocolScheme);

  static nsresult LoadURL(nsIURI *aURL);

  static void GetAppDescForScheme(const nsACString& aScheme,
                                  nsAString& aDesc);

  static already_AddRefed<nsMIMEInfoBase> GetFromExtension(const char *aFileExt);

  static already_AddRefed<nsMIMEInfoBase> GetFromType(const char *aMIMEType);
};
