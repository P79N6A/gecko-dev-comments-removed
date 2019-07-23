






































#include "nsMIMEInfoWin.h"

nsresult
nsMIMEInfoUnix::LoadUriInternal(nsIURI * aURI)
{
  return nsGNOMERegistry::LoadURL(aURI);
}

