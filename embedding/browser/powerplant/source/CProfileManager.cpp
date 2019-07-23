







































#include "CProfileManager.h"
#include "ApplIDs.h"
#include "UMacUnicode.h"


#include <LEditText.h>
#include <LTextTableView.h>
#include <LPushButton.h>
#include <LTableMonoGeometry.h>
#include <LTableArrayStorage.h>
#include <LTableSingleSelector.h>
#include <LCheckBox.h>


#include <CFPreferences.h>


#include "nsIProfile.h"
#include "nsIDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIObserverService.h"
#include "nsXPIDLString.h"
#include "nsCRT.h"


#include <climits>



const MessageT    msg_OnNewProfile 	        = 2000;
const MessageT    msg_OnDeleteProfile 	    = 2001;
const MessageT    msg_OnRenameProfile 	    = 2002;

#define kPrefShowProfilesAtStartup "start-show-dialog"





CProfileManager::CProfileManager() :
    LAttachment(msg_AnyMessage,true)
{
}

CProfileManager::~CProfileManager()
{
}

void CProfileManager::StartUp()
{
    nsresult rv;
         
    nsCOMPtr<nsIProfile> profileService = 
             do_GetService(NS_PROFILE_CONTRACTID, &rv);
    ThrowIfNil_(profileService);
        
    PRInt32 profileCount;
    rv = profileService->GetProfileCount(&profileCount);
    ThrowIfError_(rv);
    if (profileCount == 0)
    {
        
        NS_NAMED_LITERAL_STRING(newProfileName, "default");

        rv = profileService->CreateNewProfile(newProfileName.get(), nsnull, nsnull, PR_FALSE);
        ThrowIfError_(rv);
        rv = profileService->SetCurrentProfile(newProfileName.get());
        ThrowIfError_(rv);
    }
    else
    {
        
        
        
        PRBool showIt;
        rv = GetShowDialogOnStart(&showIt);
                        
        if (NS_FAILED(rv) || (profileCount > 1 && showIt))
        {
            DoManageProfilesDialog();
        }
        else
        {
            
            
            
            nsXPIDLString   currProfileName;
            rv = profileService->GetCurrentProfile(getter_Copies(currProfileName));
            ThrowIfError_(rv);
            rv = profileService->SetCurrentProfile(currProfileName);
            ThrowIfError_(rv);
        }    
    }
}

Boolean CProfileManager::DoNewProfileDialog(char *outName, UInt32 bufSize)
{
    Boolean confirmed;
    StDialogHandler	theHandler(dlog_NewProfile, LCommander::GetTopCommander());
    LWindow			 *theDialog = theHandler.GetDialog();
    
    ThrowIfNil_(theDialog);
    LEditText *responseText = dynamic_cast<LEditText*>(theDialog->FindPaneByID('Name'));
    ThrowIfNil_(responseText);
    theDialog->SetLatentSub(responseText);

    theDialog->Show();
    theDialog->Select();
	
  	while (true)  
  	{				
  		MessageT	hitMessage = theHandler.DoDialog();
  		
  		if (hitMessage == msg_OK)
  		{
  		    Str255 pStr;
  		    UInt32 outLen;
  		    
 		    responseText->GetDescriptor(pStr);
 		    outLen = pStr[0] >= bufSize ? bufSize - 1 : pStr[0];
 		    memcpy(outName, &pStr[1], outLen);
 		    outName[outLen] = '\0'; 
            confirmed = PR_TRUE;
     		break;
   		}
   		else if (hitMessage == msg_Cancel)
   		{
   		    confirmed = PR_FALSE;
   		    break;
   		}
  	}
  	return confirmed;
}


