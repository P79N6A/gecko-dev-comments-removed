




































#include "nscore.h" 
#include "nsProfile.h"
#ifdef MOZ_PROFILELOCKING
#include "nsProfileLock.h"
#endif
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

#include "pratom.h"
#include "prmem.h"
#include "plstr.h"
#include "prenv.h"

#include "nsIFactory.h"
#include "nsIComponentManager.h"
#include "nsIEnumerator.h"
#include "nsXPIDLString.h"
#include "nsEscape.h"
#include "nsIURL.h"
#include "nsNativeCharsetUtils.h"

#include "prprf.h"

#include "nsIIOService.h"
#include "nsNetUtil.h"
#include "nsPrefMigration.h"
#include "nsIPrefMigration.h"
#include "nsPrefMigrationCIDs.h"
#include "nsFileStream.h"
#include "nsIPromptService.h"
#include "nsIStreamListener.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsICookieService.h"
#include "nsICategoryManager.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIChromeRegistrySea.h"
#include "nsIStringBundle.h"
#include "nsIObserverService.h"
#include "nsHashtable.h"
#include "nsIAtom.h"
#include "nsProfileDirServiceProvider.h"
#include "nsISessionRoaming.h"


#include "nsIDocShell.h"
#include "nsIWebBrowserChrome.h"
#include "nsIBaseWindow.h"
#include "nsIDialogParamBlock.h"
#include "nsIDOMWindowInternal.h"
#include "nsIWindowWatcher.h"
#include "jsapi.h"
#include "nsIJSContextStack.h"
#include "nsEmbedCID.h"

#if defined(XP_MAC) || defined(XP_MACOSX)
#define OLD_REGISTRY_FILE_NAME "Netscape Registry"
#elif defined(XP_WIN) || defined(XP_OS2)
#define OLD_REGISTRY_FILE_NAME "nsreg.dat"
#endif



#define DEFAULT_PROFILE_NAME           (NS_LITERAL_STRING("default").get())

#define PROFILE_SELECTION_URL          "chrome://communicator/content/profile/profileSelection.xul"
#define PROFILE_SELECTION_CMD_LINE_ARG "-SelectProfile"
#define PROFILE_MANAGER_URL            "chrome://communicator/content/profile/profileSelection.xul?manage=true"
#define PROFILE_MANAGER_CMD_LINE_ARG   "-ProfileManager"
#define PROFILE_WIZARD_URL             "chrome://communicator/content/profile/createProfileWizard.xul"
#define PROFILE_WIZARD_CMD_LINE_ARG    "-ProfileWizard"
#define INSTALLER_CMD_LINE_ARG         "-installer"
#define CREATE_PROFILE_CMD_LINE_ARG    "-CreateProfile"
#define PROFILE_CMD_LINE_ARG "-P"   
#define UILOCALE_CMD_LINE_ARG "-UILocale"   
#define CONTENTLOCALE_CMD_LINE_ARG "-contentLocale"   

#define PREF_CONFIRM_AUTOMIGRATION     "profile.confirm_automigration"
#define PREF_AUTOMIGRATION             "profile.allow_automigration"
#define PREF_MIGRATE_ALL               "profile.migrate_all"
#define PREF_MIGRATION_BEHAVIOR        "profile.migration_behavior"
#define PREF_MIGRATION_DIRECTORY       "profile.migration_directory"

#if defined (XP_MAC)
#define CHROME_STYLE nsIWebBrowserChrome::CHROME_WINDOW_BORDERS | nsIWebBrowserChrome::CHROME_WINDOW_CLOSE | nsIWebBrowserChrome::CHROME_CENTER_SCREEN
#else 
#define CHROME_STYLE nsIWebBrowserChrome::CHROME_ALL | nsIWebBrowserChrome::CHROME_CENTER_SCREEN
#endif 

const char* kDefaultOpenWindowParams = "centerscreen,chrome,modal,titlebar";

const char* kBrandBundleURL = "chrome://branding/locale/brand.properties";
const char* kMigrationBundleURL = "chrome://communicator/locale/profile/migration.properties";




#undef DEBUG_profile_verbose
#ifdef DEBUG_seth
#define DEBUG_profile_verbose 1
#endif





static nsProfileAccess*    gProfileDataAccess = nsnull;
static PRInt32          gInstanceCount = 0;



static nsHashtable *gLocaleProfiles = nsnull;
static nsProfileDirServiceProvider *gDirServiceProvider = nsnull;


static NS_DEFINE_CID(kPrefMigrationCID, NS_PREFMIGRATION_CID);
static NS_DEFINE_CID(kPrefConverterCID, NS_PREFCONVERTER_CID);

#define NS_SESSIONROAMING_CONTRACTID "@mozilla.org/profile/session-roaming;1"







static
nsresult RecursiveCopy(nsIFile* srcDir, nsIFile* destDir)
{
    nsresult rv;
    PRBool isDir;
    
    rv = srcDir->IsDirectory(&isDir);
    if (NS_FAILED(rv)) return rv;
	if (!isDir) return NS_ERROR_INVALID_ARG;

    PRBool exists;
    rv = destDir->Exists(&exists);
	if (NS_SUCCEEDED(rv) && !exists)
		rv = destDir->Create(nsIFile::DIRECTORY_TYPE, 0775);
	if (NS_FAILED(rv)) return rv;

    PRBool hasMore = PR_FALSE;
    nsCOMPtr<nsISimpleEnumerator> dirIterator;
    rv = srcDir->GetDirectoryEntries(getter_AddRefs(dirIterator));
    if (NS_FAILED(rv)) return rv;
    
    rv = dirIterator->HasMoreElements(&hasMore);
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIFile> dirEntry;
    
	while (hasMore)
	{
		rv = dirIterator->GetNext((nsISupports**)getter_AddRefs(dirEntry));
		if (NS_SUCCEEDED(rv))
		{
		    rv = dirEntry->IsDirectory(&isDir);
		    if (NS_SUCCEEDED(rv))
		    {
		        if (isDir)
		        {
		            nsCOMPtr<nsIFile> destClone;
		            rv = destDir->Clone(getter_AddRefs(destClone));
		            if (NS_SUCCEEDED(rv))
		            {
		                nsCOMPtr<nsILocalFile> newChild(do_QueryInterface(destClone));
		                nsCAutoString leafName;
		                dirEntry->GetNativeLeafName(leafName);
		                newChild->AppendRelativeNativePath(leafName);
		                rv = RecursiveCopy(dirEntry, newChild);
		            }
		        }
		        else
		            rv = dirEntry->CopyToNative(destDir, EmptyCString());
		    }
		
		}
        rv = dirIterator->HasMoreElements(&hasMore);
        if (NS_FAILED(rv)) return rv;
	}

	return rv;
}





nsProfile::nsProfile()
{
    mStartingUp = PR_FALSE;
    mAutomigrate = PR_FALSE;
    mOutofDiskSpace = PR_FALSE;
    mDiskSpaceErrorQuitCalled = PR_FALSE;
    mCurrentProfileAvailable = PR_FALSE;

    mIsUILocaleSpecified = PR_FALSE;
    mIsContentLocaleSpecified = PR_FALSE;
    
    mShutdownProfileToreDownNetwork = PR_FALSE;
    
    mProfileChangeVetoed = PR_FALSE;
    mProfileChangeFailed = PR_FALSE;
}

nsProfile::~nsProfile() 
{
#if defined(DEBUG_profile_verbose)
    printf("~nsProfile \n");
#endif

   if (--gInstanceCount == 0) {
        
      delete gProfileDataAccess;
      delete gLocaleProfiles;
      NS_IF_RELEASE(gDirServiceProvider);
    }
}

nsresult
nsProfile::Init()
{
    nsresult rv = NS_OK;
    
    if (gInstanceCount++ == 0) {
        gProfileDataAccess = new nsProfileAccess;
        if (!gProfileDataAccess)
            return NS_ERROR_OUT_OF_MEMORY;
        gLocaleProfiles = new nsHashtable();
        if (!gLocaleProfiles)
            return NS_ERROR_OUT_OF_MEMORY;

        rv = NS_NewProfileDirServiceProvider(PR_FALSE, &gDirServiceProvider);
        if (NS_SUCCEEDED(rv))
            rv = gDirServiceProvider->Register();
    }
    return rv;
}




NS_IMPL_THREADSAFE_ADDREF(nsProfile)
NS_IMPL_THREADSAFE_RELEASE(nsProfile)

NS_INTERFACE_MAP_BEGIN(nsProfile)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIProfile)
    NS_INTERFACE_MAP_ENTRY(nsIProfile)
    NS_INTERFACE_MAP_ENTRY(nsIProfileInternal)
    NS_INTERFACE_MAP_ENTRY(nsIProfileChangeStatus)
NS_INTERFACE_MAP_END





NS_IMETHODIMP
nsProfile::GetAutomigrate(PRBool *aAutomigrate)
{
    NS_ENSURE_ARG_POINTER(aAutomigrate);

    *aAutomigrate = mAutomigrate;
    return NS_OK;
}

NS_IMETHODIMP
nsProfile::SetAutomigrate(PRBool aAutomigrate)
{
    mAutomigrate = aAutomigrate;
    return NS_OK;
}

