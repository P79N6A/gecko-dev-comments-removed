






































#ifndef __nsLocalHandlerAppImpl_h__
#define __nsLocalHandlerAppImpl_h__

#include "nsString.h"
#include "nsIMIMEInfo.h"
#include "nsIFile.h"
#include "nsTArray.h"

class nsLocalHandlerApp : public nsILocalHandlerApp
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIHANDLERAPP
  NS_DECL_NSILOCALHANDLERAPP

  nsLocalHandlerApp() { }

  nsLocalHandlerApp(const PRUnichar *aName, nsIFile *aExecutable) 
    : mName(aName), mExecutable(aExecutable) { }

  nsLocalHandlerApp(const nsAString & aName, nsIFile *aExecutable) 
    : mName(aName), mExecutable(aExecutable) { }
  virtual ~nsLocalHandlerApp() { }

protected:
  nsString mName;
  nsString mDetailedDescription;
  nsTArray<nsString> mParameters;
  nsCOMPtr<nsIFile> mExecutable;
  
  







  NS_HIDDEN_(nsresult) LaunchWithIProcess(const nsCString &aArg);
};



#ifdef XP_MACOSX
# ifndef NSLOCALHANDLERAPPMAC_H_  
# include "mac/nsLocalHandlerAppMac.h"
typedef nsLocalHandlerAppMac PlatformLocalHandlerApp_t;
# endif
#else 
typedef nsLocalHandlerApp PlatformLocalHandlerApp_t;
#endif

#endif 
