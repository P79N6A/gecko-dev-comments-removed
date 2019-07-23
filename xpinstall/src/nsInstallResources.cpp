 








































#include <string.h>
#include "nscore.h"
#include "nsInstallResources.h"

static nsXPIResourceTableItem XPIResTable[] = 
{
    


    { "InstallFile",        "Installing: %s" },
    { "InstallSharedFile",  "Installing Shared File: %s" },
    { "ReplaceFile",        "Replacing: %s" },
    { "ReplaceSharedFile",  "Replacing Shared File: %s" },
    { "SkipFile",           "Skipping: %s" },
    { "SkipSharedFile",     "Skipping Shared File: %s" },
    { "DeleteFile",         "Deleting file: %s" },
    { "DeleteComponent",    "Deleting component: %s" },
    { "Execute",            "Executing: %s" },
    { "ExecuteWithArgs",    "Executing: %s with argument: %s" },
    { "Patch",              "Patching: %s" },
    { "Uninstall",          "Uninstalling: %s" },
    { "RegSkin",            "Register Skin: %s" },
    { "RegLocale",          "Register Locale: %s" },
    { "RegContent",         "Register Content: %s" },
    { "RegPackage",         "Register Package: %s" },


    { "CopyFile",              "Copy File: %s to %s" },
    { "ExecuteFile",           "Execute File: %s" },
    { "ExecuteFileWithArgs",   "Execute File: %s with argument: %s" },
    { "MoveFile",              "Move File: %s to %s" },
    { "RenameFile",            "Rename File: %s to %s" },
    { "CreateFolder",          "Create Folder: %s" },
    { "RemoveFolder",          "Remove Folder: %s" },
    { "RenameFolder",          "Rename Folder: %s to %s" },
    { "WindowsShortcut",       "Windows Shortcut: %s" },
    { "MacAlias",              "Mac Alias: %s" },
    { "WindowsRegisterServer", "Windows Register Server: %s" },
    { "UnknownFileOpCommand",  "Unknown file operation command!" },

    
    

    


    { "ERROR", "ERROR" },

    { NS_XPI_EOT, "" }
};

char* 
nsInstallResources::GetDefaultVal(const char* aResName)
{
    char    *currResName = XPIResTable[0].resName;
    char    *currResVal = nsnull;
    PRInt32 idx, len = 0;

    for (idx = 0; 0 != strcmp(currResName, NS_XPI_EOT); idx++)
    {
        currResName = XPIResTable[idx].resName;
        len = strlen(currResName);

        if (0 == strncmp(currResName, aResName, len))
        {
            currResVal = XPIResTable[idx].defaultString;
            break;
        }
    }

    return currResVal;
}