void CProfileManager::DoManageProfilesDialog()
{
    nsresult rv;
    StDialogHandler	theHandler(dlog_ManageProfiles, LCommander::GetTopCommander());
    LWindow			 *theDialog = theHandler.GetDialog();

    nsCOMPtr<nsIProfile> profileService = 
             do_GetService(NS_PROFILE_CONTRACTID, &rv);
    ThrowIfNil_(profileService);
        
    
    LTextTableView *table = (LTextTableView*) theDialog->FindPaneByID('List');    
    ThrowIfNil_(table);
    LPushButton *deleteButton = (LPushButton *) theDialog->FindPaneByID('Dele');
    ThrowIfNil_(deleteButton);
    
    
    nsAutoString unicodeStr;
    nsCAutoString cStr;
    char dataBuf[256];
    UInt32 dataSize;
    
    
    STableCell selectedCell(1, 1);
    SDimension16 tableSize;
    TableIndexT rows, cols;

    table->GetFrameSize(tableSize);
	table->SetTableGeometry(new LTableMonoGeometry(table, tableSize.width, 16));
	table->SetTableStorage(new LTableArrayStorage(table, 0UL));
	table->SetTableSelector(new LTableSingleSelector(table));
	table->InsertCols(1, 0);

    
    nsXPIDLString   currProfileName;
    profileService->GetCurrentProfile(getter_Copies(currProfileName));
    
    
    PRUint32 listLen;
    PRUnichar **profileList;
    rv = profileService->GetProfileList(&listLen, &profileList);
    ThrowIfError_(rv);
    
    for (PRUint32 index = 0; index < listLen; index++)
    {
          CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(profileList[index]), cStr);
          table->InsertRows(1, LONG_MAX, cStr.get(), cStr.Length(), true);
          
          if (nsCRT::strcmp(profileList[index], currProfileName.get()) == 0)
            selectedCell.row = index + 1;
    }
    
    PRInt32 numProfiles;
    rv = profileService->GetProfileCount(&numProfiles);
    ThrowIfError_(rv);    
    (numProfiles > 1) ? deleteButton->Enable() : deleteButton->Disable();
    table->SelectCell(selectedCell);


    
    LCheckBox *showAtStartCheck = (LCheckBox*) theDialog->FindPaneByID('Show');
    ThrowIfNil_(showAtStartCheck);
    PRBool showIt;
    rv = GetShowDialogOnStart(&showIt);
    if (NS_FAILED(rv))
        showIt = PR_TRUE;
    showAtStartCheck->SetValue(showIt);

    
    theDialog->Show();
    theDialog->Select();
	
  	while (true)  
  	{				
  		MessageT	hitMessage = theHandler.DoDialog();
  		
  		if (hitMessage == msg_OK)
  		{
  		    theDialog->Hide();
            SetShowDialogOnStart(showAtStartCheck->GetValue());
   		    selectedCell = table->GetFirstSelectedCell();
   		    if (selectedCell.row > 0)
   		    {
   		        dataSize = sizeof(dataBuf) - 1;
   		        table->GetCellData(selectedCell, dataBuf, dataSize);
   		        dataBuf[dataSize] = '\0';
                CPlatformUCSConversion::GetInstance()->PlatformToUCS(nsDependentCString(dataBuf), unicodeStr);
   		        rv = profileService->SetCurrentProfile(unicodeStr.get());
            }
  		    break;
  		}
        else if (hitMessage == msg_Cancel)
        {
           	break;
        }
        else if (hitMessage == msg_OnNewProfile)
   		{
   		    if (DoNewProfileDialog(dataBuf, sizeof(dataBuf)))
   		    {
   		        CPlatformUCSConversion::GetInstance()->PlatformToUCS(nsDependentCString(dataBuf), unicodeStr);
   		        rv = profileService->CreateNewProfile(unicodeStr.get(), nsnull, nsnull, PR_FALSE);
   		        if (NS_FAILED(rv))
   		            break;
   		        
                table->InsertRows(1, LONG_MAX, dataBuf, strlen(dataBuf), true);
                table->GetTableSize(rows, cols);
                table->SelectCell(STableCell(rows, cols));
                
                rv = profileService->GetProfileCount(&numProfiles);
                (NS_SUCCEEDED(rv) && numProfiles > 1) ? deleteButton->Enable() : deleteButton->Disable();
   		    }    
   		}
   		else if (hitMessage == msg_OnDeleteProfile)
   		{
   		    selectedCell = table->GetFirstSelectedCell();
   		    if (selectedCell.row > 0)
   		    {
   		        dataSize = sizeof(dataBuf) - 1;
   		        table->GetCellData(selectedCell, dataBuf, dataSize);
   		        dataBuf[dataSize] = '\0';
   		        CPlatformUCSConversion::GetInstance()->PlatformToUCS(nsDependentCString(dataBuf), unicodeStr);
   		        
   		        rv = profileService->DeleteProfile(unicodeStr.get(), PR_TRUE);
   		        if (NS_FAILED(rv))
   		            break;
   		        
   		        table->RemoveRows(1, selectedCell.row, true);
   		        table->GetTableSize(rows, cols);    
   		        if (selectedCell.row >= rows)
   		            selectedCell.row = rows - 1;
   		        table->SelectCell(selectedCell);
   		        
                rv = profileService->GetProfileCount(&numProfiles);
                (NS_SUCCEEDED(rv) && numProfiles > 1) ? deleteButton->Enable() : deleteButton->Disable();
   		    }
   		}
   		else if (hitMessage == msg_OnRenameProfile)
   		{
	        nsAutoString oldName;

   		    selectedCell = table->GetFirstSelectedCell();
	        dataSize = sizeof(dataBuf) - 1;
	        table->GetCellData(selectedCell, dataBuf, dataSize);
	        dataBuf[dataSize] = '\0';
	        CPlatformUCSConversion::GetInstance()->PlatformToUCS(nsDependentCString(dataBuf), oldName);

   		    if (DoNewProfileDialog(dataBuf, sizeof(dataBuf)))
   		    {
   		        CPlatformUCSConversion::GetInstance()->PlatformToUCS(nsDependentCString(dataBuf), unicodeStr);
                profileService->RenameProfile(oldName.get(), unicodeStr.get());
                table->SetCellData(selectedCell, dataBuf, strlen(dataBuf)); 		        
   		    }
   		}
  	}  
}


