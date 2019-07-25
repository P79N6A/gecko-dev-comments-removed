




































#include "nsIURI.h"
#include "nsCOMPtr.h"

class nsMIMEInfoBase;

class nsGNOMERegistry
{
 public:
  static bool HandlerExists(const char *aProtocolScheme);

  static nsresult LoadURL(nsIURI *aURL);

  static void GetAppDescForScheme(const nsACString& aScheme,
                                  nsAString& aDesc);

  static already_AddRefed<nsMIMEInfoBase> GetFromExtension(const nsACString& aFileExt);

  static already_AddRefed<nsMIMEInfoBase> GetFromType(const nsACString& aMIMEType);
};
