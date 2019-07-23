


































 


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