NS_IMETHODIMP
nsProfile::StartupWithArgs(nsICmdLineService *cmdLineArgs, PRBool canInteract)
{
    nsresult rv;

    struct ScopeFlag
    {
        ScopeFlag(PRBool *flagPtr) : mFlagPtr(flagPtr)
        { *mFlagPtr = PR_TRUE; }

        ~ScopeFlag()
        { *mFlagPtr = PR_FALSE; }

        PRBool *mFlagPtr;
    };

    
    PRBool profileDirSet = PR_FALSE;
    nsCString profileURLStr("");

#ifdef DEBUG_profile_verbose
    printf("Profile Manager : Profile Wizard and Manager activites : Begin\n");
#endif

    ScopeFlag startupFlag(&mStartingUp);

    if (cmdLineArgs)
        rv = ProcessArgs(cmdLineArgs, canInteract, &profileDirSet, profileURLStr);

    
    
    if (mDiskSpaceErrorQuitCalled)
        return NS_ERROR_FAILURE;

    if (!profileDirSet) {
        rv = LoadDefaultProfileDir(profileURLStr, canInteract);

        if (NS_FAILED(rv)) return rv;
    }

    
    
    nsXPIDLString currentProfileStr;
    rv = GetCurrentProfile(getter_Copies(currentProfileStr));
    if (NS_FAILED(rv) || (*(const PRUnichar*)currentProfileStr == 0)) {
        return NS_ERROR_ABORT;
    }


    
    

    
    if (mIsUILocaleSpecified == PR_FALSE && mIsContentLocaleSpecified == PR_FALSE) {
        return NS_OK;
    }

    nsCOMPtr<nsIFile> profileDir;

    rv = GetCurrentProfileDir(getter_AddRefs(profileDir));
    if (NS_FAILED(rv)) return rv;

    nsCAutoString pathBuf;
    rv = profileDir->GetNativePath(pathBuf);
    if (NS_FAILED(rv)) return rv;

    
    
    
    
    nsCStringKey key(pathBuf);
    if (NS_PTR_TO_INT32(gLocaleProfiles->Get(&key)) == PR_TRUE) {
#ifdef DEBUG_profile_verbose
        printf(" already set UILocale and contentLocale: %s\n", pathBuf.get());
        printf(" will not install locale\n");
#endif
        return NS_OK;
    }
    gLocaleProfiles->Remove(&key);

    nsCOMPtr<nsIChromeRegistrySea> chromeRegistry =
        do_GetService(NS_CHROMEREGISTRY_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    
    nsCAutoString fileStr;
    rv = NS_GetURLSpecFromFile(profileDir, fileStr);
    if (NS_FAILED(rv)) return rv;

    
    
    
    
    if (!mUILocaleName.IsEmpty()) {
#ifdef DEBUG_profile_verbose
        printf(" install new UILocaleName: %s\n", mUILocaleName.get());
#endif
        rv = chromeRegistry->SelectLocaleForProfile(mUILocaleName,
                                          NS_ConvertUTF8toUTF16(fileStr).get());
        if (NS_FAILED(rv)) return rv;
    }

    if (!mContentLocaleName.IsEmpty()) {
#ifdef DEBUG_profile_verbose
        printf(" install new mContentLocaleName: %s\n", mContentLocaleName.get());
#endif
        rv = chromeRegistry->SelectLocaleForProfile(mContentLocaleName,
                                          NS_ConvertUTF8toUTF16(fileStr).get());
        if (NS_FAILED(rv)) return rv;
    }

#ifdef DEBUG_profile_verbose
    printf("Profile Manager : Profile Wizard and Manager activites : End\n");
#endif

    return NS_OK;
}

NS_IMETHODIMP
nsProfile::GetIsStartingUp(PRBool *aIsStartingUp)
{
    NS_ENSURE_ARG_POINTER(aIsStartingUp);
    *aIsStartingUp = mStartingUp;
    return NS_OK;
}

nsresult
nsProfile::LoadDefaultProfileDir(nsCString & profileURLStr, PRBool canInteract)
{
    nsresult rv;
    nsCOMPtr<nsIURI> profileURL;
    PRInt32 numProfiles=0;
  
    GetProfileCount(&numProfiles);

    if (profileURLStr.IsEmpty())
    {
        nsCOMPtr<nsIPrefBranch> prefBranch;
        nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
        if (NS_FAILED(rv)) return rv;
        rv = prefs->GetBranch(nsnull, getter_AddRefs(prefBranch));
        if (NS_FAILED(rv)) return rv;
    
        
        
        
        
        
        
        PRBool startWithLastUsedProfile = PR_FALSE;

        
        PRBool cantAutoSelect;
        rv = prefBranch->GetBoolPref("profile.manage_only_at_launch", &cantAutoSelect);
        if (NS_SUCCEEDED(rv) && !cantAutoSelect)
          GetStartWithLastUsedProfile(&startWithLastUsedProfile);
        
        
        
        
        if (numProfiles == 0)
        {
            rv = CreateDefaultProfile();
            if (NS_FAILED(rv)) return rv;
            
        }
        else if (numProfiles == 1 || startWithLastUsedProfile)
        {
            
            
            
            if (mCurrentProfileAvailable)
               return NS_OK;

            
            nsCOMPtr<nsIFile> curProfileDir;
            PRBool exists = PR_FALSE;
            
            rv = GetCurrentProfileDir(getter_AddRefs(curProfileDir));
            if (NS_SUCCEEDED(rv))
                rv = curProfileDir->Exists(&exists);
            if (NS_FAILED(rv) || !exists)
                profileURLStr = PROFILE_MANAGER_URL; 
            if (exists)
            {
#ifdef MOZ_PROFILELOCKING
                
                nsCOMPtr<nsILocalFile> localFile(do_QueryInterface(curProfileDir));
                nsProfileLock tempLock;
                rv = tempLock.Lock(localFile, nsnull);
                if (NS_FAILED(rv))
                    profileURLStr = PROFILE_MANAGER_URL;
#endif
            }
        }
        else
            profileURLStr = PROFILE_SELECTION_URL;
    }

    if (!profileURLStr.IsEmpty())
    {
        if (!canInteract) return NS_ERROR_PROFILE_REQUIRES_INTERACTION;

        nsCOMPtr<nsIWindowWatcher> windowWatcher(do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIDialogParamBlock> ioParamBlock(do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID, &rv));
        if (NS_FAILED(rv)) return rv;

        
        

        ioParamBlock->SetNumberStrings(1);
        ioParamBlock->SetString(0, NS_LITERAL_STRING("startup").get());

        nsCOMPtr<nsIDOMWindow> newWindow;
        rv = windowWatcher->OpenWindow(nsnull,
                                       profileURLStr.get(),
                                       "_blank",
                                       kDefaultOpenWindowParams,
                                       ioParamBlock,
                                       getter_AddRefs(newWindow));
        if (NS_FAILED(rv)) return rv;
        PRInt32 dialogConfirmed;
        ioParamBlock->GetInt(0, &dialogConfirmed);
        if (dialogConfirmed == 0) return NS_ERROR_ABORT;
    }

    nsXPIDLString currentProfileStr;    
    rv = GetCurrentProfile(getter_Copies(currentProfileStr));
    if (NS_FAILED(rv)) return rv;

    
    if (!mCurrentProfileAvailable) {
        rv = SetCurrentProfile(currentProfileStr);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

nsresult
nsProfile::ConfirmAutoMigration(PRBool canInteract, PRBool *confirmed)
{
    NS_ENSURE_ARG_POINTER(confirmed);
    nsCOMPtr<nsIPrefBranch> prefBranch;
    *confirmed = PR_FALSE;
    nsresult rv;
    
    
    
    PRBool confirmAutomigration = PR_TRUE;
    nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    rv = prefs->GetBranch(nsnull, getter_AddRefs(prefBranch));
    if (NS_FAILED(rv)) return rv;

    (void)prefBranch->GetBoolPref(PREF_CONFIRM_AUTOMIGRATION, &confirmAutomigration);
    if (!confirmAutomigration) {
        *confirmed = PR_TRUE;
        return NS_OK;
    }
    
    
    if (!canInteract)
        return NS_ERROR_PROFILE_REQUIRES_INTERACTION;

    nsCOMPtr<nsIStringBundleService> stringBundleService(do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIStringBundle> migrationBundle, brandBundle;
    rv = stringBundleService->CreateBundle(kMigrationBundleURL, getter_AddRefs(migrationBundle));
    if (NS_FAILED(rv)) return rv;
    rv = stringBundleService->CreateBundle(kBrandBundleURL, getter_AddRefs(brandBundle));
    if (NS_FAILED(rv)) return rv;
    
    nsXPIDLString brandName;
    rv = brandBundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(), getter_Copies(brandName));
    if (NS_FAILED(rv)) return rv;

    nsXPIDLString msgString, dialogTitle, button0Title, button1Title;
    const PRUnichar *formatStrings[] = { brandName.get(), brandName.get() };
    rv = migrationBundle->FormatStringFromName(NS_LITERAL_STRING("confirmMigration").get(),
                                                 formatStrings, 2, getter_Copies(msgString));
    if (NS_FAILED(rv)) return rv;
    
    rv = migrationBundle->GetStringFromName(NS_LITERAL_STRING("dialogTitle").get(), getter_Copies(dialogTitle));
    if (NS_FAILED(rv)) return rv;
    rv = migrationBundle->GetStringFromName(NS_LITERAL_STRING("migrate").get(), getter_Copies(button0Title));
    if (NS_FAILED(rv)) return rv;
    rv = migrationBundle->GetStringFromName(NS_LITERAL_STRING("manage").get(), getter_Copies(button1Title));
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIPromptService> promptService(do_GetService(NS_PROMPTSERVICE_CONTRACTID, &rv));
    if (NS_FAILED(rv)) return rv;
    PRInt32 buttonPressed;
    rv = promptService->ConfirmEx(nsnull, dialogTitle.get(), msgString.get(),
                                  (nsIPromptService::BUTTON_POS_0 * nsIPromptService::BUTTON_TITLE_IS_STRING) +
                                  (nsIPromptService::BUTTON_POS_1 * nsIPromptService::BUTTON_TITLE_IS_STRING),
                                  button0Title, button1Title, nsnull,
                                  nsnull, nsnull, &buttonPressed);
    if (NS_FAILED(rv)) return rv;
    *confirmed = (buttonPressed == 0);
    return NS_OK;
}

nsresult 
nsProfile::AutoMigrate()
{
    nsresult rv = NS_OK;
    
    rv = MigrateAllProfiles();

    
    
    if (NS_FAILED(rv) && !mOutofDiskSpace) 
    {
#ifdef DEBUG_profile
        printf("AutoMigration failed. Let's create a default 5.0 profile.\n");
#endif
        
        rv = CreateDefaultProfile();
        if (NS_FAILED(rv)) return rv;
    }   

    gProfileDataAccess->mProfileDataChanged = PR_TRUE;
    gProfileDataAccess->UpdateRegistry(nsnull);

    return rv;
}

nsresult
nsProfile::ProcessArgs(nsICmdLineService *cmdLineArgs,
                       PRBool canInteract,
                       PRBool* profileDirSet,
                       nsCString & profileURLStr)
{
    NS_ASSERTION(cmdLineArgs, "Invalid cmdLineArgs");   
    NS_ASSERTION(profileDirSet, "Invalid profileDirSet");   

    nsresult rv;
    nsXPIDLCString cmdResult;
    nsCOMPtr<nsILocalFile> currProfileDir;

	
	
	PRBool foundProfileCommandArg = PR_FALSE;

#ifdef DEBUG_profile_verbose
    printf("Profile Manager : Command Line Options : Begin\n");
#endif
 
    
    
    rv = cmdLineArgs->GetCmdLineValue(UILOCALE_CMD_LINE_ARG, getter_Copies(cmdResult));
    if (NS_SUCCEEDED(rv))
    {
        if (cmdResult) {
	    mIsUILocaleSpecified = PR_TRUE;
            mUILocaleName = cmdResult;
        }
    }

    
    rv = cmdLineArgs->GetCmdLineValue(CONTENTLOCALE_CMD_LINE_ARG, getter_Copies(cmdResult));
    if (NS_SUCCEEDED(rv))
    {
        if (cmdResult) {
            mIsContentLocaleSpecified = PR_TRUE;
            mContentLocaleName = cmdResult;
        }
    }

    
    
    
    
    rv = cmdLineArgs->GetCmdLineValue(PROFILE_CMD_LINE_ARG, getter_Copies(cmdResult));
    if (NS_SUCCEEDED(rv))
    {
        if (cmdResult) {
            foundProfileCommandArg = PR_TRUE;
            nsAutoString currProfileName; 
            rv = NS_CopyNativeToUnicode(cmdResult, currProfileName); 
            NS_ASSERTION(NS_SUCCEEDED(rv), "failed to convert ProfileName to unicode");
 
#ifdef DEBUG_profile
            printf("ProfileName : %s\n", (const char*)cmdResult);
#endif 
            PRBool exists;
            rv = ProfileExists(currProfileName.get(), &exists);
            if (NS_FAILED(rv)) return rv;            
            
            if (!exists) {
                PRInt32 num5xProfiles = 0;
                PRInt32 num4xProfiles = 0;

                GetProfileCount(&num5xProfiles);
                Get4xProfileCount(&num4xProfiles);

                if (num5xProfiles == 0 && num4xProfiles == 0) {
                    profileURLStr = PROFILE_WIZARD_URL;
                }
                else if (num5xProfiles > 0) {
                    profileURLStr = PROFILE_SELECTION_URL;
                }
                else if (num4xProfiles > 0) {
                    profileURLStr = PROFILE_MANAGER_URL;
                }
                *profileDirSet = PR_FALSE;
            }
            else {
                rv = SetCurrentProfile(currProfileName.get());
                if (NS_SUCCEEDED(rv))
                    *profileDirSet = PR_TRUE;
            }
        }
    }

    
    
    
    
    
    
    
    
    
    

    rv = cmdLineArgs->GetCmdLineValue(CREATE_PROFILE_CMD_LINE_ARG, getter_Copies(cmdResult));
    if (NS_SUCCEEDED(rv))
    {
        if (cmdResult) {

#ifdef DEBUG_profile_verbose
            printf("profileName & profileDir are: %s\n", (const char*)cmdResult);
#endif
            foundProfileCommandArg = PR_TRUE;
            nsAutoString currProfileName; 
 
            char *tmpStr;
            rv = NS_CopyNativeToUnicode(
                 nsDependentCString(nsCRT::strtok(cmdResult.BeginWriting(), " ", &tmpStr)),
                                    currProfileName);
            NS_ASSERTION(NS_SUCCEEDED(rv), "failed to convert ProfileName to unicode");

            char *currProfileDirString = nsCRT::strtok(tmpStr, " ", &tmpStr); 
            if (currProfileDirString && *currProfileDirString) {
                rv = NS_NewNativeLocalFile(nsDependentCString(currProfileDirString), 
                     PR_TRUE, getter_AddRefs(currProfileDir));
                NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
            }
            else {
                
                
                nsCOMPtr<nsIProperties> directoryService(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv));
                if (NS_FAILED(rv)) return rv;
                rv = directoryService->Get(NS_APP_USER_PROFILES_ROOT_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(currProfileDir));
                if (NS_FAILED(rv)) return rv;
            }

            nsAutoString currProfilePath;
            currProfileDir->GetPath(currProfilePath);
            rv = CreateNewProfile(currProfileName.get(), currProfilePath.get(), nsnull, PR_TRUE);
            if (NS_SUCCEEDED(rv)) {
                *profileDirSet = PR_TRUE;
                mCurrentProfileAvailable = PR_TRUE;
                gProfileDataAccess->SetCurrentProfile(currProfileName.get());

                
                
                nsCOMPtr<nsIFile> newProfileDir;
                GetProfileDir(currProfileName.get(), getter_AddRefs(newProfileDir));
                if (newProfileDir) {
                  nsCOMPtr<nsIFile> localDir;
                  GetLocalProfileDir(currProfileName.get(), getter_AddRefs(localDir));
                  gDirServiceProvider->SetProfileDir(newProfileDir, localDir);
                }
                rv = LoadNewProfilePrefs();
                gProfileDataAccess->mProfileDataChanged = PR_TRUE;
                gProfileDataAccess->UpdateRegistry(nsnull);
            }
            rv = ForgetCurrentProfile();
            if (NS_FAILED(rv)) return rv;
        }
    }

    
    rv = cmdLineArgs->GetCmdLineValue(PROFILE_MANAGER_CMD_LINE_ARG, getter_Copies(cmdResult));
    if (NS_SUCCEEDED(rv))
    {        
        if (cmdResult) {
			foundProfileCommandArg = PR_TRUE;
            profileURLStr = PROFILE_MANAGER_URL;
        }
    }
    
    
    rv = cmdLineArgs->GetCmdLineValue(PROFILE_SELECTION_CMD_LINE_ARG, getter_Copies(cmdResult));
    if (NS_SUCCEEDED(rv))
    {        
        if (cmdResult) {
			foundProfileCommandArg = PR_TRUE;
            profileURLStr = PROFILE_SELECTION_URL;
        }
    }
    
    
    
    rv = cmdLineArgs->GetCmdLineValue(PROFILE_WIZARD_CMD_LINE_ARG, getter_Copies(cmdResult));
    if (NS_SUCCEEDED(rv))
    {        
        if (cmdResult) {
			foundProfileCommandArg = PR_TRUE;
            profileURLStr = PROFILE_WIZARD_URL;
        }
    }

	PRBool forceMigration = PR_FALSE;
	if (!foundProfileCommandArg) {
		rv = gProfileDataAccess->DetermineForceMigration(&forceMigration);
		NS_ASSERTION(NS_SUCCEEDED(rv),"failed to determine if we should force migration");
	}

    nsCOMPtr<nsIPrefBranch> prefBranch;
   
    
    PRBool allowAutoMigration = PR_TRUE;
    nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    rv = prefs->GetBranch(nsnull, getter_AddRefs(prefBranch));
    if (NS_FAILED(rv)) return rv;

    (void)prefBranch->GetBoolPref(PREF_AUTOMIGRATION, &allowAutoMigration);

    
    rv = cmdLineArgs->GetCmdLineValue(INSTALLER_CMD_LINE_ARG, getter_Copies(cmdResult));
    if (allowAutoMigration && (NS_SUCCEEDED(rv) || forceMigration))
    {        
        if (cmdResult || forceMigration) {
        PRBool migrateAll = PR_FALSE;
        (void)prefBranch->GetBoolPref(PREF_MIGRATE_ALL, &migrateAll);

            rv = MigrateProfileInfo();
            if (NS_FAILED(rv)) return rv;

            PRInt32 num4xProfiles = 0;
            rv = Get4xProfileCount(&num4xProfiles);
            if (NS_FAILED(rv)) return rv;
            
            PRInt32 numProfiles = 0;
            GetProfileCount(&numProfiles);
            if (num4xProfiles == 0 && numProfiles == 0) {
                
                CreateDefaultProfile();
                if (NS_FAILED(rv)) return rv;
            }
            else if (num4xProfiles == 0 && numProfiles == 1) {
                profileURLStr = "";
            }
            else if ((num4xProfiles == 1 || migrateAll) && numProfiles == 0) {
                PRBool confirmed = PR_FALSE;
                if (NS_SUCCEEDED(ConfirmAutoMigration(canInteract, &confirmed)) && confirmed)
                    AutoMigrate();
                else
                    profileURLStr = PROFILE_MANAGER_URL;
            }
            else if (numProfiles > 1)
            {
                profileURLStr = PROFILE_SELECTION_URL;
            }
            else {
                
                profileURLStr = PROFILE_MANAGER_URL;
            }
        }
    }

#ifdef DEBUG_profile_verbose
    printf("Profile Manager : Command Line Options : End\n");
#endif

    return NS_OK;
}








NS_IMETHODIMP nsProfile::GetProfileDir(const PRUnichar *profileName, nsIFile **profileDir)
{
    NS_ENSURE_ARG(profileName);   
    NS_ENSURE_ARG_POINTER(profileDir);
    *profileDir = nsnull;

    nsresult rv = NS_OK;

#if defined(DEBUG_profile_verbose)
    printf("ProfileManager : GetProfileDir\n");
#endif

    ProfileStruct    *aProfile;

    rv = gProfileDataAccess->GetValue(profileName, &aProfile);
    if (NS_FAILED(rv)) return rv;

	if (aProfile == nsnull)
		return NS_ERROR_FAILURE;

    nsCOMPtr<nsILocalFile>aProfileDir;
    rv = aProfile->GetResolvedProfileDir(getter_AddRefs(aProfileDir));
    if (NS_SUCCEEDED(rv) && aProfileDir)
    {
#ifdef XP_MAC
        PRBool exists;
        rv = aProfileDir->Exists(&exists);
        if (NS_FAILED(rv)) return rv;
        if (exists) {
            PRBool inTrash;
            nsCOMPtr<nsIFile> trashFolder;
                            
            rv = NS_GetSpecialDirectory(NS_MAC_TRASH_DIR, getter_AddRefs(trashFolder));
            if (NS_FAILED(rv)) return rv;
            rv = trashFolder->Contains(aProfileDir, PR_TRUE, &inTrash);
            if (NS_FAILED(rv)) return rv;
            if (inTrash) {
                aProfileDir = nsnull;
                rv = NS_ERROR_FILE_NOT_FOUND;
            }  
        }
#endif
        *profileDir = aProfileDir;
        NS_IF_ADDREF(*profileDir);
    }    
	delete aProfile;
    return rv;
}

NS_IMETHODIMP nsProfile::GetProfilePath(const PRUnichar *profileName, PRUnichar **_retval)
{
    NS_ENSURE_ARG(profileName);
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = nsnull;
    
    nsCOMPtr<nsIFile> profileDir;
    nsresult rv = GetProfileDir(profileName, getter_AddRefs(profileDir));
    if (NS_FAILED(rv)) return rv;
    
    PRBool isSalted;    
    nsCOMPtr<nsIFile> prettyDir(profileDir);
    rv = IsProfileDirSalted(profileDir, &isSalted);
    if (NS_SUCCEEDED(rv) && isSalted) {
        nsCOMPtr<nsIFile> parentDir;
        rv = profileDir->GetParent(getter_AddRefs(parentDir));
        if (NS_SUCCEEDED(rv))
            prettyDir = parentDir;
    }
    nsAutoString path;
    rv = prettyDir->GetPath(path);
    if (NS_FAILED(rv)) return rv;

    *_retval = ToNewUnicode(path);
    return *_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP nsProfile::GetOriginalProfileDir(const PRUnichar *profileName, nsILocalFile **originalDir)
{
    NS_ENSURE_ARG(profileName);
    NS_ENSURE_ARG_POINTER(originalDir);
    *originalDir = nsnull;

    Update4xProfileInfo();
    return gProfileDataAccess->GetOriginalProfileDir(profileName, originalDir);
}

NS_IMETHODIMP nsProfile::GetProfileLastModTime(const PRUnichar *profileName, PRInt64 *_retval)
{
    NS_ENSURE_ARG(profileName);
    NS_ENSURE_ARG_POINTER(_retval);
    nsresult rv;
    
    
    
    
    ProfileStruct *profileInfo = nsnull;
    rv = gProfileDataAccess->GetValue(profileName, &profileInfo);
    if (NS_SUCCEEDED(rv)) {
        PRInt64 lastModTime = profileInfo->lastModTime;
        delete profileInfo;
        if (!LL_IS_ZERO(lastModTime)) {
            *_retval = lastModTime;
            return NS_OK;
        }
    }

    
    
    
    nsCOMPtr<nsIFile> profileDir;
    rv = GetProfileDir(profileName, getter_AddRefs(profileDir));
    if (NS_FAILED(rv))
        return rv;
    rv = profileDir->AppendNative(NS_LITERAL_CSTRING("prefs.js"));
    if (NS_FAILED(rv))
        return rv;
    return profileDir->GetLastModifiedTime(_retval);
}

NS_IMETHODIMP nsProfile::GetDefaultProfileParentDir(nsIFile **aDefaultProfileParentDir)
{
    NS_ENSURE_ARG_POINTER(aDefaultProfileParentDir);

    nsresult rv;
    
    nsCOMPtr<nsIFile> aDir;
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILES_ROOT_DIR, getter_AddRefs(aDir));
    NS_ENSURE_SUCCESS(rv, rv);

    *aDefaultProfileParentDir = aDir;
    NS_ADDREF(*aDefaultProfileParentDir);

    return NS_OK;
}



NS_IMETHODIMP nsProfile::GetProfileCount(PRInt32 *numProfiles)
{
    NS_ENSURE_ARG_POINTER(numProfiles);

    *numProfiles = 0;

    gProfileDataAccess->GetNumProfiles(numProfiles);
    return NS_OK;
}





NS_IMETHODIMP nsProfile::GetFirstProfile(PRUnichar **profileName)
{
    NS_ENSURE_ARG_POINTER(profileName);

    gProfileDataAccess->GetFirstProfile(profileName);
    gProfileDataAccess->SetCurrentProfile(*profileName);

    return NS_OK;
}

NS_IMETHODIMP nsProfile::GetStartWithLastUsedProfile(PRBool *aStartWithLastUsedProfile)
{
    NS_ENSURE_ARG_POINTER(aStartWithLastUsedProfile);
    return gProfileDataAccess->GetStartWithLastUsedProfile(aStartWithLastUsedProfile);
}

NS_IMETHODIMP nsProfile::SetStartWithLastUsedProfile(PRBool aStartWithLastUsedProfile)
{
    return gProfileDataAccess->SetStartWithLastUsedProfile(aStartWithLastUsedProfile);
}


NS_IMETHODIMP
nsProfile::GetCurrentProfile(PRUnichar **profileName)
{
    NS_ENSURE_ARG_POINTER(profileName);
    *profileName = nsnull;

    if (!mCurrentProfileName.IsEmpty()) 
      *profileName = ToNewUnicode(mCurrentProfileName);
    else 
      gProfileDataAccess->GetCurrentProfile(profileName);
    return (*profileName == nsnull) ? NS_ERROR_FAILURE : NS_OK;
}


NS_IMETHODIMP
nsProfile::SetCurrentProfile(const PRUnichar * aCurrentProfile)
{
    NS_ENSURE_ARG(aCurrentProfile);
    
    nsresult rv;
    nsCOMPtr<nsIFile> profileDir;
    PRBool exists;

    
    rv = GetProfileDir(aCurrentProfile, getter_AddRefs(profileDir));
    if (NS_FAILED(rv)) return rv;
    rv = profileDir->Exists(&exists);
    if (NS_FAILED(rv)) return rv;
    if (!exists) return NS_ERROR_FILE_NOT_FOUND;

    PRBool isSwitch;

    if (mCurrentProfileAvailable)
    {
        nsXPIDLString currProfileName;
        rv = GetCurrentProfile(getter_Copies(currProfileName));
        if (NS_FAILED(rv)) return rv;
        if (nsCRT::strcmp(aCurrentProfile, currProfileName.get()) == 0)
            return NS_OK;
        else
            isSwitch = PR_TRUE;
    }
    else
        isSwitch = PR_FALSE;
#ifdef MOZ_PROFILELOCKING    
    nsProfileLock localLock;
    nsCOMPtr<nsILocalFile> localProfileDir(do_QueryInterface(profileDir, &rv));
    if (NS_FAILED(rv)) return rv;
    rv = localLock.Lock(localProfileDir, nsnull);
    if (NS_FAILED(rv))
    {
        NS_ERROR("Could not get profile directory lock.");
        return rv;
    }
#endif
    nsCOMPtr<nsIObserverService> observerService = 
             do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_TRUE(observerService, NS_ERROR_FAILURE);
    
    nsISupports *subject = (nsISupports *)((nsIProfile *)this);
    NS_NAMED_LITERAL_STRING(switchString, "switch");
    NS_NAMED_LITERAL_STRING(startupString, "startup");
    const nsAFlatString& context = isSwitch ? switchString : startupString;
    
    if (isSwitch)
    {
        
        mProfileChangeVetoed = PR_FALSE;        
        observerService->NotifyObservers(subject, "profile-approve-change", context.get());
        if (mProfileChangeVetoed)
            return NS_OK;

        
        observerService->NotifyObservers(subject, "profile-change-net-teardown", context.get());
        mShutdownProfileToreDownNetwork = PR_TRUE;

        
        observerService->NotifyObservers(subject, "profile-change-teardown", context.get());
        if (mProfileChangeVetoed)
        {
            
            observerService->NotifyObservers(subject, "profile-change-teardown-veto", context.get());

            
            observerService->NotifyObservers(subject, "profile-change-net-restore", context.get());
            return NS_OK;
        }
        
        
        
        nsCOMPtr<nsIThreadJSContextStack> stack =
          do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
        if (NS_SUCCEEDED(rv))
        {
          JSContext *cx = nsnull;
          stack->GetSafeJSContext(&cx);
          if (cx)
            ::JS_GC(cx);
        }
        
        
        observerService->NotifyObservers(subject, "profile-before-change", context.get());        
        if (mProfileChangeFailed)
          return NS_ERROR_ABORT;
        
        UpdateCurrentProfileModTime(PR_FALSE);        
    }

#ifdef MOZ_PROFILELOCKING    
    
    localLock.Unlock(); 
#endif
    nsCOMPtr<nsIFile> localDir;
    GetLocalProfileDir(aCurrentProfile, getter_AddRefs(localDir));
    gDirServiceProvider->SetProfileDir(profileDir, localDir);
    mCurrentProfileName.Assign(aCurrentProfile);    
    gProfileDataAccess->SetCurrentProfile(aCurrentProfile);
    gProfileDataAccess->mProfileDataChanged = PR_TRUE;
    gProfileDataAccess->UpdateRegistry(nsnull);
            
    if (NS_FAILED(rv)) return rv;
    mCurrentProfileAvailable = PR_TRUE;
    
    if (!isSwitch)
    {
        
        
        nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Could not get prefs service");
    }

    if (mShutdownProfileToreDownNetwork)
    {
        
        observerService->NotifyObservers(subject, "profile-change-net-restore", context.get());
        mShutdownProfileToreDownNetwork = PR_FALSE;
        if (mProfileChangeFailed)
          return NS_ERROR_ABORT;
    }

    
    
    nsCOMPtr <nsISessionRoaming> roam =
      do_GetService(NS_SESSIONROAMING_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
      roam->BeginSession();

    
    observerService->NotifyObservers(subject, "profile-do-change", context.get());
    if (mProfileChangeFailed)
      return NS_ERROR_ABORT;

    
    observerService->NotifyObservers(subject, "profile-after-change", context.get());
    if (mProfileChangeFailed)
      return NS_ERROR_ABORT;

    
    rv = DefineLocaleDefaultsDir();
    NS_ASSERTION(NS_SUCCEEDED(rv), "nsProfile::DefineLocaleDefaultsDir failed");

    
    observerService->NotifyObservers(subject, "profile-initial-state", context.get());
    if (mProfileChangeFailed)
      return NS_ERROR_ABORT;

    nsCOMPtr<nsICategoryManager> catman =
             do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);

    if(NS_SUCCEEDED(rv) && catman)
    {
        nsCOMPtr<nsISimpleEnumerator> enumItem;
        rv = catman->EnumerateCategory(NS_PROFILE_STARTUP_CATEGORY, getter_AddRefs(enumItem));
        if(NS_SUCCEEDED(rv) && enumItem)
        {
           while (PR_TRUE)
           {
               nsCOMPtr<nsISupportsCString> contractid;

               rv = enumItem->GetNext(getter_AddRefs(contractid));
               if (NS_FAILED(rv) || !contractid) break;

               nsCAutoString contractidString;
               contractid->GetData(contractidString);
        
               nsCOMPtr <nsIProfileStartupListener> listener = do_GetService(contractidString.get(), &rv);
        
               if (listener)
                   listener->OnProfileStartup(aCurrentProfile);
           }
        }
    }

    nsCOMPtr<nsIPrefBranch> prefBranch;
  
    nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    rv = prefs->GetBranch(nsnull, getter_AddRefs(prefBranch));
    if (NS_FAILED(rv)) return rv;

    PRBool prefs_converted = PR_FALSE;
    (void)prefBranch->GetBoolPref("prefs.converted-to-utf8", &prefs_converted);

    if (!prefs_converted)
    {
        nsCOMPtr <nsIPrefConverter> pPrefConverter = do_GetService(kPrefConverterCID, &rv);
        if (!pPrefConverter) return NS_ERROR_FAILURE;
        rv = pPrefConverter->ConvertPrefsToUTF8();
        if (NS_FAILED(rv)) return rv;
    }
    
    return NS_OK;
}



NS_IMETHODIMP nsProfile::GetCurrentProfileDir(nsIFile **profileDir)
{
    NS_ENSURE_ARG_POINTER(profileDir);
    nsresult rv;

    nsXPIDLString profileName;
    rv = GetCurrentProfile(getter_Copies(profileName));
    if (NS_FAILED(rv)) return rv;

    rv = GetProfileDir(profileName, profileDir);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}


NS_IMETHODIMP nsProfile::ShutDownCurrentProfile(PRUint32 shutDownType)
{
    nsresult rv;
    
    
    
    if (shutDownType == SHUTDOWN_PERSIST || shutDownType == SHUTDOWN_CLEANSE) {
      nsCOMPtr<nsIObserverService> observerService = 
        do_GetService("@mozilla.org/observer-service;1", &rv);
      NS_ENSURE_TRUE(observerService, NS_ERROR_FAILURE);
      
      nsISupports *subject = (nsISupports *)((nsIProfile *)this);
      
      NS_NAMED_LITERAL_STRING(cleanseString, "shutdown-cleanse");
      NS_NAMED_LITERAL_STRING(persistString, "shutdown-persist");
      const nsAFlatString& context = (shutDownType == SHUTDOWN_CLEANSE) ? cleanseString : persistString;
      
      
      mProfileChangeVetoed = PR_FALSE;        
      observerService->NotifyObservers(subject, "profile-approve-change", context.get());
      if (mProfileChangeVetoed)
        return NS_OK;
      
      
      observerService->NotifyObservers(subject, "profile-change-net-teardown", context.get());
      mShutdownProfileToreDownNetwork = PR_TRUE;

      
      observerService->NotifyObservers(subject, "profile-change-teardown", context.get());

      
      
      nsCOMPtr<nsIThreadJSContextStack> stack =
        do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
      if (NS_SUCCEEDED(rv))
      {
        JSContext *cx = nsnull;
        stack->GetSafeJSContext(&cx);
        if (cx)
          ::JS_GC(cx);
      }
      
      
      observerService->NotifyObservers(subject, "profile-before-change", context.get());        
    }

    
    
    nsCOMPtr <nsISessionRoaming> roam =
      do_GetService(NS_SESSIONROAMING_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
      roam->EndSession();

    gDirServiceProvider->SetProfileDir(nsnull);
    UpdateCurrentProfileModTime(PR_TRUE);
    mCurrentProfileAvailable = PR_FALSE;
    mCurrentProfileName.Truncate(0);
    
    return NS_OK;
}


#define SALT_SIZE 8
#define TABLE_SIZE 36

static const char kSaltExtensionCString[] = ".slt";
#define kSaltExtensionCString_Len PRUint32(sizeof(kSaltExtensionCString)-1)

static const char table[] = 
	{ 'a','b','c','d','e','f','g','h','i','j',
	  'k','l','m','n','o','p','q','r','s','t',
	  'u','v','w','x','y','z','0','1','2','3',
	  '4','5','6','7','8','9'};




nsresult
nsProfile::AddLevelOfIndirection(nsIFile *aDir)
{
  nsresult rv;
  PRBool exists = PR_FALSE;
  if (!aDir) return NS_ERROR_NULL_POINTER;

  
  
  
  nsCOMPtr<nsIFile> prefFile;
  rv = aDir->Clone(getter_AddRefs(prefFile));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = prefFile->AppendNative(NS_LITERAL_CSTRING("prefs.js"));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = prefFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv,rv);

  if (exists) {
	
	return NS_OK;
  }

  
  PRBool hasMore = PR_FALSE;
  PRBool isDir = PR_FALSE;
  nsCOMPtr<nsISimpleEnumerator> dirIterator;
  rv = aDir->GetDirectoryEntries(getter_AddRefs(dirIterator));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = dirIterator->HasMoreElements(&hasMore);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIFile> dirEntry;

  while (hasMore) {
    rv = dirIterator->GetNext((nsISupports**)getter_AddRefs(dirEntry));
    if (NS_SUCCEEDED(rv)) {
      rv = dirEntry->IsDirectory(&isDir);
      if (NS_SUCCEEDED(rv) && isDir) {
        nsCAutoString leafName;
        rv = dirEntry->GetNativeLeafName(leafName);
	 	if (NS_SUCCEEDED(rv) && !leafName.IsEmpty()) {
		  PRUint32 length = leafName.Length();
		  
		  if (length == (SALT_SIZE + kSaltExtensionCString_Len)) {
			
			if (nsCRT::strncmp(leafName.get() + SALT_SIZE,
                         kSaltExtensionCString,
                         kSaltExtensionCString_Len) == 0) {
			  
			  rv = aDir->AppendNative(leafName);
			  return rv;
			}
		  }
		}
      }
    }
    rv = dirIterator->HasMoreElements(&hasMore);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  
  

  
  
  double fpTime;
  LL_L2D(fpTime, PR_Now());
  srand((uint)(fpTime * 1e-6 + 0.5));	

  nsCAutoString saltStr;
  PRInt32 i;
  for (i=0;i<SALT_SIZE;i++) {
  	saltStr.Append(table[rand()%TABLE_SIZE]);
  }
  saltStr.Append(kSaltExtensionCString, kSaltExtensionCString_Len);
#ifdef DEBUG_profile_verbose
  printf("directory name: %s\n",saltStr.get());
#endif

  rv = aDir->AppendNative(saltStr);
  NS_ENSURE_SUCCESS(rv,rv);

  exists = PR_FALSE;
  rv = aDir->Exists(&exists);
  NS_ENSURE_SUCCESS(rv,rv);
  if (!exists) {
    rv = aDir->Create(nsIFile::DIRECTORY_TYPE, 0700);
    NS_ENSURE_SUCCESS(rv,rv);
  }
	
  return NS_OK;
}

nsresult nsProfile::IsProfileDirSalted(nsIFile *profileDir, PRBool *isSalted)
{
    nsresult rv;
    NS_ENSURE_ARG_POINTER(isSalted);
    *isSalted = PR_FALSE;
    
    
    nsCAutoString leafName;
    rv = profileDir->GetNativeLeafName(leafName);
    if (NS_FAILED(rv)) return rv;

    PRBool endsWithSalt = PR_FALSE;    
    if (leafName.Length() >= kSaltExtensionCString_Len)
    {
        nsReadingIterator<char> stringEnd;
        leafName.EndReading(stringEnd);

        nsReadingIterator<char> stringStart = stringEnd;
        stringStart.advance( -(NS_STATIC_CAST(PRInt32, kSaltExtensionCString_Len)) );

        endsWithSalt =
            Substring(stringStart, stringEnd).Equals(kSaltExtensionCString);
    }
    if (!endsWithSalt)
        return NS_OK;
    
    
    nsCOMPtr<nsIFile> parentDir;
    rv = profileDir->GetParent(getter_AddRefs(parentDir));
    if (NS_FAILED(rv)) return rv;

    PRBool hasMore;
    nsCOMPtr<nsISimpleEnumerator> dirIterator;
    rv = parentDir->GetDirectoryEntries(getter_AddRefs(dirIterator));
    if (NS_FAILED(rv)) return rv;
    
    PRInt32 numChildren = 0;
    rv = dirIterator->HasMoreElements(&hasMore);
    
    while (NS_SUCCEEDED(rv) && hasMore && numChildren <= 1) {
        nsCOMPtr<nsIFile> child;
        rv = dirIterator->GetNext((nsISupports**)getter_AddRefs(child));    
        if (NS_SUCCEEDED(rv))
            ++numChildren;
        rv = dirIterator->HasMoreElements(&hasMore);
    }
    if (NS_SUCCEEDED(rv) && numChildren == 1)
        *isSalted = PR_TRUE;
    
    return NS_OK;
}






nsresult nsProfile::SetProfileDir(const PRUnichar *profileName, nsIFile *profileDir)
{
    NS_ENSURE_ARG(profileName);
    NS_ENSURE_ARG(profileDir);   

    nsresult rv = NS_OK;
 
    
    PRBool exists;
    rv = profileDir->Exists(&exists);
    if (NS_SUCCEEDED(rv) && !exists)
        rv = profileDir->Create(nsIFile::DIRECTORY_TYPE, 0700);
    if (NS_FAILED(rv)) 
        return rv;
    
    nsCOMPtr<nsILocalFile> localFile(do_QueryInterface(profileDir));
    NS_ENSURE_TRUE(localFile, NS_ERROR_FAILURE);                

    ProfileStruct* aProfile = new ProfileStruct();
    NS_ENSURE_TRUE(aProfile, NS_ERROR_OUT_OF_MEMORY);

    aProfile->profileName = profileName;
    aProfile->SetResolvedProfileDir(localFile);
    aProfile->isMigrated = PR_TRUE;
    aProfile->isImportType = PR_FALSE;

    
    PRInt64 oneThousand = LL_INIT(0, 1000);
    PRInt64 nowInMilliSecs = PR_Now();
    LL_DIV(aProfile->creationTime, nowInMilliSecs, oneThousand); 

    gProfileDataAccess->SetValue(aProfile);
    
    delete aProfile;

    return rv;
}


NS_IMETHODIMP 
nsProfile::CreateNewProfileWithLocales(const PRUnichar* profileName, 
                            const PRUnichar* nativeProfileDir,
                            const PRUnichar* aUILocale,
                            const PRUnichar* aContentLocale,
                            PRBool useExistingDir)
{
    NS_ENSURE_ARG_POINTER(profileName);   

    nsresult rv = NS_OK;

#if defined(DEBUG_profile)
    {
      printf("ProfileManager : CreateNewProfileWithLocales\n");

      printf("Profile Name: %s\n", NS_LossyConvertUTF16toASCII(profileName).get());

      if (nativeProfileDir)
        printf("Profile Dir: %s\n", NS_LossyConvertUTF16toASCII(nativeProfileDir).get());
    }
#endif

    nsCOMPtr<nsIFile> profileDir;
    PRBool exists;
    
    if (!nativeProfileDir)
    {
        
                
        rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILES_ROOT_DIR, getter_AddRefs(profileDir));
        if (NS_FAILED(rv)) return rv;
        rv = profileDir->Exists(&exists);
        if (NS_FAILED(rv)) return rv;        
        if (!exists)
            profileDir->Create(nsIFile::DIRECTORY_TYPE, 0700);

        
        profileDir->Append(nsDependentString(profileName));
    }
    else {
    
        rv = NS_NewLocalFile(nsDependentString(nativeProfileDir), PR_TRUE, (nsILocalFile **)((nsIFile **)getter_AddRefs(profileDir)));
        if (NS_FAILED(rv)) return rv;

        
        
        
        profileDir->Append(nsDependentString(profileName));
    }


    
    
    if (!useExistingDir) {
        rv = profileDir->CreateUnique(nsIFile::DIRECTORY_TYPE, 0700);
        if (NS_FAILED(rv)) return rv;
    }

#if defined(DEBUG_profile_verbose)
    printf("before SetProfileDir\n");
#endif

    rv = profileDir->Exists(&exists);
    if (NS_FAILED(rv)) return rv;        
    if (!exists)
    {
        rv = profileDir->Create(nsIFile::DIRECTORY_TYPE, 0700);
        if (NS_FAILED(rv)) return rv;
        useExistingDir = PR_FALSE;
    }

    
    rv = AddLevelOfIndirection(profileDir);
    if (NS_FAILED(rv)) return rv;

    
    rv = SetProfileDir(profileName, profileDir);

#if defined(DEBUG_profile_verbose)
    printf("after SetProfileDir\n");
#endif

    
    nsCOMPtr <nsIFile> profDefaultsDir;
    rv = NS_GetSpecialDirectory(NS_APP_PROFILE_DEFAULTS_NLOC_50_DIR, getter_AddRefs(profDefaultsDir));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIChromeRegistrySea> chromeRegistry =
        do_GetService(NS_CHROMEREGISTRY_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {

        nsCAutoString uiLocale, contentLocale;
        LossyCopyUTF16toASCII(aUILocale, uiLocale);
        LossyCopyUTF16toASCII(aContentLocale, contentLocale);

        
        
        

        
        
        
        
        

        
        
        

        nsCOMPtr<nsIChromeRegistrySea> packageRegistry = do_QueryInterface(chromeRegistry);
        if (uiLocale.IsEmpty() && packageRegistry) {
            nsCAutoString currentUILocaleName;
            rv = packageRegistry->GetSelectedLocale(NS_LITERAL_CSTRING("global"),
                                                    currentUILocaleName);
            if (NS_SUCCEEDED(rv)) {
                uiLocale = currentUILocaleName;
            }
        }

        if (contentLocale.IsEmpty()) {
            nsCAutoString currentContentLocaleName;
            rv = packageRegistry->GetSelectedLocale(NS_LITERAL_CSTRING("global-region"),
                                                    currentContentLocaleName);
            if (NS_SUCCEEDED(rv)) {
                contentLocale = currentContentLocaleName;
            }
        }

#if defined(DEBUG_profile_verbose)
        printf(" uiLocale=%s\n", uiLocale.get());
        printf(" contentLocale=%s\n", contentLocale.get());
#endif

        nsCAutoString pathBuf;
        rv = profileDir->GetNativePath(pathBuf);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCAutoString fileStr;
        rv = NS_GetURLSpecFromFile(profileDir, fileStr);
        if (NS_FAILED(rv)) return rv;

        if (!uiLocale.IsEmpty()) {
            rv = chromeRegistry->SelectLocaleForProfile(uiLocale, 
                                                        NS_ConvertUTF8toUTF16(fileStr).get());
            
            
            if (NS_SUCCEEDED(rv)) {
                nsCStringKey key(pathBuf);
                gLocaleProfiles->Put(&key, (void*)PR_TRUE);
            }
        }

        
        nsCAutoString currentSkinName;
        rv = packageRegistry->GetSelectedSkin(NS_LITERAL_CSTRING("global"),currentSkinName);
        if (!currentSkinName.IsEmpty()) {
            rv = chromeRegistry->SelectSkinForProfile(currentSkinName,
                                         NS_ConvertUTF8toUTF16(fileStr).get());
        }

        if (!contentLocale.IsEmpty()) {
            
            nsCOMPtr<nsIFile> locProfDefaultsDir;
            rv = profDefaultsDir->Clone(getter_AddRefs(locProfDefaultsDir));
            if (NS_FAILED(rv)) return rv;
        
            locProfDefaultsDir->AppendNative(contentLocale);
            rv = locProfDefaultsDir->Exists(&exists);
            if (NS_SUCCEEDED(rv) && exists) {
                profDefaultsDir = locProfDefaultsDir; 
#if defined(DEBUG_profile_verbose)
                nsCAutoString profilePath;
                rv = profDefaultsDir->GetNativePath(profilePath);
                if (NS_SUCCEEDED(rv))
                    printf(" profDefaultsDir is set to: %s\n", profilePath.get());
#endif
            }

            rv = chromeRegistry->SelectLocaleForProfile(contentLocale, 
                                                        NS_ConvertUTF8toUTF16(fileStr).get());
            
            
            if (NS_SUCCEEDED(rv)) {
                nsCStringKey key(pathBuf);
                gLocaleProfiles->Put(&key, (void*)PR_TRUE);
            }
        }
    }

    
    rv = profDefaultsDir->Exists(&exists);
    if (NS_SUCCEEDED(rv) && exists && (!useExistingDir))
    {
        RecursiveCopy(profDefaultsDir, profileDir);
    }

    gProfileDataAccess->mProfileDataChanged = PR_TRUE;
    gProfileDataAccess->UpdateRegistry(nsnull);
    
    return NS_OK;
}	






NS_IMETHODIMP 
nsProfile::CreateNewProfile(const PRUnichar* profileName, 
                            const PRUnichar* nativeProfileDir,
                            const PRUnichar* langcode,
                            PRBool useExistingDir)
{
    return CreateNewProfileWithLocales(profileName,nativeProfileDir,langcode,nsnull,useExistingDir);
}





NS_IMETHODIMP 
nsProfile::RenameProfile(const PRUnichar* oldName, const PRUnichar* newName)
{
    NS_ENSURE_ARG_POINTER(oldName);   
    NS_ENSURE_ARG_POINTER(newName);   

    nsresult rv = NS_OK;

#if defined(DEBUG_profile)
    {
      printf("ProfileManager : Renaming profile\n");

      nsCAutoString temp1; temp1.AssignWithConversion(oldName);
      printf("Old name:  %s\n", NS_LossyConvertUTF16toASCII(oldName).get());

      nsCAutoString temp2; temp2.AssignWithConversion(newName);
      printf("New name:  %s\n", NS_LossyConvertUTF16toASCII(newName).get());
    }
#endif

    PRBool exists;
    rv = ProfileExists(newName, &exists);
    if (NS_FAILED(rv)) return rv;
    
    
    if (exists) {
#if defined(DEBUG_profile)  
        printf("ProfileManager : Rename Operation failed : Profile exists. Provide a different new name for profile.\n");
#endif
        return NS_ERROR_FAILURE;
    }

    PRBool renamedIsCurrent = mCurrentProfileName.Equals(oldName);

    
    rv = CopyRegKey(oldName, newName);
    if (NS_FAILED(rv)) return rv;
     
    
    rv = DeleteProfile(oldName, PR_FALSE );

    
    
    if (renamedIsCurrent) {
        gProfileDataAccess->SetCurrentProfile(newName);
        gProfileDataAccess->mForgetProfileCalled = PR_FALSE;
        mCurrentProfileName.Assign(newName);    
        mCurrentProfileAvailable = PR_TRUE;
    }

    if (NS_FAILED(rv)) return rv;
     
	















    gProfileDataAccess->mProfileDataChanged = PR_TRUE;
    gProfileDataAccess->UpdateRegistry(nsnull);

    return NS_OK;
}



nsresult nsProfile::CopyRegKey(const PRUnichar *oldProfile, const PRUnichar *newProfile)
{
    NS_ENSURE_ARG_POINTER(oldProfile);   
    NS_ENSURE_ARG_POINTER(newProfile);   

    nsresult rv = NS_OK;

    ProfileStruct    *aProfile;

    rv = gProfileDataAccess->GetValue(oldProfile, &aProfile);
    if (NS_FAILED(rv)) return rv;

    aProfile->profileName        = newProfile;

    rv = gProfileDataAccess->SetValue(aProfile);

	delete aProfile;

    return rv;
}

NS_IMETHODIMP nsProfile::ForgetCurrentProfile()
{
    nsresult rv = NS_OK;

    
    PRUnichar tmp[] = { '\0' };

    gProfileDataAccess->SetCurrentProfile(tmp);
    if (NS_FAILED(rv)) return rv;  

    gProfileDataAccess->mForgetProfileCalled = PR_TRUE;
    mCurrentProfileAvailable = PR_FALSE;
    mCurrentProfileName.Truncate(0);
    
    return rv;
}




NS_IMETHODIMP nsProfile::DeleteProfile(const PRUnichar* profileName, PRBool canDeleteFiles)
{
    NS_ENSURE_ARG_POINTER(profileName);   

    nsresult rv;
 
    nsXPIDLString currProfile;
    rv = GetCurrentProfile(getter_Copies(currProfile));
    if (NS_SUCCEEDED(rv) && !nsCRT::strcmp(profileName, currProfile)) {
        rv = ForgetCurrentProfile();
        if (NS_FAILED(rv)) return rv;
    }
    rv = NS_OK;
    
    
    if (canDeleteFiles) {
    
        nsCOMPtr<nsIFile> profileDir;
        rv = GetProfileDir(profileName, getter_AddRefs(profileDir));
        if (NS_FAILED(rv)) return rv;
        
        PRBool exists;
        rv = profileDir->Exists(&exists);
        if (NS_FAILED(rv)) return rv;
        
        if (exists) {

            
            
            
            
            nsCOMPtr<nsIFile> dirToDelete(profileDir);
            PRBool isSalted;
            rv = IsProfileDirSalted(profileDir, &isSalted);
            if (NS_SUCCEEDED(rv) && isSalted) {
                nsCOMPtr<nsIFile> parentDir;
                rv = profileDir->GetParent(getter_AddRefs(parentDir));
                if (NS_SUCCEEDED(rv))
                    dirToDelete = parentDir;
            }
            rv = dirToDelete->Remove(PR_TRUE);
        }
    }

    
    gProfileDataAccess->RemoveSubTree(profileName);
    if (NS_FAILED(rv)) return rv;

    gProfileDataAccess->mProfileDataChanged = PR_TRUE;
    gProfileDataAccess->UpdateRegistry(nsnull);

    return rv;
}


NS_IMETHODIMP nsProfile::GetProfileList(PRUint32 *length, PRUnichar ***profileNames)
{
    NS_ENSURE_ARG_POINTER(length);
    *length = 0;
    NS_ENSURE_ARG_POINTER(profileNames);
    *profileNames = nsnull;
    
    return gProfileDataAccess->GetProfileList(nsIProfileInternal::LIST_ONLY_NEW, length, profileNames);
}
 

NS_IMETHODIMP nsProfile::GetProfileListX(PRUint32 whichKind, PRUint32 *length, PRUnichar ***profileNames)
{
    NS_ENSURE_ARG_POINTER(length);
    *length = 0;
    NS_ENSURE_ARG_POINTER(profileNames);
    *profileNames = nsnull;
    
    if (whichKind == nsIProfileInternal::LIST_FOR_IMPORT)
        Update4xProfileInfo();
    return gProfileDataAccess->GetProfileList(whichKind, length, profileNames);
}



nsresult nsProfile::Update4xProfileInfo()
{
    nsresult rv = NS_OK;

#ifndef XP_BEOS
    nsCOMPtr<nsIFile> oldRegFile;
    rv = GetOldRegLocation(getter_AddRefs(oldRegFile));
    if (NS_SUCCEEDED(rv))
    rv = gProfileDataAccess->Get4xProfileInfo(oldRegFile, PR_TRUE);
#endif

    return rv;
}

nsresult nsProfile::LoadNewProfilePrefs()
{
    nsresult rv;
    nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    prefs->ResetUserPrefs();
    prefs->ReadUserPrefs(nsnull);

    return NS_OK;
}

nsresult nsProfile::GetOldRegLocation(nsIFile **aOldRegFile)
{
    NS_ENSURE_ARG_POINTER(aOldRegFile);
    *aOldRegFile = nsnull;
    nsresult rv = NS_OK;
    
    
    
    
#if defined(XP_WIN) || defined(XP_OS2) || defined(XP_MAC) || defined(XP_MACOSX)
    nsCOMPtr<nsIFile> oldRegFile;

#if defined(XP_WIN)
    rv = NS_GetSpecialDirectory(NS_WIN_WINDOWS_DIR, getter_AddRefs(oldRegFile));
#elif defined(XP_OS2)
    rv = NS_GetSpecialDirectory(NS_OS2_DIR, getter_AddRefs(oldRegFile));
#elif defined(XP_MAC) || defined(XP_MACOSX)
    rv = NS_GetSpecialDirectory(NS_MAC_PREFS_DIR, getter_AddRefs(oldRegFile));
#endif

    if (NS_FAILED(rv))
        return rv;
    rv = oldRegFile->AppendNative(nsDependentCString(OLD_REGISTRY_FILE_NAME));
    if (NS_FAILED(rv))
        return rv;
    NS_ADDREF(*aOldRegFile = oldRegFile);
#endif

    return rv;
}

nsresult nsProfile::UpdateCurrentProfileModTime(PRBool updateRegistry)
{
    nsresult rv;

    
    PRInt64 oneThousand = LL_INIT(0, 1000);
    PRInt64 nowInMilliSecs = PR_Now();
    LL_DIV(nowInMilliSecs, nowInMilliSecs, oneThousand); 
    
    rv = gProfileDataAccess->SetProfileLastModTime(mCurrentProfileName.get(), nowInMilliSecs);
    if (NS_SUCCEEDED(rv) && updateRegistry) {
        gProfileDataAccess->mProfileDataChanged = PR_TRUE;
        gProfileDataAccess->UpdateRegistry(nsnull);
    }
    return rv;
}


NS_IMETHODIMP nsProfile::MigrateProfileInfo()
{
    nsresult rv = NS_OK;

#ifndef XP_BEOS
    nsCOMPtr<nsIFile> oldRegFile;
    rv = GetOldRegLocation(getter_AddRefs(oldRegFile));
    if (NS_SUCCEEDED(rv)) {
    rv = gProfileDataAccess->Get4xProfileInfo(oldRegFile, PR_FALSE);
    gProfileDataAccess->mProfileDataChanged = PR_TRUE;
    gProfileDataAccess->UpdateRegistry(nsnull);
    }
#endif

	return rv;
}

nsresult
nsProfile::CopyDefaultFile(nsIFile *profDefaultsDir, nsIFile *newProfDir, const nsACString &fileName)
{
    nsresult rv;
    nsCOMPtr<nsIFile> defaultFile;
    PRBool exists;
    
    rv = profDefaultsDir->Clone(getter_AddRefs(defaultFile));
    if (NS_FAILED(rv)) return rv;
    
    defaultFile->AppendNative(fileName);
    rv = defaultFile->Exists(&exists);
    if (NS_FAILED(rv)) return rv;
    if (exists)
        rv = defaultFile->CopyToNative(newProfDir, fileName);
    else
        rv = NS_ERROR_FILE_NOT_FOUND;

	return rv;
}

nsresult
nsProfile::DefineLocaleDefaultsDir()
{
    nsresult rv;
    
    nsCOMPtr<nsIProperties> directoryService = 
             do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
    NS_ENSURE_TRUE(directoryService, NS_ERROR_FAILURE);    

    nsCOMPtr<nsIFile> localeDefaults;
    rv = directoryService->Get(NS_APP_PROFILE_DEFAULTS_NLOC_50_DIR, NS_GET_IID(nsIFile), getter_AddRefs(localeDefaults));
    if (NS_SUCCEEDED(rv))
    {
        nsCOMPtr<nsIChromeRegistrySea> packageRegistry = 
                 do_GetService(NS_CHROMEREGISTRY_CONTRACTID, &rv);
        if (NS_SUCCEEDED(rv))
        {
            nsCAutoString localeName;
            rv = packageRegistry->GetSelectedLocale(NS_LITERAL_CSTRING("global-region"), localeName);
            if (NS_SUCCEEDED(rv))
                rv = localeDefaults->AppendNative(localeName);
        }
        rv = directoryService->Set(NS_APP_PROFILE_DEFAULTS_50_DIR, localeDefaults);
    }
    return rv;
}





nsresult 
nsProfile::MigrateProfileInternal(const PRUnichar* profileName,
                                  nsIFile* oldProfDir,
                                  nsIFile* newProfDir)
{
    NS_ENSURE_ARG_POINTER(profileName);   

#if defined(DEBUG_profile)
    printf("Inside Migrate Profile routine.\n" );
#endif

    

    nsresult rv;
    nsCOMPtr <nsIPrefMigration> pPrefMigrator =
            do_CreateInstance(kPrefMigrationCID, &rv);
    if (NS_FAILED(rv)) return rv;
        
    nsCOMPtr<nsILocalFile> oldProfDirLocal(do_QueryInterface(oldProfDir, &rv));
    if (NS_FAILED(rv)) return rv;    
    nsCOMPtr<nsILocalFile> newProfDirLocal(do_QueryInterface(newProfDir, &rv));
    if (NS_FAILED(rv)) return rv;
        
    nsCAutoString oldProfDirStr;
    nsCAutoString newProfDirStr;

    rv = oldProfDirLocal->GetPersistentDescriptor(oldProfDirStr);
    if (NS_FAILED(rv)) return rv;
    rv = newProfDirLocal->GetPersistentDescriptor(newProfDirStr);
    if (NS_FAILED(rv)) return rv;

    
    rv = pPrefMigrator->AddProfilePaths(oldProfDirStr.get(), newProfDirStr.get());  

    rv = pPrefMigrator->ProcessPrefs(PR_TRUE); 
    if (NS_FAILED(rv)) return rv;

    
    nsresult errorCode;   
    errorCode = pPrefMigrator->GetError();

    
    
    if (errorCode == MIGRATION_CREATE_NEW)
    {
        PRInt32 numProfiles = 0;
        ShowProfileWizard();

        
        
        
        
        
        
        
        
        
        
        if (!mAutomigrate)
        {
            GetProfileCount(&numProfiles);
            if (numProfiles == 0)
                mDiskSpaceErrorQuitCalled = PR_TRUE;
        }
        mOutofDiskSpace = PR_TRUE;
        return NS_ERROR_FAILURE;
    }
    else if (errorCode == MIGRATION_CANCEL) 
    {
        
        
        
        
        
        
        
        
        if (!mAutomigrate)
            mDiskSpaceErrorQuitCalled = PR_TRUE;

        ForgetCurrentProfile();
        mOutofDiskSpace = PR_TRUE;
        return NS_ERROR_FAILURE;
    }
    else if (errorCode != MIGRATION_SUCCESS) 
    {
        return NS_ERROR_FAILURE;
    }

    
    
	
    rv = SetProfileDir(profileName, newProfDir);
    if (NS_FAILED(rv)) return rv;

    gProfileDataAccess->SetMigratedFromDir(profileName, oldProfDirLocal);
    gProfileDataAccess->mProfileDataChanged = PR_TRUE;
    gProfileDataAccess->UpdateRegistry(nsnull);

    return rv;
}

NS_IMETHODIMP 
nsProfile::MigrateProfile(const PRUnichar* profileName)
{
    NS_ENSURE_ARG(profileName);   

    nsresult rv = NS_OK;

    nsCOMPtr<nsIFile> oldProfDir;    
    nsCOMPtr<nsIFile> newProfDir;
    nsCOMPtr<nsIPrefBranch> prefBranch;
    nsXPIDLCString profMigDir;
 
    rv = GetProfileDir(profileName, getter_AddRefs(oldProfDir));
    if (NS_FAILED(rv)) 
      return rv;
   
    
    
    
    
    PRInt32 profRootBehavior = 0;
    nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = prefs->GetBranch(nsnull, getter_AddRefs(prefBranch));
      if (NS_SUCCEEDED(rv))
        (void) prefBranch->GetIntPref(PREF_MIGRATION_BEHAVIOR, &profRootBehavior);
    }

    switch (profRootBehavior) {

      case 1:
        rv = oldProfDir->Clone(getter_AddRefs(newProfDir));
        if (NS_FAILED(rv)) 
          return rv;
        rv = newProfDir->SetNativeLeafName(NS_LITERAL_CSTRING("Profiles"));
        if (NS_FAILED(rv)) 
          return rv;
        break;

      case 2:
        rv = prefBranch->GetCharPref(PREF_MIGRATION_DIRECTORY, getter_Copies(profMigDir));
        if (NS_SUCCEEDED(rv) && !profMigDir.IsEmpty()) {
          nsCOMPtr<nsILocalFile> localFile(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
          NS_ENSURE_SUCCESS(rv, rv);
          rv = localFile->InitWithNativePath(nsDependentCString(profMigDir));
          if (NS_SUCCEEDED(rv)) {
            newProfDir = do_QueryInterface(localFile, &rv);
            if (NS_FAILED(rv)) 
              return rv;
          }
        }
        break;

      default:
        break;

    }
    if (!newProfDir) {
      rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILES_ROOT_DIR, getter_AddRefs(newProfDir));
      if (NS_FAILED(rv)) 
        return rv;
    }

    rv = newProfDir->Append(nsDependentString(profileName));
    if (NS_FAILED(rv)) 
      return rv;
    
    rv = newProfDir->CreateUnique(nsIFile::DIRECTORY_TYPE, 0700);
    if (NS_FAILED(rv)) 
      return rv;
    
    
    rv = AddLevelOfIndirection(newProfDir);
    if (NS_FAILED(rv))
      return rv;

    return MigrateProfileInternal(profileName, oldProfDir, newProfDir);
}

NS_IMETHODIMP 
nsProfile::RemigrateProfile(const PRUnichar* profileName)
{
    NS_ENSURE_ARG_POINTER(profileName);
    
    nsCOMPtr<nsIFile> profileDir;
    nsresult rv = GetProfileDir(profileName, getter_AddRefs(profileDir));
    NS_ENSURE_SUCCESS(rv,rv);

    nsCOMPtr<nsIFile> newProfileDir;
    rv = profileDir->Clone(getter_AddRefs(newProfileDir));
    NS_ENSURE_SUCCESS(rv,rv);

    
    
    nsCOMPtr<nsILocalFile> oldProfileDir;
    rv = GetOriginalProfileDir(profileName, getter_AddRefs(oldProfileDir));
    NS_ENSURE_SUCCESS(rv,rv);

    
    nsCAutoString origDirLeafName;
    rv = profileDir->GetNativeLeafName(origDirLeafName);
    NS_ENSURE_SUCCESS(rv,rv);
     
    
    
    nsCAutoString newDirLeafName(origDirLeafName + NS_LITERAL_CSTRING("-new"));
    rv = newProfileDir->SetNativeLeafName(newDirLeafName);
    NS_ENSURE_SUCCESS(rv,rv);
    
    
    rv = newProfileDir->CreateUnique(nsIFile::DIRECTORY_TYPE, 0700);
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to create new directory for the remigrated profile");
    if (NS_SUCCEEDED(rv)) {
        rv = MigrateProfileInternal(profileName, oldProfileDir, newProfileDir);
        NS_ASSERTION(NS_SUCCEEDED(rv), "MigrateProfileInternal failed");
    }
    return rv;
}

nsresult
nsProfile::ShowProfileWizard(void)
{
    nsresult rv;
    nsCOMPtr<nsIWindowWatcher> windowWatcher(do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIDialogParamBlock> ioParamBlock(do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID, &rv));
    if (NS_FAILED(rv)) return rv;
    ioParamBlock->SetInt(0,4); 
   
    nsCOMPtr<nsIDOMWindow> newWindow;
    rv = windowWatcher->OpenWindow(nsnull,
                                   PROFILE_WIZARD_URL,
                                   "_blank",
                                   kDefaultOpenWindowParams,
                                   ioParamBlock,
                                   getter_AddRefs(newWindow));
    return rv;
}

NS_IMETHODIMP nsProfile::ProfileExists(const PRUnichar *profileName, PRBool *exists)
{
    NS_ENSURE_ARG_POINTER(profileName); 
    NS_ENSURE_ARG_POINTER(exists);

    *exists = gProfileDataAccess->ProfileExists(profileName);
    return NS_OK;
}

NS_IMETHODIMP nsProfile::IsCurrentProfileAvailable(PRBool *available)
{
    NS_ENSURE_ARG_POINTER(available);

    *available = mCurrentProfileAvailable;
    return NS_OK;
}



NS_IMETHODIMP nsProfile::Get4xProfileCount(PRInt32 *numProfiles)
{
    NS_ENSURE_ARG_POINTER(numProfiles);

    *numProfiles = 0;

    gProfileDataAccess->GetNum4xProfiles(numProfiles);
    return NS_OK;
}



NS_IMETHODIMP nsProfile::MigrateAllProfiles()
{
    nsresult rv;

    PRUint32    numOldProfiles = 0;
    PRUnichar   **nameArray = nsnull;
    rv = GetProfileListX(nsIProfileInternal::LIST_ONLY_OLD, &numOldProfiles, &nameArray);
    if (NS_FAILED(rv)) return rv;
    for (PRUint32 i = 0; i < numOldProfiles; i++)
    {
        rv = MigrateProfile(nameArray[i]);
        if (NS_FAILED(rv)) break;
    }
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(numOldProfiles, nameArray);
    return rv;
}


NS_IMETHODIMP nsProfile::CloneProfile(const PRUnichar* newProfile)
{
    NS_ENSURE_ARG_POINTER(newProfile);   

    nsresult rv = NS_OK;

#if defined(DEBUG_profile)
    printf("ProfileManager : CloneProfile\n");
#endif
    
    nsCOMPtr<nsIFile> currProfileDir;
    rv = GetCurrentProfileDir(getter_AddRefs(currProfileDir));
    if (NS_FAILED(rv)) return rv;

    PRBool exists;    
    rv = currProfileDir->Exists(&exists);
    if (NS_SUCCEEDED(rv) && exists)
    {
        nsCOMPtr<nsIFile> aFile;
        rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILES_ROOT_DIR, getter_AddRefs(aFile));
        if (NS_FAILED(rv)) return rv;
        nsCOMPtr<nsILocalFile> destDir(do_QueryInterface(aFile, &rv));
        if (NS_FAILED(rv)) return rv;
        destDir->AppendRelativePath(nsDependentString(newProfile));

        
        rv = destDir->CreateUnique(nsIFile::DIRECTORY_TYPE, 0700);
        if (NS_FAILED(rv)) return rv;
        
        rv = RecursiveCopy(currProfileDir, destDir);
        if (NS_FAILED(rv)) return rv;           
        rv = SetProfileDir(newProfile, destDir);
    }

#if defined(DEBUG_profile_verbose)
    {
      if (NS_SUCCEEDED(rv))
      printf("ProfileManager : Cloned CurrentProfile\n");

      nsCAutoString temp; temp.AssignWithConversion(newProfile);
      printf("The new profile is ->%s<-\n", temp.get());
    }
#endif

    gProfileDataAccess->mProfileDataChanged = PR_TRUE;

    return rv;
}

nsresult
nsProfile::CreateDefaultProfile(void)
{
    nsresult rv = NS_OK;

    
    nsCOMPtr<nsIFile> profileRootDir;
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILES_ROOT_DIR, getter_AddRefs(profileRootDir));
    if (NS_FAILED(rv)) return rv;
    
    nsAutoString profilePath;
    rv = profileRootDir->GetPath(profilePath);
    if (NS_FAILED(rv)) return rv;

    rv = CreateNewProfile(DEFAULT_PROFILE_NAME, profilePath.get(), nsnull, PR_TRUE);

    return rv;
}

