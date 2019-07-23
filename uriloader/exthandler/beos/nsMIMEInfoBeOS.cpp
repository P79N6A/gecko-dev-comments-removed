


































 


#include "nsMIMEInfoBeOS.h"
#include "nsILocalFile.h"

nsMIMEInfoBeOS::~nsMIMEInfoBeOS()
{
}

nsresult
nsMIMEInfoBeOS::LaunchDefaultWithFile(nsIFile* aFile)
{
  
  nsCOMPtr<nsILocalFile> local(do_QueryInterface(aFile));
  if (!local)
    return NS_ERROR_FAILURE;

  PRBool executable = PR_TRUE;
  local->IsExecutable(&executable);
  if (executable)
    return NS_ERROR_FAILURE;

  return local->Launch();
}

nsresult
nsMIMEInfoBeOS::LoadUriInternal(nsIURI * aURL)
{
	nsresult rv = NS_OK;

	if (aURL) {
		
		nsCAutoString scheme;
		aURL->GetScheme(scheme);
		BString protoStr(scheme.get());
		protoStr.Prepend("application/x-vnd.Be.URL.");
		
		nsCAutoString spec;
		aURL->GetSpec(spec);
		const char* args[] = { spec.get() };
		
		
		BMimeType protocol;
		bool isInstalled = false;
		if (protocol.SetTo(protoStr.String()) == B_OK)
		{
			if(protocol.IsInstalled())
			{
				isInstalled = true;	
				be_roster->Launch(protoStr.String(), NS_ARRAY_LENGTH(args), (char **)args);
			}
		}
		if ((!isInstalled) && (!strcmp("mailto", scheme.get())))
			be_roster->Launch("text/x-email", NS_ARRAY_LENGTH(args), (char **)args);
	}
	return rv;
}
