









































#include "stdafx.h"
#include "QaUtils.h"
#include <stdio.h>
#include "nsProfile.h"

CProfile::CProfile(nsIWebBrowser* mWebBrowser)
{
	qaWebBrowser = mWebBrowser;
}


CProfile::~CProfile()
{
}


void CProfile::OnStartTests(UINT nMenuID)
{
	
	

	switch(nMenuID)
	{
		case ID_INTERFACES_NSIPROFILE_RUNALLTESTS	:
			RunAllTests();
			break ;
		case ID_INTERFACES_NSIPROFILE_GETPROFILECOUNT :
			GetProfileCount();
			break ;
		case ID_INTERFACES_NSIPROFILE_GETCURRENTPROFILE :
			GetCurrentProfile();
			break ;
		case ID_INTERFACES_NSIPROFILE_SETCURRENTPROFILE :
			SetCurrentProfile();
			break ;
		case ID_INTERFACES_NSIPROFILE_GETPROFILELIST :
			GetProfileList();
			break ;
		case ID_INTERFACES_NSIPROFILE_PROFILEEXISTS :
			ProfileExists();
			break ;
		case ID_INTERFACES_NSIPROFILE_CREATENEWPROFILE :
			CreateNewProfile();
			break ;
		case ID_INTERFACES_NSIPROFILE_RENAMEPROFILE :
			RenameProfile();
			break ;
		case ID_INTERFACES_NSIPROFILE_DELETEPROFILE :
			DeleteProfile();
			break ;
		case ID_INTERFACES_NSIPROFILE_CLONEPROFILE :
			CloneProfile();
			break ;
		case ID_INTERFACES_NSIPROFILE_SHUTDOWNCURRENTPROFILE :
			ShutDownCurrentProfile();
			break ;
	}

}

void CProfile::RunAllTests()
{
    
	nsCOMPtr<nsIProfile> oNsProfile (do_GetService(NS_PROFILE_CONTRACTID,&rv));

    RvTestResultDlg(rv, "do_GetService",true);

	if (!oNsProfile)
	{
	    RvTestResultDlg(rv, "Cannot get the nsIprofile object");
	    return ;
	}

	PRInt32 profileCount;
    rv = oNsProfile->GetProfileCount(&profileCount);
    RvTestResultDlg(rv, "nsIProfile::GetProfileCount() ");

    nsXPIDLString   currProfileName;
    rv = oNsProfile->GetCurrentProfile(getter_Copies(currProfileName));
    RvTestResultDlg(rv, "nsIProfile::GetCurrentProfile() ");

    rv = oNsProfile->SetCurrentProfile(currProfileName);
    RvTestResultDlg(rv, "nsIProfile::SetCurrentProfile() ");


    PRUint32    listLen;
    PRUnichar   **profileList;

    rv = oNsProfile->GetProfileList(&listLen, &profileList);
    RvTestResultDlg(rv, "oNsProfile->GetProfileList");

    for (PRUint32 index = 0; index < listLen; index++)
    {
        CString tmpStr(profileList[index]);
	    RvTestResultDlg(rv, tmpStr);
	}

    PRBool exists = FALSE;
    rv = oNsProfile->ProfileExists(currProfileName, &exists);
    RvTestResultDlg(rv, "oNsProfile->ProfileExists");

	USES_CONVERSION ;

	NS_NAMED_LITERAL_STRING(newProfileName, "nsIProfileTest");

    rv = oNsProfile->CreateNewProfile(newProfileName.get(), nsnull, nsnull, PR_TRUE);
    RvTestResultDlg(rv, "oNsProfile->CreateNewProfile");

    rv = oNsProfile->CloneProfile(currProfileName);
    RvTestResultDlg(rv, "oNsProfile->CloneProfile");

    rv = oNsProfile->RenameProfile(currProfileName, T2W("nsIProfileTestNew"));
    RvTestResultDlg(rv, "oNsProfile->RenameProfile");

    rv = oNsProfile->DeleteProfile(T2W("nsIProfileTestNew"), PR_TRUE);
    RvTestResultDlg(rv, "oNsProfile->DeleteProfile");
 
}

void CProfile::GetProfileCount()
{
	nsCOMPtr<nsIProfile> oNsProfile (do_GetService(NS_PROFILE_CONTRACTID,&rv));

    RvTestResultDlg(rv, "do_GetService",true);

	if (!oNsProfile)
	{
	    RvTestResultDlg(rv, "Cannot get the nsIprofile object");
	    return ;
	}

	PRInt32 profileCount;
    rv = oNsProfile->GetProfileCount(&profileCount);
    RvTestResultDlg(rv, "nsIProfile::GetProfileCount() ");
}

void CProfile::GetCurrentProfile()
{
	nsCOMPtr<nsIProfile> oNsProfile (do_GetService(NS_PROFILE_CONTRACTID,&rv));

    RvTestResultDlg(rv, "do_GetService",true);
	if (!oNsProfile)
	{
	    RvTestResultDlg(rv, "Cannot get the nsIprofile object");
	    return ;
	}

	nsXPIDLString   currProfileName;
    rv = oNsProfile->GetCurrentProfile(getter_Copies(currProfileName));
    RvTestResultDlg(rv, "nsIProfile::GetCurrentProfile() ");
}

