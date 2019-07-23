




































#ifndef nsmacshellservice_h____
#define nsmacshellservice_h____

#include "nsIMacShellService.h"
#include "nsIWebProgressListener.h"
#include "nsILocalFile.h"
#include "nsCOMPtr.h"

class nsMacShellService : public nsIMacShellService,
                          public nsIWebProgressListener
{
public:
  nsMacShellService() : mCheckedThisSession(PR_FALSE) {};
  virtual ~nsMacShellService() {};

  NS_DECL_ISUPPORTS
  NS_DECL_NSISHELLSERVICE
  NS_DECL_NSIMACSHELLSERVICE
  NS_DECL_NSIWEBPROGRESSLISTENER

protected:

private:
  nsCOMPtr<nsILocalFile> mBackgroundFile;

  PRBool    mCheckedThisSession;
};

#endif 
