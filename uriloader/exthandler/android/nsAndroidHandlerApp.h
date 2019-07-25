




































#ifndef nsAndroidHandlerApp_h
#define nsAndroidHandlerApp_h

#include "nsMIMEInfoImpl.h"

class nsAndroidHandlerApp : public nsIHandlerApp {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHANDLERAPP
    public:
    nsAndroidHandlerApp(nsAString& aName, nsAString& aDescription);
    virtual ~nsAndroidHandlerApp();

private:
    nsString mName;
    nsString mDescription;

};
#endif
