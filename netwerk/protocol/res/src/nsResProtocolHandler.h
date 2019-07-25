






































#ifndef nsResProtocolHandler_h___
#define nsResProtocolHandler_h___

#include "nsIResProtocolHandler.h"
#include "nsInterfaceHashtable.h"
#include "nsISupportsArray.h"
#include "nsIIOService.h"
#include "nsWeakReference.h"
#include "nsStandardURL.h"


class nsResURL : public nsStandardURL
{
public:
    nsResURL() : nsStandardURL(PR_TRUE) {}
    virtual nsStandardURL* StartClone();
    virtual nsresult EnsureFile();
    NS_IMETHOD GetClassIDNoAlloc(nsCID *aCID);
};

class nsResProtocolHandler : public nsIResProtocolHandler, public nsSupportsWeakReference
{
private:
    typedef nsInterfaceHashtable<nsCStringHashKey,nsIURI> SubstitutionTable;
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_NSIRESPROTOCOLHANDLER

    nsResProtocolHandler();
    virtual ~nsResProtocolHandler();

    nsresult Init();

    void EnumerateSubstitutions(SubstitutionTable::EnumReadFunction enumFunc,
                                void* userArg);

private:
    nsresult AddSpecialDir(const char* aSpecialDir, const nsACString& aSubstitution);
    SubstitutionTable mSubstitutions;
    nsCOMPtr<nsIIOService> mIOService;

    friend class nsResURL;
};

#endif 
