






































#include "nsMIMEInfoUnix.h"
#include "nsGNOMERegistry.h"

nsresult
nsMIMEInfoUnix::LoadUriInternal(nsIURI * aURI)
{
  return nsGNOMERegistry::LoadURL(aURI);
}

