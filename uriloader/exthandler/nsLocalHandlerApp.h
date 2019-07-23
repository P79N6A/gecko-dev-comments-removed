






































#ifndef __nsLocalHandlerAppImpl_h__
#define __nsLocalHandlerAppImpl_h__

#include "nsString.h"
#include "nsIMIMEInfo.h"
#include "nsIFile.h"

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
  nsCOMPtr<nsIFile> mExecutable;
};

#endif 
