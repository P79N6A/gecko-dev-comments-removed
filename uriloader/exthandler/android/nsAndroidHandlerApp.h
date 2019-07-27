




#ifndef nsAndroidHandlerApp_h
#define nsAndroidHandlerApp_h

#include "nsMIMEInfoImpl.h"
#include "nsIExternalSharingAppService.h"

class nsAndroidHandlerApp : public nsISharingHandlerApp {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHANDLERAPP
    NS_DECL_NSISHARINGHANDLERAPP

    nsAndroidHandlerApp(const nsAString& aName, const nsAString& aDescription,
                        const nsAString& aPackageName, 
                        const nsAString& aClassName, 
                        const nsACString& aMimeType, const nsAString& aAction);

private:
    virtual ~nsAndroidHandlerApp();

    nsString mName;
    nsString mDescription;
    nsString mPackageName;
    nsString mClassName;
    nsCString mMimeType;
    nsString mAction;
};
#endif