void CProfile::SetCurrentProfile()
{
	nsCOMPtr<nsIProfile> oNsProfile (do_GetService(NS_PROFILE_CONTRACTID,&rv));

    RvTestResultDlg(rv, "do_GetService",true);
	if (!oNsProfile)
	{
	    RvTestResultDlg(rv, "Cannot get the nsIprofile object");
	    return ;
	}

	nsXPIDLString   currProfileName;
    rv = oNsProfile->GetCurrentProfile(getter_Copies(currProfileName));
    RvTestResultDlg(rv, "nsIProfile::GetCurrentProfile() ");

    rv = oNsProfile->SetCurrentProfile(currProfileName);
    RvTestResultDlg(rv, "nsIProfile::SetCurrentProfile() ");
}

void CProfile::GetProfileList()
{
	nsCOMPtr<nsIProfile> oNsProfile (do_GetService(NS_PROFILE_CONTRACTID,&rv));

    RvTestResultDlg(rv, "do_GetService",true);
	if (!oNsProfile)
	{
	    RvTestResultDlg(rv, "Cannot get the nsIprofile object");
	    return ;
	}

	PRUint32    listLen;
    PRUnichar   **profileList;

    rv = oNsProfile->GetProfileList(&listLen, &profileList);
    RvTestResultDlg(rv, "oNsProfile->GetProfileList");

    for (PRUint32 index = 0; index < listLen; index++)
    {
        CString tmpStr(profileList[index]);
	    RvTestResultDlg(rv, tmpStr);
	}
}

void CProfile::ProfileExists()
{
	nsCOMPtr<nsIProfile> oNsProfile (do_GetService(NS_PROFILE_CONTRACTID,&rv));

    RvTestResultDlg(rv, "do_GetService",true);
	if (!oNsProfile)
	{
	    RvTestResultDlg(rv, "Cannot get the nsIprofile object");
	    return ;
	}

	nsXPIDLString   currProfileName;
    rv = oNsProfile->GetCurrentProfile(getter_Copies(currProfileName));
    RvTestResultDlg(rv, "nsIProfile::GetCurrentProfile() ");

	PRBool exists = FALSE;
    rv = oNsProfile->ProfileExists(currProfileName, &exists);
    RvTestResultDlg(rv, "oNsProfile->ProfileExists");
}

void CProfile::CreateNewProfile()
{
	nsCOMPtr<nsIProfile> oNsProfile (do_GetService(NS_PROFILE_CONTRACTID,&rv));

    RvTestResultDlg(rv, "do_GetService",true);
	if (!oNsProfile)
	{
	    RvTestResultDlg(rv, "Cannot get the nsIprofile object");
	    return ;
	}

	USES_CONVERSION ;

	rv = oNsProfile->CreateNewProfile(T2W("New Test"), nsnull, nsnull, PR_TRUE);
    RvTestResultDlg(rv, "oNsProfile->CreateNewProfile");
   
}

void CProfile::RenameProfile()
{
	nsCOMPtr<nsIProfile> oNsProfile (do_GetService(NS_PROFILE_CONTRACTID,&rv));

    RvTestResultDlg(rv, "do_GetService",true);
	if (!oNsProfile)
	{
	    RvTestResultDlg(rv, "Cannot get the nsIprofile object");
	    return ;
	}

	USES_CONVERSION ;

	nsXPIDLString   currProfileName;
    rv = oNsProfile->GetCurrentProfile(getter_Copies(currProfileName));
    RvTestResultDlg(rv, "nsIProfile::GetCurrentProfile() ");

	rv = oNsProfile->RenameProfile(currProfileName, T2W("New default"));
    RvTestResultDlg(rv, "oNsProfile->RenameProfile");
}

void CProfile::DeleteProfile()
{
	nsCOMPtr<nsIProfile> oNsProfile (do_GetService(NS_PROFILE_CONTRACTID,&rv));

    RvTestResultDlg(rv, "do_GetService",true);
	if (!oNsProfile)
	{
	    RvTestResultDlg(rv, "Cannot get the nsIprofile object");
	    return ;
	}

	USES_CONVERSION ;

	nsXPIDLString   currProfileName;
    rv = oNsProfile->GetCurrentProfile(getter_Copies(currProfileName));
    RvTestResultDlg(rv, "nsIProfile::GetCurrentProfile() ");

    rv = oNsProfile->DeleteProfile(currProfileName, PR_TRUE);
    RvTestResultDlg(rv, "oNsProfile->DeleteProfile");
}

void CProfile::CloneProfile()
{
	nsCOMPtr<nsIProfile> oNsProfile (do_GetService(NS_PROFILE_CONTRACTID,&rv));

    RvTestResultDlg(rv, "do_GetService",true);
	if (!oNsProfile)
	{
	    RvTestResultDlg(rv, "Cannot get the nsIprofile object");
	    return ;
	}
 
	USES_CONVERSION ;

	nsXPIDLString   currProfileName;
    rv = oNsProfile->GetCurrentProfile(getter_Copies(currProfileName));
    RvTestResultDlg(rv, "nsIProfile::GetCurrentProfile() ");

    rv = oNsProfile->CloneProfile(currProfileName);
    RvTestResultDlg(rv, "oNsProfile->CloneProfile");
}

void CProfile::ShutDownCurrentProfile()
{
	nsCOMPtr<nsIProfile> oNsProfile (do_GetService(NS_PROFILE_CONTRACTID,&rv));

    RvTestResultDlg(rv, "do_GetService",true);
	if (!oNsProfile)
	{
	    RvTestResultDlg(rv, "Cannot get the nsIprofile object");
	    return ;
	}
}

