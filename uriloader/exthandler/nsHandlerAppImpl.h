






































#ifndef __nshandlerappimpl_h__
#define __nshandlerappimpl_h__

#include "nsString.h"
#include "nsIMIMEInfo.h"
#include "nsIFile.h"

class nsHandlerAppBase : public nsIHandlerApp
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIHANDLERAPP

  nsHandlerAppBase() NS_HIDDEN {}
  nsHandlerAppBase(const PRUnichar *aName) NS_HIDDEN  { mName.Assign(aName); }
  nsHandlerAppBase(const nsAString & aName) NS_HIDDEN  { mName.Assign(aName); }
  virtual ~nsHandlerAppBase() {}

protected:
  nsString mName;
};

class nsLocalHandlerApp : public nsHandlerAppBase, public nsILocalHandlerApp
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSILOCALHANDLERAPP
  
  nsLocalHandlerApp() {}

  nsLocalHandlerApp(const PRUnichar *aName, nsIFile *aExecutable) 
    : nsHandlerAppBase(aName), mExecutable(aExecutable) {}

  nsLocalHandlerApp(const nsAString & aName, nsIFile *aExecutable) 
    : nsHandlerAppBase(aName), mExecutable(aExecutable) {}

  virtual ~nsLocalHandlerApp() {}

  
  
  NS_IMETHOD GetName(nsAString & aName);
    
protected: 
  nsCOMPtr<nsIFile> mExecutable;
};

class nsWebHandlerApp : public nsHandlerAppBase, public nsIWebHandlerApp
{
  public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIWEBHANDLERAPP

  nsWebHandlerApp(const PRUnichar *aName, const nsACString &aUriTemplate)
    : nsHandlerAppBase(aName), mUriTemplate(aUriTemplate) { }

  virtual ~nsWebHandlerApp() {}

  protected:
  nsCString mUriTemplate;
      
};

#endif 
