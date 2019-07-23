




































#include "nsIProfile.h"
#include "nsIProfileInternal.h"
#include "nsIProfileStartupListener.h"
#include "nsIProfileChangeStatus.h"
#include "nsCOMPtr.h"
#include "nsISupports.h"
#include "nsIRegistry.h"
#include "nsString.h"
#include "nsICmdLineService.h"
#include "nsProfileAccess.h"


#include "nsIXULWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIURIContentListener.h"
#include "nsIDirectoryService.h"

#define _MAX_LENGTH   256

class nsProfile: public nsIProfileInternal,
                 public nsIProfileChangeStatus
{
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROFILE
    NS_DECL_NSIPROFILEINTERNAL
    NS_DECL_NSIPROFILECHANGESTATUS

private:
    nsresult ProcessArgs(nsICmdLineService *service,
                         PRBool canInteract,
                         PRBool *profileDirSet,
                         nsCString & profileURLStr);
    nsresult LoadDefaultProfileDir(nsCString & profileURLStr, PRBool canInterract);
	nsresult ConfirmAutoMigration(PRBool canInteract, PRBool *confirmed);
	nsresult CopyDefaultFile(nsIFile *profDefaultsDir,
	                         nsIFile *newProfDir,
                             const nsACString &fileName);
	nsresult LoadNewProfilePrefs();
    nsresult SetProfileDir(const PRUnichar *profileName, nsIFile *profileDir);
								
    nsresult AddLevelOfIndirection(nsIFile *aDir);
    nsresult IsProfileDirSalted(nsIFile *profileDir, PRBool *isSalted);
    nsresult DefineLocaleDefaultsDir();
    nsresult Update4xProfileInfo();
    nsresult GetOldRegLocation(nsIFile **aOldRegFile);
    nsresult UpdateCurrentProfileModTime(PRBool updateRegistry);
    nsresult MigrateProfileInternal(const PRUnichar *profileName,
                                    nsIFile *oldProfDir, nsIFile *newProfDir);

    nsresult GetLocalProfileDir(const PRUnichar *profileName, nsIFile** localDir);

    PRBool mStartingUp;
    PRBool mAutomigrate;
    PRBool mOutofDiskSpace;
    PRBool mDiskSpaceErrorQuitCalled;
    PRBool mProfileChangeVetoed;
    PRBool mProfileChangeFailed;

    nsString mCurrentProfileName;
    PRBool mCurrentProfileAvailable;

    PRBool mIsUILocaleSpecified;
    nsCString mUILocaleName;

    PRBool mIsContentLocaleSpecified;
    nsCString mContentLocaleName;
    
    PRBool mShutdownProfileToreDownNetwork;
    
public:
    nsProfile();
    virtual ~nsProfile();
    
    nsresult Init();

    
    nsresult CopyRegKey(const PRUnichar *oldProfile, const PRUnichar *newProfile);

    nsresult AutoMigrate();

    nsresult ShowProfileWizard(void);
};


