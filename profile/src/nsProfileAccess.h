





































#ifndef __nsProfileAccess_h___
#define __nsProfileAccess_h___

#include "nsCOMPtr.h"
#include "nsISupports.h"
#include "nsString.h"
#include "nsIRegistry.h"
#include "nsXPIDLString.h"
#include "nsVoidArray.h"
#include "nsIFile.h"
#include "nsILocalFile.h"

#ifdef XP_WIN
#include <windows.h>
#endif

#ifdef XP_OS2
#define INCL_DOSERRORS
#define INCL_DOSFILEMGR
#include <os2.h>
#endif

class ProfileStruct
{    
public:
     ProfileStruct();
     ProfileStruct(const ProfileStruct& src);
                
    ~ProfileStruct() { }
    
     ProfileStruct& operator=(const ProfileStruct& rhs);
    
    





            
    nsresult    GetResolvedProfileDir(nsILocalFile **aDirectory);
    
    




    nsresult    SetResolvedProfileDir(nsILocalFile *aDirectory);
    
    


    nsresult    CopyProfileLocation(ProfileStruct *destStruct);
    
    



    nsresult    InternalizeLocation(nsIRegistry *aRegistry, nsRegistryKey profKey, PRBool is4x);
    nsresult    ExternalizeLocation(nsIRegistry *aRegistry, nsRegistryKey profKey);
    nsresult    InternalizeMigratedFromLocation(nsIRegistry *aRegistry, nsRegistryKey profKey);
    nsresult    ExternalizeMigratedFromLocation(nsIRegistry *aRegistry, nsRegistryKey profKey);
    
public:
    nsString    profileName;
    PRBool      isMigrated;

    
    
    nsCOMPtr<nsILocalFile> migratedFrom;

    nsString    NCProfileName;
    nsString    NCDeniedService;
    nsString    NCEmailAddress;
    nsString    NCHavePregInfo;
    PRBool      updateProfileEntry;
    
    PRBool      isImportType; 
    
    
    
    PRInt64     creationTime;
    PRInt64     lastModTime; 

private:
    nsresult    EnsureDirPathExists(nsILocalFile *aFile, PRBool *wasCreated);
    
private:
    
    nsString regLocationData;
    nsCOMPtr<nsILocalFile> resolvedLocation;
};


class nsProfileAccess
{

private:
    nsCOMPtr <nsIFile> mNewRegFile;

    
    
    nsVoidArray*  mProfiles;

    nsString      mCurrentProfile;
    nsString      mHavePREGInfo;
    PRBool        m4xProfilesAdded;
    PRBool        mStartWithLastProfile;
public:
    PRBool        mProfileDataChanged;
    PRBool        mForgetProfileCalled;

public:

    nsProfileAccess();
    virtual ~nsProfileAccess();

    void GetNumProfiles(PRInt32 *numProfiles);
    void GetNum4xProfiles(PRInt32 *numProfiles);
    void GetFirstProfile(PRUnichar **firstProfile);
    nsresult GetProfileList(PRInt32 whichKind, PRUint32 *length, PRUnichar ***result);
    nsresult GetOriginalProfileDir(const PRUnichar *profileName, nsILocalFile **orginalDir);
    nsresult SetMigratedFromDir(const PRUnichar *profileName, nsILocalFile *orginalDir);
    nsresult SetProfileLastModTime(const PRUnichar *profileName, PRInt64 lastModTime);
    nsresult GetStartWithLastUsedProfile(PRBool *aStartWithLastUsedProfile);
    nsresult SetStartWithLastUsedProfile(PRBool aStartWithLastUsedProfile);
    
    
    
    nsresult Get4xProfileInfo(nsIFile *registryFile, PRBool fromImport);

    void SetCurrentProfile(const PRUnichar *profileName);
    void GetCurrentProfile(PRUnichar **profileName);
    
    nsresult GetValue(const PRUnichar* profileName, ProfileStruct** aProfile);
    nsresult SetValue(ProfileStruct* aProfile);
    void CheckRegString(const PRUnichar *profileName, char** regString);
    void RemoveSubTree(const PRUnichar* profileName);

    PRBool ProfileExists(const PRUnichar *profileName);

    nsresult DetermineForceMigration(PRBool *forceMigration);
    nsresult UpdateRegistry(nsIFile* regName);

private:
    nsresult FillProfileInfo(nsIFile* regName);



    nsresult HavePregInfo(char **info);

    
    
    PRInt32	 FindProfileIndex(const PRUnichar* profileName, PRBool forImport);

    void SetPREGInfo(const char* pregInfo);
    void FreeProfileMembers(nsVoidArray *aProfile);
    nsresult ResetProfileMembers();
};


#endif 