void CProfileManager::DoLogout()
{
    enum { iPersist = 1, iCancel, iCleanse };

    nsresult rv;
    nsCOMPtr<nsIProfile> profileService = 
             do_GetService(NS_PROFILE_CONTRACTID, &rv);
    ThrowIfNil_(profileService);
    
    nsXPIDLString currentProfile;
    Str255 pStr;
    profileService->GetCurrentProfile(getter_Copies(currentProfile));
   	CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(currentProfile.get()), pStr);
    ::ParamText(pStr, "\p", "\p", "\p");
    
    DialogItemIndex item = UModalAlerts::StopAlert(alrt_ConfirmLogout);
    if (item == iCancel)
      return;

    if (item == iPersist)
        rv = profileService->ShutDownCurrentProfile(nsIProfile::SHUTDOWN_PERSIST);
    else    
        rv = profileService->ShutDownCurrentProfile(nsIProfile::SHUTDOWN_CLEANSE);
    if (NS_SUCCEEDED(rv)) {
        
        DoManageProfilesDialog();
    }
}









nsresult CProfileManager::GetShowDialogOnStart(PRBool* showIt)
{
    *showIt = PR_TRUE;

    CFStringRef showDialogKey = CFSTR(kPrefShowProfilesAtStartup);
    Boolean keyExistsAndIsValid, value;
    
    value = CFPreferencesGetAppBooleanValue(showDialogKey,
                kCFPreferencesCurrentApplication,
                &keyExistsAndIsValid);
            
    if (!keyExistsAndIsValid)
      return NS_ERROR_FAILURE;
    *showIt = value;
    
    return NS_OK;
}

nsresult CProfileManager::SetShowDialogOnStart(PRBool showIt)
{
    CFStringRef showDialogKey = CFSTR(kPrefShowProfilesAtStartup);
    
    CFPreferencesSetAppValue(showDialogKey,
      showIt ? kCFBooleanTrue : kCFBooleanFalse,
      kCFPreferencesCurrentApplication);
    
    
    CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
    
    return NS_OK;
}





void CProfileManager::ExecuteSelf(MessageT inMessage, void *ioParam)
{
	mExecuteHost = true;
	
	if (inMessage == msg_CommandStatus) {
		SCommandStatus	*status = (SCommandStatus *)ioParam;
		if (status->command == cmd_ManageProfiles) {
			*status->enabled = true;
			*status->usesMark = false;
			mExecuteHost = false; 
		}
		else if (status->command == cmd_Logout) {
			*status->enabled = true;
			*status->usesMark = false;
			mExecuteHost = false; 
		}
	}
	else if (inMessage == cmd_ManageProfiles) {
	    DoManageProfilesDialog();
	    mExecuteHost = false; 
	}
	else if (inMessage == cmd_Logout) {
	    DoLogout();
	    mExecuteHost = false; 
	}
}
