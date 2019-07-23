












































 
#ifndef __ApplIDs__
#define __ApplIDs__
 





enum {
    wind_DownloadProgress = 1305
};


enum {
    view_BrowserStatusBar = 130,
    view_BrowserToolBar = 131,
    view_DownloadProgressItem = 1306
};


enum {
    dlog_FindDialog = 1281
};


enum {
    dlog_Alert = 1282,
    dlog_AlertCheck = 1289,
    dlog_Confirm = 1283,
    dlog_ConfirmCheck = 1284,
    dlog_Prompt = 1285,
    dlog_PromptNameAndPass = 1286,
    dlog_PromptPassword = 1287,
    dlog_ConfirmEx = 1288
};


enum {
    dlog_OS9PrintProgress = 1290
};


enum {
    dlog_ManageProfiles = 1300,
    dlog_NewProfile = 1301
};


enum {
    alrt_ConfirmProfileSwitch   = 1500,
    alrt_ConfirmLogout          = 1501
};


enum {
    menu_Buzzwords              = 1000
};


enum {
    mcmd_BrowserShellContextMenuCmds = 1200
};



enum {
    STRx_FileLocProviderStrings = 5000,
    
    str_AppRegistryName         = 1,
    str_ProfilesRootDirName,
    str_DefaultsDirName,
    str_PrefDefaultsDirName,
    str_ProfileDefaultsDirName,
    str_ResDirName,
    str_ChromeDirName,
    str_PlugInsDirName,
    str_SearchPlugInsDirName
};

enum {
    STRx_StdButtonTitles        = 5001,
    
    str_Blank                   = 1,
    str_OK,
    str_Cancel,
    str_Yes,
    str_No,
    str_Save,
    str_DontSave,
    str_Revert,
    str_Allow,
    str_DenyAll
};

enum {
    STRx_StdAlertStrings        = 5002,
    
    str_OpeningPopupWindow      = 1,
    str_OpeningPopupWindowExp,  
    str_ConfirmCloseDownloads,
    str_ConfirmCloseDownloadsExp
};

enum {
    STRx_DownloadStatus         = 5003,
    
    str_ProgressFormat          = 1,
    
    str_About5Seconds,
    str_About10Seconds,
    str_LessThan1Minute,
    str_About1Minute,
    str_AboutNMinutes,
    str_About1Hour,
    str_AboutNHours
};


enum {
    icon_LockInsecure       = 1320,
    icon_LockSecure,
    icon_LockBroken
};






enum {
    msg_OnStartLoadDocument 	= 1000,
    msg_OnEndLoadDocument 	    = 1001
};





enum {
    cmd_OpenDirectory           = 'ODir',
    cmd_OpenLinkInNewWindow     = 'OLnN',
    
	cmd_Back                    = 'Back',
	cmd_Forward                 = 'Forw',
	cmd_Reload                  = 'Rlod',
	cmd_Stop                    = 'Stop',

	cmd_Find                    = 'Find',
	cmd_FindNext                = 'FNxt',

	cmd_ViewPageSource          = 'VSrc',
	cmd_ViewImage               = 'VImg',
	cmd_ViewBackgroundImage     = 'VBIm',
	
	cmd_CopyImage               = 'CpIm',

	cmd_CopyLinkLocation        = 'CLnk',
	cmd_CopyImageLocation       = 'CImg',

    cmd_SaveFormData            = 'SFrm',
    cmd_PrefillForm             = 'PFFm',
    
    
    cmd_ManageProfiles          = 'MPrf',
    cmd_Logout                  = 'LOut',
    
    cmd_SaveLinkTarget          = 'DnlL',
    cmd_SaveImage               = 'DlIm'
};





enum {
    
    keyGetURLReferrer           = 'refe'
};

#endif 
