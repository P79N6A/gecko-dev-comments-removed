




































#ifndef nsAndroidHandlerApp_h
#define nsAndroidHandlerApp_h

#include "nsMIMEInfoImpl.h"

class nsAndroidHandlerApp : public nsIHandlerApp {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHANDLERAPP
    public:
    nsAndroidHandlerApp(const nsAString& aName, const nsAString& aDescription,
                        const nsAString& aPackageName, const nsAString& aClassName, 
                        const nsACString& aMimeType);
    virtual ~nsAndroidHandlerApp();

private:
    nsString mName;
    nsString mDescription;
    nsCString mMimeType;
    nsString mClassName;
    nsString mPackageName;
};
#endif
