








































#ifndef _MOZSTORAGESERVICE_H_
#define _MOZSTORAGESERVICE_H_

#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsIObserver.h"
#include "prlock.h"

#include "mozIStorageService.h"

class mozStorageConnection;
class nsIXPConnect;

class mozStorageService : public mozIStorageService
                        , public nsIObserver
{
    friend class mozStorageConnection;

public:
    
    nsresult Init();

    static mozStorageService *GetSingleton();

    NS_DECL_ISUPPORTS
    NS_DECL_MOZISTORAGESERVICE
    NS_DECL_NSIOBSERVER

    



    static already_AddRefed<nsIXPConnect> XPConnect();
private:
    virtual ~mozStorageService();

    



    PRLock *mLock;

    


    void Shutdown();
protected:
    nsCOMPtr<nsIFile> mProfileStorageFile;

    static mozStorageService *gStorageService;

    static nsIXPConnect *sXPConnect;
};

#endif 
