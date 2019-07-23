







































#include "stdafx.h"
#include "Testembed.h"
#include "ProfileMgr.h"
#include "ProfilesDlg.h"


#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsIRegistry.h"
#include "nsIProfile.h"


#define kRegistryGlobalPrefsSubtreeString (NS_LITERAL_STRING("global-prefs"))
#define kRegistryShowProfilesAtStartup "start-show-dialog"





CProfileMgr::CProfileMgr()
{
}

CProfileMgr::~CProfileMgr()
{
}





    
nsresult CProfileMgr::StartUp()
{
    nsresult rv;
         
    
    nsCOMPtr<nsIProfile> profileService(do_GetService(NS_PROFILE_CONTRACTID,&rv));
    if (NS_FAILED(rv)) return rv;
        
    PRInt32 profileCount;
    rv = profileService->GetProfileCount(&profileCount);
    if (NS_FAILED(rv)) return rv;
    if (profileCount == 0)
    {
        
        NS_NAMED_LITERAL_STRING(newProfileName, "default");

        rv = profileService->CreateNewProfile(newProfileName.get(), nsnull, nsnull, PR_FALSE);
        if (NS_FAILED(rv)) return rv;
        rv = profileService->SetCurrentProfile(newProfileName.get());
        if (NS_FAILED(rv)) return rv;
    }
    else
    {
        
        
        
        PRBool showIt;
        rv = GetShowDialogOnStart(&showIt);
                        
        if (NS_FAILED(rv) || (profileCount > 1 && showIt))
        {
            DoManageProfilesDialog(TRUE);
        }
        else
        {
            
            
            
            nsXPIDLString   currProfileName;
            rv = profileService->GetCurrentProfile(getter_Copies(currProfileName));
            if (NS_FAILED(rv)) return rv;
            rv = profileService->SetCurrentProfile(currProfileName);
            if (NS_FAILED(rv)) return rv;
        }    
    }

    return NS_OK;
}

nsresult CProfileMgr::DoManageProfilesDialog(PRBool bAtStartUp)
{
    CProfilesDlg    dialog;
    nsresult        rv;
    PRBool          showIt;

    rv = GetShowDialogOnStart(&showIt);
    dialog.m_bAtStartUp = bAtStartUp;
    dialog.m_bAskAtStartUp = NS_SUCCEEDED(rv) ? showIt : TRUE;

    if (dialog.DoModal() == IDOK)
    {
        SetShowDialogOnStart(dialog.m_bAskAtStartUp);
         
        
	    nsCOMPtr<nsIProfile> profileService(do_GetService(NS_PROFILE_CONTRACTID,&rv));
        if (NS_FAILED(rv))
               rv = profileService->SetCurrentProfile(dialog.m_SelectedProfile.get());
    }
    return NS_OK;
}
    
 



     
nsresult CProfileMgr::GetShowDialogOnStart(PRBool* showIt)
{
    nsresult rv = NS_OK;
        
    *showIt = PR_TRUE;
                
    nsCOMPtr<nsIRegistry> registry(do_CreateInstance(NS_REGISTRY_CONTRACTID, &rv));
    rv = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationRegistry);
    if (NS_FAILED(rv)) return rv;

    nsRegistryKey profilesTreeKey;
    
    rv = registry->GetKey(nsIRegistry::Common, 
                          kRegistryGlobalPrefsSubtreeString.get(), 
                          &profilesTreeKey);

    if (NS_SUCCEEDED(rv)) 
    {
        PRInt32 flagValue;
        rv = registry->GetInt(profilesTreeKey, 
                              kRegistryShowProfilesAtStartup, 
                              &flagValue);
         
        if (NS_SUCCEEDED(rv))
            *showIt = (flagValue != 0);
    }
    return rv;        
}

nsresult CProfileMgr::SetShowDialogOnStart(PRBool showIt)
{
    nsresult rv = NS_OK;
                        
    nsCOMPtr<nsIRegistry> registry(do_CreateInstance(NS_REGISTRY_CONTRACTID, &rv));
    rv = registry->OpenWellKnownRegistry(nsIRegistry::ApplicationRegistry);
    if (NS_FAILED(rv)) return rv;

    nsRegistryKey profilesTreeKey;
    
    rv = registry->GetKey(nsIRegistry::Common, 
                          kRegistryGlobalPrefsSubtreeString.get(), 
                          &profilesTreeKey);

    if (NS_FAILED(rv)) 
    {
        rv = registry->AddKey(nsIRegistry::Common, 
                              kRegistryGlobalPrefsSubtreeString.get(), 
                              &profilesTreeKey);
    }
    if (NS_SUCCEEDED(rv))
    {
    
        rv = registry->SetInt(profilesTreeKey, 
                              kRegistryShowProfilesAtStartup, 
                              showIt);
    }
    
    return rv;        
}

