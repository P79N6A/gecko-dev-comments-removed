






































#ifndef __nsDBusHandlerAppImpl_h__
#define __nsDBusHandlerAppImpl_h__

#include "nsString.h"
#include "nsIMIMEInfo.h"

class nsDBusHandlerApp : public nsIDBusHandlerApp
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIHANDLERAPP
  NS_DECL_NSIDBUSHANDLERAPP

  nsDBusHandlerApp() { }

  virtual ~nsDBusHandlerApp() { }

protected:
  nsString mName;
  nsString mDetailedDescription;
  nsCString mService;
  nsCString mMethod;
  nsCString mInterface;
  nsCString mObjpath;

};
#endif
