




































#ifndef nsAndroidHandlerApp_h
#define nsAndroidHandlerApp_h

#include "nsMIMEInfoImpl.h"
#include "nsIExternalSharingAppService.h"

class nsAndroidHandlerApp : public nsISharingHandlerApp {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHANDLERAPP
    NS_DECL_NSISHARINGHANDLERAPP

    public:
    nsAndroidHandlerApp(const nsAString& aName, const nsAString& aDescription,
                        const nsAString& aPackageName, 
                        const nsAString& aClassName, 
                        const nsACString& aMimeType, const nsAString& aAction);
    virtual ~nsAndroidHandlerApp();

private:
    nsString mName;
    nsString mDescription;
    nsCString mMimeType;
    nsString mClassName;
    nsString mPackageName;
    nsString mAction;
};
#endif