NS_IMETHODIMP 
nsProfile::UpdateRegistry(nsIFile* regName)
{
   nsresult rv = NS_OK;

   gProfileDataAccess->mProfileDataChanged = PR_TRUE;
   rv= gProfileDataAccess->UpdateRegistry(regName);

   return rv;
}

NS_IMETHODIMP 
nsProfile::SetRegStrings(const PRUnichar* profileName, 
                         const PRUnichar* regString,
                         const PRUnichar* regName,
                         const PRUnichar* regEmail,
                         const PRUnichar* regOption)
{
   nsresult rv = NS_OK;

   ProfileStruct*    aProfile;

   rv = gProfileDataAccess->GetValue(profileName, &aProfile);
   if (NS_FAILED(rv)) return rv;

   if (aProfile == nsnull)
     return NS_ERROR_FAILURE;
   
   aProfile->NCHavePregInfo = regString;

   if (regName)    aProfile->NCProfileName   = regName;
   if (regEmail)   aProfile->NCEmailAddress  = regEmail;
   if (regOption)  aProfile->NCDeniedService = regOption;

   gProfileDataAccess->SetValue(aProfile);

   delete aProfile;

   return rv;
}

NS_IMETHODIMP 
nsProfile::IsRegStringSet(const PRUnichar *profileName, char **regString)
{
    NS_ENSURE_ARG_POINTER(profileName);   
    NS_ENSURE_ARG_POINTER(regString);

    gProfileDataAccess->CheckRegString(profileName, regString);
    return NS_OK;
}





