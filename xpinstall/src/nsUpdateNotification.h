








































#include "nsCOMPtr.h"

#include "nsIRDFDataSource.h"
#include "nsRDFCID.h"
#include "nsIRDFXMLSink.h"
#include "nsIRDFResource.h"
#include "nsIRDFService.h"

#include "VerReg.h"

#include "nsIUpdateNotification.h"

#define NS_XPI_UPDATE_NOTIFIER_DATASOURCE_CID \
{ 0x69fdc800, 0x4050, 0x11d3, { 0xbe, 0x2f, 0x0, 0x10, 0x4b, 0xde, 0x60, 0x48 } }

#define NS_XPI_UPDATE_NOTIFIER_CID \
{ 0x68a24e36, 0x042d, 0x11d4, { 0xac, 0x85, 0x0, 0xc0, 0x4f, 0xa0, 0xd2, 0x6b } }

#define NS_XPI_UPDATE_NOTIFIER_CONTRACTID "@mozilla.org/xpinstall/notifier;1"

#define BASE_DATASOURCE_URL "chrome://communicator/content/xpinstall/SoftwareUpdates.rdf"



class nsXPINotifierImpl : public nsIUpdateNotification, public nsIRDFXMLSinkObserver
{

public:
    static NS_IMETHODIMP New(nsISupports* aOuter, REFNSIID aIID, void** aResult);
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRDFXMLSINKOBSERVER
    NS_DECL_NSIUPDATENOTIFICATION    

protected:
    
    nsXPINotifierImpl();
    virtual ~nsXPINotifierImpl();

    nsresult NotificationEnabled(PRBool* aReturn);
    nsresult Init();
    nsresult OpenRemoteDataSource(const char* aURL, PRBool blocking, nsIRDFDataSource** aResult);
    
    PRBool IsNewerOrUninstalled(const char* regKey, const char* versionString);
    PRInt32 CompareVersions(VERSION *oldversion, VERSION *newVersion);
    void   StringToVersionNumbers(const nsString& version, int32 *aMajor, int32 *aMinor, int32 *aRelease, int32 *aBuild);
    
    nsCOMPtr<nsISupports> mNotifications;
    nsIRDFService* mRDF;

    PRUint32 mPendingRefreshes;

    static nsIRDFResource* kXPI_NotifierSources;
    static nsIRDFResource* kXPI_NotifierPackages;
    static nsIRDFResource* kXPI_NotifierPackage_Title;
    static nsIRDFResource* kXPI_NotifierPackage_Version;
    static nsIRDFResource* kXPI_NotifierPackage_Description;
    static nsIRDFResource* kXPI_NotifierPackage_RegKey;

    static nsIRDFResource* kNC_NotificationRoot;
    static nsIRDFResource* kNC_Source;
	static nsIRDFResource* kNC_Name;
	static nsIRDFResource* kNC_URL;
	static nsIRDFResource* kNC_Child;
	
};