NS_IMETHODIMP nsProfile::VetoChange()
{
    mProfileChangeVetoed = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP nsProfile::ChangeFailed()
{
    mProfileChangeFailed = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsProfile::GetRegStrings(const PRUnichar *aProfileName, 
                        PRUnichar **aRegString, 
                        PRUnichar **aRegName, 
                        PRUnichar **aRegEmail, 
                        PRUnichar **aRegOption)
{
    NS_ENSURE_ARG_POINTER(aProfileName);
    NS_ENSURE_ARG_POINTER(aRegString);
    NS_ENSURE_ARG_POINTER(aRegName);
    NS_ENSURE_ARG_POINTER(aRegEmail);
    NS_ENSURE_ARG_POINTER(aRegOption);

    ProfileStruct*    profileVal;

    nsresult rv = gProfileDataAccess->GetValue(aProfileName, &profileVal);
    if (NS_FAILED(rv)) 
      return rv;

    if (profileVal == nsnull)
      return NS_ERROR_FAILURE;

    *aRegString = ToNewUnicode(profileVal->NCHavePregInfo);
    if (!*aRegString)
      return NS_ERROR_OUT_OF_MEMORY;    

    *aRegName = ToNewUnicode(profileVal->NCProfileName);
    if (!*aRegName)
      return NS_ERROR_OUT_OF_MEMORY;    

    *aRegEmail = ToNewUnicode(profileVal->NCEmailAddress);
    if (!*aRegEmail)
      return NS_ERROR_OUT_OF_MEMORY;    

    *aRegOption = ToNewUnicode(profileVal->NCDeniedService);
    if (!*aRegOption)
      return NS_ERROR_OUT_OF_MEMORY;    

     delete profileVal;

     return NS_OK;
}

nsresult nsProfile::GetLocalProfileDir(const PRUnichar* aProfileName,
                                       nsIFile** aLocalDir)
{
  *aLocalDir = nsnull;
  nsresult rv;
  nsCOMPtr<nsIProperties> directoryService = 
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;
  nsCOMPtr<nsIFile> localDir;
  rv = directoryService->Get(NS_APP_USER_PROFILES_LOCAL_ROOT_DIR,
                             NS_GET_IID(nsIFile),
                             getter_AddRefs(localDir));
  if (NS_FAILED(rv))
    return rv;
  rv = localDir->Append(nsDependentString(aProfileName));
  if (NS_FAILED(rv))
    return rv;
  localDir.swap(*aLocalDir);
  return NS_OK;
}
